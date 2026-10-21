using System.Buffers.Binary;

namespace ModPorter.Core.Assets;

public static class EmbeddedTextures {
    // Follow typed Legacy roots only. No signature-scanning of arbitrary bytes.
    // Preserve the whole graphics allocation: models share it with geometry.
    public static TextureResult Downscale(byte[] data, string extension, int cap, CancellationToken cancel = default) {
        if (cap is not (0 or 256 or 512 or 1024 or 2048 or 4096)) throw new ArgumentException("Unsupported texture cap.");
        var r = ResourceContainer.Read(data); byte[] system = (byte[])r.System.Clone(), graphics = (byte[])r.Graphics.Clone();
        ulong Read64(int p) { if (p < 0 || p > system.Length - 8) throw new InvalidDataException("Embedded pointer field out of range."); return BinaryPrimitives.ReadUInt64LittleEndian(system.AsSpan(p)); }
        int Ptr(ulong p, int size) { if ((p & 0xfffffffff0000000UL) != 0x50000000UL || (p & 0xfffffff) + (ulong)size > (ulong)system.Length) throw new InvalidDataException("Embedded resource pointer out of range."); return (int)(p & 0xfffffff); }
        var dictionaries = new HashSet<int>();
        void Drawable(ulong pointer) {
            if (pointer == 0) return; int at = Ptr(pointer, 32); ulong shader = Read64(at + 0x10); if (shader == 0) return;
            int group = Ptr(shader, 64); ulong dictionary = Read64(group + 8); if (dictionary != 0) dictionaries.Add(Ptr(dictionary, 64));
        }
        switch (extension.ToLowerInvariant()) {
            case ".ydr": case ".ndr": if (r.Version != 165) throw new InvalidDataException("Unsupported drawable profile."); Drawable(0x50000000); break;
            case ".ydd": case ".ndd": {
                if (r.Version != 165 || system.Length < 64) throw new InvalidDataException("Unsupported drawable dictionary profile.");
                int count = BinaryPrimitives.ReadUInt16LittleEndian(system.AsSpan(0x38)); if (count > 4096) throw new InvalidDataException("Too many embedded drawables.");
                if (count == 0) break; int list = Ptr(Read64(0x30), checked(count * 8)); for (int i = 0; i < count; i++) Drawable(Read64(list + i * 8)); break;
            }
            case ".yft": case ".nft": {
                if (r.Version != 162 || system.Length < 256) throw new InvalidDataException("Unsupported fragment profile.");
                Drawable(Read64(0x30)); Drawable(Read64(0xf8));
                uint count = BinaryPrimitives.ReadUInt32LittleEndian(system.AsSpan(0x48)); if (count > 4096) throw new InvalidDataException("Too many fragment drawables.");
                if (count > 0) { int list = Ptr(Read64(0x38), checked((int)count * 8)); for (int i = 0; i < count; i++) Drawable(Read64(list + i * 8)); } break;
            }
            default: return new(data, r.Graphics.Length, r.Graphics.Length, []);
        }
        var textures = dictionaries.SelectMany(d => TextureDictionary.InspectAt(r, d)).DistinctBy(t => t.Descriptor).ToList();
        var ranges = textures.OrderBy(t => t.GraphicsOffset).ToList();
        for (int i = 1; i < ranges.Count; i++) if (ranges[i - 1].GraphicsOffset + ranges[i - 1].DataBytes > ranges[i].GraphicsOffset) throw new InvalidDataException("Shared/overlapping embedded texture spans need a dedicated layout conversion.");
        var changes = new List<TextureChange>();
        foreach (var t in textures) {
            cancel.ThrowIfCancellationRequested(); if (cap == 0 || Math.Max(t.Width, t.Height) <= cap) continue;
            int removed = 0, skip = 0, width = t.Width, height = t.Height;
            while (Math.Max(width, height) > cap) { skip += TextureDictionary.MipBytes(t.Format, width, height); width = Math.Max(1, width / 2); height = Math.Max(1, height / 2); removed++; }
            int levels = t.Levels - removed; string method;
            if (levels > 0) {
                // Existing mip tail can be selected without moving any graphics bytes.
                BinaryPrimitives.WriteUInt64LittleEndian(system.AsSpan(t.Descriptor + 0x70), 0x60000000UL + (uint)(t.GraphicsOffset + skip));
                method = "Embedded encoded mip tail selected; shared geometry and graphics allocation preserved";
            } else {
                int mipSkip = 0; for (int i = 0; i < t.Levels - 1; i++) mipSkip += TextureDictionary.MipBytes(t.Format, Math.Max(1, t.Width >> i), Math.Max(1, t.Height >> i));
                int sw = Math.Max(1, t.Width >> (t.Levels - 1)), sh = Math.Max(1, t.Height >> (t.Levels - 1));
                var encoded = TextureResampler.Reencode(graphics.AsSpan(t.GraphicsOffset + mipSkip, TextureDictionary.MipBytes(t.Format, sw, sh)).ToArray(), sw, sh, width, height, t.Format, cancel);
                if (encoded.Data.Length > t.DataBytes) throw new InvalidDataException("Embedded result exceeds its original span.");
                encoded.Data.CopyTo(graphics, t.GraphicsOffset); levels = encoded.Levels;
                method = "Embedded BC texture resized in its existing span; graphics allocation preserved";
            }
            BinaryPrimitives.WriteUInt16LittleEndian(system.AsSpan(t.Descriptor + 0x50), (ushort)width); BinaryPrimitives.WriteUInt16LittleEndian(system.AsSpan(t.Descriptor + 0x52), (ushort)height);
            BinaryPrimitives.WriteUInt16LittleEndian(system.AsSpan(t.Descriptor + 0x56), (ushort)Math.Max(1, t.Stride >> removed)); system[t.Descriptor + 0x5d] = (byte)levels;
            changes.Add(new(t.Name, t.Width, t.Height, width, height, removed, method));
        }
        if (changes.Count == 0) return new(data, r.Graphics.Length, r.Graphics.Length, []);
        var changed = new ResourceContainer(r.Version, r.SystemFlags, r.GraphicsFlags, system, graphics);
        foreach (int dictionary in dictionaries) _ = TextureDictionary.InspectAt(changed, dictionary);
        return new(changed.Write(), r.Graphics.Length, r.Graphics.Length, changes);
    }
}
