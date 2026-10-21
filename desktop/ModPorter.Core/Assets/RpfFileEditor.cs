using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;

namespace ModPorter.Core.Assets;

public sealed record ArchiveEntry(string Path, bool Resource, long StoredBytes);
public sealed record ArchiveEdit(string SourceSha256, string OutputSha256, long SourceBytes, long OutputBytes, IReadOnlyList<string> Entries);

// File-backed OPEN RPF7 editing. Retain every existing payload byte, append
// replacements, and update only their table rows. Never modify the source.
public static class RpfFileEditor {
    sealed record Row(string Path, int Index, bool Resource, long Offset, uint Size, uint Uncompressed, uint Flags);
    sealed class Index : IDisposable {
        public FileStream File { get; }
        public byte[] Table { get; }
        public int Shift { get; }
        public Dictionary<string, Row> Rows { get; } = new(StringComparer.OrdinalIgnoreCase);
        public Index(string path, CancellationToken cancel) {
            File = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
            try {
                byte[] header = new byte[16]; File.ReadExactly(header);
                uint count = U32(header, 4), namesWord = U32(header, 8), encryption = U32(header, 12);
                int names = (int)(namesWord & 0xfffffff); Shift = (int)(namesWord >> 28);
                Need(U32(header, 0) == 0x52504637 && count is >= 1 and <= 65536 && names is >= 1 and <= 1048576 && Shift <= 4, "Unsupported or invalid RPF7 table.");
                Need(encryption is 0 or 0x4e45504f, "This archive has an encrypted table. Select an OPEN copy saved by OpenIV or CodeWalker.");
                Table = new byte[checked(16 + (int)count * 16 + names)]; header.CopyTo(Table, 0); File.ReadExactly(Table.AsSpan(16));
                Need(U32(Table,20)==0x7fffff00,"RPF root is not a directory.");
                int namesAt = 16 + (int)count * 16; var used = new HashSet<int>(); var ranges = new List<(long Start,long End)>();
                void Walk(int i, string prefix, int depth) {
                    cancel.ThrowIfCancellationRequested(); Need(i >= 0 && i < count && depth <= 32 && used.Add(i), "Cyclic/aliased RPF table.");
                    int at = 16 + i * 16; bool directory = U32(Table, at + 4) == 0x7fffff00;
                    uint nameOffset = directory ? U32(Table, at) : BinaryPrimitives.ReadUInt16LittleEndian(Table.AsSpan(at));
                    Need(nameOffset < names, "RPF name is outside its name pool.");
                    int start = namesAt + (int)nameOffset, end = Array.IndexOf(Table, (byte)0, start, names - (int)nameOffset);
                    Need(end >= start && end - start <= 180, "Invalid RPF entry name."); string name = new UTF8Encoding(false,true).GetString(Table,start,end-start);
                    if (i != 0 || name.Length > 0) { InputLoader.SafeRelative(name); Need(!name.Contains('/') && !name.Contains('\\'), "Invalid RPF name segment."); }
                    string full = i == 0 ? "" : prefix + name;
                    if (directory) {
                        uint first = U32(Table,at+8), children = U32(Table,at+12); Need((ulong)first + children <= count, "Invalid RPF child range.");
                        var unique = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                        for(int n=0;n<children;n++) {
                            int child = checked((int)first+n); Walk(child,full.Length==0?"":full+"/",depth+1);
                            int childAt=16+child*16; uint no=U32(Table,childAt+4)==0x7fffff00?U32(Table,childAt):BinaryPrimitives.ReadUInt16LittleEndian(Table.AsSpan(childAt));
                            int p=namesAt+(int)no; Need(unique.Add(Encoding.UTF8.GetString(Table,p,Array.IndexOf(Table,(byte)0,p)-p)),"Duplicate archive entry names.");
                        }
                    } else {
                        bool resource = (Table[at+7]&0x80)!=0; uint size=U24(Table,at+2); long offset=(long)(U24(Table,at+5)&0x7fffff) << (9+Shift);
                        uint uncompressed=U32(Table,at+8),flags=U32(Table,at+12);
                        if(resource && size==0xffffff) { Need(offset>=Table.Length && offset<=File.Length-16,"Invalid large resource offset."); byte[] h=new byte[16]; File.Position=offset;File.ReadExactly(h);size=(uint)(h[7]|h[14]<<8|h[5]<<16|h[2]<<24); }
                        if(!resource && size==0)size=uncompressed;
                        Need(size==0&&!resource || offset>=Table.Length && offset<=File.Length-size,"Archive payload outside file bounds.");
                        if(size>0)ranges.Add((offset,offset+size));
                        Need(Rows.TryAdd(full,new(full,i,resource,offset,size,uncompressed,flags)),"Duplicate archive path.");
                    }
                }
                Walk(0,"",0);Need(used.Count==count,"Archive contains unreachable rows.");
                var sorted=ranges.OrderBy(r=>r.Start).ToList();for(int n=1;n<sorted.Count;n++)Need(sorted[n-1].End<=sorted[n].Start,"Overlapping archive payloads are unsupported.");
            } catch { File.Dispose(); throw; }
        }
        public Row Find(string path) {
            path=InputLoader.SafeRelative(path);return Rows.TryGetValue(path,out var row)?row:throw new InvalidDataException("Entry not found: "+path+". Select the archive that actually contains this asset.");
        }
        public void Dispose()=>File.Dispose();
    }
    static void Need(bool value,string message){if(!value)throw new InvalidDataException(message);}
    static uint U32(byte[] b,int p)=>BinaryPrimitives.ReadUInt32LittleEndian(b.AsSpan(p));
    static uint U24(byte[] b,int p)=>(uint)(b[p]|b[p+1]<<8|b[p+2]<<16);
    static void W24(byte[] b,int p,uint value){b[p]=(byte)value;b[p+1]=(byte)(value>>8);b[p+2]=(byte)(value>>16);}
    public static IReadOnlyList<ArchiveEntry> List(string archive,CancellationToken cancel=default){using var index=new Index(archive,cancel);return index.Rows.Values.Select(r=>new ArchiveEntry(r.Path,r.Resource,r.Size)).ToList();}
    public static byte[] ReadBinary(string archive,string entry,int maximum=4*1024*1024,CancellationToken cancel=default) {
        using var index=new Index(archive,cancel);var row=index.Find(entry);
        Need(!row.Resource&&row.Flags==0,"Selected entry is a resource or encrypted binary; XML/data extraction requires an ordinary binary entry.");
        Need(row.Size<=maximum&&row.Uncompressed<=maximum,"Selected archive entry exceeds its read limit.");
        byte[] data=new byte[row.Size];index.File.Position=row.Offset;index.File.ReadExactly(data);cancel.ThrowIfCancellationRequested();
        return U24(index.Table,16+row.Index*16+2)==0?data:ResourceContainer.Inflate(data,maximum,checked((int)row.Uncompressed));
    }
    public static string HashFile(string path,CancellationToken cancel=default,Action<string>? progress=null) {
        using var file=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.Read);return Hash(file,null,cancel,progress,"Verifying archive");
    }
    static string Hash(Stream source,Stream? copy,CancellationToken cancel,Action<string>? progress,string label) {
        source.Position=0;using var hash=IncrementalHash.CreateHash(HashAlgorithmName.SHA256);byte[] buffer=new byte[128*1024];int got,last=-1;
        while((got=source.Read(buffer))>0){cancel.ThrowIfCancellationRequested();hash.AppendData(buffer,0,got);copy?.Write(buffer,0,got);int percent=(int)(100*source.Position/Math.Max(1,source.Length))/10*10;if(percent!=last){progress?.Invoke(label+" "+percent+"%");last=percent;}}
        cancel.ThrowIfCancellationRequested();return Convert.ToHexString(hash.GetHashAndReset()).ToLowerInvariant();
    }
    public static ArchiveEdit WriteCopy(string archive,string output,IReadOnlyDictionary<string,byte[]> replacements,CancellationToken cancel=default,Action<string>? progress=null,IReadOnlyDictionary<string,byte[]>? expected=null) {
        archive=Path.GetFullPath(archive);output=Path.GetFullPath(output);
        if(File.Exists(output)||Directory.Exists(output)||output.Equals(archive,StringComparison.OrdinalIgnoreCase))throw new IOException("Choose a new archive output file.");
        if(replacements.Count is <1 or >128)throw new ArgumentException("Select 1-128 existing entries.");
        using var index=new Index(archive,cancel);var prepared=new List<(Row Row,byte[] Data)>();var paths=new HashSet<string>(StringComparer.OrdinalIgnoreCase);long total=0;
        foreach(var item in replacements){var row=index.Find(item.Key);Need(paths.Add(row.Path),"Duplicate replacement entry.");Need(item.Value.Length<=ResourceContainer.MaxBytes&&(total+=item.Value.Length)<=RpfArchive.MaxBytes,"Replacement data exceeds its memory limit.");if(row.Resource)_=ResourceContainer.Read(item.Value);prepared.Add((row,item.Value));}
        if(expected!=null)foreach(var item in expected){var row=index.Find(item.Key);Need(!row.Resource&&row.Flags==0&&row.Size<=4*1024*1024&&row.Uncompressed<=4*1024*1024,"Cannot validate original entry.");byte[] original=new byte[row.Size];index.File.Position=row.Offset;index.File.ReadExactly(original);if(U24(index.Table,16+row.Index*16+2)!=0)original=ResourceContainer.Inflate(original,4*1024*1024,checked((int)row.Uncompressed));Need(original.SequenceEqual(item.Value),"The selected archive changed during preparation. Choose it again and retry.");}
        string parent=Path.GetDirectoryName(output)!;Directory.CreateDirectory(parent);string temporary=Path.Combine(parent,".porter-"+Guid.NewGuid().ToString("N")+".partial");
        try {
            string sourceHash;
            using(var writer=new FileStream(temporary,FileMode.CreateNew,FileAccess.ReadWrite,FileShare.None)) {
                sourceHash=Hash(index.File,writer,cancel,progress,"Copying current archive");
                int alignment=1<<(9+index.Shift);
                foreach(var item in prepared){cancel.ThrowIfCancellationRequested();long padding=(alignment-writer.Length%alignment)%alignment;writer.Position=writer.Length;writer.Write(new byte[padding]);long offset=writer.Position;Need(offset/alignment<=0x7fffff,"Replacement exceeds the archive offset range.");byte[] data=item.Data;int at=16+item.Row.Index*16;
                    if(item.Row.Resource){data=(byte[])data.Clone();uint size=(uint)data.Length;W24(index.Table,at+2,Math.Min(size,0xffffff));W24(index.Table,at+5,(uint)(offset/alignment)|0x800000);BinaryPrimitives.WriteUInt32LittleEndian(index.Table.AsSpan(at+8),U32(data,8));BinaryPrimitives.WriteUInt32LittleEndian(index.Table.AsSpan(at+12),U32(data,12));if(size>=0xffffff){data[7]=(byte)size;data[14]=(byte)(size>>8);data[5]=(byte)(size>>16);data[2]=(byte)(size>>24);}}
                    else {W24(index.Table,at+2,0);W24(index.Table,at+5,(uint)(offset/alignment));BinaryPrimitives.WriteUInt32LittleEndian(index.Table.AsSpan(at+8),(uint)data.Length);BinaryPrimitives.WriteUInt32LittleEndian(index.Table.AsSpan(at+12),0);}
                    for(int p=0;p<data.Length;p+=128*1024){cancel.ThrowIfCancellationRequested();writer.Write(data,p,Math.Min(128*1024,data.Length-p));}writer.Position=at;writer.Write(index.Table,at,16);
                }
                writer.Position=writer.Length;writer.Write(new byte[(alignment-writer.Length%alignment)%alignment]);
                writer.Flush(true);
            }
            // Parse the result independently and compare every new stored payload.
            using(var check=new Index(temporary,cancel))foreach(var item in prepared){var row=check.Find(item.Row.Path);byte[] data=new byte[row.Size];check.File.Position=row.Offset;check.File.ReadExactly(data);if(item.Row.Resource){BinaryPrimitives.WriteUInt32LittleEndian(data,0x37435352);BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(4),((row.Uncompressed>>28)<<4)|(row.Flags>>28));BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(8),row.Uncompressed);BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(12),row.Flags);var resource=ResourceContainer.Read(data);var requested=ResourceContainer.Read(item.Data);Need(resource.Version==requested.Version&&resource.System.SequenceEqual(requested.System)&&resource.Graphics.SequenceEqual(requested.Graphics),"Resource replacement readback differs.");}else Need(data.SequenceEqual(item.Data),"Binary replacement readback differs.");}
            string outputHash=HashFile(temporary,cancel,progress);long bytes=new FileInfo(temporary).Length;cancel.ThrowIfCancellationRequested();File.Move(temporary,output);
            return new(sourceHash,outputHash,index.File.Length,bytes,prepared.Select(p=>p.Row.Path).ToList());
        } finally {if(File.Exists(temporary))File.Delete(temporary);}
    }
}
