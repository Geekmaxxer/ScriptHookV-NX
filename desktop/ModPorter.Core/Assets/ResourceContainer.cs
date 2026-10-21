using System.Buffers.Binary;
using System.IO.Compression;

namespace ModPorter.Core.Assets;

// RSC7 fields describe resource pages, independently of the filename prefix.
public sealed record ResourceContainer(uint Version, uint SystemFlags, uint GraphicsFlags, byte[] System, byte[] Graphics) {
    public const int MaxBytes = 256 * 1024 * 1024;
    public static int PageBytes(uint flags) {
        int[] positions = [27, 26, 25, 24, 17, 11, 7, 5, 4];
        uint[] masks = [1, 1, 1, 1, 127, 63, 15, 3, 1];
        long units = 0;
        for (int i = 0; i < positions.Length; i++) units += ((flags >> positions[i]) & masks[i]) * (1L << i);
        long size = units * (512L << (int)(flags & 15));
        if (size > MaxBytes) throw new InvalidDataException("Resource page allocation exceeds 256 MiB.");
        return (int)size;
    }
    public static byte[] Inflate(byte[] input, int maximum, int? exact = null) {
        using var compressed = new MemoryStream(input, false); using var stream = new DeflateStream(compressed, CompressionMode.Decompress);
        using var output = new MemoryStream(); var buffer = new byte[65536]; int n;
        while ((n = stream.Read(buffer)) != 0) { if (output.Length + n > maximum) throw new InvalidDataException("Compressed data exceeds its allocation limit."); output.Write(buffer, 0, n); }
        if (exact.HasValue && output.Length != exact.Value) throw new InvalidDataException("Resource payload size does not match its page flags.");
        return output.ToArray();
    }
    public static ResourceContainer Read(byte[] data) {
        if (data.Length < 16 || data.Length > MaxBytes || BinaryPrimitives.ReadUInt32LittleEndian(data) != 0x37435352) throw new InvalidDataException("Expected a standalone RSC7 resource.");
        uint version = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(4)), sys = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(8)), gfx = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(12));
        int systemSize = PageBytes(sys), graphicsSize = PageBytes(gfx), total = checked(systemSize + graphicsSize);
        if (total > MaxBytes || systemSize < 16) throw new InvalidDataException("Invalid resource allocation.");
        byte[] payload = Inflate(data[16..], total, total);
        return new(version, sys, gfx, payload[..systemSize], payload[systemSize..]);
    }
    public byte[] Write() {
        if (System.Length != PageBytes(SystemFlags) || Graphics.Length != PageBytes(GraphicsFlags)) throw new InvalidDataException("Resource allocation mismatch.");
        using var output = new MemoryStream(); using (var header = new BinaryWriter(output, global::System.Text.Encoding.UTF8, true)) { header.Write(0x37435352u); header.Write(Version); header.Write(SystemFlags); header.Write(GraphicsFlags); }
        using (var compressed = new DeflateStream(output, CompressionLevel.Optimal, true)) { compressed.Write(System); compressed.Write(Graphics); }
        return output.ToArray();
    }
    public static uint SinglePageFlags(int bytes, uint versionNibble) {
        if (bytes <= 0) return versionNibble << 28;
        long size = 512; int power = 0; while (size < bytes) { size <<= 1; power++; }
        int exponent = Math.Min(power, 15), weight = power - exponent;
        int[] bits = [27, 26, 25, 24, 17, 11, 7, 5, 4];
        if (weight >= bits.Length || size > MaxBytes) throw new InvalidDataException("Graphics allocation is too large.");
        return (versionNibble << 28) | (uint)exponent | (1u << bits[weight]);
    }
}
