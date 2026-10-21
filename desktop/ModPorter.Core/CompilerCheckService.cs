using System.Text;

namespace ModPorter.Core;
public static class CompilerCheckService {
    public static async Task<string> CheckAsync(string project, ToolSettings settings, Action<string> log, CancellationToken cancel = default) {
        var manifest = PortProject.Read(project); var catalog = NativeCatalog.Load(settings.SdkPath);
        if (manifest.HeaderHash.Length > 0 && manifest.HeaderHash != catalog.HeaderSha256) throw new InvalidDataException("Project SDK fingerprint differs.");
        string compiler = Path.Combine(settings.DevkitProPath,"devkitA64","bin",OperatingSystem.IsWindows()?"aarch64-none-elf-g++.exe":"aarch64-none-elf-g++");
        var output = new StringBuilder(); bool passed = true;
        foreach (string source in manifest.Sources) {
            cancel.ThrowIfCancellationRequested();
            var args = new List<string> { "-std=c++17", "-march=armv8-a+crc", "-ffreestanding", "-fno-exceptions", "-fno-rtti", "-fsyntax-only", "-Wall", "-Wextra", "-Werror", "-I" + Path.Combine(settings.SdkPath,"include") };
            args.AddRange(manifest.Includes.Select(i=>"-I"+i)); args.Add(source);
            int result = await BuildService.RunAsync(compiler,args,Path.GetDirectoryName(Path.GetFullPath(project))!,s=>{lock(output){output.AppendLine(s);log(s);}},cancel);
            passed &= result == 0;
        }
        string resultText = passed ? "Compiler syntax/type checks passed for all selected translation units. Switch gameplay is not established." : "Compiler checks failed. Review the diagnostics above.";
        log(resultText); output.AppendLine(resultText);
        return output.ToString();
    }
}
