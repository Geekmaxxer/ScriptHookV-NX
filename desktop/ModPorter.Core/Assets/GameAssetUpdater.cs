using System.Text;
using System.Xml;

namespace ModPorter.Core.Assets;

public sealed record GameAssetUpdate(string Directory,string UpdatedArchive,string Manifest,ArchiveEdit Edit);
public static class GameAssetUpdater {
    public const string TitleId="0100b00b51230000";
    const string DlcList="common/data/dlclist.xml";
    public static string ResolveUpdate(string selected) {
        selected=Path.GetFullPath(selected);Regular(selected);
        if(File.Exists(selected))return selected;
        if(!Directory.Exists(selected))throw new DirectoryNotFoundException("Select a mounted SD drive, game folder or local update.rpf copy.");
        string[] candidates=["atmosphere/contents/"+TitleId+"/romfs/update/update.rpf","romfs/update/update.rpf","update/update.rpf","update.rpf"];
        var found=candidates.Select(p=>Path.Combine(selected,p.Replace('/',Path.DirectorySeparatorChar))).Where(File.Exists).Distinct(StringComparer.OrdinalIgnoreCase).ToList();
        if(found.Count!=1)throw new IOException(found.Count==0?"No update.rpf found. Choose the archive file directly, or select its game/SD folder.":"More than one update.rpf found. Choose the exact archive file to update.");
        Regular(found[0]);return found[0];
    }
    static void Regular(string path) {
        for(string? p=Path.GetFullPath(path);p!=null;p=Path.GetDirectoryName(p))if((File.Exists(p)||Directory.Exists(p))&&(File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Choose ordinary files/folders; linked archive paths are unsupported.");
    }
    static string NewOutput(string output) {
        output=Path.GetFullPath(output);Regular(output);
        if(Directory.Exists(output)||File.Exists(output))throw new IOException("Choose a new output folder. Existing game files are never overwritten.");
        return output;
    }
    public static GameAssetUpdate AddDlc(string convertedDlc,string selectedGame,string id,string output,CancellationToken cancel=default,Action<string>? progress=null) {
        if(!PortProject.ValidId(id)||id!=id.ToLowerInvariant())throw new ArgumentException("DLC ID requires 1-31 lowercase letters, digits, underscores or hyphens.");
        convertedDlc=Path.GetFullPath(convertedDlc);Regular(convertedDlc);string archive=ResolveUpdate(selectedGame);output=NewOutput(output);
        // Keep both source identities stable throughout preparation/copying.
        using var dlc=new FileStream(convertedDlc,FileMode.Open,FileAccess.Read,FileShare.Read);
        if(dlc.Length>ResourceContainer.MaxBytes)throw new InvalidDataException("Prepared DLC exceeds 256 MiB. Reduce or split the asset pack first.");
        progress?.Invoke("Checking prepared DLC and current game list");
        var report=AssetService.Analyze(convertedDlc,new(0),cancel);
        if(report.ConvertedNames!=0)throw new InvalidDataException("This DLC still needs Switch naming conversion. Use Prepare Switch assets first.");
        byte[] data=new byte[dlc.Length];dlc.ReadExactly(data);var tree=RpfArchive.Read(data,cancel);
        if(!new[]{"content.xml","setup2.xml"}.All(name=>tree.Children.Any(n=>!n.Directory&&n.Name.Equals(name,StringComparison.OrdinalIgnoreCase))))throw new InvalidDataException("Select a complete prepared DLC with content.xml and setup2.xml.");
        byte[] originalList=RpfFileEditor.ReadBinary(archive,DlcList,cancel:cancel),merged=DlcPackageService.MergeList(originalList,id);
        cancel.ThrowIfCancellationRequested();Directory.CreateDirectory(output);
        string romfs=Path.Combine(output,"atmosphere","contents",TitleId,"romfs"),updated=Path.Combine(romfs,"update","update.rpf");
        var edit=RpfFileEditor.WriteCopy(archive,updated,new Dictionary<string,byte[]>{{DlcList,merged}},cancel,progress,new Dictionary<string,byte[]>{{DlcList,originalList}});
        string target=Path.Combine(romfs,"update","switch","dlcpacks",id,"dlc.rpf");Directory.CreateDirectory(Path.GetDirectoryName(target)!);
        progress?.Invoke("Copying prepared DLC");
        using(var dest=new FileStream(target,FileMode.CreateNew,FileAccess.Write,FileShare.None)){dlc.Position=0;byte[] buffer=new byte[128*1024];int n;while((n=dlc.Read(buffer))>0){cancel.ThrowIfCancellationRequested();dest.Write(buffer,0,n);}dest.Flush(true);}
        string dlcHash=RpfFileEditor.HashFile(convertedDlc,cancel),copyHash=RpfFileEditor.HashFile(target,cancel);if(dlcHash!=copyHash)throw new IOException("Prepared DLC copy failed verification.");
        string manifest=Path.Combine(output,"game-update.json");JsonFiles.Write(manifest,new{operation="add_dlc",id,source_archive=archive,source_archive_sha256=edit.SourceSha256,updated_archive=Path.GetRelativePath(output,updated),updated_archive_sha256=edit.OutputSha256,dlc_sha256=copyHash,changed_entries=edit.Entries,source_files_modified=false,complete=true,switch_runtime_verified=false});
        File.WriteAllText(Path.Combine(output,"READ-ME-FIRST.md"),"# Prepared game files\n\nClose GTA. Keep your existing update.rpf and any dlcpacks/"+id+" folder backed up, then copy atmosphere to the SD root. The updated archive is based on the current game file selected during preparation and retains its other entries. Restart GTA and test the asset.\n\nUse this result with that same game installation. To undo it, restore the original update.rpf and original DLC folder, or remove the newly added DLC folder. No ScriptHookV-NX runtime update is required for asset files.\n",new UTF8Encoding(false));
        progress?.Invoke("Updated game files verified");return new(output,updated,manifest,edit);
    }
    public static GameAssetUpdate ReplaceAsset(string convertedAsset,string selectedArchive,string entry,string output,CancellationToken cancel=default,Action<string>? progress=null) {
        convertedAsset=Path.GetFullPath(convertedAsset);Regular(convertedAsset);string archive=Path.GetFullPath(selectedArchive);Regular(archive);output=NewOutput(output);
        using var source=new FileStream(convertedAsset,FileMode.Open,FileAccess.Read,FileShare.Read);
        if(source.Length>ResourceContainer.MaxBytes)throw new InvalidDataException("Asset exceeds 256 MiB.");
        string ext=Path.GetExtension(convertedAsset).ToLowerInvariant();
        if(AssetService.ResourceExtensions.Contains(ext)){var analysis=AssetService.Analyze(convertedAsset,new(0),cancel);if(analysis.ConvertedNames!=0)throw new InvalidDataException("Prepare the asset for Switch before replacing it.");}
        else if(ext is not (".xml" or ".meta"))throw new InvalidDataException("Choose a prepared resource or XML/meta file. Nested archives and scripts use separate tools.");
        byte[] bytes=new byte[source.Length];source.ReadExactly(bytes);
        if(ext is ".xml" or ".meta"){using var xml=XmlReader.Create(new MemoryStream(bytes,false),new XmlReaderSettings{DtdProcessing=DtdProcessing.Prohibit,XmlResolver=null,MaxCharactersInDocument=4*1024*1024});while(xml.Read())cancel.ThrowIfCancellationRequested();}
        entry=InputLoader.SafeRelative(entry);if(!Path.GetExtension(entry).Equals(ext,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("The replacement and target entry must have the same extension.");
        string updated=Path.Combine(output,"updated",Path.GetFileName(archive));
        var edit=RpfFileEditor.WriteCopy(archive,updated,new Dictionary<string,byte[]>{{entry,bytes}},cancel,progress);
        string manifest=Path.Combine(output,"game-update.json");JsonFiles.Write(manifest,new{operation="replace_existing_asset",source_archive=archive,source_archive_sha256=edit.SourceSha256,updated_archive=Path.GetRelativePath(output,updated),updated_archive_sha256=edit.OutputSha256,changed_entries=edit.Entries,source_files_modified=false,complete=true,switch_runtime_verified=false});
        File.WriteAllText(Path.Combine(output,"READ-ME-FIRST.md"),"# Updated archive copy\n\nClose GTA. Back up the selected original archive, then replace it in its original game location with the file in updated/. Other archive entries are retained. Restart GTA and test the replacement. Restore the original archive to undo this change.\n\nThis result is an archive copy, not an SD-root package. Use it with the installation selected during preparation.\n",new UTF8Encoding(false));
        return new(output,updated,manifest,edit);
    }
}
