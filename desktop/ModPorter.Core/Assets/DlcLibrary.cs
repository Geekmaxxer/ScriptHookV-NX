namespace ModPorter.Core.Assets;

public sealed record InstalledDlc(string Id, string Archive, bool Registered, bool Exists) {
    public override string ToString() => Id + " · " + (!Exists ? "Not in this folder" : Registered ? "Registered" : "Not registered");
}
public sealed record DlcLibraryResult(string UpdateArchive, string PacksDirectory, IReadOnlyList<InstalledDlc> Packs);

public static class DlcLibrary {
    // Directory/list discovery deliberately avoids loading every model archive.
    public static DlcLibraryResult Scan(string game, CancellationToken cancel = default) {
        string archive = GameAssetUpdater.ResolveUpdate(game);
        var xml = AddonAudit.Xml(RpfFileEditor.ReadBinary(archive,"common/data/dlclist.xml",cancel:cancel));
        if (xml.Root?.Name.LocalName != "SMandatoryPacksData") throw new InvalidDataException("Unrecognized DLC list root.");
        var paths = xml.Root.Elements().SingleOrDefault(e=>e.Name.LocalName=="Paths") ?? throw new InvalidDataException("DLC list has no Paths section.");
        string parent = Path.GetDirectoryName(archive)!;
        string packs = Path.Combine(parent,"switch","dlcpacks");
        var registered = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var entry in paths.Elements().Where(e=>e.Name.LocalName=="Item")) {
            cancel.ThrowIfCancellationRequested(); string value=entry.Value.Trim().Replace('\\','/');
            if (!value.StartsWith("dlcpacks:/",StringComparison.OrdinalIgnoreCase)) continue;
            string id=value[10..].TrimEnd('/');
            if (id.Length is < 1 or > 63 || id.Any(c=>!(char.IsAsciiLetterOrDigit(c)||c is '_' or '-'))) throw new InvalidDataException("Unsafe or malformed DLC registration: "+value);
            registered.Add(id);
            if(registered.Count>2048)throw new InvalidDataException("DLC browser supports at most 2,048 registrations.");
        }
        var folders = new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
        if (Directory.Exists(packs)) foreach (string folder in Directory.EnumerateDirectories(packs)) {
            cancel.ThrowIfCancellationRequested();
            if (folders.Count>=2048) throw new InvalidDataException("DLC browser supports at most 2,048 folders.");
            string id=Path.GetFileName(folder);
            if ((File.GetAttributes(folder)&FileAttributes.ReparsePoint)!=0) continue;
            if (!folders.TryAdd(id,folder)) throw new InvalidDataException("Case-colliding DLC folders: "+id);
        }
        var result = new List<InstalledDlc>();
        foreach(string id in registered.Concat(folders.Keys).Distinct(StringComparer.OrdinalIgnoreCase).Order(StringComparer.OrdinalIgnoreCase)) {
            cancel.ThrowIfCancellationRequested();
            string file=Path.Combine(folders.GetValueOrDefault(id,Path.Combine(packs,id)),"dlc.rpf");
            bool exists=File.Exists(file)&&(File.GetAttributes(file)&FileAttributes.ReparsePoint)==0;
            result.Add(new(id,file,registered.Contains(id),exists));
        }
        return new(archive,packs,result);
    }
}
