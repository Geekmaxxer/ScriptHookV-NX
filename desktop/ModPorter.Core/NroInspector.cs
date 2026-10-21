using System.Buffers.Binary;
using System.Text;

namespace ModPorter.Core;

public sealed record NroInfo(int ImageBytes, uint BssBytes, long AccountedBytes, string Sha256, bool QueryExportPresent);
public static class NroInspector {
    public static NroInfo Inspect(string path) {
        if (new FileInfo(path).Length > 2 * 1024 * 1024) throw new InvalidDataException("NRO image exceeds the runtime's 2 MiB image limit.");
        var b = File.ReadAllBytes(path);
        void Need(bool ok, string message) { if (!ok) throw new InvalidDataException(message); }
        uint U32(long at) { Need(at >= 0 && at <= b.Length - 4, "NRO offset out of range."); return BinaryPrimitives.ReadUInt32LittleEndian(b.AsSpan((int)at, 4)); }
        ulong U64(long at) { Need(at >= 0 && at <= b.Length - 8, "NRO offset out of range."); return BinaryPrimitives.ReadUInt64LittleEndian(b.AsSpan((int)at, 8)); }
        Need(b.Length >= 4096 && b.Length % 4096 == 0, "NRO must be page aligned.");
        Need(Encoding.ASCII.GetString(b, 0x10, 4) == "NRO0", "Missing NRO0 header.");
        Need(U32(0x14) == 0 && U32(0x18) == b.Length && U32(0x1c) == 0 && U32(0x20) == 0, "Unsupported NRO header.");
        uint ts = U32(0x24), ro = U32(0x28), rs = U32(0x2c), rw = U32(0x30), ds = U32(0x34), bss = U32(0x38);
        Need(ts >= 4096 && new[] { ts, rs, ds, bss }.All(n => n % 4096 == 0) && ro == ts && (ulong)rw == (ulong)ro + rs && (ulong)rw + ds == (ulong)b.Length && bss <= 2 * 1024 * 1024, "Invalid NRO segment layout.");
        uint mod = U32(4); Need(mod >= 0x80 && (ulong)mod + 28 <= (ulong)b.Length && U32(mod) == 0x30444f4d, "Missing MOD0.");
        long dyn = mod + (long)unchecked((int)U32(mod + 4)); var tags = new Dictionary<ulong, ulong>(); bool end = false;
        for (long at = dyn; at >= 0 && at <= b.Length - 16; at += 16) {
            ulong tag = U64(at), value = U64(at + 8); if (tag == 0) { end = true; break; }
            Need(tag != 1, "NRO has unresolved shared-library dependencies.");
            Need(!(tag is 12 or 13 or 25 or 26 or 32 && value != 0), "Global constructors/destructors are unsupported; use explicit initialization."); tags[tag] = value;
        }
        Need(end && new ulong[] { 4, 5, 6, 10, 11 }.All(tags.ContainsKey) && tags[11] == 24, "Missing/unterminated NRO dynamic symbol tables.");
        Need(tags[4] <= (ulong)b.Length - 8 && tags[5] <= (ulong)b.Length && tags[10] <= (ulong)b.Length - tags[5] && tags[6] <= (ulong)b.Length, "NRO tables exceed the image.");
        uint symbols = U32((long)tags[4] + 4); Need(symbols <= 65536 && tags[6] + (ulong)symbols * 24 <= (ulong)b.Length, "Invalid NRO symbol count.");
        bool found = false;
        for (uint i = 0; i < symbols; i++) {
            long at = (long)tags[6] + i * 24; uint name = U32(at); byte info = b[at + 4], visibility = b[at + 5]; ushort section = BinaryPrimitives.ReadUInt16LittleEndian(b.AsSpan((int)at + 6, 2));
            Need(name < tags[10], "Symbol name out of range."); int begin = checked((int)tags[5] + (int)name), limit = checked((int)(tags[5] + tags[10]));
            int stop = Array.IndexOf(b, (byte)0, begin, limit - begin); Need(stop >= 0, "Unterminated symbol."); string text = Encoding.ASCII.GetString(b, begin, stop - begin);
            Need(section != 0 || name == 0, "Unresolved NRO import: " + text);
            if (text == "shvPluginQuery" && section != 0 && info >> 4 is 1 or 2 && (info & 15) == 2 && (visibility & 3) == 0 && U64(at + 8) < ts) found = true;
        }
        Need(found, "Missing exported shvPluginQuery function.");
        return new(b.Length, bss, 2L * (b.Length + Math.Max(bss, 4096)) + 4096, JsonFiles.Hash(b), true);
    }
    public static AnalysisReport Report(string path) {
        var n = Inspect(path); return new AnalysisReport {
            Input = path, Kind = "nro", Status = "NRO structural checks passed",
            Details = new() { ["Image bytes"] = n.ImageBytes.ToString(), ["BSS bytes"] = n.BssBytes.ToString(), ["Loader accounted bytes"] = n.AccountedBytes.ToString(), ["SHA-256"] = n.Sha256 },
            Findings = [new(Severity.Review, "CONSOLE_TEST", Path.GetFileName(path), 0, "Structure and query export passed inspection; descriptor initialization and game behavior remain untested.", "Install with a compatible runtime and test on Switch.")]
        };
    }
}
