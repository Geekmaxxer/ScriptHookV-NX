using System.Buffers.Binary;
using System.Text;
using System.Xml.Linq;
using ModPorter.Core;
using ModPorter.Core.Assets;

static class RefinementTests {
    static void Must(bool value) { if (!value) throw new Exception("Refinement assertion failed"); }
    static void Reject(Action action) { try { action(); } catch (Exception e) when (e is IOException or InvalidDataException or ArgumentException or InvalidOperationException or System.Xml.XmlException) { return; } throw new Exception("Expected rejection"); }
    public static void Assets(string scratch, Action<string,Action> check) {
        byte[] Model(string ext, int levels = 10) {
            var r = ResourceContainer.Read(AssetTests.Dictionary(levels));
            r.System.AsSpan(0,64).CopyTo(r.System.AsSpan(0x400)); r.System.AsSpan(0x10,48).Clear();
            void Ptr(int at, uint offset) => BinaryPrimitives.WriteUInt64LittleEndian(r.System.AsSpan(at), 0x50000000UL + offset);
            int drawable = ext == ".ydr" ? 0 : 0x600; Ptr(drawable + 0x10, 0x500); Ptr(0x508, 0x400);
            if (ext == ".ydd") { Ptr(0x30,0x700); BinaryPrimitives.WriteUInt16LittleEndian(r.System.AsSpan(0x38),1); Ptr(0x700,0x600); }
            if (ext == ".yft") Ptr(0x30,0x600); r.Graphics[^1] = 0x7b;
            return new ResourceContainer(ext == ".yft" ? 162u : 165u, r.SystemFlags, r.GraphicsFlags,r.System,r.Graphics).Write();
        }
        foreach (string ext in new[]{".ydr",".ydd",".yft"}) check("Embedded texture typed graph " + ext, () => {
            var original = ResourceContainer.Read(Model(ext)); var result = EmbeddedTextures.Downscale(original.Write(),ext,256);
            var r = ResourceContainer.Read(result.Data); var t = TextureDictionary.InspectAt(r,0x400).Single();
            Must(t.Width == 256 && t.Levels == 9 && result.Changes.Count == 1 && original.Graphics.SequenceEqual(r.Graphics));
            Must(r.GraphicsFlags == original.GraphicsFlags && r.SystemFlags == original.SystemFlags && r.System.AsSpan(0x600,0x100).SequenceEqual(original.System.AsSpan(0x600,0x100)));
        });
        check("Embedded no-mip resize preserves geometry", () => { var original = ResourceContainer.Read(Model(".ydr",1)); var r = ResourceContainer.Read(EmbeddedTextures.Downscale(original.Write(),".ydr",256).Data); Must(TextureDictionary.InspectAt(r,0x400).Single().Width == 256 && r.Graphics[^1] == 0x7b && r.Graphics.Length == original.Graphics.Length); });
        check("Embedded invalid typed pointer rejected", () => { var r = ResourceContainer.Read(Model(".ydr")); BinaryPrimitives.WriteUInt64LittleEndian(r.System.AsSpan(0x10),0x70000000); Reject(()=>EmbeddedTextures.Downscale(r.Write(),".ydr",256)); });
        check("Embedded Off is byte identical", () => { byte[] data = Model(".yft"); Must(data.SequenceEqual(EmbeddedTextures.Downscale(data,".yft",0).Data)); });
        string dlc = Path.Combine(scratch,"registration-dlc.rpf"), list = Path.Combine(scratch,"current-dlclist.xml");
        var root = new AssetNode { Directory = true }; foreach (string name in new[]{"content.xml","setup2.xml"}) root.Children.Add(new AssetNode { Name=name,Data=Encoding.UTF8.GetBytes("<root/>") });
        File.WriteAllBytes(dlc,RpfArchive.Write(root)); File.WriteAllText(list,"<SMandatoryPacksData><Paths><Item>dlcpacks:/original/</Item></Paths></SMandatoryPacksData>");
        string originalList = File.ReadAllText(list);
        check("DLC registration preserves original entries and source files", () => { string output = Path.Combine(scratch,"dlc-registration"); string zip = DlcPackageService.Prepare(dlc,list,"my-addon",output); Must(XDocument.Load(Path.Combine(output,"dlclist.xml")).Descendants("Item").Select(n=>n.Value).SequenceEqual(new[]{"dlcpacks:/original/","dlcpacks:/my-addon/"}) && File.Exists(zip) && File.ReadAllText(list) == originalList); Must(File.ReadAllBytes(dlc).SequenceEqual(File.ReadAllBytes(Path.Combine(output,"dlcpacks/my-addon/dlc.rpf")))); });
        check("Folder-only DLC registration matches ZIP files and retains the current list", () => {
            string output = Path.Combine(scratch, "DLC folder only");
            Must(DlcPackageService.Prepare(dlc, list, "my-addon", output, createZip: false) == output);
            Must(!Directory.EnumerateFiles(output, "*.zip", SearchOption.AllDirectories).Any());
            var existing = Path.Combine(scratch, "dlc-registration");
            foreach (string file in Directory.EnumerateFiles(existing, "*", SearchOption.AllDirectories).Where(f => !f.EndsWith(".zip"))) Must(File.ReadAllBytes(file).SequenceEqual(File.ReadAllBytes(Path.Combine(output, Path.GetRelativePath(existing, file)))));
            Must(File.ReadAllText(list) == originalList);
        });
        check("DLC registration avoids duplicate entries", () => { string output=Path.Combine(scratch,"dlc-duplicate");DlcPackageService.Prepare(dlc,Path.Combine(scratch,"dlc-registration/dlclist.xml"),"my-addon",output);Must(XDocument.Load(Path.Combine(output,"dlclist.xml")).Descendants("Item").Count()==2); });
        check("DLC registration rejects unsafe IDs",()=>Reject(()=>DlcPackageService.Prepare(dlc,list,"../bad",Path.Combine(scratch,"bad-dlc"))));
        check("DLC registration refuses an existing output",()=>Reject(()=>DlcPackageService.Prepare(dlc,list,"my-addon",Path.Combine(scratch,"dlc-registration"))));
        check("DLC list rejects DTD entities",()=>{string file=Path.Combine(scratch,"unsafe-list.xml");File.WriteAllText(file,"<!DOCTYPE x [<!ENTITY x SYSTEM 'file:///C:/private'>]><SMandatoryPacksData><Paths>&x;</Paths></SMandatoryPacksData>");Reject(()=>DlcPackageService.Prepare(dlc,file,"my-addon",Path.Combine(scratch,"unsafe-dlc")));});
        check("Metadata dependency is reported",()=>{string dir=Path.Combine(scratch,"missing-dependency");Directory.CreateDirectory(dir);File.WriteAllBytes(Path.Combine(dir,"test.ytd"),AssetTests.Dictionary());File.WriteAllText(Path.Combine(dir,"vehicles.meta"),"<root><modelName>missing_model</modelName></root>");Must(AssetService.Analyze(dir,new()).Notes.Any(n=>n.Contains("missing_model")));});
        check("Malformed XML metadata rejected",()=>{string dir=Path.Combine(scratch,"bad-xml");Directory.CreateDirectory(dir);File.WriteAllBytes(Path.Combine(dir,"test.ytd"),AssetTests.Dictionary());File.WriteAllText(Path.Combine(dir,"vehicles.meta"),"<root>");Reject(()=>AssetService.Analyze(dir,new()));});
    }
    public static void Deployment(string scratch, string nro, Action<string,Action> check) {
        string sd=Path.Combine(scratch,"mock-sd");Directory.CreateDirectory(sd); string plugin=Path.Combine(sd,"switch/gtav-scripthook/plugins/deploy-test.nro.manual");
        check("SD install defaults Manual and restore removes new install",()=>{string record=DeploymentService.InstallNro(nro,"deploy-test",sd);Must(File.Exists(plugin));DeploymentService.Restore(record,sd);Must(!File.Exists(plugin));});
        Directory.CreateDirectory(Path.GetDirectoryName(plugin)!);string disabled=plugin.Replace(".manual",".disabled");File.WriteAllBytes(disabled,[1,2,3]);
        check("SD update preserves policy and restores original bytes",()=>{string record=DeploymentService.InstallNro(nro,"deploy-test",sd);Must(File.Exists(disabled)&&!File.Exists(plugin));DeploymentService.Restore(record,sd);Must(File.ReadAllBytes(disabled).SequenceEqual(new byte[]{1,2,3}));});
        check("Restore protects later local changes",()=>{string record=DeploymentService.InstallNro(nro,"deploy-test",sd);File.AppendAllText(disabled,"changed");Reject(()=>DeploymentService.Restore(record,sd));});
        check("Conflicting startup copies rejected",()=>{File.WriteAllBytes(plugin,[1]);Reject(()=>DeploymentService.InstallNro(nro,"deploy-test",sd));File.Delete(plugin);});
        check("Install rejects invalid NRO",()=>{string bad=Path.Combine(scratch,"invalid.nro");File.WriteAllBytes(bad,[1,2,3]);Reject(()=>DeploymentService.InstallNro(bad,"deploy-test",sd));});
        check("Restore rejects path traversal",()=>{string record=Path.Combine(scratch,"evil-install.json");JsonFiles.Write(record,new DeploymentRecord("deploy-test","../outside","bad",null,null,DateTimeOffset.UtcNow));Reject(()=>DeploymentService.Restore(record,sd));});
        check("Runtime report zero formats and plugin grouping",()=>{string file=Path.Combine(scratch,"runtime-report.txt");File.WriteAllText(file,"Report\nloader_error=0x0000\ncleanup_failures=00\nplugin=test.nro\nerror=0x202\n");var r=RuntimeReportService.Read(file);Must(r.Findings.Count==1&&r.Findings[0].Contains("test.nro")&&r.Values["test.nro:error"]=="0x202");});
        check("Runtime report rejects arbitrary text",()=>{string file=Path.Combine(scratch,"not-report.txt");File.WriteAllText(file,"hello");Reject(()=>RuntimeReportService.Read(file));});
    }
}
