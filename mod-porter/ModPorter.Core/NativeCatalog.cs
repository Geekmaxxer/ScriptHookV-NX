using System.Text.Json;

namespace ModPorter.Core;

public sealed record NativeEntry(string Name, string Hash, string Signature, bool Registered);
public sealed class NativeCatalog {
    public string TargetBuild { get; private init; } = "";
    public string HeaderSha256 { get; private init; } = "";
    public Dictionary<string, NativeEntry> Entries { get; } = new(StringComparer.Ordinal);
    public HashSet<string> Namespaces { get; } = new(StringComparer.Ordinal);
    public static NativeCatalog Load(string sdk) {
        string index = Path.Combine(sdk, "sdk", "native-index.json"), header = Path.Combine(sdk, "include", "natives.h");
        if (!File.Exists(index) || !File.Exists(header)) throw new InvalidDataException("SDK is missing sdk/native-index.json or include/natives.h.");
        if (new FileInfo(index).Length > 8 * 1024 * 1024) throw new InvalidDataException("Native index exceeds 8 MiB.");
        using var doc = JsonDocument.Parse(File.ReadAllText(index)); var root = doc.RootElement;
        string hash = root.GetProperty("header_sha256").GetString()!;
        if (root.GetProperty("format").GetInt32() != 1 || !hash.Equals(JsonFiles.HashFile(header), StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Native index does not match the SDK headers. Choose a matched ScriptHookV-NX SDK.");
        var catalog = new NativeCatalog { TargetBuild = root.GetProperty("target_build").GetString()!, HeaderSha256 = hash };
        foreach (var n in root.GetProperty("natives").EnumerateArray()) {
            var e = new NativeEntry(n.GetProperty("name").GetString()!, n.GetProperty("hash").GetString()!, n.GetProperty("signature").GetString()!, n.GetProperty("registered").GetBoolean());
            catalog.Entries.Add(e.Name, e); catalog.Namespaces.Add(e.Name.Split("::")[0]);
        }
        return catalog;
    }
    public IEnumerable<NativeEntry> Search(string query, int limit = 100) => Entries.Values.Where(n => n.Name.Contains(query, StringComparison.OrdinalIgnoreCase) || n.Hash.Equals(query, StringComparison.OrdinalIgnoreCase)).OrderByDescending(n => n.Registered).ThenBy(n => n.Name).Take(limit);
}
