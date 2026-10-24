using System.Diagnostics;
using System.IO.Compression;
using System.Text;
using System.Text.Json;

namespace ModPorter.Core;

public sealed record ToolSettings(string SdkPath, string PythonPath, string DevkitProPath);
public sealed record CheckResult(string Name, bool Passed, string Detail);
public sealed record BuildResult(string Directory, string NroPath, string InstallZip, NroInfo Nro);
public static class BuildService {
    public static async Task<int> RunAsync(string executable, IEnumerable<string> args, string directory, Action<string> log, CancellationToken cancel) {
        cancel.ThrowIfCancellationRequested();
        var start = new ProcessStartInfo(executable) { WorkingDirectory = directory, UseShellExecute = false, RedirectStandardError = true, RedirectStandardOutput = true, CreateNoWindow = true };
        foreach (var arg in args) start.ArgumentList.Add(arg);
        start.Environment["PYTHONUTF8"] = "1"; start.Environment["PYTHONDONTWRITEBYTECODE"] = "1";
        using var process = new Process { StartInfo = start };
        if (!process.Start()) throw new IOException("Could not start " + executable);
        using var registration = cancel.Register(() => { try { if (!process.HasExited) process.Kill(entireProcessTree: true); } catch (InvalidOperationException) { } catch (System.ComponentModel.Win32Exception) { } });
        async Task Pump(StreamReader stream) { while (await stream.ReadLineAsync() is { } line) log(line.Length > 16000 ? line[..16000] : line); }
        await Task.WhenAll(Pump(process.StandardOutput), Pump(process.StandardError), process.WaitForExitAsync(CancellationToken.None));
        cancel.ThrowIfCancellationRequested(); return process.ExitCode;
    }
    public static async Task<List<CheckResult>> DoctorAsync(ToolSettings settings, CancellationToken cancel = default) {
        var checks = new List<CheckResult>();
        foreach (var relative in new[] { "tools/build_plugin.py", "include/script.h", "sdk/plugin_api.cpp", "sdk/plugin_start.s", "sdk/plugin_link.ld", "distribution/" + SharedSdAccess.NpdmPath, "tools/sd_permissions.py" }) checks.Add(new(relative, File.Exists(Path.Combine(settings.SdkPath, relative)), Path.Combine(settings.SdkPath, relative)));
        try { var c = NativeCatalog.Load(settings.SdkPath); checks.Add(new("Native index", true, $"{c.Entries.Count:N0} declarations; target {c.TargetBuild}")); }
        catch (Exception ex) when (ex is IOException or JsonException or InvalidOperationException or KeyNotFoundException) { checks.Add(new("Native index", false, ex.Message)); }
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancel); timeout.CancelAfter(TimeSpan.FromSeconds(20));
        var output = new StringBuilder();
        try {
            int exit = await RunAsync(settings.PythonPath, ["-c", "import sys; print(sys.version.split()[0]); raise SystemExit(0 if sys.version_info >= (3,10) else 1)"], Path.GetTempPath(), s => { lock (output) output.AppendLine(s); }, timeout.Token);
            checks.Add(new("Python 3.10+", exit == 0, output.ToString().Trim()));
        } catch (Exception ex) when (ex is IOException or System.ComponentModel.Win32Exception or OperationCanceledException) { cancel.ThrowIfCancellationRequested(); checks.Add(new("Python 3.10+", false, ex is OperationCanceledException ? "Python check timed out." : ex.Message)); }
        string compiler = Path.Combine(settings.DevkitProPath, "devkitA64", "bin", OperatingSystem.IsWindows() ? "aarch64-none-elf-g++.exe" : "aarch64-none-elf-g++");
        string converter = Path.Combine(settings.DevkitProPath, "tools", "bin", OperatingSystem.IsWindows() ? "elf2nro.exe" : "elf2nro");
        checks.Add(new("devkitA64", File.Exists(compiler), compiler)); checks.Add(new("switch-tools / elf2nro", File.Exists(converter), converter));
        return checks;
    }
    public static async Task<BuildResult> BuildAsync(string project, ToolSettings settings, Action<string> log, CancellationToken cancel = default, bool createInstallZip = true) {
        project = Path.GetFullPath(project); string root = Path.GetDirectoryName(project)!;
        var manifest = PortProject.Read(project); var catalog = NativeCatalog.Load(settings.SdkPath);
        if (manifest.HeaderHash.Length > 0 && manifest.HeaderHash != catalog.HeaderSha256) throw new InvalidDataException("The selected SDK differs from this port's native index. Re-port against the selected SDK before building.");
        string output = Path.Combine(root, "builds", DateTime.Now.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N")[..6]);
        // Read only the selected project include trees; never run third-party build scripts.
        var sourceFiles = new Dictionary<string, SourceFile>(StringComparer.OrdinalIgnoreCase);
        foreach (var include in manifest.Includes) {
            if (InputLoader.Inside(output, include)) throw new InvalidDataException("Build output would be inside an include directory. Move headers under src/.");
            foreach (var f in EnumerateHeaders(include, root, cancel)) sourceFiles[f.RelativePath] = f;
        }
        foreach (var p in manifest.Sources) {
            cancel.ThrowIfCancellationRequested(); if (new FileInfo(p).Length > 8 * 1024 * 1024) throw new InvalidDataException("Source exceeds 8 MiB: " + p);
            var bytes = File.ReadAllBytes(p); string text = new UTF8Encoding(false, true).GetString(bytes).TrimStart('\ufeff');
            string relative = Path.GetRelativePath(root, p).Replace('\\', '/'); sourceFiles[relative] = new(relative, text, JsonFiles.Hash(bytes));
        }
        var input = new ModInput { Path = root, Files = sourceFiles.Values.ToList() };
        var analysis = SourceAnalyzer.Analyze(input, catalog, manifest.Sources.Select(p => Path.GetRelativePath(root, p).Replace('\\', '/')));
        // Block unresolved portability issues; editing the copied source and re-running is the supported escape path.
        if (analysis.Blockers > 0) {
            JsonFiles.Write(Path.Combine(root, "build-preflight.json"), analysis); File.WriteAllText(Path.Combine(root, "BUILD-PREFLIGHT.md"), analysis.ToMarkdown());
            throw new InvalidDataException($"Build preflight found {analysis.Blockers} blocker(s). See BUILD-PREFLIGHT.md in the project.");
        }
        bool descriptor = sourceFiles.Values.Any(f => CppLexer.Tokens(f.Text).Any(t => SourceAnalyzer.IsDescriptorMacro(t.Text) || t.Text == "shvPluginQuery"));
        if (!descriptor) throw new InvalidDataException("No NX plugin descriptor found. Add SHV_NX_PLUGIN once after ScriptMain.");
        Directory.CreateDirectory(Path.GetDirectoryName(output)!);
        var args = new List<string> { Path.GetFullPath(Path.Combine(settings.SdkPath, "tools", "build_plugin.py")), "--source" };
        args.AddRange(manifest.Sources); args.AddRange(["--output", output, "--devkitpro", Path.GetFullPath(settings.DevkitProPath)]);
        foreach (var inc in manifest.Includes) args.AddRange(["--include", inc]);
        var messages = new StringBuilder(); var gate = new object();
        void Record(string s) { lock (gate) { if (messages.Length < 4 * 1024 * 1024) messages.AppendLine(s); } log(s); }
        Record("Building " + manifest.Id + " with the selected Switch SDK…");
        try {
            int exit = await RunAsync(settings.PythonPath, args, root, Record, cancel);
            if (exit != 0) throw new InvalidOperationException("Compiler/build failed (exit " + exit + "). See build.log and the compiler messages.");
            cancel.ThrowIfCancellationRequested(); string nro = Path.Combine(output, "plugin.nro"); var info = NroInspector.Inspect(nro);
            using (var readBack = JsonDocument.Parse(File.ReadAllText(Path.Combine(output, "plugin-build.json")))) {
                if (readBack.RootElement.GetProperty("nro_sha256").GetString() != info.Sha256) throw new InvalidDataException("NRO hash differs from the builder's manifest.");
                var hashes = readBack.RootElement.GetProperty("source_sha256");
                if (hashes.GetProperty("include/natives.h").GetString() != catalog.HeaderSha256) throw new InvalidDataException("The SDK changed during compilation; no install ZIP was produced.");
                for (int i = 0; i < manifest.Sources.Count; i++) {
                    string expected = sourceFiles[Path.GetRelativePath(root, manifest.Sources[i]).Replace('\\', '/')].Sha256;
                    if (hashes.GetProperty(i + "-" + Path.GetFileName(manifest.Sources[i])).GetString() != expected) throw new InvalidDataException("Source changed after preflight; build again with a stable project.");
                }
                foreach (var h in hashes.EnumerateObject().Where(p => p.Name.StartsWith("headers/include-", StringComparison.Ordinal))) {
                    string[] parts = h.Name.Split('/'); int index = int.Parse(parts[1][8..], System.Globalization.CultureInfo.InvariantCulture);
                    if (index >= manifest.Includes.Count) throw new InvalidDataException("Unexpected include snapshot.");
                    string original = Path.GetRelativePath(root, Path.Combine(manifest.Includes[index], Path.Combine(parts[2..]))).Replace('\\', '/');
                    if (!sourceFiles.TryGetValue(original, out var expected) || h.Value.GetString() != expected.Sha256) throw new InvalidDataException("Headers changed after preflight; build again with a stable project.");
                }
            }
            SharedSdAccess.ValidateBundle(output);
            var package = createInstallZip ? Path.Combine(output, manifest.Id + "-Switch-test.zip") : "";
            if (createInstallZip) {
              foreach (string legal in new[] { "LICENSE", "NOTICE" }) if (!File.Exists(Path.Combine(settings.SdkPath, legal))) throw new InvalidDataException("SDK is missing " + legal + "; use the complete SDK to package required attribution.");
              using (var zip = ZipFile.Open(package, ZipArchiveMode.Create)) {
                foreach(string relative in SharedSdAccess.PackageFiles) zip.CreateEntryFromFile(Path.Combine(output,relative),relative,CompressionLevel.Optimal);
                zip.CreateEntryFromFile(nro, "switch/gtav-scripthook/plugins/" + manifest.Id + ".nro", CompressionLevel.Optimal);
                foreach (string legal in new[] { "LICENSE", "NOTICE" }) zip.CreateEntryFromFile(Path.Combine(settings.SdkPath, legal), legal + "-ScriptHookV-NX.txt", CompressionLevel.Optimal);
                string originals = Path.Combine(root, "original");
                if (Directory.Exists(originals)) foreach (string file in Directory.EnumerateFiles(originals, "*", SearchOption.AllDirectories).Where(InputLoader.IsLegalNotice)) {
                    if ((File.GetAttributes(file) & FileAttributes.ReparsePoint) != 0 || new FileInfo(file).Length > 1024 * 1024) throw new InvalidDataException("Invalid original license/notice file.");
                    zip.CreateEntryFromFile(file, "licenses/original/" + Path.GetRelativePath(originals, file).Replace('\\', '/'), CompressionLevel.Optimal);
                }
                if (Directory.Exists(originals)) foreach (string file in Directory.EnumerateFiles(originals,"*.ini",SearchOption.TopDirectoryOnly)) {
                    string name=InputLoader.SafeRelative(Path.GetFileName(file));
                    if ((File.GetAttributes(file)&FileAttributes.ReparsePoint)!=0||new FileInfo(file).Length>16384)throw new InvalidDataException("Invalid INI defaults file.");
                    zip.CreateEntryFromFile(file,"switch/gtav-scripthook/settings/"+manifest.Id+"/"+name,CompressionLevel.Optimal);
                }
                using var writer = new StreamWriter(zip.CreateEntry("READ-ME-FIRST.md").Open(), new UTF8Encoding(false));
                writer.Write($"# {manifest.Id} — Switch test build\n\nRequires ScriptHookV-NX for GTA V build {catalog.TargetBuild}. Close GTA, then copy atmosphere and switch to the SD root. For a customized main.npdm, run Install-SD-Access.cmd first and preserve its patched file. Restart GTA to load the plugin.\n\nSettings use settings/{manifest.Id}/ with the updated runtime. Upgrade the runtime and run existing mods once to migrate older flat settings before extracting default INIs. Preserve customized files.\n\nThis source port passed local compilation and NRO structural checks. Switch gameplay testing is required.\n\nTo disable: close GTA and rename {manifest.Id}.nro to {manifest.Id}.nro.disabled.\n\nBuilt with ScriptHookV-NX by MEGATARD (@Geekmaxver). Source: https://github.com/Geekmaxxer/ScriptHookV-NX\n");
              }
            }
            JsonFiles.Write(Path.Combine(output, "port-build.json"), new { tool_version = "0.6.0", plugin_id = manifest.Id, target_build = catalog.TargetBuild, header_sha256 = catalog.HeaderSha256, nro = info, install_zip_created = createInstallZip, source_hashes = sourceFiles.ToDictionary(x => x.Key, x => x.Value.Sha256), switch_runtime_verified = false });
            Record("NRO structural checks passed. " + (createInstallZip ? "Install ZIP: " + package : "NRO: " + nro));
            return new(output, nro, package, info);
        } finally {
            Directory.CreateDirectory(output); lock (gate) File.WriteAllText(Path.Combine(output, "build.log"), messages.ToString(), new UTF8Encoding(false));
        }
    }
    static IEnumerable<SourceFile> EnumerateHeaders(string include, string root, CancellationToken cancel) {
        var pending = new Stack<string>(); pending.Push(include); int count = 0; long bytes = 0;
        while (pending.Count > 0) foreach (var p in Directory.EnumerateFileSystemEntries(pending.Pop())) {
            cancel.ThrowIfCancellationRequested(); if (++count > 8192) throw new InvalidDataException("Include tree contains too many entries.");
            var attributes = File.GetAttributes(p); if ((attributes & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Include tree contains a symbolic link.");
            if ((attributes & FileAttributes.Directory) != 0) { pending.Push(p); continue; }
            if (!new[] { ".h", ".hpp", ".hh", ".hxx", ".inc", ".inl", ".ipp", ".tpp" }.Contains(Path.GetExtension(p).ToLowerInvariant())) continue;
            long size = new FileInfo(p).Length; if (size > 2 * 1024 * 1024 || (bytes += size) > 16 * 1024 * 1024) throw new InvalidDataException("Selected headers exceed the SDK include limits.");
            var data = File.ReadAllBytes(p); string text = new UTF8Encoding(false, true).GetString(data).TrimStart('\ufeff');
            yield return new(Path.GetRelativePath(root, p).Replace('\\', '/'), text, JsonFiles.Hash(data));
        }
    }
}
