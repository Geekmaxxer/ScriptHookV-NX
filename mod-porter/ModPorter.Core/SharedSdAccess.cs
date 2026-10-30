using System.Buffers.Binary;

namespace ModPorter.Core;

public static class SharedSdAccess {
    public const string NpdmPath = "atmosphere/contents/0100b00b51230000/exefs/main.npdm";
    public const string ExpectedSha256 = "db97307619f2dd536f06c38703721e6164470742a81522edfb273778937e650f";
    public static readonly string[] PackageFiles = [NpdmPath, "Install-SD-Access.ps1", "Install-SD-Access.cmd", "SD-ACCESS.md"];
    const ulong SdCard = 1UL << 21;
    static byte[] Read(string path) {
        long length=new FileInfo(path).Length;
        if(length<128 || length>1048576)throw new InvalidDataException("Invalid NPDM file size.");
        return File.ReadAllBytes(path);
    }
    static int[] Inspect(byte[] data) {
        uint U32(int at) { if (at < 0 || at > data.Length - 4) throw new InvalidDataException("NPDM field out of bounds."); return BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(at,4)); }
        ulong U64(int at) { if (at < 0 || at > data.Length - 8) throw new InvalidDataException("NPDM field out of bounds."); return BinaryPrimitives.ReadUInt64LittleEndian(data.AsSpan(at,8)); }
        if (data.Length < 128 || data.Length > 1048576 || !data.AsSpan(0,4).SequenceEqual("META"u8)) throw new InvalidDataException("Invalid META NPDM.");
        int aci=checked((int)U32(112)), aciSize=checked((int)U32(116)), acid=checked((int)U32(120)), acidSize=checked((int)U32(124));
        if (aci<128 || acid<128 || aciSize<64 || acidSize<576 || (long)aci+aciSize>data.Length || (long)acid+acidSize>data.Length || Math.Max(aci,acid)<Math.Min((long)aci+aciSize,(long)acid+acidSize)) throw new InvalidDataException("Invalid NPDM sections.");
        if (!data.AsSpan(aci,4).SequenceEqual("ACI0"u8) || !data.AsSpan(acid+512,4).SequenceEqual("ACID"u8)) throw new InvalidDataException("Invalid NPDM section magic.");
        ulong title=U64(aci+16);
        if (title!=0x0100b00b51230000UL || U64(acid+528)>title || U64(acid+536)<title) throw new InvalidDataException("NPDM targets another title.");
        var masks=new List<int>();
        foreach (var p in new[] {(Base:aci,Size:aciSize,Header:64,Table:32,Minimum:28),(Base:acid,Size:acidSize,Header:576,Table:544,Minimum:44)}) {
            var sections=new List<(long Begin,long End)>();
            for(int i=0;i<3;i++) {
                long offset=U32(p.Base+p.Table+i*8),length=U32(p.Base+p.Table+i*8+4);
                if(i==0 && length<p.Minimum)throw new InvalidDataException("Short FS permission section.");
                if(length==0)continue;
                if(offset<p.Header || offset+length>p.Size || sections.Any(s=>Math.Max(offset,s.Begin)<Math.Min(offset+length,s.End)))throw new InvalidDataException("Invalid or overlapping FS sections.");
                sections.Add((offset,offset+length));
            }
            int fs=checked(p.Base+(int)U32(p.Base+p.Table));
            if(U32(fs)!=1)throw new InvalidDataException("Unsupported NPDM FS version.");
            masks.Add(fs+4);
        }
        return masks.ToArray();
    }
    public static byte[] WithSdAccess(byte[] original) {
        var output=(byte[])original.Clone();
        foreach(int offset in Inspect(original)) BinaryPrimitives.WriteUInt64LittleEndian(output.AsSpan(offset,8),BinaryPrimitives.ReadUInt64LittleEndian(original.AsSpan(offset,8))|SdCard);
        return output;
    }
    public static void ValidateBundle(string folder) {
        foreach(string relative in PackageFiles)if(!File.Exists(Path.Combine(folder,relative)))throw new FileNotFoundException("SDK/build is missing shared SD-access metadata: "+relative);
        byte[] data=Read(Path.Combine(folder,NpdmPath));
        if(JsonFiles.Hash(data)!=ExpectedSha256 || !data.SequenceEqual(WithSdAccess(data)))throw new InvalidDataException("Bundled SD-access NPDM is invalid.");
    }
    static void RegularPath(string path) {
        if(File.Exists(path) && (File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)throw new IOException("NPDM cannot be a link.");
        for(DirectoryInfo? p=new(Path.GetDirectoryName(Path.GetFullPath(path))!);p!=null;p=p.Parent)
            if(p.Exists&&(p.Attributes&FileAttributes.ReparsePoint)!=0)throw new IOException("NPDM paths cannot contain links/junctions.");
    }
    public static void EnsureForNro(string nro,string sdRoot) {
        string target=Path.Combine(sdRoot,NpdmPath);RegularPath(target);
        bool exists=File.Exists(target);
        byte[] original;
        if(exists) original=Read(target);
        else {
            string folder=Path.GetDirectoryName(Path.GetFullPath(nro))!;
            ValidateBundle(folder);original=Read(Path.Combine(folder,NpdmPath));
        }
        byte[] patched=WithSdAccess(original);
        if(exists && original.SequenceEqual(patched))return;
        if(exists) {
            string backup=Path.Combine(sdRoot,"switch/gtav-scripthook/npdm-backups/main."+JsonFiles.Hash(original)+".npdm");
            RegularPath(backup);Directory.CreateDirectory(Path.GetDirectoryName(backup)!);
            if(!File.Exists(backup))File.WriteAllBytes(backup,original);
            if(!File.ReadAllBytes(backup).SequenceEqual(original))throw new IOException("NPDM backup verification failed.");
        }
        Directory.CreateDirectory(Path.GetDirectoryName(target)!);
        string temporary=target+".sd-access-"+Guid.NewGuid().ToString("N");
        try {
            File.WriteAllBytes(temporary,patched);
            if(!File.ReadAllBytes(temporary).SequenceEqual(patched))throw new IOException("NPDM staging verification failed.");
            File.Move(temporary,target,true);
        } finally { if(File.Exists(temporary))File.Delete(temporary); }
    }
}
