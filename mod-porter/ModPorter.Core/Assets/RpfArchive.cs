using System.Buffers.Binary;
using System.Text;

namespace ModPorter.Core.Assets;

public sealed class AssetNode {
    public string Name { get; set; } = "";
    public bool Directory { get; init; }
    public List<AssetNode> Children { get; } = [];
    public byte[] Data { get; set; } = [];
    public bool Resource { get; init; }
    public uint SystemFlags { get; set; }
    public uint GraphicsFlags { get; set; }
}
public static class RpfArchive {
    public const int MaxBytes = 512 * 1024 * 1024;
    const uint Magic = 0x52504637, Open = 0x4e45504f;
    static string EngineName(string name) => string.Concat(name.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + ('a' - 'A')) : c));
    static void Need(bool ok, string message) { if (!ok) throw new InvalidDataException(message); }
    static uint U24(byte[] b, int at) => (uint)(b[at] | b[at + 1] << 8 | b[at + 2] << 16);
    static void W24(byte[] b, int at, uint value) { b[at] = (byte)value; b[at + 1] = (byte)(value >> 8); b[at + 2] = (byte)(value >> 16); }
    public static bool IsArchive(byte[] data) => data.Length >= 16 && BinaryPrimitives.ReadUInt32LittleEndian(data) == Magic;
    public static AssetNode Read(byte[] data, CancellationToken cancel = default) {
        Need(data.Length <= MaxBytes && IsArchive(data), "Expected an RPF7 archive (maximum 512 MiB).");
        uint U32(int at) { Need(at >= 0 && at <= data.Length - 4, "Truncated RPF field."); return BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(at)); }
        uint count = U32(4), namesWord = U32(8), encryption = U32(12);
        Need(encryption == Open || encryption == 0, "Encrypted RPF TOCs are unsupported. Save an OPEN/decrypted copy in OpenIV or CodeWalker first.");
        int shift = (int)(namesWord >> 28), namesLength = (int)(namesWord & 0xfffffff);
        Need(count is >= 1 and <= 65536 && namesLength is >= 1 and <= 1048576 && shift <= 4, "Invalid RPF table limits.");
        int namesStart = checked(16 + (int)count * 16); Need((long)namesStart + namesLength <= data.Length, "RPF name pool exceeds the archive.");
        var used = new HashSet<int>(); var occupied = new List<(long Start, long End)>(); long decoded = 0;
        AssetNode ReadNode(int index, int depth) {
            cancel.ThrowIfCancellationRequested(); Need(index >= 0 && index < count && depth <= 32 && used.Add(index), "Cyclic/aliased RPF directory table.");
            int at = 16 + index * 16; bool directory = U32(at + 4) == 0x7fffff00;
            uint nameOffset = directory ? U32(at) : BinaryPrimitives.ReadUInt16LittleEndian(data.AsSpan(at));
            Need(nameOffset < namesLength, "RPF name points outside the name pool.");
            int start = namesStart + (int)nameOffset, end = Array.IndexOf(data, (byte)0, start, namesLength - (int)nameOffset);
            Need(end >= 0 && end - start <= 180, "Invalid RPF entry name.");
            string name = new UTF8Encoding(false, true).GetString(data, start, end - start);
            if (index != 0 || name.Length > 0) { InputLoader.SafeRelative(name); Need(!name.Contains('/') && !name.Contains('\\'), "RPF entry names must be single path segments."); }
            var node = new AssetNode { Name = name, Directory = directory, Resource = !directory && (data[at + 7] & 0x80) != 0 };
            if (directory) {
                uint first = U32(at + 8), children = U32(at + 12); Need(children <= count && (ulong)first + children <= count, "RPF child range is invalid.");
                var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                for (int i = 0; i < children; i++) { var child = ReadNode(checked((int)first + i), depth + 1); Need(names.Add(child.Name), "Duplicate/case-colliding RPF entry names."); node.Children.Add(child); }
            } else {
                uint size = U24(data, at + 2), offset = U24(data, at + 5) & 0x7fffff;
                long physical = (long)offset << (9 + shift); Need(physical >= namesStart + namesLength && physical <= data.Length - (node.Resource ? 16 : 0), "RPF payload offset is invalid.");
                if (node.Resource && size == 0xffffff) size = (uint)(data[physical + 7] | data[physical + 14] << 8 | data[physical + 5] << 16 | data[physical + 2] << 24);
                uint uncompressed = U32(at + 8);
                if (!node.Resource) { Need(U32(at + 12) == 0, "Encrypted RPF file entry is unsupported."); if (size == 0) size = uncompressed; }
                Need((size > 0 || !node.Resource && uncompressed == 0) && size <= ResourceContainer.MaxBytes && (node.Resource || uncompressed <= ResourceContainer.MaxBytes) && physical + size <= data.Length, "RPF payload size is invalid or too large.");
                Need(!occupied.Any(r => physical < r.End && physical + size > r.Start), "Overlapping RPF payloads are unsupported."); occupied.Add((physical, physical + size));
                byte[] stored = data.AsSpan((int)physical, (int)size).ToArray();
                if (node.Resource) {
                    node.SystemFlags = U32(at + 8); node.GraphicsFlags = U32(at + 12);
                    Need(stored.Length >= 16, "Truncated resource header.");
                    BinaryPrimitives.WriteUInt32LittleEndian(stored, 0x37435352);
                    BinaryPrimitives.WriteUInt32LittleEndian(stored.AsSpan(4), ((node.SystemFlags >> 28) << 4) | (node.GraphicsFlags >> 28));
                    BinaryPrimitives.WriteUInt32LittleEndian(stored.AsSpan(8), node.SystemFlags); BinaryPrimitives.WriteUInt32LittleEndian(stored.AsSpan(12), node.GraphicsFlags);
                    node.Data = stored;
                } else {
                    try { node.Data = U24(data, at + 2) != 0 ? ResourceContainer.Inflate(stored, ResourceContainer.MaxBytes, checked((int)uncompressed)) : stored; }
                    catch (InvalidDataException e) { throw new InvalidDataException(name + ": " + e.Message, e); }
                }
                decoded += node.Data.Length; Need(decoded <= MaxBytes, "RPF decoded payloads exceed 512 MiB.");
            }
            return node;
        }
        var root = ReadNode(0, 0); Need(root.Directory && used.Count == count, "RPF contains unreachable entries or no root directory."); return root;
    }
    public static byte[] Write(AssetNode root, CancellationToken cancel = default) {
        Need(root.Directory, "RPF root must be a directory.");
        var entries = new List<AssetNode> { root }; var seen = new HashSet<AssetNode> { root }; var ranges = new Dictionary<AssetNode, (int First, int Count)>();
        for (int i = 0; i < entries.Count; i++) if (entries[i].Directory) {
            var dir = entries[i]; var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var child in dir.Children) { Need(seen.Add(child), "Cyclic/aliased output RPF tree."); InputLoader.SafeRelative(child.Name); Need(!child.Name.Contains('/') && !child.Name.Contains('\\') && Encoding.UTF8.GetByteCount(child.Name) <= 180, "Invalid output RPF name."); Need(names.Add(child.Name), "Converted archive has colliding entry names."); }
            // The engine lowercases each path segment and binary-searches the
            // directory range. Sort by the exact names written to the pool;
            // case-insensitive ordering alone does not satisfy that contract.
            ranges[dir] = (entries.Count, dir.Children.Count); entries.AddRange(dir.Children.OrderBy(c => EngineName(c.Name), StringComparer.Ordinal)); Need(entries.Count <= 65536, "RPF contains too many entries.");
        }
        using var namesBuffer = new MemoryStream(); var offsets = new List<int>();
        foreach (var entry in entries) { offsets.Add((int)namesBuffer.Length); byte[] name = Encoding.UTF8.GetBytes(EngineName(entry.Name)); namesBuffer.Write(name); namesBuffer.WriteByte(0); }
        Need(namesBuffer.Length <= 65535, "RPF name pool exceeds supported 16-bit file-name offsets.");
        byte[] table = new byte[16 + entries.Count * 16 + namesBuffer.Length];
        void W32(int at, uint v) => BinaryPrimitives.WriteUInt32LittleEndian(table.AsSpan(at), v);
        W32(0, Magic); W32(4, (uint)entries.Count); W32(8, (uint)namesBuffer.Length); W32(12, Open); namesBuffer.ToArray().CopyTo(table, 16 + entries.Count * 16);
        using var output = new MemoryStream(); output.Write(table);
        for (int i = 0; i < entries.Count; i++) {
            cancel.ThrowIfCancellationRequested(); var entry = entries[i]; int at = 16 + i * 16;
            if (entry.Directory) { W32(at, (uint)offsets[i]); W32(at + 4, 0x7fffff00); W32(at + 8, (uint)ranges[entry].First); W32(at + 12, (uint)ranges[entry].Count); continue; }
            while (output.Position % 512 != 0) output.WriteByte(0);
            Need(output.Position + entry.Data.Length <= MaxBytes, "Converted archive exceeds 512 MiB.");
            BinaryPrimitives.WriteUInt16LittleEndian(table.AsSpan(at), (ushort)offsets[i]); uint length = (uint)entry.Data.Length;
            W24(table, at + 2, entry.Resource ? Math.Min(length, 0xffffff) : 0);
            W24(table, at + 5, (uint)(output.Position / 512) | (entry.Resource ? 0x800000u : 0));
            if (entry.Resource) {
                Need(length >= 16 && BinaryPrimitives.ReadUInt32LittleEndian(entry.Data) == 0x37435352, "Resource entry has no standalone RSC7 header.");
                W32(at + 8, BinaryPrimitives.ReadUInt32LittleEndian(entry.Data.AsSpan(8))); W32(at + 12, BinaryPrimitives.ReadUInt32LittleEndian(entry.Data.AsSpan(12)));
                byte[] payload = (byte[])entry.Data.Clone();
                if (length >= 0xffffff) { payload[7] = (byte)length; payload[14] = (byte)(length >> 8); payload[5] = (byte)(length >> 16); payload[2] = (byte)(length >> 24); }
                output.Write(payload);
            } else { W32(at + 8, length); W32(at + 12, 0); output.Write(entry.Data); }
        }
        while (output.Position % 512 != 0) output.WriteByte(0);
        output.Position = 0; output.Write(table); return output.ToArray();
    }
}
