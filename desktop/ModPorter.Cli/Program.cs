using ModPorter.Core;
using ModPorter.Core.Assets;
using System.Text.Json;

try {
    if (args.Length == 0 || args[0] is "--help" or "help") {
        Console.WriteLine("""
            ScriptHookV-NX Mod Porter 0.6.0
            analyze <source-folder|zip|asi|dll|nro> --sdk <SDK> [--report <new-folder>]
            port <source-folder|zip> --sdk <SDK> --output <new-folder> --id <plugin-id>
                 [--source <relative.cpp> ...] [--keep-dllmain] [--adapt-sleep]
            build <project.json> --sdk <SDK> --devkitpro <root> [--python <executable>] [--nro-only]
            doctor --sdk <SDK> --devkitpro <root> [--python <executable>]
            assets-analyze <folder|zip|rpf|resource> [--cap 1024] [--kind Vehicles]
            assets-convert <folder|zip|rpf|resource> --output <new-folder> [--cap 1024] [--kind Vehicles] [--no-zip]
            check <project.json> --sdk <SDK> --devkitpro <root>
            install <plugin.nro> --id <plugin-id> --sd <SD-root> [--auto]
            restore <installation.json> --sd <SD-root>
            report <runtime-report.txt>
            dlc-prepare <converted-dlc.rpf> --list <current-dlclist.xml> --id <pack-id> --output <new-folder> [--no-zip]
            game-add-dlc <prepared-dlc.rpf> --game <update.rpf|SD-root|game-folder> --id <pack-id> --output <new-folder>
            game-replace <prepared-asset> --archive <current.rpf> --entry <path/in/archive> --output <new-folder>
            archive-list <current.rpf>
            addon-check <prepared-dlc.rpf>
            addon-batch <folder-with-pack-id-subfolders> --game <current-update.rpf|SD-root> --output <new-folder>
            addon-ped <source-folder|zip> --id <dlc-id> --model <model> --template <peds.meta|common.rpf>
                      --base-ped <target-ped-name> --output <new-folder> [--cap 1024] [--no-zip]

            ASI analysis does not execute or convert Windows binaries.
            Asset preparation also saves a ZIP unless --no-zip is set. NRO builds use --nro-only.
            Console testing remains required for prepared mods.
            """);
        return 0;
    }
    string Get(string name, string? fallback = null) {
        int i = Array.IndexOf(args, name); return i >= 0 && i + 1 < args.Length ? args[i + 1] : fallback ?? throw new ArgumentException("Missing " + name);
    }
    using var cts = new CancellationTokenSource(); Console.CancelKeyPress += (_, e) => { e.Cancel = true; cts.Cancel(); };
    var settings = new ToolSettings(Get("--sdk", Path.Combine(AppContext.BaseDirectory, "SDK")), Get("--python", "python"), Get("--devkitpro", Environment.GetEnvironmentVariable("DEVKITPRO") ?? "C:/devkitPro"));
    switch (args[0]) {
        case "addon-batch": {
            var result = AddonBatch.Prepare(AddonBatch.Discover(args[1]), Get("--game"), Get("--output"), cts.Token, Console.WriteLine);
            Console.WriteLine(result.Manifest); return 0;
        }
        case "addon-check": {
            var report = AddonAudit.Inspect(File.ReadAllBytes(args[1]), cts.Token);
            Console.WriteLine(report.Text()); return report.ReadyForTest ? 0 : 2;
        }
        case "addon-ped": {
            var options = new PedAddonOptions(Get("--id"), Get("--model"), Get("--template"), Get("--base-ped"), int.Parse(Get("--cap", "1024")));
            var result = AddonBuilder.Ped(args[1], Get("--output"), options, cts.Token, createZip: !args.Contains("--no-zip"));
            Console.WriteLine(result.Audit.Text()); Console.WriteLine(result.DlcArchive);
            Console.WriteLine(result.Archive.Length > 0 ? result.Archive : result.Directory); return 0;
        }
        case "check": {
            string result = await CompilerCheckService.CheckAsync(args[1], settings, Console.WriteLine, cts.Token);
            return result.Contains("checks failed") ? 2 : 0;
        }
        case "install": Console.WriteLine(DeploymentService.InstallNro(args[1],Get("--id"),Get("--sd"),!args.Contains("--auto"))); return 0;
        case "restore": DeploymentService.Restore(args[1],Get("--sd")); Console.WriteLine("Previous plugin file restored."); return 0;
        case "report": Console.WriteLine(JsonSerializer.Serialize(RuntimeReportService.Read(args[1]),JsonFiles.Options)); return 0;
        case "dlc-prepare": Console.WriteLine(DlcPackageService.Prepare(args[1],Get("--list"),Get("--id"),Get("--output"),cts.Token,createZip: !args.Contains("--no-zip"))); return 0;
        case "game-add-dlc": Console.WriteLine(GameAssetUpdater.AddDlc(args[1],Get("--game"),Get("--id"),Get("--output"),cts.Token,Console.WriteLine).Directory); return 0;
        case "game-replace": Console.WriteLine(GameAssetUpdater.ReplaceAsset(args[1],Get("--archive"),Get("--entry"),Get("--output"),cts.Token,Console.WriteLine).UpdatedArchive); return 0;
        case "archive-list": foreach(var entry in RpfFileEditor.List(args[1],cts.Token))Console.WriteLine(entry.Path);return 0;
        case "assets-analyze": {
            var result = AssetService.Analyze(args[1], new(int.Parse(Get("--cap", "1024")), Get("--kind", "Auto")), cts.Token);
            Console.WriteLine(result.Text()); return 0;
        }
        case "assets-convert": {
            var result = AssetService.Convert(args[1], Get("--output"), new(int.Parse(Get("--cap", "1024")), Get("--kind", "Auto")), cts.Token, createZip: !args.Contains("--no-zip"));
            Console.WriteLine(result.Report.Text()); Console.WriteLine(result.Archive.Length > 0 ? result.Archive : result.Directory); return 0;
        }
        case "doctor": {
            var checks = await BuildService.DoctorAsync(settings, cts.Token);
            foreach (var c in checks) Console.WriteLine($"{(c.Passed ? "OK" : "FAIL")} {c.Name}: {c.Detail}");
            return checks.All(c => c.Passed) ? 0 : 2;
        }
        case "analyze": {
            if (args.Length < 2) throw new ArgumentException("Provide an input path.");
            var input = InputLoader.Load(args[1], cts.Token);
            var report = input.IsBinary ? (Path.GetExtension(input.Path).Equals(".nro", StringComparison.OrdinalIgnoreCase) ? NroInspector.Report(input.Path) : AsiAnalyzer.Analyze(input.Path, cts.Token)) : SourceAnalyzer.Analyze(input, NativeCatalog.Load(settings.SdkPath));
            Console.WriteLine(JsonSerializer.Serialize(report, JsonFiles.Options));
            if (args.Contains("--report")) {
                string folder = Path.GetFullPath(Get("--report")); if (Directory.Exists(folder)) throw new IOException("Report directory already exists.");
                Directory.CreateDirectory(folder); JsonFiles.Write(Path.Combine(folder, "analysis.json"), report); File.WriteAllText(Path.Combine(folder, "REPORT.md"), report.ToMarkdown());
            }
            return report.Blockers > 0 ? 2 : 0;
        }
        case "port": {
            if (args.Length < 2) throw new ArgumentException("Provide an input path.");
            var input = InputLoader.Load(args[1], cts.Token); var selected = new List<string>();
            for (int i = 0; i + 1 < args.Length; i++) if (args[i] == "--source") selected.Add(args[i + 1]);
            var result = PortProject.Create(input, NativeCatalog.Load(settings.SdkPath), settings.SdkPath, Get("--output"), new(Get("--id"), !args.Contains("--keep-dllmain"), args.Contains("--adapt-sleep")), selected.Count > 0 ? selected : null, cts.Token);
            Console.WriteLine(result.ProjectFile); Console.WriteLine(result.Report.Status + $"; {result.Report.Blockers} blocker(s), {result.Changes.Count} recorded edits.");
            return result.Report.Blockers > 0 ? 2 : 0;
        }
        case "build": {
            if (args.Length < 2) throw new ArgumentException("Provide project.json.");
            var result = await BuildService.BuildAsync(args[1], settings, Console.WriteLine, cts.Token, createInstallZip: !args.Contains("--nro-only")); Console.WriteLine(result.InstallZip.Length > 0 ? result.InstallZip : result.NroPath); return 0;
        }
        default: throw new ArgumentException("Unknown command; run --help.");
    }
} catch (OperationCanceledException) { Console.Error.WriteLine("Cancelled. Partial outputs are preserved for diagnosis."); return 130; }
catch (Exception error) when (error is not OutOfMemoryException) { Console.Error.WriteLine(error.Message); return 1; }
