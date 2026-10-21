using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace ModPorter.Core;

public sealed record PortOptions(string PluginId, bool AdaptDllMain = true, bool AdaptSleep = false, bool AddDescriptor = true);
public sealed record Change(string File, int Line, string Operation, string Before, string After);
public sealed record PortResult(string Directory, string ProjectFile, AnalysisReport Report, IReadOnlyList<Change> Changes);
public static class PortProject {
    public const string CompatHeader = "nx_port_compat.h";
    public static bool ValidId(string id) => id.Length is >= 1 and <= 31 && id.All(c => char.IsAsciiLetterOrDigit(c) || c is '-' or '_');
    public static string SuggestId(string name) {
        var chars = name.ToLowerInvariant().Select(c => char.IsAsciiLetterOrDigit(c) || c is '-' or '_' ? c : '-').ToArray();
        string id = new string(chars).Trim('-'); return id.Length == 0 ? "my-mod" : id[..Math.Min(31, id.Length)];
    }
    public static PortResult Create(ModInput input, NativeCatalog catalog, string sdk, string output, PortOptions options, IEnumerable<string>? selected = null, CancellationToken cancel = default) {
        if (input.IsBinary) throw new InvalidOperationException("A source project is required. Binary-only ASI conversion is not implemented.");
        if (!ValidId(options.PluginId)) throw new ArgumentException("Plugin ID must contain 1-31 ASCII letters, digits, hyphens or underscores.");
        output = Path.GetFullPath(output); sdk = Path.GetFullPath(sdk);
        if (Directory.Exists(output) || File.Exists(output)) throw new IOException("Choose a new output folder; existing projects are preserved.");
        if (InputLoader.Inside(output, sdk) || Directory.Exists(input.Path) && InputLoader.Inside(output, input.Path)) throw new IOException("Create the project outside the SDK and original source directories.");
        var chosen = (selected ?? input.Files.Where(f => f.IsTranslationUnit).Select(f => f.RelativePath)).ToHashSet(StringComparer.OrdinalIgnoreCase);
        if (chosen.Count == 0 || chosen.Any(s => !input.Files.Any(f => f.IsTranslationUnit && f.RelativePath.Equals(s, StringComparison.OrdinalIgnoreCase)))) throw new InvalidDataException("Select at least one source file from this input.");
        if (input.Files.Any(f => Path.GetFileName(f.RelativePath).Equals(CompatHeader, StringComparison.OrdinalIgnoreCase))) throw new IOException("Input already contains the reserved adapter header; open the generated project instead of porting it again.");
        string? existingId = SourceAnalyzer.DescriptorId(input.Files.Where(f => !f.IsTranslationUnit || chosen.Contains(f.RelativePath)));
        if (existingId != null && existingId != options.PluginId) throw new InvalidDataException($"This source already declares plugin ID '{existingId}'. Use that ID to preserve its saved-file namespace, or edit the source explicitly before porting.");
        var copies = new List<SourceFile>(); var changes = new List<Change>();
        string entryName = SourceAnalyzer.DiscoverEntry(input.Files.Where(f => !f.IsTranslationUnit || chosen.Contains(f.RelativePath)));
        string MappedPath(SourceFile f) => !SourceAnalyzer.IsVendorHeader(f) && !f.IsTranslationUnit && SourceAnalyzer.SdkHeaders.Contains(Path.GetFileName(f.RelativePath)) ? "nx_user_headers/" + f.RelativePath : f.RelativePath;
        var mappedPaths = input.Files.Where(f => !SourceAnalyzer.IsVendorHeader(f)).Select(MappedPath).ToList();
        if (mappedPaths.Distinct(StringComparer.OrdinalIgnoreCase).Count() != mappedPaths.Count) throw new InvalidDataException("Input conflicts with the nx_user_headers adapter directory. Rename that source directory.");
        foreach (var file in input.Files) {
            cancel.ThrowIfCancellationRequested();
            // Original vendor SDK files remain in the input snapshot, never in the compile include path.
            if (SourceAnalyzer.IsVendorHeader(file)) continue;
            bool codeFile = InputLoader.SourceExtensions.Contains(Path.GetExtension(file.RelativePath)) || new[] { ".h", ".hh", ".hpp", ".hxx", ".inl", ".inc", ".ipp", ".tpp" }.Contains(Path.GetExtension(file.RelativePath).ToLowerInvariant());
            string text = file.Text;
            if (codeFile) {
                var edits = new List<(int Start, int Length, string Text, string Operation)>();
                var dlls = SourceAnalyzer.DllEntries(file, entryName);
                foreach (var d in dlls.Where(d => d.Standard && options.AdaptDllMain)) edits.Add((d.Start, d.End - d.Start, "/* Windows registration replaced by the NX plugin descriptor. */", "Replace standard DllMain registration"));
                bool InRemovedDll(int at) => edits.Any(e => at >= e.Start && at < e.Start + e.Length);
                string stripped = CppLexer.CodeOnly(text);
                foreach (Match m in SourceAnalyzer.Includes().Matches(text)) {
                    int hash = text.IndexOf('#', m.Index, m.Length); if (hash < 0 || stripped[hash] != '#' || InRemovedDll(m.Index)) continue;
                    string name = m.Groups[2].Value.Replace('\\', '/').Split('/')[^1]; var local = SourceAnalyzer.ResolveInclude(file, m.Groups[2].Value, input.Files);
                    if (local != null && !SourceAnalyzer.IsVendorHeader(local))
                        edits.Add((m.Index, m.Length, "\n#include \"" + MappedPath(local) + "\"", "Preserve project header using a canonical include path"));
                    else if (SourceAnalyzer.SdkHeaders.Contains(name) || name.Equals("windows.h", StringComparison.OrdinalIgnoreCase) && options.AdaptDllMain)
                        edits.Add((m.Index, m.Length, "\n#include \"" + CompatHeader + "\"", "Use Switch compatibility declarations"));
                }
                var tokens = CppLexer.Tokens(text);
                for (int i = 0; i < tokens.Count; i++) {
                    var t = tokens[i]; if (InRemovedDll(t.Start) || edits.Any(e => t.Start >= e.Start && t.Start < e.Start + e.Length)) continue;
                    if (i + 1 < tokens.Count && tokens[i + 1].Text == "::" && SourceAnalyzer.NamespaceAliases.TryGetValue(t.Text, out string? alias))
                        edits.Add((t.Start, t.Length, alias, "Use Switch native namespace"));
                    if (i + 3 < tokens.Count && tokens[i + 1].Text == "::" && tokens[i + 3].Text == "(") {
                        string ns = SourceAnalyzer.NamespaceAliases.GetValueOrDefault(t.Text, t.Text), key = ns + "::" + tokens[i + 2].Text;
                        if (SourceAnalyzer.NativeAliases.TryGetValue(key, out string? renamed) && catalog.Entries.TryGetValue(renamed, out var native) && native.Registered)
                            edits.Add((tokens[i + 2].Start, tokens[i + 2].Length, renamed.Split("::")[1], "Use verified legacy native symbol alias"));
                    }
                    if (options.AdaptSleep && t.Text == "Sleep" && i + 1 < tokens.Count && tokens[i + 1].Text == "(")
                        edits.Add((t.Start, t.Length, "WAIT", "Replace Sleep with cooperative WAIT (call context requires review)"));
                }
                int prior = int.MaxValue;
                foreach (var e in edits.OrderByDescending(e => e.Start)) {
                    if (e.Start + e.Length > prior) throw new InvalidDataException("Overlapping adaptation edits in " + file.RelativePath);
                    int line = 1 + file.Text.AsSpan(0, e.Start).Count('\n');
                    changes.Add(new(file.RelativePath, line, e.Operation, text.Substring(e.Start, e.Length), e.Text));
                    text = text[..e.Start] + e.Text + text[(e.Start + e.Length)..]; prior = e.Start;
                }
                if (!text.Contains("#include \"" + CompatHeader + "\"", StringComparison.Ordinal)) text = "#include \"" + CompatHeader + "\"\n" + text;
            }
            string mapped = MappedPath(file);
            if (mapped != file.RelativePath) changes.Add(new(file.RelativePath, 1, "Preserve custom header without shadowing the Switch SDK", file.RelativePath, mapped));
            copies.Add(new(mapped, text, JsonFiles.Hash(Encoding.UTF8.GetBytes(text))));
        }
        var active = copies.Where(f => chosen.Contains(f.RelativePath)).ToList();
        var entries = active.SelectMany(f => SourceAnalyzer.Entries(f, entryName)).ToList();
        bool descriptor = copies.Any(f => CppLexer.Tokens(f.Text).Any(t => SourceAnalyzer.IsDescriptorMacro(t.Text) || t.Text == "shvPluginQuery"));
        if (!descriptor && options.AddDescriptor && entries.Count == 1) {
            var entry = copies.Single(f => f.RelativePath == entries[0].File); string addition = $"\nSHV_NX_PLUGIN(\"{options.PluginId}\", {entryName})\n";
            copies[copies.IndexOf(entry)] = entry with { Text = entry.Text + addition, Sha256 = JsonFiles.Hash(Encoding.UTF8.GetBytes(entry.Text + addition)) };
            changes.Add(new(entry.RelativePath, 1 + entry.Text.Count(c => c == '\n'), "Add NX plugin descriptor", "", addition));
        }
        string compat = """
            #pragma once
            #include <script.h>
            // Deliberately limited source compatibility. No Windows APIs are emulated.
            #ifndef TRUE
            #define TRUE 1
            #endif
            #ifndef FALSE
            #define FALSE 0
            #endif
            """ + "\n";
        copies.Add(new(CompatHeader, compat, JsonFiles.Hash(Encoding.UTF8.GetBytes(compat))));
        var adapted = new ModInput { Path = output, Name = options.PluginId, Files = copies };
        // In generated sources the compatibility header itself is a local include.
        var report = SourceAnalyzer.Analyze(adapted, catalog, chosen);
        report.Details["Original input"] = input.Path;
        report.Details["Recorded edits"] = changes.Count.ToString();
        Directory.CreateDirectory(output);
        try {
            foreach (var file in input.Files) {
                cancel.ThrowIfCancellationRequested(); string dest = Path.Combine(output, "original", file.RelativePath);
                Directory.CreateDirectory(Path.GetDirectoryName(dest)!);
                File.WriteAllBytes(dest, file.OriginalBytes ?? Encoding.UTF8.GetBytes(file.Text));
            }
            foreach (var file in copies) {
                cancel.ThrowIfCancellationRequested(); string dest = Path.Combine(output, "src", file.RelativePath);
                Directory.CreateDirectory(Path.GetDirectoryName(dest)!); File.WriteAllText(dest, file.Text, new UTF8Encoding(false));
            }
            // Header lookup order is deterministic and explicit; copied vendor bindings never shadow NX wrappers.
            var includeDirs = copies.Where(f => !f.IsTranslationUnit && Path.GetExtension(f.RelativePath) is not ".md" and not ".txt" and not ".json").Select(f => Path.GetDirectoryName(f.RelativePath) ?? "").Distinct(StringComparer.OrdinalIgnoreCase).Order().ToList();
            includeDirs.Remove(""); includeDirs.Insert(0, "");
            if (includeDirs.Count > 16) throw new InvalidDataException("More than 16 header directories; consolidate includes in the copied project.");
            var manifest = new {
                format = 1, kind = "nro", id = options.PluginId,
                sources = chosen.Order().Select(s => "src/" + s).ToArray(),
                includes = includeDirs.Select(s => ("src/" + s).TrimEnd('/').Replace('\\', '/')).ToArray(),
                mod_porter = new { version = "0.4.0", target_build = catalog.TargetBuild, header_sha256 = catalog.HeaderSha256, original_input = input.Path, original_hashes = input.Files.ToDictionary(f => f.RelativePath, f => f.Sha256), options, switch_runtime_verified = false }
            };
            JsonFiles.Write(Path.Combine(output, "project.json"), manifest);
            JsonFiles.Write(Path.Combine(output, "changes.json"), changes);
            JsonFiles.Write(Path.Combine(output, "analysis.json"), report);
            File.WriteAllText(Path.Combine(output, "PORTING-REPORT.md"), report.ToMarkdown(), new UTF8Encoding(false));
            File.WriteAllText(Path.Combine(output, "README.md"), $"# {options.PluginId}\n\nEdit the copied files in src, then open project.json in Mod Porter and build. Original input snapshots are in original; changes.json records automatic edits.\n\nBuilds require Python 3.10+, devkitA64, switch-tools and a matched ScriptHookV-NX SDK. Do not build or install through the original PC project.\n\nTarget: {catalog.TargetBuild}. Console testing is required.\n", new UTF8Encoding(false));
            return new(output, Path.Combine(output, "project.json"), report, changes);
        } catch (Exception ex) when (ex is not OutOfMemoryException) {
            // Preserve partial files for diagnosis; never recursively delete a computed path.
            File.WriteAllText(Path.Combine(output, "INCOMPLETE.txt"), "Project generation did not complete: " + ex.Message);
            throw;
        }
    }
    public static (string Id, List<string> Sources, List<string> Includes, string HeaderHash) Read(string manifestPath) {
        if (new FileInfo(manifestPath).Length > 1024 * 1024) throw new InvalidDataException("Project manifest exceeds 1 MiB.");
        using var doc = JsonDocument.Parse(File.ReadAllText(manifestPath)); var r = doc.RootElement;
        if (r.GetProperty("format").GetInt32() != 1 || r.GetProperty("kind").GetString() != "nro") throw new InvalidDataException("Select a format 1 NRO project.json.");
        string root = Path.GetDirectoryName(Path.GetFullPath(manifestPath))!, id = r.GetProperty("id").GetString() ?? "";
        if (!ValidId(id)) throw new InvalidDataException("Invalid plugin ID in project.json.");
        List<string> Paths(string property, bool dirs) {
            var values = r.GetProperty(property).EnumerateArray().Select(x => x.GetString() ?? "").ToList();
            if (values.Count > (dirs ? 16 : 256)) throw new InvalidDataException("Too many project paths.");
            return values.Select(value => {
                string relative = InputLoader.SafeRelative(value.TrimEnd('/', '\\')); string p = Path.GetFullPath(Path.Combine(root, relative));
                if (!InputLoader.Inside(p, root) || !(dirs ? Directory.Exists(p) : File.Exists(p))) throw new InvalidDataException("Missing/unsafe project path: " + value);
                // Reparse points anywhere below the project root must not escape the manifest directory.
                for (string? current = p; current != null && !current.Equals(root, StringComparison.OrdinalIgnoreCase); current = Path.GetDirectoryName(current))
                    if ((File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Project paths cannot contain symbolic links.");
                return p;
            }).ToList();
        }
        var sources = Paths("sources", false); if (sources.Count == 0 || sources.Any(p => !InputLoader.SourceExtensions.Contains(Path.GetExtension(p)))) throw new InvalidDataException("Select C/C++ translation units.");
        string hash = r.TryGetProperty("mod_porter", out var meta) && meta.TryGetProperty("header_sha256", out var h) ? h.GetString() ?? "" : "";
        return (id, sources, Paths("includes", true), hash);
    }
}
