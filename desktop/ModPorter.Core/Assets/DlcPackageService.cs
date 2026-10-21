using System.IO.Compression;
using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public static class DlcPackageService {
    public static byte[] MergeList(byte[] current,string id) {
        if(!PortProject.ValidId(id)||id!=id.ToLowerInvariant())throw new ArgumentException("DLC ID requires 1-31 lowercase letters, digits, underscores or hyphens.");
        if(current.Length>4*1024*1024)throw new InvalidDataException("DLC list exceeds 4 MiB.");
        using var input=new MemoryStream(current,false);
        using var reader=XmlReader.Create(input,new XmlReaderSettings{DtdProcessing=DtdProcessing.Prohibit,XmlResolver=null,MaxCharactersInDocument=4*1024*1024});
        var doc=XDocument.Load(reader,LoadOptions.PreserveWhitespace);
        var paths=doc.Root?.Elements().SingleOrDefault(n=>n.Name.LocalName=="Paths")??throw new InvalidDataException("Expected SMandatoryPacksData/Paths in dlclist.xml.");
        if(doc.Root?.Name.LocalName!="SMandatoryPacksData")throw new InvalidDataException("Unrecognized DLC list root.");
        string entry="dlcpacks:/"+id+"/";
        if(!paths.Elements().Any(n=>n.Name.LocalName=="Item"&&n.Value.Trim().Replace('\\','/').Equals(entry,StringComparison.OrdinalIgnoreCase)))paths.Add(new XElement(paths.Name.Namespace+"Item",entry));
        using var output=new MemoryStream();using(var writer=XmlWriter.Create(output,new XmlWriterSettings{Encoding=new UTF8Encoding(false),Indent=true}))doc.Save(writer);
        return output.ToArray();
    }
    public static string Prepare(string dlcArchive, string currentDlcList, string id, string output, CancellationToken cancel = default) {
        if (!PortProject.ValidId(id) || id != id.ToLowerInvariant()) throw new ArgumentException("DLC ID requires 1–31 lowercase letters, digits, underscores or hyphens.");
        if (Directory.Exists(output) || File.Exists(output)) throw new IOException("Choose a new DLC package output directory.");
        if (new FileInfo(dlcArchive).Length > 256 * 1024 * 1024 || new FileInfo(currentDlcList).Length > 4 * 1024 * 1024) throw new InvalidDataException("DLC/list exceeds preparation limits.");
        var archive = RpfArchive.Read(File.ReadAllBytes(dlcArchive), cancel);
        if (!archive.Children.Any(n => n.Name.Equals("content.xml", StringComparison.OrdinalIgnoreCase)) || !archive.Children.Any(n => n.Name.Equals("setup2.xml", StringComparison.OrdinalIgnoreCase))) throw new InvalidDataException("Choose a complete converted DLC archive with content.xml and setup2.xml.");
        byte[] mergedList=MergeList(File.ReadAllBytes(currentDlcList),id);
        cancel.ThrowIfCancellationRequested(); Directory.CreateDirectory(output);
        string target = Path.Combine(output, "dlcpacks", id); Directory.CreateDirectory(target);
        File.Copy(dlcArchive, Path.Combine(target, "dlc.rpf"));
        string merged = Path.Combine(output, "dlclist.xml");
        File.WriteAllBytes(merged,mergedList);
        File.WriteAllText(Path.Combine(output, "READ-ME-FIRST.md"), "# DLC import package\n\nImport dlcpacks/" + id + "/dlc.rpf into the game's DLC packs location. Import this merged dlclist.xml into update.rpf/common/data/dlclist.xml. It retains the entries from the list selected during preparation.\n\nUse the list from your actual installation, back up the affected archives and test the mod on Switch. These are archive import files, not an SD-root installation.\n", new UTF8Encoding(false));
        JsonFiles.Write(Path.Combine(output, "dlc-package.json"), new { id, archive_sha256 = JsonFiles.HashFile(dlcArchive), original_list_sha256 = JsonFiles.HashFile(currentDlcList), merged_list_sha256 = JsonFiles.HashFile(merged), switch_runtime_verified = false });
        string zipPath = Path.Combine(output, id + "-DLC-import.zip");
        using (var zip = ZipFile.Open(zipPath, ZipArchiveMode.Create)) foreach (string file in Directory.EnumerateFiles(output, "*", SearchOption.AllDirectories).Where(f => f != zipPath)) zip.CreateEntryFromFile(file, Path.GetRelativePath(output,file).Replace('\\','/'));
        using (var zip = ZipFile.OpenRead(zipPath)) foreach (var e in zip.Entries) { using var stream = e.Open(); if (Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream)).ToLowerInvariant() != JsonFiles.HashFile(Path.Combine(output,e.FullName.Replace('/',Path.DirectorySeparatorChar)))) throw new InvalidDataException("DLC ZIP readback mismatch."); }
        return zipPath;
    }
}
