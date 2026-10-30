using System.Buffers.Binary;
using System.Text;
using System.Xml.Linq;
using System.IO.Compression;
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
        check("DLC audit detects conflicting handling definitions",()=>{var tree=ValidPack(AssetTests.Dictionary());tree.Children.Add(F("first.meta",B("<CHandlingDataMgr><HandlingData><Item><handlingName>SAME</handlingName><mass value=\"100\"/></Item></HandlingData></CHandlingDataMgr>")));tree.Children.Add(F("second.meta",B("<CHandlingDataMgr><HandlingData><Item><handlingName>SAME</handlingName><mass value=\"200\"/></Item></HandlingData></CHandlingDataMgr>")));Must(AddonAudit.Inspect(tree).Issues.Any(i=>i.Code=="conflicting-handling"));});
        check("DLC audit rejects conflicting streamed resources across separate archives",()=>{
            var tree=ValidPack(AssetTests.Dictionary());tree.Children.Add(F("other.rpf",RpfArchive.Write(D("",F("test.ntd",TextureDictionary.Downscale(AssetTests.Dictionary(),256).Data,true)))));
            Edit(tree,"content.xml",x=>x.Replace("</dataFiles>","<Item><filename>dlc_test:/other.rpf</filename><fileType>RPF_FILE</fileType></Item></dataFiles>"));Must(AddonAudit.Inspect(tree).Issues.Any(i=>i.Code=="conflicting-streamed-resource"));
        });
        string input = Path.Combine(scratch, "loose-ped"); Directory.CreateDirectory(input);
        byte[] Empty(uint version) => new ResourceContainer(version, ResourceContainer.SinglePageFlags(512, version >> 4), ResourceContainer.SinglePageFlags(0, version & 15), new byte[512], []).Write();
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.ydd"), Empty(165));
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.yft"), Empty(162));
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.ymt"), "PSINtest"u8.ToArray());
        System.IO.File.WriteAllBytes(Path.Combine(input, "sample.ytd"), AssetTests.Dictionary());
        string template = Path.Combine(scratch, "peds.meta");
        System.IO.File.WriteAllText(template, "<CPedModelInfo__InitDataList><residentTxd>resident</residentTxd><residentAnims/><InitDatas><Item><Name>base_ped</Name><Pedtype>CIVMALE</Pedtype></Item></InitDatas></CPedModelInfo__InitDataList>");
        AddonBuild? built = null;
        check("Four loose assets build a complete human DLC without game metadata or names", () => {
            var automatic = AddonBuilder.Ped(input, Path.Combine(scratch, "automatic-ped"), new(), createZip: false);
            Must(automatic.Model == "sample" && automatic.Id == "nx_sample" && automatic.MetadataSource == "built-in human male" && automatic.Audit.ReadyForTest);
            var pack = RpfArchive.Read(System.IO.File.ReadAllBytes(automatic.DlcArchive));
            var metadata = pack.Children.Single(n => n.Name == "common").Children.Single().Children.Single().Data;
            var doc = XDocument.Parse(Encoding.UTF8.GetString(metadata));
            Must(doc.Descendants("Pedtype").Single().Value == "CIVMALE" && doc.Descendants("MovementClipSet").Single().Value == "move_m@generic");
            var nested = pack.Children.Single(n => n.Name == "switch").Children.Single().Children.Single().Children.Single();
            var resources = RpfArchive.Read(nested.Data);
            Must(resources.Children.Select(n => n.Name).ToHashSet().SetEquals(new[] { "sample.ndd", "sample.nft", "sample.nmt", "sample.ntd" }));
            foreach (string ext in new[] { "ydd", "yft", "ymt" }) Must(resources.Children.Single(n => n.Name == "sample.n" + ext[1..]).Data.SequenceEqual(System.IO.File.ReadAllBytes(Path.Combine(input, "sample." + ext))));
        });
        check("Female human profile uses female movement without an external base definition", () => {
            var female = AddonBuilder.Ped(input, Path.Combine(scratch, "female-ped"), new(Profile: "female"));
            var meta = RpfArchive.Read(System.IO.File.ReadAllBytes(female.DlcArchive)).Children.Single(n => n.Name == "common").Children.Single().Children.Single().Data;
            var doc = XDocument.Parse(Encoding.UTF8.GetString(meta));
            Must(doc.Descendants("Pedtype").Single().Value == "CIVFEMALE" && doc.Descendants("PedCapsuleName").Single().Value == "STANDARD_FEMALE" && doc.Descendants("MovementClipSet").Single().Value == "move_f@generic");
        });
        check("PED ZIP input auto-detects names inside nested folders", () => {
            string zipPath = Path.Combine(scratch, "PED source.zip");
            using (var zip = ZipFile.Open(zipPath, ZipArchiveMode.Create)) foreach (string file in Directory.GetFiles(input)) zip.CreateEntryFromFile(file, "download/variant/" + Path.GetFileName(file));
            var ped = AddonBuilder.Ped(zipPath, Path.Combine(scratch, "zip-input-ped"), new(), createZip: false);
            Must(ped.Model == "sample" && ped.Audit.ReadyForTest);
        });
        check("Blank input and empty or malformed metadata have actionable errors before output exists", () => {
            try { AddonBuilder.Ped("", "", new()); throw new Exception("Expected input error"); } catch (ArgumentException e) { Must(e.Message.Contains("four PED assets")); }
            foreach (string text in new[] { "", "<!-- empty -->", "<broken>", "<unrelated/>" }) {
                string bad = Path.Combine(scratch, "bad-template.meta"), dest = Path.Combine(scratch, "invalid-template-output"); System.IO.File.WriteAllText(bad, text);
                try { AddonBuilder.Ped(input, dest, new(Template: bad)); throw new Exception("Expected metadata rejection"); }
                catch (InvalidDataException e) { Must(e.Message.Contains("bad-template.meta") && e.Message.Contains("built-in human")); }
                Must(!Directory.Exists(dest));
            }
        });
        check("Mod-supplied metadata is reused automatically without a manual base name", () => {
            string supplied = Path.Combine(input, "peds.meta");
            System.IO.File.WriteAllText(supplied, "<CPedModelInfo__InitDataList><InitDatas><Item><Name>sample</Name><Pedtype>CIVFEMALE</Pedtype></Item></InitDatas></CPedModelInfo__InitDataList>");
            try {
                var ped = AddonBuilder.Ped(input, Path.Combine(scratch, "mod-template-ped"), new(), createZip: false);
                Must(ped.MetadataSource == "mod-supplied metadata");
            } finally { System.IO.File.Delete(supplied); }
        });
        check("Single custom definition needs no manual base name", () => {
            var ped = AddonBuilder.Ped(input, Path.Combine(scratch, "automatic-custom-base"), new(Template: template), createZip: false);
            Must(ped.MetadataSource == "custom metadata" && ped.Audit.ReadyForTest);
        });
        check("One-step PED installation preserves existing registrations and original game files", () => {
            string game = Path.Combine(scratch, "ped-current-update.rpf");
            byte[] list = B("<SMandatoryPacksData><Paths><Item>dlcpacks:/existing/</Item></Paths></SMandatoryPacksData>");
            System.IO.File.WriteAllBytes(game, RpfArchive.Write(D("", D("common", D("data", F("dlclist.xml", list), F("keep.meta", B("<keep/>")))))));
            var before = System.IO.File.ReadAllBytes(game);
            foreach (bool zip in new[] { false, true }) {
                var install = AddonBuilder.PedForGame(input, Path.Combine(scratch, "ped-game-" + zip), new(), game, createZip: zip);
                var merged = XDocument.Parse(Encoding.UTF8.GetString(RpfFileEditor.ReadBinary(install.GameFiles.UpdatedArchive, "common/data/dlclist.xml")));
                Must(merged.Descendants("Item").Select(e => e.Value).SequenceEqual(new[] { "dlcpacks:/existing/", "dlcpacks:/nx_sample/" }));
                Must(RpfFileEditor.ReadBinary(install.GameFiles.UpdatedArchive, "common/data/keep.meta").SequenceEqual(B("<keep/>")) && System.IO.File.ReadAllBytes(game).SequenceEqual(before));
                if (zip) { using var archive = ZipFile.OpenRead(install.Archive); Must(archive.Entries.Count(e => e.FullName.EndsWith("/dlc.rpf")) == 1 && archive.Entries.Any(e => e.FullName.EndsWith("/update.rpf")) && archive.Entries.All(e => !e.FullName.EndsWith(".zip"))); }
                else Must(install.Archive == "" && !Directory.EnumerateFiles(install.Ped.Directory, "*.zip", SearchOption.AllDirectories).Any());
            }
            string rejected = Path.Combine(scratch, "ped-no-game-output"); Reject(() => AddonBuilder.PedForGame(input, rejected, new(), "")); Must(!Directory.Exists(rejected));
        });
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
        check("Batch refresh of registered DLCs omits update.rpf while new packs register once",()=>{
            Must(built!=null);string current=Path.Combine(scratch,"batch-current.rpf");
            byte[] list=B("<SMandatoryPacksData><Paths><Item>dlcpacks:/nx_sample/</Item></Paths></SMandatoryPacksData>");
            System.IO.File.WriteAllBytes(current,RpfArchive.Write(D("",D("common",D("data",F("dlclist.xml",list))))));var before=System.IO.File.ReadAllBytes(current);
            var items=new[]{new PreparedAddon("nx_sample",built!.DlcArchive)};var refresh=AddonBatch.Prepare(items,current,Path.Combine(scratch,"batch-refresh"));
            Must(!refresh.UpdateArchiveWritten&&refresh.UpdatedArchive==""&&!Directory.EnumerateFiles(refresh.Directory,"update.rpf",SearchOption.AllDirectories).Any()&&System.IO.File.ReadAllBytes(current).SequenceEqual(before));
            var added=AddonBatch.Prepare(new[]{new PreparedAddon("nx_new",built.DlcArchive)},current,Path.Combine(scratch,"batch-register"));
            Must(added.UpdateArchiveWritten&&DlcPackageService.Contains(RpfFileEditor.ReadBinary(added.UpdatedArchive,"common/data/dlclist.xml"),"nx_new")&&System.IO.File.ReadAllBytes(current).SequenceEqual(before));
            string otherInput=Path.Combine(scratch,"batch-other-input");Directory.CreateDirectory(otherInput);
            foreach(string ext in new[]{".ydd",".yft",".ymt",".ytd"})System.IO.File.Copy(Path.Combine(input,"sample"+ext),Path.Combine(otherInput,"other"+ext));
            var second=AddonBuilder.Ped(otherInput,Path.Combine(scratch,"batch-other-ped"),new("nx_other","other",template,"base_ped",256),createZip:false);
            var mixed=AddonBatch.Prepare(new[]{items[0],new PreparedAddon("nx_other",second.DlcArchive)},current,Path.Combine(scratch,"batch-mixed"));
            byte[] merged=RpfFileEditor.ReadBinary(mixed.UpdatedArchive,"common/data/dlclist.xml");
            Must(mixed.UpdateArchiveWritten&&DlcPackageService.Contains(merged,"nx_sample")&&DlcPackageService.Contains(merged,"nx_other")&&Directory.EnumerateFiles(mixed.Directory,"dlc.rpf",SearchOption.AllDirectories).Count()==2&&System.IO.File.ReadAllBytes(current).SequenceEqual(before));
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
