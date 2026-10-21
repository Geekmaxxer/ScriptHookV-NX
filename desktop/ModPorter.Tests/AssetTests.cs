using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;
using ModPorter.Core;
using ModPorter.Core.Assets;

static class AssetTests {
    // Generated format fixture; no Rockstar texture, model or other game content.
    public static byte[] Dictionary(int levels = 10, uint version = 13) {
        byte[] sys = new byte[4096];
        void U16(int at, ushort v) => BinaryPrimitives.WriteUInt16LittleEndian(sys.AsSpan(at), v);
        void U32(int at, uint v) => BinaryPrimitives.WriteUInt32LittleEndian(sys.AsSpan(at), v);
        void Ptr(int at, uint v) => BinaryPrimitives.WriteUInt64LittleEndian(sys.AsSpan(at), 0x50000000UL + v);
        Ptr(8, 0x100); sys[0x108] = 1; sys[0x109] = 1;
        Ptr(0x20, 0x140); U16(0x28, 1); U16(0x2a, 1); U32(0x140, 0x12345678);
        Ptr(0x30, 0x150); U16(0x38, 1); U16(0x3a, 1); Ptr(0x150, 0x200);
        Ptr(0x228, 0x300); Encoding.ASCII.GetBytes("test_texture\0").CopyTo(sys, 0x300);
        U16(0x250, 512); U16(0x252, 512); U16(0x254, 1); U16(0x256, 512); U32(0x258, 0x35545844); sys[0x25d] = (byte)levels;
        BinaryPrimitives.WriteUInt64LittleEndian(sys.AsSpan(0x270), 0x60000000);
        int bytes = Enumerable.Range(0, levels).Sum(i => TextureDictionary.MipBytes(0x35545844, Math.Max(1, 512 >> i), Math.Max(1, 512 >> i)));
        uint gfxFlags = ResourceContainer.SinglePageFlags(bytes, 13);
        byte[] gfx = new byte[ResourceContainer.PageBytes(gfxFlags)];
        for (int i = 0; i < gfx.Length; i += 16) { gfx[i] = 255; gfx[i + 1] = 255; gfx[i + 8] = 0x00; gfx[i + 9] = 0xf8; }
        return new ResourceContainer(version, ResourceContainer.SinglePageFlags(sys.Length, 0), gfxFlags, sys, gfx).Write();
    }
    public static void Run(string scratch, Action<string, Action> check) {
        void Must(bool v) { if (!v) throw new Exception("Asset assertion failed"); }
        void Reject(Action action) { try { action(); } catch (Exception e) when (e is InvalidDataException or IOException or ArgumentException) { return; } throw new Exception("Expected asset rejection"); }
        AssetNode Root(params AssetNode[] nodes) { var root = new AssetNode { Directory = true }; root.Children.AddRange(nodes); return root; }
        var dictionary = Dictionary();
        check("RSC7 page flags and payload roundtrip", () => { var r = ResourceContainer.Read(dictionary); Must(r.Version == 13 && r.System.Length == 4096 && ResourceContainer.Read(r.Write()).Graphics.SequenceEqual(r.Graphics)); });
        check("Texture dictionary identity and dimensions", () => { var t = TextureDictionary.Inspect(ResourceContainer.Read(dictionary)).Single(); Must(t.Name == "test_texture" && t.Width == 512 && t.Levels == 10); });
        check("Texture Off preserves exact input bytes", () => Must(TextureDictionary.Downscale(dictionary, 0).Data.SequenceEqual(dictionary)));
        check("Within-cap dictionaries are byte identical", () => Must(TextureDictionary.Downscale(dictionary, 1024).Data.SequenceEqual(dictionary)));
        check("Existing BC mip tail retained exactly", () => {
            var input = ResourceContainer.Read(dictionary); var converted = TextureDictionary.Downscale(dictionary, 256); var output = ResourceContainer.Read(converted.Data);
            var texture = TextureDictionary.Inspect(output).Single();
            Must(texture.Width == 256 && texture.Levels == 9 && converted.GraphicsBytesAfter < converted.GraphicsBytesBefore);
            Must(input.Graphics.AsSpan(512 * 512, texture.DataBytes).SequenceEqual(output.Graphics.AsSpan(texture.GraphicsOffset, texture.DataBytes)));
            Must(output.System[0x109] == 1 && converted.Changes.Single().Method.Contains("Retained"));
        });
        check("No-mip BC texture is decoded and re-encoded", () => {
            var result = TextureDictionary.Downscale(Dictionary(1), 256); var resource = ResourceContainer.Read(result.Data); var t = TextureDictionary.Inspect(resource).Single();
            Must(t.Width == 256 && t.Height == 256 && t.Levels > 1 && t.Format == 0x35545844 && result.Changes.Single().Method.Contains("re-encoded"));
            Must(result.GraphicsBytesAfter < result.GraphicsBytesBefore);
            var decoder = new BCnEncoder.Decoder.BcDecoder(); var decoded = decoder.DecodeRawAsync(resource.Graphics.AsSpan(t.GraphicsOffset, 256 * 256).ToArray(), BCnEncoder.Shared.CompressionFormat.Bc3, 256, 256).GetAwaiter().GetResult();
            Must(decoded.Length == 256 * 256 && decoded.All(p => p.a > 240 && p.r > 240));
        });
        check("RGBA downsample keeps independent channels", () => { byte[] output = TextureResampler.ResizeRgba([0, 0, 0, 0, 100, 50, 200, 128, 200, 150, 0, 255, 100, 200, 200, 129], 2, 2, 1, 1, default); Must(output.SequenceEqual(new byte[] { 100, 100, 100, 128 })); });
        check("Single-mip 32-bit textures gain a full chain without swapping channels", () => {
            byte[] pixels = Enumerable.Range(0, 16).SelectMany(_ => new byte[] { 3, 47, 201, 128 }).ToArray();
            foreach (uint format in new uint[] { 21, 22, 32 }) {
                var result = TextureResampler.Reencode(pixels, 4, 4, 2, 2, format, default);
                Must(result.Levels == 2 && result.Data.Length == 20);
                for (int i = 0; i < result.Data.Length; i += 4) Must(result.Data.AsSpan(i, 4).SequenceEqual(new byte[] { 3, 47, 201, 128 }));
            }
        });
        check("Enhanced dictionaries rejected", () => Reject(() => TextureDictionary.Inspect(ResourceContainer.Read(Dictionary(version: 5)))));
        check("Invalid texture pointers rejected", () => { var r = ResourceContainer.Read(dictionary); BinaryPrimitives.WriteUInt64LittleEndian(r.System.AsSpan(0x270), 0x70000000); Reject(() => TextureDictionary.Inspect(r)); });
        check("Invalid resource allocations rejected", () => { var data = (byte[])dictionary.Clone(); BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(12), 0xffffffff); Reject(() => ResourceContainer.Read(data)); });
        check("RPF empty binary and nested entries roundtrip", () => {
            var dir = new AssetNode { Name = "x64", Directory = true }; dir.Children.Add(new AssetNode { Name = "test.ytd", Resource = true, Data = dictionary });
            var bytes = RpfArchive.Write(Root(dir, new AssetNode { Name = "empty.meta", Data = [] })); var r = RpfArchive.Read(bytes);
            Must(r.Children.Count == 2 && r.Children.Single(n => n.Name == "x64").Children.Single().Data.SequenceEqual(dictionary) && r.Children.Single(n => n.Name == "empty.meta").Data.Length == 0);
        });
        check("Engine binary lookup reaches every normalized RPF path", () => {
            var common = new AssetNode { Name = "Common", Directory = true };
            common.Children.AddRange([new() { Name = "z.meta", Data = [3] }, new() { Name = "A.meta", Data = [4] }]);
            var root = Root(new() { Name = "content.xml", Data = [1] }, new() { Name = "setup2.xml", Data = [2] }, common, new() { Name = "Switch", Directory = true });
            byte[] bytes = RpfArchive.Write(root);
            uint Word(int at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(at));
            int namesAt = 16 + (int)Word(4) * 16;
            string NameAt(int index) { int at = 16 + index * 16; int offset = (int)(Word(at + 4) == 0x7fffff00 ? Word(at) : Word(at) & 65535); int start = namesAt + offset; return Encoding.UTF8.GetString(bytes, start, Array.IndexOf(bytes, (byte)0, start) - start); }
            foreach (string path in new[] { "CONTENT.XML", "setup2.xml", "Common/A.meta", "common/z.meta", "Switch" }) {
                int current = 0;
                foreach (string part in path.ToLowerInvariant().Split('/')) {
                    int at = 16 + current * 16, low = (int)Word(at + 8), high = low + (int)Word(at + 12) - 1, found = -1;
                    while (low <= high) { int mid = (low + high) / 2; int cmp = string.CompareOrdinal(part, NameAt(mid)); if (cmp < 0) high = mid - 1; else if (cmp > 0) low = mid + 1; else { found = mid; break; } }
                    Must(found >= 0); current = found;
                }
            }
            Must(common.Name == "Common" && common.Children[0].Name == "z.meta"); // writing must not mutate input
        });
        check("Cyclic output RPF rejected", () => { var root = Root(); root.Children.Add(root); Reject(() => RpfArchive.Write(root)); });
        check("Unsafe output archive names rejected", () => Reject(() => RpfArchive.Write(Root(new AssetNode { Name = "../evil", Data = [1] }))));
        check("RPF case collisions rejected", () => Reject(() => RpfArchive.Write(Root(new AssetNode { Name = "a.meta", Data = [1] }, new AssetNode { Name = "A.meta", Data = [2] }))));
        check("Encrypted RPF rejected", () => { var data = RpfArchive.Write(Root(new AssetNode { Name = "a.meta", Data = [1] })); BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(12), 0x12345678); Reject(() => RpfArchive.Read(data)); });
        check("Cyclic input RPF rejected", () => { var data = RpfArchive.Write(Root(new AssetNode { Name = "a.meta", Data = [1] })); BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(24), 0); Reject(() => RpfArchive.Read(data)); });
        check("Platform names map without changing model identity", () => Must(AssetService.SwitchName("test.yft") == "test.nft" && AssetService.SwitchName("x64vehicles.rpf") == "switchvehicles.rpf" && AssetService.SwitchName("mycar_hi.yft") == "mycar_hi.nft"));
        string input = Path.Combine(scratch, "Asset input"), output = Path.Combine(scratch, "Asset output"); Directory.CreateDirectory(input);
        var nested = RpfArchive.Write(Root(new AssetNode { Name = "test.ytd", Resource = true, Data = dictionary }, new AssetNode { Name = "content.xml", Data = Encoding.UTF8.GetBytes("<root>x64:/models/test.ytd</root>") }));
        File.WriteAllBytes(Path.Combine(input, "dlc.rpf"), RpfArchive.Write(Root(new AssetNode { Name = "x64textures.rpf", Data = nested })));
        check("Inspect leaves inputs unchanged and does not claim output", () => { var r = AssetService.Analyze(input, new(256)); Must(r.Files.Count == 4 && r.Files.Where(f => f.Output.EndsWith(".rpf")).All(f => f.Operation.Contains("no output"))); });
        check("Nested DLC naming, metadata, textures and ZIP", () => {
            byte[] original = File.ReadAllBytes(Path.Combine(input, "dlc.rpf")); var conversion = AssetService.Convert(input, output, new(256));
            var dlc = RpfArchive.Read(File.ReadAllBytes(Path.Combine(output, "converted/dlc.rpf"))); var inner = RpfArchive.Read(dlc.Children.Single().Data);
            Must(dlc.Children.Single().Name == "switchtextures.rpf" && inner.Children.Any(n => n.Name == "test.ntd"));
            Must(Encoding.UTF8.GetString(inner.Children.Single(n => n.Name == "content.xml").Data) == "<root>switch:/models/test.ntd</root>");
            Must(TextureDictionary.Inspect(ResourceContainer.Read(inner.Children.Single(n => n.Name == "test.ntd").Data)).Single().Width == 256);
            Must(File.ReadAllBytes(Path.Combine(input, "dlc.rpf")).SequenceEqual(original) && conversion.Report.GraphicsBytesSaved > 0 && !conversion.Report.SwitchRuntimeVerified);
            using var zip = ZipFile.OpenRead(conversion.Archive); Must(zip.Entries.Count == 3 && zip.GetEntry("converted/dlc.rpf") != null);
        });
        check("Assets never overwrite existing output", () => Reject(() => AssetService.Convert(input, output, new())));
        check("Assets cannot write inside input", () => Reject(() => AssetService.Convert(input, Path.Combine(input, "out"), new())));
        check("Scripts cannot become assets by renaming", () => { string p = Path.Combine(scratch, "scriptpack"); Directory.CreateDirectory(p); File.WriteAllBytes(Path.Combine(p, "test.ytd"), dictionary); File.WriteAllBytes(Path.Combine(p, "code.ysc"), [1]); Reject(() => AssetService.Analyze(p, new())); });
        check("Asset ZIP traversal rejected", () => { string p = Path.Combine(scratch, "asset-traversal.zip"); using (var zip = ZipFile.Open(p, ZipArchiveMode.Create)) { using var writer = new StreamWriter(zip.CreateEntry("../evil.ytd").Open()); writer.Write("x"); } Reject(() => AssetService.Analyze(p, new())); });
        check("Asset conversion cancellation", () => { using var c = new CancellationTokenSource(); c.Cancel(); try { AssetService.Analyze(input, new(), c.Token); } catch (OperationCanceledException) { return; } throw new Exception("Cancellation ignored"); });
        check("UTF-16 metadata encoding and text retained", () => {
            string source = Path.Combine(scratch, "utf16-assets"), dest = Path.Combine(scratch, "utf16-converted"); Directory.CreateDirectory(source);
            File.WriteAllBytes(Path.Combine(source, "test.ytd"), dictionary); File.WriteAllText(Path.Combine(source, "content.xml"), "<a>x64:/test.ytd 中文</a>", Encoding.Unicode);
            AssetService.Convert(source, dest, new(0)); byte[] bytes = File.ReadAllBytes(Path.Combine(dest, "converted/content.xml"));
            Must(bytes[0] == 0xff && bytes[1] == 0xfe && File.ReadAllText(Path.Combine(dest, "converted/content.xml")) == "<a>switch:/test.ntd 中文</a>");
        });
        check("Undecodable metadata is rejected without replacement characters", () => {
            string source = Path.Combine(scratch, "bad-encoding-assets"); Directory.CreateDirectory(source);
            File.WriteAllBytes(Path.Combine(source, "test.ytd"), dictionary); File.WriteAllBytes(Path.Combine(source, "content.xml"), [0xff, 0x80]);
            Reject(() => AssetService.Analyze(source, new()));
        });
    }
}
