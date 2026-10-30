using System.Buffers.Binary;
using System.Text;

namespace ModPorter.Core.Assets;

public sealed record TextureInfo(string Name, int Width, int Height, int Levels, uint Format, int DataBytes, int Descriptor, int GraphicsOffset, int Stride);
public sealed record TextureChange(string Name, int OriginalWidth, int OriginalHeight, int Width, int Height, int RemovedMips, string Method = "Retained existing encoded mip levels");
public sealed record TextureResult(byte[] Data, int GraphicsBytesBefore, int GraphicsBytesAfter, IReadOnlyList<TextureChange> Changes);
public static class TextureDictionary {
    static void Require(bool ok, string message) { if (!ok) throw new InvalidDataException(message); }
    static ushort U16(byte[] b, int p) { Require(p >= 0 && p <= b.Length - 2, "Texture field out of range."); return BinaryPrimitives.ReadUInt16LittleEndian(b.AsSpan(p)); }
    static uint U32(byte[] b, int p) { Require(p >= 0 && p <= b.Length - 4, "Texture field out of range."); return BinaryPrimitives.ReadUInt32LittleEndian(b.AsSpan(p)); }
    static ulong U64(byte[] b, int p) { Require(p >= 0 && p <= b.Length - 8, "Texture pointer out of range."); return BinaryPrimitives.ReadUInt64LittleEndian(b.AsSpan(p)); }
    static int Offset(ulong value, uint arena, int length, int count) {
        Require((value & 0xfffffffff0000000UL) == arena && (value & 0xfffffff) + (ulong)count <= (ulong)length, "Texture pointer does not belong to the expected resource arena.");
        return (int)(value & 0xfffffff);
    }
    public static int MipBytes(uint format, int width, int height) {
        Require(width is >= 1 and <= 16384 && height is >= 1 and <= 16384, "Invalid texture dimensions.");
        return format switch {
            0x31545844 or 0x31495441 => checked(Math.Max(1, (width + 3) / 4) * Math.Max(1, (height + 3) / 4) * 8),
            0x33545844 or 0x35545844 or 0x32495441 or 0x20374342 => checked(Math.Max(1, (width + 3) / 4) * Math.Max(1, (height + 3) / 4) * 16),
            21 or 22 or 32 => checked(width * height * 4),
            25 => checked(width * height * 2),
            28 or 50 => checked(width * height),
            _ => throw new InvalidDataException($"Unsupported legacy texture format 0x{format:x8}.")
        };
    }
    public static List<TextureInfo> Inspect(ResourceContainer resource) {
        Require(resource.Version == 13, "Texture dictionaries must use Legacy RSC7 version 13. Enhanced/Gen9 dictionaries need a separate conversion.");
        return InspectAt(resource, 0);
    }
    public static List<TextureInfo> InspectAt(ResourceContainer resource, int root) {
        byte[] s = resource.System; Require(root >= 0 && root <= s.Length - 64, "Truncated texture dictionary.");
        int count = U16(s, root + 0x38), capacity = U16(s, root + 0x3a); Require(count <= 8192 && capacity >= count, "Invalid texture pointer list.");
        if (count == 0) { Require(U16(s, root + 0x28) == 0, "Empty texture dictionary has hashes."); return []; }
        int list = Offset(U64(s, root + 0x30), 0x50000000, s.Length, checked(count * 8));
        int hashCount = U16(s, root + 0x28); Require(hashCount == count, "Texture hashes and entries have different counts.");
        _ = Offset(U64(s, root + 0x20), 0x50000000, s.Length, checked(count * 4));
        var result = new List<TextureInfo>(); var descriptors = new HashSet<int>();
        for (int i = 0; i < count; i++) {
            int descriptor = Offset(U64(s, list + i * 8), 0x50000000, s.Length, 144);
            Require(descriptors.Add(descriptor), "Aliased texture descriptors are unsupported.");
            int nameAt = Offset(U64(s, descriptor + 0x28), 0x50000000, s.Length, 1);
            int nameEnd = Array.IndexOf(s, (byte)0, nameAt, Math.Min(1024, s.Length - nameAt)); Require(nameEnd >= 0, "Unterminated texture name.");
            string name = new UTF8Encoding(false, true).GetString(s, nameAt, nameEnd - nameAt);
            int width = U16(s, descriptor + 0x50), height = U16(s, descriptor + 0x52), depth = U16(s, descriptor + 0x54), stride = U16(s, descriptor + 0x56), levels = s[descriptor + 0x5d];
            uint format = U32(s, descriptor + 0x58);
            Require(depth == 1 && s[descriptor + 0x5c] == 0 && levels is >= 1 and <= 15 && width >= 1 && height >= 1 && stride > 0, "Only ordinary two-dimensional Legacy textures are supported.");
            int length = 0;
            for (int m = 0; m < levels; m++) length = checked(length + MipBytes(format, Math.Max(1, width >> m), Math.Max(1, height >> m)));
            int offset = Offset(U64(s, descriptor + 0x70), 0x60000000, resource.Graphics.Length, length);
            result.Add(new(name, width, height, levels, format, length, descriptor, offset, stride));
        }
        return result;
    }
    public static TextureResult Downscale(byte[] data, int maximumDimension, CancellationToken cancel = default) {
        Require(maximumDimension is 0 or 256 or 512 or 1024 or 2048 or 4096, "Texture cap must be Off, 256, 512, 1024, 2048 or 4096.");
        var resource = ResourceContainer.Read(data); var textures = Inspect(resource); var changes = new List<TextureChange>();
        if (maximumDimension == 0 || textures.All(t => Math.Max(t.Width, t.Height) <= maximumDimension)) return new(data, resource.Graphics.Length, resource.Graphics.Length, changes);
        byte[] system = (byte[])resource.System.Clone(); using var graphics = new MemoryStream();
        foreach (var texture in textures) {
            cancel.ThrowIfCancellationRequested(); int removed = 0, skip = 0, width = texture.Width, height = texture.Height;
            while (Math.Max(width, height) > maximumDimension) {
                skip += MipBytes(texture.Format, width, height); removed++; width = Math.Max(1, width / 2); height = Math.Max(1, height / 2);
            }
            byte[] pixels; int levels; string method;
            if (removed < texture.Levels) {
                pixels = resource.Graphics.AsSpan(texture.GraphicsOffset + skip, texture.DataBytes - skip).ToArray(); levels = texture.Levels - removed;
                method = "Retained existing encoded mip levels";
            } else {
                int available = texture.Levels - 1, mipSkip = 0;
                for (int m = 0; m < available; m++) mipSkip += MipBytes(texture.Format, Math.Max(1, texture.Width >> m), Math.Max(1, texture.Height >> m));
                int sourceWidth = Math.Max(1, texture.Width >> available), sourceHeight = Math.Max(1, texture.Height >> available);
                var mip = resource.Graphics.AsSpan(texture.GraphicsOffset + mipSkip, MipBytes(texture.Format, sourceWidth, sourceHeight)).ToArray();
                var encoded = TextureResampler.Reencode(mip, sourceWidth, sourceHeight, width, height, texture.Format, cancel);
                pixels = encoded.Data; levels = encoded.Levels; method = "Decoded, area-downsampled and re-encoded; original texture format preserved";
            }
            while (graphics.Position % 256 != 0) graphics.WriteByte(0);
            BinaryPrimitives.WriteUInt64LittleEndian(system.AsSpan(texture.Descriptor + 0x70), 0x60000000UL + (ulong)graphics.Position);
            graphics.Write(pixels);
            if (removed > 0) {
                BinaryPrimitives.WriteUInt16LittleEndian(system.AsSpan(texture.Descriptor + 0x50), (ushort)width);
                BinaryPrimitives.WriteUInt16LittleEndian(system.AsSpan(texture.Descriptor + 0x52), (ushort)height);
                BinaryPrimitives.WriteUInt16LittleEndian(system.AsSpan(texture.Descriptor + 0x56), (ushort)Math.Max(1, texture.Stride >> removed));
                system[texture.Descriptor + 0x5d] = (byte)levels;
                changes.Add(new(texture.Name, texture.Width, texture.Height, width, height, removed, method));
            }
        }
        uint flags = ResourceContainer.SinglePageFlags(checked((int)graphics.Length), resource.GraphicsFlags >> 28);
        int allocation = ResourceContainer.PageBytes(flags);
        // Avoid increasing resource allocation when a dictionary's original page packing is tighter.
        if (allocation >= resource.Graphics.Length) throw new InvalidDataException("This dictionary cannot be reduced with the supported page layout; its original file was preserved.");
        graphics.SetLength(allocation);
        int pages = Offset(U64(system, 8), 0x50000000, system.Length, 16);
        Require(system[pages + 9] >= 1, "Missing graphics page metadata.");
        system[pages + 9] = 1;
        var converted = new ResourceContainer(resource.Version, resource.SystemFlags, flags, system, graphics.ToArray());
        byte[] output = converted.Write(); var verified = Inspect(ResourceContainer.Read(output));
        Require(verified.Count == textures.Count && verified.Select(t => t.Name).SequenceEqual(textures.Select(t => t.Name)), "Texture identity changed during conversion.");
        Require(verified.All(t => Math.Max(t.Width, t.Height) <= maximumDimension), "Texture cap verification failed.");
        return new(output, resource.Graphics.Length, allocation, changes);
    }
}
