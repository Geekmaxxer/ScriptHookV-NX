using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public sealed record AssetOptions(int MaximumTextureDimension = 1024, string Kind = "Auto");
public sealed record AssetFileResult(string Input, string Output, string Operation, int InputBytes, int OutputBytes, string Sha256, long GraphicsBytesBefore, long GraphicsBytesAfter, IReadOnlyList<TextureChange> Textures);
public sealed class AssetReport {
    public string Input { get; set; } = "";
    public string Output { get; set; } = "";
    public string Kind { get; set; } = "Auto";
    public string Status { get; set; } = "Needs review";
    public List<AssetFileResult> Files { get; } = [];
    public List<string> Notes { get; } = [];
    public int ConvertedNames { get; set; }
    public bool SwitchRuntimeVerified { get; set; }
    public long GraphicsBytesSaved => Files.Sum(f => f.GraphicsBytesBefore - f.GraphicsBytesAfter);
    public string Text() => $"{Status}\n\n{Files.Count} files inspected; {ConvertedNames} names/references changed; {GraphicsBytesSaved / 1048576.0:N2} MiB of texture resource allocation removed.\n\n" + string.Join("\n", Notes.Select(n => "• " + n)) + "\n\n" + string.Join("\n", Files.Select(f => $"{f.Input} → {f.Output}\n  {f.Operation}"));
}
public sealed record AssetConversion(string Directory, string Archive, AssetReport Report);
public static class AssetService {
    public static readonly HashSet<string> ResourceExtensions = new(StringComparer.OrdinalIgnoreCase) { ".ytd", ".ntd", ".yft", ".nft", ".ydr", ".ndr", ".ydd", ".ndd", ".ybn", ".nbn", ".ymt", ".nmt", ".ycd", ".ncd", ".ymap", ".nmap", ".ytyp", ".ntyp", ".yld", ".nld", ".ypt", ".npt", ".yed", ".ned", ".ymf", ".nmf" };
    static readonly HashSet<string> DataExtensions = new(StringComparer.OrdinalIgnoreCase) { ".xml", ".meta", ".dat", ".awc", ".rel", ".rpf" };
    public static string SwitchName(string name) {
        string ext = Path.GetExtension(name);
        if (ResourceExtensions.Contains(ext) && ext[1] is 'y' or 'Y') name = name[..^ext.Length] + ".n" + ext[2..].ToLowerInvariant();
        if (name.StartsWith("x64", StringComparison.OrdinalIgnoreCase)) name = "switch" + name[3..];
        return name;
    }
    static string PlatformReferences(string text) {
        text = Regex.Replace(text, @"(?i)(?<![a-z0-9_])x64(?=[/\\.:_]|\b)", "switch");
        foreach (var ext in ResourceExtensions.Where(e => e[1] == 'y')) text = Regex.Replace(text, Regex.Escape(ext) + @"\b", ".n" + ext[2..], RegexOptions.IgnoreCase);
        return text;
    }
    static (string Text, Encoding Encoding) ReadMetadata(byte[] bytes) {
        if (bytes.Length >= 4 && (bytes.AsSpan(0, 4).SequenceEqual(new byte[] { 0xff, 0xfe, 0, 0 }) || bytes.AsSpan(0, 4).SequenceEqual(new byte[] { 0, 0, 0xfe, 0xff }))) throw new InvalidDataException("Save UTF-32 metadata as UTF-8 or UTF-16 before conversion.");
        bool le = bytes.Length >= 2 && bytes[0] == 0xff && bytes[1] == 0xfe, be = bytes.Length >= 2 && bytes[0] == 0xfe && bytes[1] == 0xff;
        bool utf8Bom = bytes.Length >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf;
        Encoding encoding = le ? new UnicodeEncoding(false, true, true) : be ? new UnicodeEncoding(true, true, true) : new UTF8Encoding(utf8Bom, true);
        try {
            string text = encoding.GetString(bytes, le || be ? 2 : utf8Bom ? 3 : 0, bytes.Length - (le || be ? 2 : utf8Bom ? 3 : 0));
            var declared = Regex.Match(text, "^\\s*<\\?xml[^>]*encoding\\s*=\\s*['\"]([^'\"]+)['\"]", RegexOptions.IgnoreCase);
            if (declared.Success && !new[] { "utf-8", "utf8", "utf-16", "utf-16le", "utf-16be" }.Contains(declared.Groups[1].Value.ToLowerInvariant())) throw new InvalidDataException("Save metadata with an explicit UTF-8/UTF-16 encoding before conversion.");
            return (text, encoding);
        } catch (DecoderFallbackException e) { throw new InvalidDataException("Metadata is not valid UTF-8/UTF-16; save it as UTF-8 before conversion.", e); }
    }
    static AssetNode Load(string path, CancellationToken cancel) {
        path = Path.GetFullPath(path); long total = 0; int count = 0;
        var root = new AssetNode { Name = "root", Directory = true }; var files = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        void Add(string name, byte[] bytes) {
            cancel.ThrowIfCancellationRequested(); name = InputLoader.SafeRelative(name);
            if (++count > 8192 || bytes.Length > ResourceContainer.MaxBytes || (total += bytes.Length) > RpfArchive.MaxBytes) throw new InvalidDataException("Asset input exceeds 8,192 files / 256 MiB per file / 512 MiB total.");
            if (!files.Add(name)) throw new InvalidDataException("Duplicate/case-colliding asset path: " + name);
            string[] parts = name.Split('/'); var directory = root;
            foreach (string segment in parts[..^1]) {
                var existing = directory.Children.FirstOrDefault(c => c.Name.Equals(segment, StringComparison.OrdinalIgnoreCase));
                if (existing is { Directory: false }) throw new InvalidDataException("Asset directory/file collision.");
                if (existing == null) { existing = new AssetNode { Name = segment, Directory = true }; directory.Children.Add(existing); }
                directory = existing;
            }
            directory.Children.Add(new AssetNode { Name = parts[^1], Data = bytes, Resource = bytes.Length >= 16 && BinaryPrimitives.ReadUInt32LittleEndian(bytes) == 0x37435352 });
        }
        byte[] ReadBounded(Stream input, long length) {
            if (length > ResourceContainer.MaxBytes || total + length > RpfArchive.MaxBytes) throw new InvalidDataException("Asset file is too large.");
            using var m = new MemoryStream(); var b = new byte[65536]; int n;
            while ((n = input.Read(b)) != 0) { cancel.ThrowIfCancellationRequested(); if (m.Length + n > length) throw new InvalidDataException("Asset exceeds its declared size."); m.Write(b, 0, n); }
            if (m.Length != length) throw new InvalidDataException("Asset changed while reading."); return m.ToArray();
        }
        if (Directory.Exists(path)) {
            int entries = 0;
            void Walk(string dir) {
                if ((File.GetAttributes(dir) & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Asset symbolic links are unsupported.");
                foreach (string p in Directory.EnumerateFileSystemEntries(dir).Order(StringComparer.OrdinalIgnoreCase)) {
                    cancel.ThrowIfCancellationRequested(); if (++entries > 16384) throw new InvalidDataException("Too many asset directory entries.");
                    var attr = File.GetAttributes(p); if ((attr & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Asset symbolic links are unsupported.");
                    if ((attr & FileAttributes.Directory) != 0) { if (Path.GetFileName(p) is not ".git" and not "build" and not "releases") Walk(p); }
                    else { using var input = File.OpenRead(p); Add(Path.GetRelativePath(path, p), ReadBounded(input, input.Length)); }
                }
            }
            Walk(path);
        } else if (File.Exists(path) && Path.GetExtension(path).Equals(".zip", StringComparison.OrdinalIgnoreCase)) {
            using var zip = ZipFile.OpenRead(path); if (zip.Entries.Count > 8192) throw new InvalidDataException("Asset ZIP has too many entries.");
            foreach (var e in zip.Entries) {
                if (((e.ExternalAttributes >> 16) & 0xf000) == 0xa000 || (e.ExternalAttributes & 0x400) != 0) throw new InvalidDataException("Asset ZIP links are unsupported.");
                if (e.FullName.EndsWith('/')) { InputLoader.SafeRelative(e.FullName.TrimEnd('/')); continue; }
                using var input = e.Open(); Add(e.FullName, ReadBounded(input, e.Length));
            }
        } else if (File.Exists(path)) { using var input = File.OpenRead(path); Add(Path.GetFileName(path), ReadBounded(input, input.Length)); }
        else throw new FileNotFoundException("Choose a mod asset folder, ZIP, OPEN RPF or loose resource.", path);
        return root;
    }
    static AssetReport Process(AssetNode root, string input, AssetOptions options, bool convert, CancellationToken cancel) {
        if (options.MaximumTextureDimension is not (0 or 256 or 512 or 1024 or 2048 or 4096)) throw new ArgumentException("Unsupported texture cap.");
        var report = new AssetReport { Input = input, Kind = options.Kind }; int processed = 0; long total = 0; var paths = new List<string>();
        var definitions = new List<(string File,string Kind,string Name)>();
        void Walk(AssetNode parent, string prefix, int archiveDepth) {
            if (archiveDepth > 4) throw new InvalidDataException("Nested RPF depth exceeds four.");
            var outputNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var node in parent.Children) {
                cancel.ThrowIfCancellationRequested(); if (++processed > 16384 || (total += node.Data.Length) > 1024L * 1024 * 1024) throw new InvalidDataException("Expanded asset tree exceeds processing limits.");
                string originalName = node.Name, originalPath = prefix + originalName, outputName = SwitchName(originalName);
                if (!outputNames.Add(outputName)) throw new InvalidDataException("Converted asset names would collide: " + originalPath);
                if (outputName != originalName) report.ConvertedNames++;
                if (convert) node.Name = outputName;
                if (node.Directory) { Walk(node, prefix + outputName + "/", archiveDepth); continue; }
                string ext = Path.GetExtension(originalName).ToLowerInvariant(); byte[] original = node.Data; int before = 0, after = 0; IReadOnlyList<TextureChange> textures = [];
                string operation;
                if (ext == ".rpf") {
                    var nested = RpfArchive.Read(node.Data, cancel); Walk(nested, prefix + outputName + "::", archiveDepth + 1);
                    if (convert) { node.Data = RpfArchive.Write(nested, cancel); _ = RpfArchive.Read(node.Data, cancel); }
                    operation = convert ? "OPEN RPF tree converted, rebuilt and read back." : "OPEN RPF tree inspected; no output written.";
                } else if (ext is ".ysc" or ".nsc" or ".asi" or ".dll" or ".exe") {
                    throw new InvalidDataException("Asset pack contains executable/script code: " + originalPath + ". Port scripts separately; changing a script extension is not a valid conversion.");
                } else if (ResourceExtensions.Contains(ext)) {
                    if (ext is ".ymt" or ".nmt" or ".ymf" or ".nmf" && node.Data.Length >= 4 && Encoding.ASCII.GetString(node.Data, 0, 4) is "PSIN" or "RBF0") {
                        paths.Add(prefix + outputName);
                        report.Files.Add(new(originalPath, prefix + outputName, "Compiled PSO/RBF metadata preserved; platform filename prepared.", original.Length, node.Data.Length, JsonFiles.Hash(node.Data), 0, 0, []));
                        continue;
                    }
                    ResourceContainer container;
                    try { container = ResourceContainer.Read(node.Data); }
                    catch (InvalidDataException error) { throw new InvalidDataException(originalPath + ": " + error.Message, error); }
                    if (ext is ".ytd" or ".ntd") {
                        var info = TextureDictionary.Inspect(container); before = after = container.Graphics.Length;
                        if (convert) { var optimized = TextureDictionary.Downscale(node.Data, options.MaximumTextureDimension, cancel); node.Data = optimized.Data; before = optimized.GraphicsBytesBefore; after = optimized.GraphicsBytesAfter; textures = optimized.Changes; }
                        operation = $"Legacy texture dictionary: {info.Count} textures; {textures.Count} resized. " + string.Join("; ", textures.Select(t => t.Name + ": " + t.Method));
                        if (!convert && options.MaximumTextureDimension > 0) foreach (var texture in info.Where(t => Math.Max(t.Width, t.Height) > options.MaximumTextureDimension)) report.Notes.Add($"{originalPath}: {texture.Name} is {texture.Width}×{texture.Height}; target cap {options.MaximumTextureDimension}.");
                    } else {
                        if (ext is ".ydr" or ".ndr" or ".ydd" or ".ndd" && container.Version != 165 || ext is ".yft" or ".nft" && container.Version != 162 || ext is ".ybn" or ".nbn" && container.Version != 43)
                            throw new InvalidDataException("Resource version is outside the supported Legacy profile: " + originalPath + $" (version {container.Version}).");
                        operation = "Legacy resource envelope checked; payload preserved, platform filename prepared.";
                        if (convert && options.MaximumTextureDimension > 0 && ext is ".ydr" or ".ndr" or ".ydd" or ".ndd" or ".yft" or ".nft") {
                            var embedded = EmbeddedTextures.Downscale(node.Data, ext, options.MaximumTextureDimension, cancel);
                            node.Data = embedded.Data; textures = embedded.Changes;
                            if (textures.Count > 0) operation = $"{textures.Count} embedded textures reduced; model geometry and total graphics allocation preserved.";
                        }
                        if (container.Graphics.Length > 0 && options.MaximumTextureDimension > 0) report.Notes.Add(originalPath + ": embedded reductions preserve the shared graphics allocation; physics-child graphs and unsupported layouts still need separate review.");
                    }
                    paths.Add(prefix + outputName);
                } else if (ext is ".xml" or ".meta") {
                    var (text, encoding) = ReadMetadata(node.Data);
                    if (!string.IsNullOrWhiteSpace(text)) {
                        try {
                            using var xml = XmlReader.Create(new StringReader(text), new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null, MaxCharactersInDocument = 4 * 1024 * 1024 });
                            var doc = XDocument.Load(xml);
                            foreach (var field in doc.Descendants().Where(n => n.Name.LocalName is "modelName" or "txdName" or "handlingId")) if (!field.HasElements && field.Value.Length is > 0 and <= 128) definitions.Add((originalPath,field.Name.LocalName,field.Value.Trim()));
                        } catch (XmlException e) { throw new InvalidDataException(originalPath + ": invalid XML metadata: " + e.Message, e); }
                    }
                    string replaced = PlatformReferences(text);
                    if (replaced != text) { report.ConvertedNames++; if (convert) {
                        node.Data = encoding.GetPreamble().Concat(encoding.GetBytes(replaced)).ToArray();
                    } }
                    paths.Add(prefix + outputName); operation = "Platform paths and resource extension references prepared; metadata definitions preserved.";
                } else if (DataExtensions.Contains(ext) || InputLoader.IsLegalNotice(originalName) || ext is ".md" or ".txt" or ".ini") operation = "Supporting data preserved.";
                else throw new InvalidDataException("Unsupported file in asset pack: " + originalPath + ". Choose only the asset/DLC folder.");
                report.Files.Add(new(originalPath, prefix + outputName, operation, original.Length, node.Data.Length, JsonFiles.Hash(node.Data), before, after, textures));
            }
        }
        Walk(root, "", 0);
        var resourceNames = paths.Where(p => ResourceExtensions.Contains(Path.GetExtension(p))).Select(p => Path.GetFileNameWithoutExtension(p.Split("::").Last())).ToHashSet(StringComparer.OrdinalIgnoreCase);
        foreach (var definition in definitions.Distinct().Where(d => d.Kind is "modelName" or "txdName")) if (!resourceNames.Contains(definition.Name)) report.Notes.Add($"{definition.File}: {definition.Kind} '{definition.Name}' has no resource in this input; confirm it exists in the base game or another dependency.");
        if (!paths.Any(p => ResourceExtensions.Contains(Path.GetExtension(p)))) throw new InvalidDataException("No supported vehicle, weapon, clothing, ped or texture resources were found.");
        report.Notes.Add("Compiled asset formats are separate from NRO plugins. This operation does not register new model/weapon/ped definitions in the game.");
        report.Notes.Add("Add-on DLC packs still require their original metadata and a dlclist.xml registration in the target game archive. Loose files require an exact replacement destination.");
        if (options.Kind == "Vehicles") {
            if (!paths.Any(p => p.EndsWith("vehicles.meta", StringComparison.OrdinalIgnoreCase))) report.Notes.Add("No vehicles.meta found: these files alone are insufficient to register an add-on car.");
            if (!paths.Any(p => p.EndsWith("_hi.nft", StringComparison.OrdinalIgnoreCase))) report.Notes.Add("No _hi.nft found: check the vehicle's high-detail/collision model requirements before installation.");
        }
        if (options.Kind == "Weapons" && !paths.Any(p => p.Contains("weapons", StringComparison.OrdinalIgnoreCase) && p.EndsWith(".meta", StringComparison.OrdinalIgnoreCase))) report.Notes.Add("No weapon metadata found: use an existing compatible replacement slot or supply the add-on's definitions.");
        if (options.Kind is "Clothes" or "Peds") report.Notes.Add("Ped skeletons, component collections, IDs and clothing slots must match the target. A filename change does not fix incompatible rigging or collection metadata.");
        report.Status = convert ? "Assets prepared — Switch testing required" : "Assets inspected — review requirements before conversion";
        return report;
    }
    public static AssetReport Analyze(string input, AssetOptions options, CancellationToken cancel = default) => Process(Load(input, cancel), input, options, false, cancel);
    public static AssetConversion Convert(string input, string output, AssetOptions options, CancellationToken cancel = default) {
        input = Path.GetFullPath(input); output = Path.GetFullPath(output);
        if (Directory.Exists(output) || File.Exists(output) || Directory.Exists(input) && InputLoader.Inside(output, input)) throw new IOException("Choose a new output folder outside the input assets.");
        var root = Load(input, cancel); var report = Process(root, input, options, true, cancel); report.Output = output;
        Directory.CreateDirectory(output); string converted = Path.Combine(output, "converted"); Directory.CreateDirectory(converted);
        void Save(AssetNode directory, string target) {
            foreach (var node in directory.Children) { cancel.ThrowIfCancellationRequested(); string dest = Path.Combine(target, node.Name); if (node.Directory) { Directory.CreateDirectory(dest); Save(node, dest); } else File.WriteAllBytes(dest, node.Data); }
        }
        Save(root, converted); JsonFiles.Write(Path.Combine(output, "asset-report.json"), report);
        File.WriteAllText(Path.Combine(output, "ASSET-REPORT.txt"), report.Text(), new UTF8Encoding(false));
        string install = "# Prepared Switch assets\n\nImport the contents of converted into the corresponding mod/game RPF location using OpenIV, CodeWalker or the Switch Toolbox. These files are not an NRO or a complete SD-root installation.\n\nFor add-on dlc.rpf packs, preserve their mod folder and add their registration to the game's dlclist.xml. Do not replace the complete list with a one-entry file. Keep the original metadata and test the mod on Switch.\n\nTexture reductions retain existing encoded mip levels where possible; otherwise supported BC textures are resized and re-encoded. Standalone texture allocations are rebuilt. Supported embedded model textures are reduced while retaining the shared graphics allocation and geometry. See ASSET-REPORT.txt for required follow-up.\n";
        File.WriteAllText(Path.Combine(output, "READ-ME-FIRST.md"), install, new UTF8Encoding(false));
        string archive = Path.Combine(output, "Switch-assets-test.zip");
        using (var zip = ZipFile.Open(archive, ZipArchiveMode.Create)) {
            foreach (string path in Directory.EnumerateFiles(converted, "*", SearchOption.AllDirectories)) zip.CreateEntryFromFile(path, "converted/" + Path.GetRelativePath(converted, path).Replace('\\', '/'), CompressionLevel.Optimal);
            foreach (string name in new[] { "READ-ME-FIRST.md", "ASSET-REPORT.txt" }) zip.CreateEntryFromFile(Path.Combine(output, name), name, CompressionLevel.Optimal);
        }
        using (var zip = ZipFile.OpenRead(archive)) foreach (var entry in zip.Entries) {
            using var stream = entry.Open(); string hash = System.Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream)).ToLowerInvariant();
            if (hash != JsonFiles.Hash(File.ReadAllBytes(Path.Combine(output, entry.FullName.Replace('/', Path.DirectorySeparatorChar))))) throw new InvalidDataException("Asset ZIP readback differs: " + entry.FullName);
        }
        return new(output, archive, report);
    }
}
