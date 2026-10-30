using System.IO.Compression;
using System.Text;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public sealed record DlcModel(string Name, string Kind, string MetadataPath, string Type) {
    public string Key => MetadataPath + "|" + Name;
    public override string ToString() => Name + " · " + Kind + (Type.Length == 0 ? "" : " · " + Type);
}
public sealed record DlcAsset(string Path, string Name, int Bytes) { public override string ToString() => Name + " · " + (Bytes / 1024.0).ToString("N1") + " KiB"; }
public sealed record DlcField(string Path, string Value);
public sealed record DlcWorkspaceOutput(string Directory, string DlcArchive, string Archive, bool UpdateArchiveWritten);

public sealed partial class DlcWorkspace {
    AssetNode root;
    AssetNode? saved;
    string savedId = "";
    AssetNode? indexedRoot;
    Dictionary<string, AssetNode>? fileIndex;
    Dictionary<string, AssetNode> Files => ReferenceEquals(indexedRoot,root) && fileIndex != null ? fileIndex : Reindex();
    Dictionary<string, AssetNode> Reindex() { fileIndex=DlcTree.Files(root); indexedRoot=root; return fileIndex; }
    readonly List<AssetNode> undo = [], redo = [];
    public string Id { get; private set; }
    public string OriginalPath { get; private set; } = "";
    public bool IsDirty => !ReferenceEquals(root, saved) || Id != savedId;
    public bool CanUndo => undo.Count != 0;
    public bool CanRedo => redo.Count != 0;
    DlcWorkspace(AssetNode tree, string id) { CheckId(id); root = tree; Id = id; }
    static void CheckId(string id) { if (!PortProject.ValidId(id) || id != id.ToLowerInvariant()) throw new ArgumentException("Use a lowercase DLC folder ID with 1-31 letters, digits, underscores or hyphens."); }
    internal static void CheckModel(string name) { if (name.Length is < 1 or > 63 || name != name.ToLowerInvariant() || name.Any(c => !(char.IsAsciiLetterOrDigit(c) || c == '_'))) throw new ArgumentException("Model names require 1-63 lowercase letters, digits or underscores."); }
    public static DlcWorkspace Create(string id = "my-mods") {
        CheckId(id);
        var setup = new XDocument(new XElement("SSetupData", new XElement("deviceName", "dlc_" + id), new XElement("datFile", "content.xml"), new XElement("nameHash", id),
            new XElement("contentChangeSetGroups", new XElement("Item", new XElement("NameHash", "GROUP_STARTUP"), new XElement("ContentChangeSets"))),
            new XElement("type", "EXTRACONTENT_COMPAT_PACK"), new XElement("order", new XAttribute("value", "100"))));
        var content = new XDocument(new XElement("CDataFileMgr__ContentsOfDataFileXml", new XElement("disabledFiles"), new XElement("includedXmlFiles"), new XElement("includedDataFiles"), new XElement("dataFiles"), new XElement("contentChangeSets")));
        return new(DlcTree.Dir("", DlcTree.File("setup2.xml", DlcTree.Xml(setup)), DlcTree.File("content.xml", DlcTree.Xml(content))), id);
    }
    public static DlcWorkspace Open(string path, CancellationToken cancel = default) {
        path = System.IO.Path.GetFullPath(path);
        if (new FileInfo(path).Length > RpfArchive.MaxBytes) throw new InvalidDataException("DLC exceeds the editor's 512 MiB archive limit.");
        var tree = RpfArchive.Read(File.ReadAllBytes(path), cancel); var files = DlcTree.TopFiles(tree);
        var setup = AddonAudit.Xml(files.TryGetValue("setup2.xml", out var node) ? node.Data : throw new InvalidDataException("Choose a complete DLC archive containing setup2.xml."));
        if (setup.Root?.Name.LocalName != "SSetupData") throw new InvalidDataException("Unrecognized DLC setup metadata.");
        string id = System.IO.Path.GetFileName(System.IO.Path.GetDirectoryName(path)) ?? "";
        if (!PortProject.ValidId(id) || new[] { "sp", "converted", "output", "addon" }.Contains(id.ToLowerInvariant())) id = DlcTree.Value(setup.Root, "nameHash").ToLowerInvariant();
        if (!PortProject.ValidId(id)) id = DlcTree.Value(setup.Root, "deviceName").Replace("dlc_", "", StringComparison.OrdinalIgnoreCase).ToLowerInvariant();
        _ = Models(tree);
        var result = new DlcWorkspace(tree, id.ToLowerInvariant()) { OriginalPath = path }; result.saved = tree; result.savedId=result.Id; return result;
    }
    public void SetId(string id) { CheckId(id); Id = id; }
    internal void Commit(Action<AssetNode> edit, bool requireReady = false, CancellationToken cancel = default) {
        cancel.ThrowIfCancellationRequested(); var next = DlcTree.Clone(root); edit(next); cancel.ThrowIfCancellationRequested();
        // Working copies may have repairable asset errors, but every committed
        // edit must still be indexable by the model editor and undo controls.
        _ = Models(next);
        if (requireReady) RequireReady(next, cancel);
        undo.Add(root); root = next; redo.Clear(); TrimHistory();
    }
    void TrimHistory() {
        // History shares unchanged byte arrays and has a bounded extra payload budget.
        while (undo.Count + redo.Count > 12 || HistoryBytes() > 128L * 1024 * 1024) {
            if (undo.Count > 0) undo.RemoveAt(0); else if (redo.Count > 0) redo.RemoveAt(0); else break;
        }
    }
    long HistoryBytes() {
        var seen = new HashSet<byte[]>(ReferenceEqualityComparer.Instance);
        void Mark(AssetNode n) { seen.Add(n.Data); foreach (var c in n.Children) Mark(c); } Mark(root);
        long bytes = 0;
        void Count(AssetNode n) { if (seen.Add(n.Data)) bytes += n.Data.Length; foreach (var c in n.Children) Count(c); }
        foreach (var n in undo.Concat(redo)) Count(n); return bytes;
    }
    public void Undo() { if (!CanUndo) return; redo.Add(root); root = undo[^1]; undo.RemoveAt(undo.Count - 1); }
    public void Redo() { if (!CanRedo) return; undo.Add(root); root = redo[^1]; redo.RemoveAt(redo.Count - 1); }
    internal static string ContentPath(AssetNode tree) => DlcTree.Value(AddonAudit.Xml(tree.Children.Single(n=>n.Name.Equals("setup2.xml",StringComparison.OrdinalIgnoreCase)).Data).Root, "datFile");
    internal static string Device(AssetNode tree) => DlcTree.Value(AddonAudit.Xml(tree.Children.Single(n=>n.Name.Equals("setup2.xml",StringComparison.OrdinalIgnoreCase)).Data).Root, "deviceName");
    internal static string Local(string value, string device) {
        value = value.Trim().Replace('\\', '/').Replace("%PLATFORM%", "switch", StringComparison.OrdinalIgnoreCase);
        if (value.StartsWith(device + ":/", StringComparison.OrdinalIgnoreCase)) value = value[(device.Length + 2)..];
        return InputLoader.SafeRelative(value);
    }
    internal static IReadOnlyList<DlcModel> Models(AssetNode tree, Dictionary<string,AssetNode>? index = null) {
        var files = index ?? DlcTree.TopFiles(tree);
        if(!files.TryGetValue("setup2.xml",out var setupNode))throw new InvalidDataException("DLC setup2.xml is missing.");
        var setup = AddonAudit.Xml(setupNode.Data);
        if(setup.Root?.Name.LocalName!="SSetupData")throw new InvalidDataException("DLC setup must have an SSetupData root.");
        string device = DlcTree.Value(setup.Root, "deviceName"), path = DlcTree.Value(setup.Root, "datFile");
        if(device.Length is < 1 or > 63||device.Any(c=>!(char.IsAsciiLetterOrDigit(c)||c is '_' or '-')))throw new InvalidDataException("DLC deviceName must be a valid identifier.");
        _ = InputLoader.SafeRelative(path);
        var content = AddonAudit.Xml(files.TryGetValue(path, out var contentNode) ? contentNode.Data : throw new InvalidDataException("DLC content file is missing: " + path));
        if(content.Root?.Name.LocalName!="CDataFileMgr__ContentsOfDataFileXml")throw new InvalidDataException("Unrecognized DLC content root.");
        var lists=content.Root.Elements().Where(e=>e.Name.LocalName=="dataFiles").ToArray();
        if(lists.Length!=1)throw new InvalidDataException("DLC content must contain one dataFiles section.");
        var result = new List<DlcModel>();
        var keys=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var entry in lists[0].Elements()) {
            string type = DlcTree.Value(entry, "fileType"); if (type is not ("PED_METADATA_FILE" or "VEHICLE_METADATA_FILE")) continue;
            string filename = Local(DlcTree.Value(entry, "filename"), device);
            if (!files.TryGetValue(filename, out var metadata)) throw new InvalidDataException("Registered model metadata is missing: "+filename);
            var doc = AddonAudit.Xml(metadata.Data);
            string expected=type=="PED_METADATA_FILE"?"CPedModelInfo__InitDataList":"CVehicleModelInfo__InitDataList";
            var definitions=doc.Root?.Elements().Where(e=>e.Name.LocalName=="InitDatas").ToArray();
            if(doc.Root?.Name.LocalName!=expected||definitions?.Length!=1)throw new InvalidDataException("Model metadata requires "+expected+"/InitDatas: "+filename);
            foreach (var item in definitions[0].Elements()) {
                bool ped = type == "PED_METADATA_FILE";
                var identity=item.Elements().Where(e=>e.Name.LocalName==(ped?"Name":"modelName")).ToArray();
                if(item.Name.LocalName!="Item"||identity.Length!=1||string.IsNullOrWhiteSpace(identity[0].Value))throw new InvalidDataException("Each model Item requires exactly one non-empty "+(ped?"Name":"modelName")+" field: "+filename);
                string name=identity[0].Value.Trim();
                if(!keys.Add(filename+"|"+name))throw new InvalidDataException("Duplicate model definition: "+name+" in "+filename);
                result.Add(new(name, ped ? "PED" : "Vehicle", filename, DlcTree.Value(item, ped ? "Pedtype" : "type")));
            }
        }
        return result.OrderBy(m => m.Kind).ThenBy(m => m.Name, StringComparer.OrdinalIgnoreCase).ToArray();
    }
    public IReadOnlyList<DlcModel> Models() => Models(root);
    public IReadOnlyList<string> MetadataFiles() => Files.Keys.Where(p => System.IO.Path.GetExtension(p).ToLowerInvariant() is ".meta" or ".xml").Order(StringComparer.OrdinalIgnoreCase).ToArray();
    public byte[] ReadAsset(string path) => Files.TryGetValue(path, out var n) ? (byte[])n.Data.Clone() : throw new InvalidDataException("Asset not found: " + path);
    public string ReadMetadata(string path) => AddonAudit.Xml(ReadAsset(path)).ToString();
    internal static XElement Definition(XDocument doc, DlcModel model) => doc.Root!.Elements().Where(e => e.Name.LocalName == "InitDatas").Elements()
        .SingleOrDefault(e => DlcTree.Value(e, model.Kind == "PED" ? "Name" : "modelName").Equals(model.Name, StringComparison.OrdinalIgnoreCase)) ?? throw new InvalidDataException("Model definition is missing or duplicated: " + model.Name);
    public string DefinitionXml(DlcModel model) => Definition(AddonAudit.Xml(ReadAsset(model.MetadataPath)), model).ToString();
    public IReadOnlyList<DlcAsset> Assets(DlcModel model) {
        var doc = AddonAudit.Xml(ReadAsset(model.MetadataPath)); var item = Definition(doc, model);
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { model.Name, model.Name + "_p", model.Name + "_hi" };
        if (model.Kind == "Vehicle" && DlcTree.Value(item, "txdName") is { Length: > 0 } txd) names.Add(txd);
        return Files.Where(p => p.Key.Contains("::", StringComparison.Ordinal) && AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(p.Value.Name)) && names.Contains(System.IO.Path.GetFileNameWithoutExtension(p.Value.Name)))
            .Select(p => new DlcAsset(p.Key, p.Value.Name, p.Value.Data.Length)).OrderBy(p => p.Path, StringComparer.OrdinalIgnoreCase).ToArray();
    }
    public AddonAudit Audit(CancellationToken cancel = default) => AddonAudit.Inspect(root, cancel);
    static AddonAudit RequireReady(AssetNode tree, CancellationToken cancel) {
        var report = AddonAudit.Inspect(tree, cancel); if (!report.ReadyForTest) throw new InvalidDataException(report.Text()); return report;
    }
    static string Output(string output, string id) {
        if (string.IsNullOrWhiteSpace(output)) output = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "ScriptHookV-NX Projects", "DLC-" + id + "-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N")[..6]);
        output = System.IO.Path.GetFullPath(output);
        if (Directory.Exists(output) || File.Exists(output)) throw new IOException("Choose a new output folder; existing DLCs are preserved.");
        return output;
    }
    static string Archive(string directory, string output, CancellationToken cancel) {
        using (var zip = ZipFile.Open(output, ZipArchiveMode.Create)) foreach (string p in Directory.EnumerateFiles(directory, "*", SearchOption.AllDirectories).Order(StringComparer.Ordinal)) {
            cancel.ThrowIfCancellationRequested(); if (System.IO.Path.GetFullPath(p) != System.IO.Path.GetFullPath(output)) zip.CreateEntryFromFile(p, System.IO.Path.GetRelativePath(directory, p).Replace('\\', '/'), CompressionLevel.Optimal);
        }
        using var readback = ZipFile.OpenRead(output);
        foreach (var e in readback.Entries) { cancel.ThrowIfCancellationRequested(); using var s = e.Open(); if (Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(s)).ToLowerInvariant() != JsonFiles.HashFile(System.IO.Path.Combine(directory,e.FullName))) throw new IOException("DLC ZIP readback failed."); }
        return output;
    }
    public DlcWorkspaceOutput Export(string output, bool createZip = false, CancellationToken cancel = default) {
        var audit = RequireReady(root, cancel); output = Output(output, Id); byte[] data = RpfArchive.Write(root, cancel); RequireReady(RpfArchive.Read(data, cancel), cancel);
        string relative = "atmosphere/contents/" + GameAssetUpdater.TitleId + "/romfs/update/switch/dlcpacks/" + Id + "/dlc.rpf";
        string target = System.IO.Path.Combine(output, relative); Directory.CreateDirectory(System.IO.Path.GetDirectoryName(target)!); File.WriteAllBytes(target, data);
        if (JsonFiles.HashFile(target) != JsonFiles.Hash(data)) throw new IOException("DLC file readback failed.");
        JsonFiles.Write(System.IO.Path.Combine(output,"dlc-workspace.json"), new { format=1,id=Id,dlc=relative,models=Models(),sha256=JsonFiles.Hash(data),complete=true,switch_runtime_verified=false });
        File.WriteAllText(System.IO.Path.Combine(output,"READ-ME-FIRST.md"),"# DLC collection\n\nClose GTA, back up this DLC and copy atmosphere to the SD root. An already registered pack needs only this dlc.rpf. For a new DLC, use Build with game files once to register it.\n\nFolder ID: "+Id+". Spawn models: "+string.Join(", ",audit.Models)+".\n",new UTF8Encoding(false));
        string zip = createZip ? Archive(output,System.IO.Path.Combine(output,Id+"-DLC.zip"),cancel) : ""; saved = root; savedId=Id; return new(output,target,zip,false);
    }
    public DlcWorkspaceOutput PrepareGame(string game, string output, bool createZip = false, CancellationToken cancel = default, Action<string>? progress = null) {
        string current = GameAssetUpdater.ResolveUpdate(game); RequireReady(root, cancel); output = Output(output, Id); Directory.CreateDirectory(output);
        string stage = System.IO.Path.Combine(output,"workspace-stage.rpf"); File.WriteAllBytes(stage,RpfArchive.Write(root,cancel));
        GameAssetUpdate update;
        try { update = GameAssetUpdater.AddDlc(stage,current,Id,System.IO.Path.Combine(output,"install"),cancel,progress); }
        finally { File.Delete(stage); }
        string target = System.IO.Path.Combine(update.Directory,"atmosphere/contents/"+GameAssetUpdater.TitleId+"/romfs/update/switch/dlcpacks/"+Id+"/dlc.rpf");
        string zip = createZip ? Archive(update.Directory,System.IO.Path.Combine(output,Id+"-install.zip"),cancel) : "";
        saved = root; savedId=Id; return new(update.Directory,target,zip,update.UpdateArchiveWritten);
    }
}
