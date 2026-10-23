using System.Globalization;
using System.Text;
using System.Xml;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public sealed record AssetIssue(string Severity, string Code, string Path, string Message);
public sealed record AddonModKit(uint Id, string Name);
public sealed class AddonAudit {
    internal static uint ModelHash(string value) { uint hash = 0; foreach (char c in value.ToLowerInvariant()) { hash += c; hash += hash << 10; hash ^= hash >> 6; } hash += hash << 3; hash ^= hash >> 11; return hash + (hash << 15); }
    public List<AssetIssue> Issues { get; } = [];
    public List<string> Models { get; } = [];
    public List<AddonModKit> ModKits { get; } = [];
    public long DeclaredSystemBytes { get; private set; }
    public long DeclaredGraphicsBytes { get; private set; }
    public int Archives { get; private set; }
    public int Resources { get; private set; }
    public bool ReadyForTest => Issues.All(i => i.Severity != "error");
    public string Text() => (ReadyForTest ? "Registration checks passed; gameplay test required." : "Registration blockers must be resolved.")
        + "\n" + string.Join("\n", Issues.Select(i => $"{i.Severity.ToUpperInvariant()} [{i.Code}] {i.Path}: {i.Message}"));
    internal static XDocument Xml(byte[] data) {
        using var stream = new MemoryStream(data, false);
        using var reader = XmlReader.Create(stream, new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null, MaxCharactersInDocument = 4 * 1024 * 1024 });
        return XDocument.Load(reader);
    }
    static string Value(XElement? node, string child) => node?.Elements().FirstOrDefault(e => e.Name.LocalName == child)?.Value.Trim() ?? "";
    public static AddonAudit Inspect(byte[] data, CancellationToken cancel = default) => Inspect(RpfArchive.Read(data, cancel), cancel);
    public static AddonAudit Inspect(AssetNode root, CancellationToken cancel = default) {
        var report = new AddonAudit();
        void Issue(string severity, string code, string path, string text) => report.Issues.Add(new(severity, code, path, text));
        var files = new Dictionary<string, AssetNode>(StringComparer.OrdinalIgnoreCase);
        var xmls = new Dictionary<string, XDocument>(StringComparer.OrdinalIgnoreCase);
        var textureNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var textureReferences = new Dictionary<string, HashSet<string>>(StringComparer.OrdinalIgnoreCase);
        int count = 0; long total = 0;
        void Walk(AssetNode dir, string prefix, int depth) {
            if (depth > 4) throw new InvalidDataException("Add-on archive nesting exceeds four.");
            foreach (var node in dir.Children) {
                cancel.ThrowIfCancellationRequested();
                if (++count > 16384 || (total += node.Data.Length) > 1024L * 1024 * 1024) throw new InvalidDataException("Expanded add-on exceeds audit limits.");
                string path = prefix + node.Name;
                if (node.Directory) { Walk(node, path + "/", depth); continue; }
                if (!files.TryAdd(path, node)) throw new InvalidDataException("Ambiguous add-on path: " + path);
                string ext = System.IO.Path.GetExtension(path).ToLowerInvariant();
                if (ext == ".rpf") { report.Archives++; Walk(RpfArchive.Read(node.Data, cancel), path + "::", depth + 1); }
                else if (AssetService.ResourceExtensions.Contains(ext)) {
                    report.Resources++;
                    if (node.Data.AsSpan().StartsWith("RSC7"u8)) {
                        if (node.Data.Length < 16) throw new InvalidDataException("Truncated resource header.");
                        int systemBytes = ResourceContainer.PageBytes(System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(node.Data.AsSpan(8)));
                        report.DeclaredSystemBytes += systemBytes; report.DeclaredGraphicsBytes += ResourceContainer.PageBytes(System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(node.Data.AsSpan(12)));
                        if (systemBytes > 16 * 1024 * 1024 && ext is ".ndd" or ".nft" or ".ndr") Issue("warning", "large-model-allocation", path, $"Declared system allocation is {systemBytes / 1048576.0:N1} MiB; inspect mesh/LOD complexity. Texture caps do not reduce it.");
                    }
                    if (ext.StartsWith(".y", StringComparison.Ordinal)) Issue("error", "pc-resource-name", path, "Prepare this resource with its Switch extension first.");
                    if (ext is ".ntd" or ".ytd") foreach (var texture in TextureDictionary.Inspect(ResourceContainer.Read(node.Data))) textureNames.Add(texture.Name);
                    else if (ext is ".ndd" or ".nft" or ".ndr" or ".ydd" or ".yft" or ".ydr") {
                        var references = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                        _ = EmbeddedTextures.Downscale(node.Data, ext, 0, cancel, references, textureNames); textureReferences[path] = references;
                    }
                } else if (ext is ".meta" or ".xml") {
                    try {
                        var doc = Xml(node.Data); xmls.Add(path, doc);
                        foreach (var attr in doc.Descendants().Attributes().Where(a => a.Name.LocalName is "x" or "y" or "z" or "w"))
                            if (!float.TryParse(attr.Value, NumberStyles.Float, CultureInfo.InvariantCulture, out float number) || !float.IsFinite(number))
                                Issue("error", "invalid-number", path, $"{attr.Parent?.Name.LocalName}/@{attr.Name.LocalName} has invalid numeric value '{attr.Value}'.");
                    } catch (XmlException e) { Issue("error", "invalid-xml", path, e.Message); }
                }
            }
        }
        Walk(root, "", 0);
        foreach (var pair in textureReferences) foreach (string name in pair.Value.Where(n => !textureNames.Contains(n)).Order(StringComparer.OrdinalIgnoreCase))
            Issue("warning", "external-shader-texture", pair.Key, "Texture '" + name + "' is referenced but not supplied by this pack; check its declared shared/base-game dictionaries. A missing texture is not reconstructed automatically.");
        if (!xmls.TryGetValue("setup2.xml", out var setup) || setup.Root?.Name.LocalName != "SSetupData") {
            Issue("error", "missing-setup", "setup2.xml", "A complete DLC needs SSetupData metadata."); return report;
        }
        string device = Value(setup.Root, "deviceName"), datFile = Value(setup.Root, "datFile");
        if (device.Length is < 1 or > 63 || device.Any(c => !(char.IsAsciiLetterOrDigit(c) || c is '_' or '-'))) Issue("error", "invalid-device", "setup2.xml", "Supply a valid DLC deviceName.");
        if (!xmls.TryGetValue(datFile, out var content) || content.Root?.Name.LocalName != "CDataFileMgr__ContentsOfDataFileXml") {
            Issue("error", "missing-content", "setup2.xml", "datFile must reference the pack's content.xml."); return report;
        }
        string? Local(string value) {
            value = value.Trim().Replace('\\', '/').Replace("%PLATFORM%", "switch", StringComparison.OrdinalIgnoreCase);
            int marker = value.IndexOf(":/", StringComparison.Ordinal);
            if (marker >= 0) {
                if (!value[..marker].Equals(device, StringComparison.OrdinalIgnoreCase)) return null;
                value = value[(marker + 2)..];
            }
            try { return InputLoader.SafeRelative(value); }
            catch (InvalidDataException) { return null; }
        }
        var registered = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var item in content.Root.Elements().Where(e => e.Name.LocalName == "dataFiles").Elements()) {
            if (registered.Count >= 4096) throw new InvalidDataException("DLC has too many data-file registrations.");
            string filename = Value(item, "filename"), type = Value(item, "fileType");
            string? path = Local(filename);
            if (path == null) { Issue("error", "invalid-local-path", datFile, "Registration does not belong to this DLC device: " + filename); continue; }
            if (!registered.TryAdd(path, type)) { Issue("error", "duplicate-registration", path, "Remove the duplicate dataFiles entry."); continue; }
            if (!files.TryGetValue(path, out var node)) Issue("error", "missing-registered-file", path, "Supply the declared file, or explicitly remove its unused registration and activation.");
            else if (type == "RPF_FILE" && !RpfArchive.IsArchive(node.Data)) Issue("error", "invalid-rpf-registration", path, "RPF_FILE must reference a valid RPF archive.");
        }
        if (registered.Count == 0) Issue("error", "empty-registration", datFile, "No data files are registered.");
        var sets = content.Descendants().Where(e => e.Name.LocalName == "contentChangeSets").Elements().ToArray();
        var groups = setup.Descendants().Where(e => e.Name.LocalName == "contentChangeSetGroups").Elements()
            .Where(e => Value(e, "NameHash").Equals("GROUP_STARTUP", StringComparison.OrdinalIgnoreCase)).ToArray();
        var startup = groups.SelectMany(g => g.Elements().Where(e => e.Name.LocalName == "ContentChangeSets").Elements()).Select(e => e.Value.Trim()).ToHashSet(StringComparer.OrdinalIgnoreCase);
        if (startup.Count == 0) Issue("error", "missing-startup", "setup2.xml", "Activate the content change set through GROUP_STARTUP.");
        var available = sets.Select(s => Value(s, "changeSetName")).ToHashSet(StringComparer.OrdinalIgnoreCase);
        foreach (string name in startup) if (!available.Contains(name)) Issue("error", "missing-change-set", "setup2.xml", "Startup refers to a missing change set: " + name);
        var enabled = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var set in sets.Where(s => startup.Contains(Value(s, "changeSetName"))))
            foreach (var item in set.Elements().Where(e => e.Name.LocalName == "filesToEnable").Elements()) {
                string? path = Local(item.Value);
                if (path == null || !registered.ContainsKey(path)) Issue("error", "unregistered-activation", datFile, "filesToEnable has no local dataFiles entry: " + item.Value);
                else enabled.Add(path);
            }
        foreach (var entry in registered) {
            var item = content.Descendants().FirstOrDefault(e => e.Name.LocalName == "filename" && Local(e.Value)?.Equals(entry.Key, StringComparison.OrdinalIgnoreCase) == true)?.Parent;
            bool disabled = item?.Elements().FirstOrDefault(e => e.Name.LocalName == "disabled")?.Attribute("value")?.Value.Equals("true", StringComparison.OrdinalIgnoreCase) == true;
            if (disabled && !enabled.Contains(entry.Key)) Issue("error", "inactive-file", entry.Key, "Disabled registration is never enabled by GROUP_STARTUP.");
        }
        var mountedResources = files.Where(f => AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(f.Key)))
            .Where(f => registered.Any(r => r.Value == "RPF_FILE" && (!IsDisabled(r.Key) || enabled.Contains(r.Key)) && f.Key.StartsWith(r.Key + "::", StringComparison.OrdinalIgnoreCase)))
            .Select(f => System.IO.Path.GetFileName(f.Key.Split("::").Last())).ToHashSet(StringComparer.OrdinalIgnoreCase);
        bool IsDisabled(string path) => content.Descendants().FirstOrDefault(e => e.Name.LocalName == "filename" && Local(e.Value)?.Equals(path, StringComparison.OrdinalIgnoreCase) == true)?.Parent?
            .Elements().FirstOrDefault(e => e.Name.LocalName == "disabled")?.Attribute("value")?.Value.Equals("true", StringComparison.OrdinalIgnoreCase) == true;
        var modelHashes = new Dictionary<uint, string>();
        var handling = xmls.Values.SelectMany(d => d.Descendants().Where(e => e.Name.LocalName == "handlingName")).Select(e => e.Value.Trim()).ToHashSet(StringComparer.OrdinalIgnoreCase);
        var modKitIds = new HashSet<uint>();
        foreach (var pair in xmls) foreach (var kit in pair.Value.Descendants().Where(e => e.Name.LocalName == "Kits").Elements()) {
            string name = Value(kit, "kitName"); if (name.Length == 0) continue;
            string? raw = kit.Elements().FirstOrDefault(e => e.Name.LocalName == "id")?.Attribute("value")?.Value;
            if (!uint.TryParse(raw, NumberStyles.None, CultureInfo.InvariantCulture, out uint id)) { Issue("error", "invalid-modkit-id", pair.Key, "Mod-kit IDs must be unsigned numbers."); continue; }
            if (!modKitIds.Add(id)) Issue("error", "duplicate-modkit-id", pair.Key, "Two mod kits use ID " + id + ".");
            report.ModKits.Add(new(id, name));
        }
        void Model(string name, string path, params string[] extensions) {
            if (string.IsNullOrWhiteSpace(name) || name.Length > 63 || name.Any(c => !(char.IsAsciiLetterOrDigit(c) || c == '_'))) { Issue("error", "invalid-model-name", path, "Model names require 1-63 ASCII letters, digits or underscores."); return; }
            uint hash = ModelHash(name);
            if (modelHashes.TryGetValue(hash, out var old)) Issue("error", "model-hash-collision", path, $"{name} conflicts with {old} in this pack."); else modelHashes.Add(hash, name);
            report.Models.Add(name);
            foreach (string ext in extensions) if (!mountedResources.Contains(name + ext)) Issue("error", "missing-model-resource", path, $"{name + ext} is absent from the pack's mounted resource archives.");
        }
        foreach (var pair in registered.Where(p => xmls.ContainsKey(p.Key) && (p.Value is "PED_METADATA_FILE" or "VEHICLE_METADATA_FILE"))) {
            var doc = xmls[pair.Key];
            if (pair.Value == "PED_METADATA_FILE") {
                foreach (var item in doc.Descendants().Where(e => e.Name.LocalName == "InitDatas").Elements()) Model(Value(item, "Name"), pair.Key, ".ndd", ".nft", ".nmt", ".ntd");
            } else {
                foreach (var item in doc.Descendants().Where(e => e.Name.LocalName == "InitDatas").Elements()) {
                    string name = Value(item, "modelName"); Model(name, pair.Key, ".nft");
                    string handlingId = Value(item, "handlingId");
                    if (handlingId.Length > 0 && !handling.Contains(handlingId)) Issue("warning", "external-handling", pair.Key, "Verify handling definition '" + handlingId + "' in the current game or another pack.");
                    string txd = Value(item, "txdName");
                    if (txd.Length > 0 && !mountedResources.Contains(txd + ".ntd")) Issue("warning", "external-texture-dictionary", pair.Key, txd + " must exist in the current game or a declared dependency.");
                    string audio = Value(item, "audioNameHash");
                    if (audio.Length > 0 && audio != "0" && !audio.Equals("null", StringComparison.OrdinalIgnoreCase)) Issue("warning", "vehicle-audio-dependency", pair.Key, "Verify engine audio '" + audio + "' against the installed game; it is not inferred from the model.");
                }
            }
        }
        if (report.Models.Count == 0) Issue("warning", "no-model-registration", datFile, "This pack declares no vehicle/PED models; verify its intended asset category.");
        return report;
    }
}
