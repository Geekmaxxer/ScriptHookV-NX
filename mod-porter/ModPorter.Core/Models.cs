using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace ModPorter.Core;

public enum Severity { Info, Review, Blocker }
public sealed record Finding(Severity Severity, string Code, string File, int Line, string Message, string Action);
public sealed record NativeUse(string Name, string Hash, string Signature, bool Registered, string File, int Line);
public sealed record SourceFile(string RelativePath, string Text, string Sha256) {
    public bool IsTranslationUnit => InputLoader.SourceExtensions.Contains(Path.GetExtension(RelativePath));
    [JsonIgnore] public byte[]? OriginalBytes { get; init; }
}
public sealed class ModInput {
    public string Path { get; init; } = "";
    public string Name { get; init; } = "";
    public List<SourceFile> Files { get; init; } = [];
    public List<string> SkippedFiles { get; init; } = [];
    public bool IsBinary { get; init; }
}
public sealed class AnalysisReport {
    public int Format { get; set; } = 1;
    public string ToolVersion { get; set; } = "0.2.0";
    public string Input { get; set; } = "";
    public string Kind { get; set; } = "source";
    public string TargetBuild { get; set; } = "";
    public string Status { get; set; } = "Needs review";
    public bool SwitchRuntimeVerified { get; set; }
    public List<Finding> Findings { get; set; } = [];
    public List<NativeUse> Natives { get; set; } = [];
    public List<string> TranslationUnits { get; set; } = [];
    public Dictionary<string, string> SourceHashes { get; set; } = [];
    public Dictionary<string, string> Details { get; set; } = [];
    public int Blockers => Findings.Count(f => f.Severity == Severity.Blocker);
    public int Reviews => Findings.Count(f => f.Severity == Severity.Review);
    public string ToMarkdown() {
        var s = new StringBuilder($"# ScriptHookV-NX Mod Porter report\n\nStatus: {Status}\n\nInput: `{Input.Replace('`', '\'')}`\n\n");
        if (TargetBuild.Length > 0) s.AppendLine($"Target build: `{TargetBuild}`\n");
        s.AppendLine("Static analysis and successful compilation do not establish Switch runtime compatibility.\n");
        foreach (var d in Details) s.AppendLine($"- {d.Key}: {d.Value}");
        foreach (var f in Findings) s.AppendLine($"\n## {f.Severity}: {f.Code}\n\n{f.File}:{f.Line} — {f.Message}\n\n{f.Action}");
        if (Natives.Count > 0) {
            s.AppendLine("\n## Native references\n");
            foreach (var n in Natives) s.AppendLine($"- `{n.Signature}` — {n.Hash}; registration {(n.Registered ? "present" : "not found")}; {n.File}:{n.Line}");
        }
        return s.ToString();
    }
}
public static class JsonFiles {
    public static readonly JsonSerializerOptions Options = new() {
        WriteIndented = true, PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower,
        PropertyNameCaseInsensitive = true, Converters = { new JsonStringEnumConverter() }
    };
    public static void Write<T>(string path, T value) => File.WriteAllText(path, JsonSerializer.Serialize(value, Options) + "\n", new UTF8Encoding(false));
    public static string Hash(byte[] data) => Convert.ToHexString(SHA256.HashData(data)).ToLowerInvariant();
    public static string HashFile(string path) { using var s = File.OpenRead(path); return Convert.ToHexString(SHA256.HashData(s)).ToLowerInvariant(); }
}
