using System.Buffers.Binary;
using System.Text;

namespace ModPorter.Core;

public static class AsiAnalyzer {
    public static AnalysisReport Analyze(string path, CancellationToken cancel = default) {
        if (new FileInfo(path).Length > 128L * 1024 * 1024) throw new InvalidDataException("PE input exceeds 128 MiB.");
        byte[] bytes = File.ReadAllBytes(path);
        void Need(long at, long size) { if (at < 0 || size < 0 || at > bytes.Length - size) throw new InvalidDataException("Truncated or malformed PE image."); }
        ushort U16(long at) { Need(at, 2); return BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan((int)at, 2)); }
        uint U32(long at) { Need(at, 4); return BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan((int)at, 4)); }
        ulong U64(long at) { Need(at, 8); return BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan((int)at, 8)); }
        if (U16(0) != 0x5a4d) throw new InvalidDataException("Missing Windows MZ executable header.");
        long pe = U32(0x3c); if (U32(pe) != 0x00004550) throw new InvalidDataException("Missing PE signature.");
        ushort machine = U16(pe + 4), count = U16(pe + 6), optionalSize = U16(pe + 20), characteristics = U16(pe + 22);
        if (count is < 1 or > 96) throw new InvalidDataException("PE must have 1-96 sections.");
        long opt = pe + 24; Need(opt, optionalSize);
        ushort magic = U16(opt); bool is64 = magic == 0x20b;
        if (magic != 0x10b && !is64) throw new InvalidDataException("Unsupported PE optional header.");
        int minimum = is64 ? 112 : 96; if (optionalSize < minimum) throw new InvalidDataException("Optional header is too small.");
        uint directories = U32(opt + minimum - 4); if (directories > 16 || minimum + directories * 8 > optionalSize) throw new InvalidDataException("PE data directories exceed the optional header.");
        uint headerSize = U32(opt + 60); if (headerSize > bytes.Length) throw new InvalidDataException("PE header size exceeds the file.");
        ulong imageBase = is64 ? U64(opt + 24) : U32(opt + 28);
        var sections = new List<(uint Rva, uint VirtualSize, uint Offset, uint Size)>(); long sectionAt = opt + optionalSize;
        for (int i = 0; i < count; i++) {
            long at = sectionAt + i * 40; Need(at, 40);
            var row = (Rva: U32(at + 12), VirtualSize: U32(at + 8), Offset: U32(at + 20), Size: U32(at + 16));
            Need(row.Offset, row.Size); sections.Add(row);
        }
        long Offset(uint rva, int size = 1) {
            if (rva < headerSize && (long)rva + size <= headerSize) { Need(rva, size); return rva; }
            foreach (var s in sections) if (rva >= s.Rva && (ulong)rva - s.Rva + (uint)size <= s.Size) {
                long at = s.Offset + (long)rva - s.Rva; Need(at, size); return at;
            }
            throw new InvalidDataException("PE RVA points outside file-backed sections.");
        }
        string StringAtRva(uint rva) {
            var s = new StringBuilder();
            for (uint i = 0; i < 1024; i++) {
                if ((ulong)rva + i > uint.MaxValue) break;
                byte c = bytes[Offset(rva + i)]; if (c == 0) return s.ToString();
                if (c < 32 || c > 126) throw new InvalidDataException("Non-ASCII import name."); s.Append((char)c);
            }
            throw new InvalidDataException("Unterminated or oversized PE import name.");
        }
        var report = new AnalysisReport { Input = Path.GetFullPath(path), Kind = "asi", Status = "Binary conversion unavailable" };
        report.SourceHashes[Path.GetFileName(path)] = JsonFiles.Hash(bytes);
        report.Details["Architecture"] = machine switch { 0x8664 => "x86-64 (PC)", 0x14c => "x86 (32-bit)", 0xaa64 => "ARM64", _ => $"Unknown (0x{machine:x4})" };
        report.Details["Image type"] = (characteristics & 0x2000) != 0 ? "Windows DLL / ASI" : "Windows executable";
        report.Details["Sections"] = count.ToString(); report.Details["Bytes"] = bytes.Length.ToString();
        report.Findings.Add(new(Severity.Blocker, "BINARY_ONLY", Path.GetFileName(path), 0, "Compiled Windows binaries cannot currently be converted into Switch NROs by this tool.", "Import the mod's C++ source to prepare a native rebuild. This analysis does not load or execute the ASI."));
        if (machine != 0x8664) report.Findings.Add(new(Severity.Review, "PE_ARCHITECTURE", "", 0, "This is not a standard x86-64 GTA V PC plugin.", "Check that the input belongs to the intended game and platform."));
        int importCount = 0;
        uint DirectoryRva(int index) => directories > index ? U32(opt + minimum + index * 8) : 0;
        uint DirectorySize(int index) => directories > index ? U32(opt + minimum + index * 8 + 4) : 0;
        void Thunks(string dll, uint lookup) {
            var names = new List<string>();
            for (uint j = 0; j < 16384; j++) {
                cancel.ThrowIfCancellationRequested();
                long rv = lookup + (long)j * (is64 ? 8 : 4); if (rv > uint.MaxValue) throw new InvalidDataException("Import lookup overflow.");
                long at = Offset((uint)rv, is64 ? 8 : 4); ulong value = is64 ? U64(at) : U32(at); if (value == 0) break;
                if (++importCount > 16384) throw new InvalidDataException("Too many imported functions.");
                bool ordinal = (value & (is64 ? 1UL << 63 : 1UL << 31)) != 0;
                if (!ordinal && value > uint.MaxValue - 2) throw new InvalidDataException("Invalid import-name pointer.");
                names.Add(ordinal ? "ordinal #" + (value & 0xffff) : StringAtRva((uint)value + 2));
                if (j == 16383) throw new InvalidDataException("Unterminated import lookup table.");
            }
            report.Details["Imports: " + dll] = string.Join(", ", names);
            string lower = dll.ToLowerInvariant();
            bool hook = lower == "scripthookv.dll";
            report.Findings.Add(new(hook ? Severity.Info : Severity.Review, hook ? "SCRIPTHOOK_IMPORT" : "WINDOWS_IMPORT", dll, 0,
                hook ? "PC ScriptHookV dependency detected." : "Windows library dependency: " + dll,
                hook ? "Familiar entry/native APIs are available for source ports; binary ABI compatibility is not implemented." : "Port each used API or replace the library with a Switch-compatible implementation."));
        }
        uint imports = DirectoryRva(1), importSize = DirectorySize(1);
        if (imports != 0) {
            if (importSize < 20 || importSize > 1024 * 1024) throw new InvalidDataException("Invalid import directory size.");
            bool ended = false;
            for (uint i = 0; i < Math.Min(importSize / 20, 256); i++) {
                cancel.ThrowIfCancellationRequested();
                if ((ulong)imports + i * 20 + 20 > uint.MaxValue) throw new InvalidDataException("Import directory overflow.");
                long at = Offset(imports + i * 20, 20); uint lookup = U32(at), name = U32(at + 12), first = U32(at + 16);
                if (lookup == 0 && name == 0 && first == 0) { ended = true; break; }
                if (name == 0 || lookup == 0 && first == 0) throw new InvalidDataException("Malformed import descriptor.");
                Thunks(StringAtRva(name), lookup != 0 ? lookup : first);
            }
            if (!ended) throw new InvalidDataException("Import directory has no bounded terminator.");
        }
        if (DirectoryRva(13) != 0) report.Findings.Add(new(Severity.Review, "DELAY_IMPORTS", "", 0, "Delay-loaded dependencies are present and are not enumerated by this version.", "Review delay imports manually; the normal import list is incomplete."));
        if (DirectoryRva(14) != 0) report.Findings.Add(new(Severity.Blocker, "MANAGED_BINARY", "", 0, "CLR/.NET metadata is present.", "ScriptHookVDotNet binaries require a separate managed-runtime/source port."));
        if (DirectoryRva(9) != 0) report.Findings.Add(new(Severity.Review, "TLS", "", 0, "Thread-local storage metadata is present.", "Review TLS and initialization requirements during a source port."));
        report.Details["Imported functions"] = importCount.ToString();
        report.Details["Inspection limits"] = "Imports and PE structure only. Dynamically resolved APIs, native hashes, game memory access and behavior are not proven by this scan.";
        _ = imageBase;
        return report;
    }
}
