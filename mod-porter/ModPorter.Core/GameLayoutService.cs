using System.Text;
using System.Text.Json;

namespace ModPorter.Core;

public sealed record GameLayoutReport(string Profile, string TitleId, string SdRoot, string Romfs,
    string[] MissingOptional, string[] Placeholders, string[] CoreProblems, string[] InvalidPaths,
    string[] Findings, bool RecoveryAllowed, string? Record) {
    public string Text() => $"GTA game layout — {Profile}\n" +
        $"Missing optional paths: {MissingOptional.Length}; placeholders: {Placeholders.Length}; base problems: {CoreProblems.Length}; invalid paths: {InvalidPaths.Length}\n\n" +
        string.Join("\n", MissingOptional.Select(p => "Missing: " + p).Concat(Findings)) +
        (Record == null ? "" : "\n\nRecovery record: " + Record);
}

public static class GameLayoutService {
    public static async Task<GameLayoutReport> RunAsync(string sdk, string sdRoot, bool recover = false, CancellationToken cancel = default) {
        string script = Path.Combine(sdk, "packaging", "Check-Game-Layout.ps1");
        if (!File.Exists(script)) throw new FileNotFoundException("The SDK is missing Check-Game-Layout.ps1. Select the current SDK in Tools > Build tools.", script);
        var output = new StringBuilder();
        string shell = OperatingSystem.IsWindows() ? "powershell.exe" : "pwsh";
        int exit = await BuildService.RunAsync(shell,
            ["-NoProfile", "-ExecutionPolicy", "Bypass", "-File", Path.GetFullPath(script),
             "-SdRoot", Path.GetFullPath(sdRoot), "-Action", recover ? "Recover" : "Check", "-Json"],
            Path.GetDirectoryName(Path.GetFullPath(script))!, line => { lock (output) output.AppendLine(line); }, cancel);
        try {
            var report = JsonSerializer.Deserialize<GameLayoutReport>(output.ToString(), new JsonSerializerOptions { PropertyNameCaseInsensitive = true });
            if (exit is >= 0 and <= 2 && report is { MissingOptional: not null, Findings: not null }) return report;
        } catch (JsonException) { }
        throw new IOException("Game layout check could not complete: " + output.ToString().Trim());
    }
}
