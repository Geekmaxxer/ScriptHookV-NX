namespace ModPorter.Core;

public sealed record RuntimeReport(string Title, IReadOnlyDictionary<string,string> Values, IReadOnlyList<string> Findings);
public static class RuntimeReportService {
    public static RuntimeReport Read(string path) {
        if (new FileInfo(path).Length > 128 * 1024) throw new InvalidDataException("Report exceeds 128 KiB.");
        var lines = File.ReadAllLines(path); if (lines.Length > 4096) throw new InvalidDataException("Report has too many lines.");
        var values = new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase); var findings = new List<string>();
        string currentPlugin = "";
        foreach (string line in lines) {
            int split = line.IndexOf('='); if (split < 1) continue;
            string key = line[..split].Trim(), value = line[(split+1)..].Trim();
            if (key == "plugin") currentPlugin = value;
            values[currentPlugin.Length == 0 ? key : currentPlugin + ":" + key] = value;
            bool Nonzero(string s) {
                if (s.Equals("none", StringComparison.OrdinalIgnoreCase) || s.Length == 0) return false;
                return s.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                    ? !ulong.TryParse(s[2..], System.Globalization.NumberStyles.HexNumber, System.Globalization.CultureInfo.InvariantCulture, out var n) || n != 0
                    : !long.TryParse(s, out var decimalValue) || decimalValue != 0;
            }
            if ((key == "error" || key.EndsWith("_error") || key.EndsWith("_failures") || key.EndsWith("_failed")) && Nonzero(value)) findings.Add((currentPlugin.Length > 0 ? currentPlugin + ": " : "") + key + " = " + value);
            if (key == "error_name" && value != "none") findings.Add(currentPlugin + ": " + value);
            if (key == "disarms") findings.Add("Recorded disarms: " + value);
        }
        if (!values.Any()) throw new InvalidDataException("No key=value runtime fields found.");
        if (findings.Count == 0) findings.Add("No nonzero failure fields found. This report alone does not prove gameplay correctness.");
        return new(lines.FirstOrDefault() ?? "Runtime report", values, findings);
    }
}
