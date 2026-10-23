using System.Buffers.Binary;
using System.Text;
using System.Xml.Linq;
using ModPorter.Core.Assets;

static class AddonTests {
    static AssetNode D(string name, params AssetNode[] children) { var n = new AssetNode { Name = name, Directory = true }; n.Children.AddRange(children); return n; }
    static AssetNode F(string name, byte[] data, bool resource = false) => new() { Name = name, Data = data, Resource = resource };
    static byte[] B(string xml) => Encoding.UTF8.GetBytes(xml);
    internal static AssetNode ValidPack(byte[] texture) => D("",
        F("setup2.xml", B("<SSetupData><deviceName>dlc_test</deviceName><datFile>content.xml</datFile><contentChangeSetGroups><Item><NameHash>GROUP_STARTUP</NameHash><ContentChangeSets><Item>test_AUTOGEN</Item></ContentChangeSets></Item></contentChangeSetGroups></SSetupData>")),
        F("content.xml", B("<CDataFileMgr__ContentsOfDataFileXml><dataFiles><Item><filename>dlc_test:/textures.rpf</filename><fileType>RPF_FILE</fileType><disabled value=\"true\" /></Item></dataFiles><contentChangeSets><Item><changeSetName>test_AUTOGEN</changeSetName><filesToEnable><Item>dlc_test:/textures.rpf</Item></filesToEnable></Item></contentChangeSets></CDataFileMgr__ContentsOfDataFileXml>")),
        F("textures.rpf", RpfArchive.Write(D("", F("test.ntd", texture, true)))));
    public static void Run(string scratch, Action<string, Action> check) {
        void Must(bool ok) { if (!ok) throw new Exception("Add-on assertion failed"); }
        void Reject(Action action) { try { action(); } catch (Exception e) when (e is IOException or InvalidDataException or ArgumentException or InvalidOperationException) { return; } throw new Exception("Expected rejection"); }
        void Edit(AssetNode tree, string name, Func<string, string> edit) { var node = tree.Children.Single(n => n.Name == name); node.Data = B(edit(Encoding.UTF8.GetString(node.Data))); }
        check("DLC audit follows mounted archives and startup activation", () => { var report = AddonAudit.Inspect(ValidPack(AssetTests.Dictionary())); Must(report.ReadyForTest && report.Archives == 1 && report.Resources == 1); });
        check("DLC audit diagnoses missing startup change set", () => { var tree = ValidPack(AssetTests.Dictionary()); Edit(tree, "setup2.xml", x => x.Replace("test_AUTOGEN", "absent")); Must(AddonAudit.Inspect(tree).Issues.Any(i => i.Code == "missing-change-set")); });
        check("DLC audit diagnoses missing registered files", () => { var tree = ValidPack(AssetTests.Dictionary()); tree.Children.RemoveAll(n => n.Name == "textures.rpf"); Must(!AddonAudit.Inspect(tree).ReadyForTest); });
        check("DLC audit diagnoses inactive data", () => { var tree = ValidPack(AssetTests.Dictionary()); Edit(tree, "content.xml", x => x.Replace("<Item>dlc_test:/textures.rpf</Item>", "")); Must(AddonAudit.Inspect(tree).Issues.Any(i => i.Code == "inactive-file")); });
        check("DLC audit rejects device mismatch and traversal", () => { foreach (string path in new[] { "other:/textures.rpf", "dlc_test:/../textures.rpf" }) { var tree = ValidPack(AssetTests.Dictionary()); Edit(tree, "content.xml", x => x.Replace("dlc_test:/textures.rpf", path)); Must(!AddonAudit.Inspect(tree).ReadyForTest); } });
        check("DLC audit reports malformed vector numbers", () => { var tree = ValidPack(AssetTests.Dictionary()); tree.Children.Add(F("vehicles.meta", B("<root><offset x=\"0.-050000\" /></root>"))); Must(AddonAudit.Inspect(tree).Issues.Any(i => i.Code == "invalid-number")); });
        check("DLC audit rejects DTD entities", () => { var tree = ValidPack(AssetTests.Dictionary()); Edit(tree, "content.xml", x => "<!DOCTYPE x [<!ENTITY x SYSTEM 'file:///secret'>]>" + x); Must(!AddonAudit.Inspect(tree).ReadyForTest); });
        string input = Path.Combine(scratch, "loose-ped"); Directory.CreateDirectory(input);
        byte[] Empty(uint version) => new ResourceContainer(version, ResourceContainer.SinglePageFlags(512, version >> 4), ResourceContainer.SinglePageFlags(0, version & 15), new byte[512], []).Write();
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.ydd"), Empty(165));
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.yft"), Empty(162));
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.ymt"), "PSINtest"u8.ToArray());
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.ytd"), AssetTests.Dictionary());
        string template = Path.Combine(scratch, "peds.meta");
        System.IO.File.WriteAllText(template, "<CPedModelInfo__InitDataList><residentTxd>resident</residentTxd><residentAnims/><InitDatas><Item><Name>base_ped</Name><Pedtype>CIVMALE</Pedtype></Item></InitDatas></CPedModelInfo__InitDataList>");
        AddonBuild? built = null;
        check("Loose PED builder preserves target template and registers all four resources", () => {
            string before = System.IO.File.ReadAllText(template);
            built = AddonBuilder.Ped(input, Path.Combine(scratch, "built-ped"), new("nx_sample", "sample", template, "base_ped", 256));
            Must(built.Audit.ReadyForTest && built.Audit.Models.SequenceEqual(new[] { "sample" }) && System.IO.File.Exists(built.Archive) && System.IO.File.ReadAllText(template) == before);
            var tree = RpfArchive.Read(System.IO.File.ReadAllBytes(built.DlcArchive));
            var meta = tree.Children.Single(n => n.Name == "common").Children.Single().Children.Single().Data;
            var doc = XDocument.Parse(Encoding.UTF8.GetString(meta)); Must(doc.Descendants("Name").Single().Value == "sample" && doc.Descendants("Pedtype").Single().Value == "CIVMALE");
        });
        check("Folder-only PED add-on matches the audited ZIP build without nested ZIPs", () => {
            Must(built != null);
            var folder = AddonBuilder.Ped(input, Path.Combine(scratch, "PED folder only"), new("nx_sample", "sample", template, "base_ped", 256), createZip: false);
            Must(folder.Archive == "" && folder.Audit.ReadyForTest && !Directory.EnumerateFiles(folder.Directory, "*.zip", SearchOption.AllDirectories).Any());
            Must(System.IO.File.ReadAllBytes(folder.DlcArchive).SequenceEqual(System.IO.File.ReadAllBytes(built!.DlcArchive)));
            foreach (string name in new[] { "READ-ME-FIRST.md", "addon-manifest.json" }) Must(System.IO.File.ReadAllBytes(Path.Combine(folder.Directory, name)).SequenceEqual(System.IO.File.ReadAllBytes(Path.Combine(built.Directory, name))));
            Must(Directory.EnumerateFiles(built.Directory, "*.zip", SearchOption.AllDirectories).SequenceEqual(new[] { built.Archive }));
        });
        check("Loose PED builder rejects missing base and duplicate variants", () => {
            Reject(() => AddonBuilder.Ped(input, Path.Combine(scratch, "bad-base"), new("nx_bad", "sample", template, "unknown")));
            string extra = Path.Combine(input, "other.yft"); System.IO.File.WriteAllBytes(extra, Empty(162));
            try { Reject(() => AddonBuilder.Ped(input, Path.Combine(scratch, "bad-extra"), new("nx_bad", "sample", template, "base_ped"))); } finally { System.IO.File.Delete(extra); }
        });
        check("Loose PED builder refuses existing output", () => { Must(built != null); Reject(() => AddonBuilder.Ped(input, built!.Directory, new("nx_sample", "sample", template, "base_ped"))); });
        check("Batch rejects duplicate models across differently named packs", () => {
            Must(built != null); string output=Path.Combine(scratch,"colliding-models");
            Reject(()=>AddonBatch.Prepare(new[]{new PreparedAddon("first",built!.DlcArchive),new PreparedAddon("second",built.DlcArchive)},"missing-game.rpf",output));
            Must(!Directory.Exists(output));
        });
        check("DLC audit detects duplicate mod-kit IDs", () => {
            var tree=ValidPack(AssetTests.Dictionary());tree.Children.Add(F("carcols.meta",B("<CVehicleModelInfoVarGlobal><Kits><Item><kitName>first</kitName><id value=\"1000\"/></Item><Item><kitName>second</kitName><id value=\"1000\"/></Item></Kits></CVehicleModelInfoVarGlobal>")));
            Must(AddonAudit.Inspect(tree).Issues.Any(i=>i.Code=="duplicate-modkit-id"));
        });
        check("Loose PED builder rejects incomplete input", () => { System.IO.File.Delete(Path.Combine(input, "sample.yft")); Reject(() => AddonBuilder.Ped(input, Path.Combine(scratch, "missing-fragment"), new("nx_sample", "sample", template, "base_ped"))); });
        check("Physics-child texture traversal preserves physics and graphics bytes", () => {
            var original = ResourceContainer.Read(AssetTests.Dictionary()); byte[] s = (byte[])original.System.Clone();
            original.System.AsSpan(0, 64).CopyTo(s.AsSpan(0x400)); s.AsSpan(0, 0x130).Clear();
            void P(int at, uint value) => BinaryPrimitives.WriteUInt64LittleEndian(s.AsSpan(at), 0x50000000UL + value);
            P(0xf0, 0x500); P(0x510, 0x600); P(0x6d0, 0x800); s[0x71d] = 1;
            P(0x800, 0x900); P(0x9a0, 0xb00); P(0xb10, 0xc00); P(0xc08, 0x400);
            var data = new ResourceContainer(162, original.SystemFlags, original.GraphicsFlags, s, original.Graphics).Write();
            var result = EmbeddedTextures.Downscale(data, ".nft", 256); var after = ResourceContainer.Read(result.Data);
            Must(result.Changes.Count == 1 && TextureDictionary.InspectAt(after, 0x400).Single().Width == 256);
            Must(after.System.AsSpan(0x500, 0x600).SequenceEqual(s.AsSpan(0x500, 0x600)) && after.Graphics.SequenceEqual(original.Graphics));
            P(0xc10,0xd00);BinaryPrimitives.WriteUInt16LittleEndian(s.AsSpan(0xc18),1);BinaryPrimitives.WriteUInt16LittleEndian(s.AsSpan(0xc1a),1);
            P(0xd00,0xd20);P(0xd20,0xe00);s[0xd30]=1;P(0xe08,0xe40);P(0xe68,0xf00);Encoding.ASCII.GetBytes("external_accessory\0").CopyTo(s,0xf00);
            var references=new HashSet<string>(StringComparer.OrdinalIgnoreCase);var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            _=EmbeddedTextures.Downscale(new ResourceContainer(162,original.SystemFlags,original.GraphicsFlags,s,original.Graphics).Write(),".nft",0,references:references,embeddedNames:names);
            Must(references.Contains("external_accessory")&&names.Contains("test_texture"));
            P(0x800, 0xfffffff); Reject(() => EmbeddedTextures.Downscale(new ResourceContainer(162, original.SystemFlags, original.GraphicsFlags, s, original.Graphics).Write(), ".nft", 256));
        });
    }
}
