using BCnEncoder.Decoder;
using BCnEncoder.Encoder;
using BCnEncoder.Shared;
using System.Runtime.InteropServices;

namespace ModPorter.Core.Assets;

public static class TextureResampler {
    static CompressionFormat Compression(uint format) => format switch {
        0x31545844 => CompressionFormat.Bc1WithAlpha,
        0x33545844 => CompressionFormat.Bc2,
        0x35545844 => CompressionFormat.Bc3,
        0x31495441 => CompressionFormat.Bc4,
        0x32495441 => CompressionFormat.Bc5,
        0x20374342 => CompressionFormat.Bc7,
        _ => throw new InvalidDataException("Texture resampling supports BC1/2/3/4/5/7 and 32-bit byte-channel textures; other formats need existing mip levels.")
    };
    public static byte[] ResizeRgba(byte[] input, int width, int height, int outputWidth, int outputHeight, CancellationToken cancel) {
        if (input.Length != checked(width * height * 4) || outputWidth < 1 || outputHeight < 1) throw new InvalidDataException("Invalid RGBA image dimensions.");
        // Area averaging preserves alpha and independent packed normal-map channels.
        var output = new byte[checked(outputWidth * outputHeight * 4)];
        for (int y = 0; y < outputHeight; y++) {
            cancel.ThrowIfCancellationRequested();
            int y0 = y * height / outputHeight, y1 = Math.Max(y0 + 1, (y + 1) * height / outputHeight);
            for (int x = 0; x < outputWidth; x++) {
                int x0 = x * width / outputWidth, x1 = Math.Max(x0 + 1, (x + 1) * width / outputWidth); long count = (long)(x1 - x0) * (y1 - y0);
                for (int c = 0; c < 4; c++) {
                    long sum = 0;
                    for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) sum += input[(yy * width + xx) * 4 + c];
                    output[(y * outputWidth + x) * 4 + c] = (byte)((sum + count / 2) / count);
                }
            }
        }
        return output;
    }
    public static (byte[] Data, int Levels) Reencode(byte[] sourceMip, int width, int height, int outputWidth, int outputHeight, uint format, CancellationToken cancel) {
        if ((long)width * height > 8192L * 8192) throw new InvalidDataException("Textures above 8192×8192 need an existing smaller mip level before decoding.");
        if (format is 21 or 22 or 32) {
            // D3D A8R8G8B8/X8R8G8B8/A8B8G8R8 are byte-channel formats.
            // Average bytes independently, preserving their stored channel order.
            byte[] mip = ResizeRgba(sourceMip, width, height, outputWidth, outputHeight, cancel);
            using var chain = new MemoryStream(); int levels = 0, w = outputWidth, h = outputHeight;
            while (true) {
                cancel.ThrowIfCancellationRequested(); chain.Write(mip); levels++;
                if (w == 1 && h == 1) break;
                int nextW = Math.Max(1, w / 2), nextH = Math.Max(1, h / 2);
                mip = ResizeRgba(mip, w, h, nextW, nextH, cancel); w = nextW; h = nextH;
            }
            return (chain.ToArray(), levels);
        }
        var compression = Compression(format); var decoder = new BcDecoder();
        var decoded = decoder.DecodeRawAsync(sourceMip, compression, width, height, cancel).GetAwaiter().GetResult();
        byte[] pixels = MemoryMarshal.AsBytes(decoded.AsSpan()).ToArray();
        byte[] resized = ResizeRgba(pixels, width, height, outputWidth, outputHeight, cancel);
        var encoder = new BcEncoder(compression); encoder.OutputOptions.GenerateMipMaps = true; encoder.OutputOptions.Quality = CompressionQuality.Balanced;
        var mips = encoder.EncodeToRawBytesAsync(resized, outputWidth, outputHeight, PixelFormat.Rgba32, cancel).GetAwaiter().GetResult();
        if (mips.Length is < 1 or > 15) throw new InvalidDataException("Invalid generated mip chain.");
        using var result = new MemoryStream();
        for (int i = 0; i < mips.Length; i++) {
            if (mips[i].Length != TextureDictionary.MipBytes(format, Math.Max(1, outputWidth >> i), Math.Max(1, outputHeight >> i))) throw new InvalidDataException("Encoded mip size does not match the preserved texture format.");
            result.Write(mips[i]);
        }
        return (result.ToArray(), mips.Length);
    }
}
