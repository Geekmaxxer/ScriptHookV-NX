using System.IO.Compression;
using System.Text;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public sealed record PedAddonOptions(string Id = "", string Model = "", string Template = "", string BasePed = "", int TextureCap = 1024, string Profile = "male");
public sealed record AddonBuild(string Directory, string DlcArchive, string Archive, AddonAudit Audit, string Model = "", string Id = "", string MetadataSource = "");
public sealed record PedInstallBuild(AddonBuild Ped, GameAssetUpdate GameFiles, string Archive);
public static class AddonBuilder {
    static AssetNode Dir(string name, params AssetNode[] children) { var node = new AssetNode { Name = name, Directory = true }; node.Children.AddRange(children); return node; }
    static AssetNode File(string name, byte[] data, bool resource = false) => new() { Name = name, Data = data, Resource = resource };
    static byte[] Bytes(XDocument doc) { using var stream = new MemoryStream(); using (var writer = System.Xml.XmlWriter.Create(stream, new() { Encoding = new UTF8Encoding(false), Indent = true })) doc.Save(writer); return stream.ToArray(); }
    public static byte[] ReadPedTemplate(string path, CancellationToken cancel = default) {
        if (string.IsNullOrWhiteSpace(path)) throw new ArgumentException("Choose custom PED metadata, or leave the custom metadata option off to use a built-in profile.");
        if (Path.GetExtension(path).Equals(".rpf", StringComparison.OrdinalIgnoreCase)) {
            var matches = RpfFileEditor.List(path, cancel).Where(e => e.Path.EndsWith("/peds.meta", StringComparison.OrdinalIgnoreCase) || e.Path.Equals("peds.meta", StringComparison.OrdinalIgnoreCase)).ToArray();
            if (matches.Length != 1) throw new InvalidDataException("Choose a peds.meta file directly; the selected archive has zero or multiple matching entries.");
            return RpfFileEditor.ReadBinary(path, matches[0].Path, cancel: cancel);
        }
        if (new FileInfo(path).Length > 4 * 1024 * 1024) throw new InvalidDataException("PED template exceeds 4 MiB.");
        cancel.ThrowIfCancellationRequested(); return System.IO.File.ReadAllBytes(path);
    }
    public static IReadOnlyList<string> PedNames(string path, CancellationToken cancel = default) {
        var doc = PedMetadata.Parse(ReadPedTemplate(path, cancel), path);
        return doc.Descendants().Where(e => e.Name.LocalName == "InitDatas").Elements().SelectMany(e => e.Elements().Where(n => n.Name.LocalName == "Name")).Select(e => e.Value).Distinct(StringComparer.OrdinalIgnoreCase).Order(StringComparer.OrdinalIgnoreCase).ToArray();
    }
    public static PedInstallBuild PedForGame(string input, string output, PedAddonOptions options, string game, CancellationToken cancel = default, bool createZip = true, Action<string>? progress = null) {
        if (string.IsNullOrWhiteSpace(game)) throw new ArgumentException("Choose the SD/game folder once so the current DLC list can be preserved.");
        string archive = GameAssetUpdater.ResolveUpdate(game);
        // Validate registration metadata before spending time preparing the assets.
        _ = DlcPackageService.MergeList(RpfFileEditor.ReadBinary(archive, "common/data/dlclist.xml", cancel: cancel), "nx_ped_validation");
        var ped = Ped(input, output, options, cancel, createZip: false);
        var installed = GameAssetUpdater.AddDlc(ped.DlcArchive, archive, ped.Id, Path.Combine(ped.Directory, "game-install"), cancel, progress);
        string guide = Path.Combine(installed.Directory, "READ-ME-FIRST.md");
        System.IO.File.AppendAllText(guide, "\nSpawn model: " + ped.Model + ". Metadata: " + ped.MetadataSource + ".\n", new UTF8Encoding(false));
        string zipPath = createZip ? Path.Combine(ped.Directory, ped.Id + "-Switch-install.zip") : "";
        if (createZip) {
            using (var zip = ZipFile.Open(zipPath, ZipArchiveMode.Create))
                foreach (string file in Directory.EnumerateFiles(installed.Directory, "*", SearchOption.AllDirectories)) {
                    cancel.ThrowIfCancellationRequested(); zip.CreateEntryFromFile(file, Path.GetRelativePath(installed.Directory, file).Replace('\\', '/'), CompressionLevel.Optimal);
                }
            using var readback = ZipFile.OpenRead(zipPath);
            foreach (var entry in readback.Entries) {
                cancel.ThrowIfCancellationRequested(); using var stream = entry.Open();
                if (Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream)).ToLowerInvariant() != JsonFiles.HashFile(Path.Combine(installed.Directory, entry.FullName))) throw new IOException("PED install ZIP readback failed.");
            }
        }
        return new(ped, installed, zipPath);
    }
    public static AddonBuild Ped(string input, string output, PedAddonOptions options, CancellationToken cancel = default, bool createZip = true) {
        if (string.IsNullOrWhiteSpace(input)) throw new ArgumentException("Choose or drop the folder/ZIP containing the four PED assets first.");
        input = Path.GetFullPath(input.Trim());
        if (System.IO.File.Exists(input) && new[] { ".ydd", ".ndd", ".yft", ".nft", ".ymt", ".nmt", ".ytd", ".ntd" }.Contains(Path.GetExtension(input).ToLowerInvariant())) input = Path.GetDirectoryName(input)!;
        var source = AssetService.Load(input, cancel);
        IEnumerable<AssetNode> Leaves(AssetNode node) => node.Directory ? node.Children.SelectMany(Leaves) : new[] { node };
        var sourceFiles = Leaves(source).ToArray();
        var modelFiles = sourceFiles.Where(n => AssetService.ResourceExtensions.Contains(Path.GetExtension(n.Name))).ToArray();
        var stems = modelFiles.Select(n => Path.GetFileNameWithoutExtension(n.Name)).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        if(stems.Length>1){
            if(!string.IsNullOrWhiteSpace(options.Model)||!string.IsNullOrWhiteSpace(options.Template)||!string.IsNullOrWhiteSpace(options.BasePed))throw new InvalidDataException("Several models were detected. Leave the single-model name/template blank to bundle them automatically, or use the DLC collection editor for per-model overrides.");
            var collection=DlcWorkspace.Create(string.IsNullOrWhiteSpace(options.Id)?"my-mods":options.Id.Trim());
            collection.ImportLoaded(source,input,options.TextureCap,options.Profile,cancel);
            var bundle=collection.Export(output,createZip,cancel);var models=collection.Models();
            return new(bundle.Directory,bundle.DlcArchive,bundle.Archive,collection.Audit(cancel),string.Join(", ",models.Select(m=>m.Name)),collection.Id,"automatic multi-model collection");
        }
        if (stems.Length != 1) throw new InvalidDataException("Choose one PED variant: four matching .ydd, .yft, .ymt and .ytd files. " + (stems.Length == 0 ? "No model resources were found." : "Several models/variants were found: " + string.Join(", ", stems.Take(8))));
        string model = stems[0].ToLowerInvariant();
        foreach (string ext in new[] { ".ndd", ".nft", ".nmt", ".ntd" })
            if (modelFiles.Count(n => AssetService.SwitchName(n.Name).EndsWith(ext, StringComparison.OrdinalIgnoreCase)) != 1)
                throw new InvalidDataException("The PED needs exactly one " + model + ext.Replace(".n", ".y") + " (or its Switch equivalent). Select the folder with all four matching assets.");
        options = options with { Model = string.IsNullOrWhiteSpace(options.Model) ? model : options.Model.Trim(), Id = string.IsNullOrWhiteSpace(options.Id) ? "nx_" + PortProject.SuggestId(model)[..Math.Min(28, PortProject.SuggestId(model).Length)] : options.Id.Trim() };
        if (!PortProject.ValidId(options.Id) || options.Id != options.Id.ToLowerInvariant()) throw new ArgumentException("Use a lowercase DLC ID with 1-31 letters, digits, underscores or hyphens.");
        if (options.Model.Length is < 1 or > 63 || options.Model.Any(c => !(char.IsAsciiLetterOrDigit(c) || c == '_')) || options.Model != options.Model.ToLowerInvariant()) throw new ArgumentException("Use a lowercase model name with 1-63 letters, digits or underscores.");
        if (!options.Model.Equals(model, StringComparison.OrdinalIgnoreCase)) throw new ArgumentException("The spawn model must match the asset filenames: " + model + ". Leave it blank to detect it automatically.");
        if (string.IsNullOrWhiteSpace(output)) output = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "ScriptHookV-NX Projects", "PED-" + model + "-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N")[..6]);
        output = Path.GetFullPath(output.Trim());
        if (Directory.Exists(output) || System.IO.File.Exists(output) || Directory.Exists(input) && InputLoader.Inside(output, input)) throw new IOException("Choose a new output folder outside the input.");
        string metadataSource;
        XDocument template;
        if (!string.IsNullOrWhiteSpace(options.Template)) {
            template = PedMetadata.Parse(ReadPedTemplate(options.Template, cancel), options.Template); metadataSource = "custom metadata";
        } else {
            // Use supplied mod definitions when present; four loose files need none.
            var supplied = sourceFiles.Where(n => n.Name.Equals("peds.meta", StringComparison.OrdinalIgnoreCase)).ToArray();
            if (supplied.Length > 1) throw new InvalidDataException("Several peds.meta files were found. Select one PED variant or choose custom metadata explicitly.");
            if (supplied.Length == 1) { template = PedMetadata.Parse(supplied[0].Data, "peds.meta in input assets"); metadataSource = "mod-supplied metadata"; }
            else { template = PedMetadata.Human(options.Model, options.Profile); metadataSource = "built-in human " + options.Profile; }
        }
        byte[] templateBytes = Bytes(template);
        var named = template.Root!.Elements().Where(e => e.Name.LocalName == "InitDatas").Elements().ToArray();
        string basePed = options.BasePed.Trim();
        if (basePed.Length == 0) {
            var matching = named.Where(e => e.Elements().Any(c => c.Name.LocalName == "Name" && c.Value.Equals(model, StringComparison.OrdinalIgnoreCase))).ToArray();
            if (matching.Length == 1) basePed = matching[0].Elements().Single(e => e.Name.LocalName == "Name").Value;
            else if (named.Length == 1) basePed = named[0].Elements().Single(e => e.Name.LocalName == "Name").Value;
            else throw new InvalidDataException("Custom metadata contains several PEDs without a matching model. Choose its base definition, or clear custom metadata to use the built-in profile.");
        }
        var definitions = template.Root.Elements().Where(e => e.Name.LocalName == "InitDatas").Elements()
            .Where(e => e.Elements().Any(c => c.Name.LocalName == "Name" && c.Value.Equals(basePed, StringComparison.OrdinalIgnoreCase))).ToArray();
        if (definitions.Length != 1) throw new InvalidDataException("The selected custom base PED is missing or duplicated. Choose an existing definition, or clear custom metadata to use the built-in human profile.");
        var definition = new XElement(definitions[0]); definition.Elements().Single(e => e.Name.LocalName == "Name").Value = options.Model;
        var peds = new XDocument(new XElement(template.Root.Name,
            template.Root.Elements().Where(e => e.Name.LocalName is not ("InitDatas" or "residentAnims")).Select(e => new XElement(e)),
            new XElement("residentAnims"), new XElement("InitDatas", definition)));
        var conversion = AssetService.ConvertLoaded(source, input, Path.Combine(output, "resource-preparation"), new(options.TextureCap, "Peds"), cancel, createZip: false);
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
        JsonFiles.Write(Path.Combine(output, "addon-manifest.json"), new { format = 1, category = "ped", id = options.Id, model = options.Model, base_ped = basePed, metadata_source = metadataSource,
            template_sha256 = templateHash, dlc_sha256 = JsonFiles.HashFile(dlc), audit = readback, complete = true, switch_runtime_verified = false });
        string guide = "# Prepared PED add-on\n\nSpawn model: " + options.Model + ".\n\nUse Add to game with addon/" + options.Id + "/dlc.rpf and your current SD/game folder to register it automatically. Close GTA and back up replaced files before installing.\n\nMetadata: " + metadataSource + ". Test appearance and animations on Switch. Human presets do not repair incompatible rigs.\n";
        System.IO.File.WriteAllText(Path.Combine(output, "READ-ME-FIRST.md"), guide, new UTF8Encoding(false));
        string zipPath = createZip ? Path.Combine(output, options.Id + "-addon-test.zip") : "";
        if (createZip) {
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
        }
        return new(output, dlc, zipPath, readback, options.Model, options.Id, metadataSource);
    }
}
