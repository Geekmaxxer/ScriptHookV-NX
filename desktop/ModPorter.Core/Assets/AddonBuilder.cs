using System.IO.Compression;
using System.Text;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public sealed record PedAddonOptions(string Id, string Model, string Template, string BasePed, int TextureCap = 1024);
public sealed record AddonBuild(string Directory, string DlcArchive, string Archive, AddonAudit Audit);
public static class AddonBuilder {
    static AssetNode Dir(string name, params AssetNode[] children) { var node = new AssetNode { Name = name, Directory = true }; node.Children.AddRange(children); return node; }
    static AssetNode File(string name, byte[] data, bool resource = false) => new() { Name = name, Data = data, Resource = resource };
    static byte[] Bytes(XDocument doc) { using var stream = new MemoryStream(); using (var writer = System.Xml.XmlWriter.Create(stream, new() { Encoding = new UTF8Encoding(false), Indent = true })) doc.Save(writer); return stream.ToArray(); }
    public static byte[] ReadPedTemplate(string path, CancellationToken cancel = default) {
        if (Path.GetExtension(path).Equals(".rpf", StringComparison.OrdinalIgnoreCase)) {
            var matches = RpfFileEditor.List(path, cancel).Where(e => e.Path.EndsWith("/peds.meta", StringComparison.OrdinalIgnoreCase) || e.Path.Equals("peds.meta", StringComparison.OrdinalIgnoreCase)).ToArray();
            if (matches.Length != 1) throw new InvalidDataException("Choose a peds.meta file directly; the selected archive has zero or multiple matching entries.");
            return RpfFileEditor.ReadBinary(path, matches[0].Path, cancel: cancel);
        }
        if (new FileInfo(path).Length > 4 * 1024 * 1024) throw new InvalidDataException("PED template exceeds 4 MiB.");
        cancel.ThrowIfCancellationRequested(); return System.IO.File.ReadAllBytes(path);
    }
    public static IReadOnlyList<string> PedNames(string path, CancellationToken cancel = default) {
        var doc = AddonAudit.Xml(ReadPedTemplate(path, cancel));
        if (doc.Root?.Name.LocalName != "CPedModelInfo__InitDataList") throw new InvalidDataException("Choose target-game PED definition metadata.");
        return doc.Descendants().Where(e => e.Name.LocalName == "InitDatas").Elements().SelectMany(e => e.Elements().Where(n => n.Name.LocalName == "Name")).Select(e => e.Value).Distinct(StringComparer.OrdinalIgnoreCase).Order(StringComparer.OrdinalIgnoreCase).ToArray();
    }
    public static AddonBuild Ped(string input, string output, PedAddonOptions options, CancellationToken cancel = default) {
        if (!PortProject.ValidId(options.Id) || options.Id != options.Id.ToLowerInvariant()) throw new ArgumentException("Use a lowercase DLC ID with 1-31 letters, digits, underscores or hyphens.");
        if (options.Model.Length is < 1 or > 63 || options.Model.Any(c => !(char.IsAsciiLetterOrDigit(c) || c == '_')) || options.Model != options.Model.ToLowerInvariant()) throw new ArgumentException("Use a lowercase model name with 1-63 letters, digits or underscores.");
        input = Path.GetFullPath(input); output = Path.GetFullPath(output);
        if (Directory.Exists(output) || System.IO.File.Exists(output) || Directory.Exists(input) && InputLoader.Inside(output, input)) throw new IOException("Choose a new output folder outside the input.");
        byte[] templateBytes = ReadPedTemplate(options.Template, cancel);
        var template = AddonAudit.Xml(templateBytes);
        if (template.Root?.Name.LocalName != "CPedModelInfo__InitDataList") throw new InvalidDataException("The PED template must be target-game CPedModelInfo__InitDataList metadata.");
        var definitions = template.Root.Elements().Where(e => e.Name.LocalName == "InitDatas").Elements()
            .Where(e => e.Elements().Any(c => c.Name.LocalName == "Name" && c.Value.Equals(options.BasePed, StringComparison.OrdinalIgnoreCase))).ToArray();
        if (definitions.Length != 1) throw new InvalidDataException("The selected base PED must identify exactly one definition in the target metadata.");
        var definition = new XElement(definitions[0]); definition.Elements().Single(e => e.Name.LocalName == "Name").Value = options.Model;
        var peds = new XDocument(new XElement(template.Root.Name,
            template.Root.Elements().Where(e => e.Name.LocalName is not ("InitDatas" or "residentAnims")).Select(e => new XElement(e)),
            new XElement("residentAnims"), new XElement("InitDatas", definition)));
        var conversion = AssetService.Convert(input, Path.Combine(output, "resource-preparation"), new(options.TextureCap, "Peds"), cancel);
        var resources = Dir("");
        string converted = Path.Combine(conversion.Directory, "converted");
        var files = Directory.EnumerateFiles(converted, "*", SearchOption.AllDirectories).ToArray();
        foreach (string ext in new[] { ".ndd", ".nft", ".nmt", ".ntd" }) {
            var matches = files.Where(p => Path.GetFileName(p).Equals(options.Model + ext, StringComparison.OrdinalIgnoreCase)).ToArray();
            if (matches.Length != 1) throw new InvalidDataException("Select one PED variant with exactly one " + options.Model + ext + ".");
            byte[] data = System.IO.File.ReadAllBytes(matches[0]);
            resources.Children.Add(File(options.Model + ext, data, data.AsSpan().StartsWith("RSC7"u8)));
        }
        var extra = files.Where(p => AssetService.ResourceExtensions.Contains(Path.GetExtension(p)) && !Path.GetFileNameWithoutExtension(p).Equals(options.Model, StringComparison.OrdinalIgnoreCase)).ToArray();
        if (extra.Length != 0) throw new InvalidDataException("Select a folder containing one PED model/variant; additional model resources were found.");
        string device = "dlc_" + options.Id, changeSet = options.Id + "_AUTOGEN";
        string archivePath = device + ":/%PLATFORM%/models/cdimages/peds.rpf", metaPath = device + ":/common/data/peds.meta";
        string[] paths = [archivePath, metaPath];
        var content = new XDocument(new XElement("CDataFileMgr__ContentsOfDataFileXml", new XElement("disabledFiles"), new XElement("includedXmlFiles"), new XElement("includedDataFiles"),
            new XElement("dataFiles", paths.Select((p, i) => new XElement("Item", new XElement("filename", p), new XElement("fileType", i == 0 ? "RPF_FILE" : "PED_METADATA_FILE"),
                new XElement("locked", new XAttribute("value", "false")), new XElement("disabled", new XAttribute("value", "true")),
                new XElement("persistent", new XAttribute("value", "false")), new XElement("overlay", new XAttribute("value", "false"))))),
            new XElement("contentChangeSets", new XElement("Item", new XElement("changeSetName", changeSet), new XElement("filesToEnable", paths.Select(p => new XElement("Item", p)))))));
        var setup = new XDocument(new XElement("SSetupData", new XElement("deviceName", device), new XElement("datFile", "content.xml"), new XElement("nameHash", options.Id),
            new XElement("contentChangeSetGroups", new XElement("Item", new XElement("NameHash", "GROUP_STARTUP"), new XElement("ContentChangeSets", new XElement("Item", changeSet)))),
            new XElement("type", "EXTRACONTENT_COMPAT_PACK"), new XElement("order", new XAttribute("value", 100))));
        var pack = Dir("", File("content.xml", Bytes(content)), File("setup2.xml", Bytes(setup)), Dir("common", Dir("data", File("peds.meta", Bytes(peds)))),
            Dir("switch", Dir("models", Dir("cdimages", File("peds.rpf", RpfArchive.Write(resources, cancel))))));
        var audit = AddonAudit.Inspect(pack, cancel);
        if (!audit.ReadyForTest) throw new InvalidDataException(audit.Text());
        string dlc = Path.Combine(output, "addon", options.Id, "dlc.rpf"); Directory.CreateDirectory(Path.GetDirectoryName(dlc)!);
        System.IO.File.WriteAllBytes(dlc, RpfArchive.Write(pack, cancel));
        var readback = AddonAudit.Inspect(System.IO.File.ReadAllBytes(dlc), cancel); if (!readback.ReadyForTest) throw new IOException("DLC readback failed registration validation.");
        string templateHash = JsonFiles.Hash(templateBytes);
        JsonFiles.Write(Path.Combine(output, "addon-manifest.json"), new { format = 1, category = "ped", id = options.Id, model = options.Model, base_ped = options.BasePed,
            template_sha256 = templateHash, dlc_sha256 = JsonFiles.HashFile(dlc), audit = readback, complete = true, switch_runtime_verified = false });
        string guide = "# Prepared PED add-on\n\nUse Add to game with addon/" + options.Id + "/dlc.rpf and the update.rpf from your current installation. This is a complete DLC wrapper; it still needs DLC-list registration.\n\nSpawn model: " + options.Model + ". Base PED: " + options.BasePed + ". Test appearance, animations, variations and persistence on Switch. Skeleton/texture compatibility is not established by registration checks.\n";
        System.IO.File.WriteAllText(Path.Combine(output, "READ-ME-FIRST.md"), guide, new UTF8Encoding(false));
        string zipPath = Path.Combine(output, options.Id + "-addon-test.zip");
        using (var zip = ZipFile.Open(zipPath, ZipArchiveMode.Create)) {
            zip.CreateEntryFromFile(dlc, "addon/" + options.Id + "/dlc.rpf");
            foreach (string name in new[] { "READ-ME-FIRST.md", "addon-manifest.json" }) zip.CreateEntryFromFile(Path.Combine(output, name), name);
            foreach (string notice in files.Where(p => InputLoader.IsLegalNotice(Path.GetFileName(p)))) zip.CreateEntryFromFile(notice, "notices/" + Path.GetRelativePath(converted, notice).Replace('\\', '/'));
        }
        using (var zip = ZipFile.OpenRead(zipPath)) foreach (var entry in zip.Entries) {
            cancel.ThrowIfCancellationRequested(); using var stream = entry.Open(); string hash = Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream)).ToLowerInvariant();
            string original = entry.FullName.StartsWith("notices/", StringComparison.Ordinal) ? Path.Combine(converted, entry.FullName[8..]) : Path.Combine(output, entry.FullName);
            if (hash != JsonFiles.HashFile(original)) throw new IOException("Add-on ZIP readback mismatch.");
        }
        return new(output, dlc, zipPath, readback);
    }
}
