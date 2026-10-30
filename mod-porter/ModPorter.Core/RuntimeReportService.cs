using System.Buffers.Binary;

namespace ModPorter.Core;

public sealed record RuntimeReport(string Title, IReadOnlyDictionary<string,string> Values, IReadOnlyList<string> Findings);
public static class RuntimeReportService {
    public static RuntimeReport Read(string path) {
        if (new FileInfo(path).Length > 128 * 1024) throw new InvalidDataException("Report exceeds 128 KiB.");
        byte[] bytes = File.ReadAllBytes(path);
        if (bytes.AsSpan().StartsWith("AFE2"u8)) return ReadFatal(bytes);
        if (Path.GetExtension(path).Equals(".bin", StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Only the AFE2 Atmosphere fatal format is supported.");
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
            if (key.EndsWith("_worst_us",StringComparison.Ordinal)&&ulong.TryParse(value,out var us)&&us>=2000)
                findings.Add(currentPlugin+": "+key+" = "+us+" us elapsed (includes native work, I/O and OS preemption; not an FPS measurement).");
        }
        if (!values.Any()) throw new InvalidDataException("No key=value runtime fields found.");
        if (findings.Count == 0) findings.Add("No nonzero failure fields found. This report alone does not prove gameplay correctness.");
        return new(lines.FirstOrDefault() ?? "Runtime report", values, findings);
    }
    static RuntimeReport ReadFatal(byte[] bytes) {
        if (bytes.Length != 0x450) throw new InvalidDataException("Invalid AFE2 fatal report size.");
        ulong U64(int at) => BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan(at,8));
        uint error = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(4,4));
        ulong title = U64(8), pc = U64(0x110), module = U64(0x118), count = U64(0x140);
        if (count > 32 || U64(0x148) > 256) throw new InvalidDataException("Invalid AFE2 stack bounds.");
        string Offset(ulong address) => address >= module ? "0x" + (address-module).ToString("X") : "outside module base";
        var values = new Dictionary<string,string> {
            ["format"] = "AFE2", ["program_id"] = title.ToString("X16"), ["error_desc"] = "0x"+error.ToString("X"),
            ["pc"] = "0x"+pc.ToString("X"), ["module_base"] = "0x"+module.ToString("X"), ["pc_module_offset"] = Offset(pc)
        };
        var findings = new List<string> { $"Process: {title:X16}; error: 0x{error:X}" + (error == 0xFFE ? " (std::abort)." : ".") };
        if (title == 0x010041544D530000UL) {
            findings.Add("This process is Atmosphere's ams.mitm service. Its ID is not a DLC identifier. The process ID alone does not identify the cause.");
            findings.Add("For startup failure after DLC removal, check the game layout in Tools > Diagnostics. GAME-STARTUP.md explains the known missing-path recovery and its limits.");
            findings.Add("The service can fail before GTA or ScriptHookV-NX starts; a loader report can therefore predate this launch. Do not delete saves or assume a runtime regression from this process ID.");
        } else findings.Add("This report identifies another process. The DLC path recovery is not a general repair for Atmosphere fatals.");
        findings.Add("Module-relative PC: " + Offset(pc) + ". Symbolization requires the exact binary/version; offsets are not interpreted using another build.");
        for (int i=0; i<(int)count; i++) {
            ulong address = U64(0x150+i*8); if (address == 0) continue;
            values["trace_"+i] = Offset(address); findings.Add("Backtrace " + i + ": " + Offset(address));
        }
        return new("Atmosphere AFE2 fatal report", values, findings);
    }
}
