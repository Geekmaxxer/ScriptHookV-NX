using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using System.Xml.Linq;
using ModPorter.Core;
using ModPorter.Core.Assets;

static class GameArchiveTests {
    static void Must(bool value){if(!value)throw new Exception("Archive update assertion failed");}
    static void Reject(Action action){try{action();}catch(Exception e)when(e is IOException or InvalidDataException or ArgumentException or InvalidOperationException){return;}throw new Exception("Expected archive update rejection");}
    public static void Run(string scratch,Action<string,Action> check) {
        AssetNode Dir(string name,params AssetNode[] children){var n=new AssetNode{Name=name,Directory=true};n.Children.AddRange(children);return n;}
        AssetNode Data(string name,byte[] bytes,bool resource=false)=>new(){Name=name,Data=bytes,Resource=resource};
        byte[] list=Encoding.UTF8.GetBytes("<SMandatoryPacksData><Paths><Item>dlcpacks:/existing/</Item></Paths></SMandatoryPacksData>");
        byte[] texture=AssetTests.Dictionary();
        string dir=Path.Combine(scratch,"game-archive-tests");Directory.CreateDirectory(dir);
        string archive=Path.Combine(dir,"update.rpf");
        File.WriteAllBytes(archive,RpfArchive.Write(Dir("",Dir("common",Dir("data",Data("dlclist.xml",list),Data("keep.meta",Encoding.UTF8.GetBytes("<old/>") ))),Data("test.ntd",texture,true))));
        byte[] original=File.ReadAllBytes(archive);
        check("File-backed archive lists and reads existing entries",()=>{Must(RpfFileEditor.List(archive).Count==3&&RpfFileEditor.ReadBinary(archive,"COMMON/data/dlclist.xml").SequenceEqual(list));});
        check("Archive copy changes only selected table rows in original byte prefix",()=>{
            string output=Path.Combine(dir,"prefix-copy.rpf");byte[] replacement=DlcPackageService.MergeList(list,"new-pack");var proof=RpfFileEditor.WriteCopy(archive,output,new Dictionary<string,byte[]>{{"common/data/dlclist.xml",replacement}});
            byte[] changed=File.ReadAllBytes(output);var diffs=Enumerable.Range(0,original.Length).Where(i=>original[i]!=changed[i]).ToArray();
            Must(diffs.Length>0&&diffs.All(i=>i>=16&&i<16+BinaryPrimitives.ReadUInt32LittleEndian(original.AsSpan(4))*16)&&diffs.Max()-diffs.Min()<16);
            Must(RpfFileEditor.ReadBinary(output,"common/data/keep.meta").SequenceEqual(Encoding.UTF8.GetBytes("<old/>"))&&File.ReadAllBytes(archive).SequenceEqual(original));
            Must(proof.SourceSha256==JsonFiles.Hash(original)&&proof.OutputSha256==JsonFiles.HashFile(output));
        });
        check("Archive resource replacement retains other payloads",()=>{
            string output=Path.Combine(dir,"resource-copy.rpf");byte[] replacement=TextureDictionary.Downscale(texture,256).Data;
            RpfFileEditor.WriteCopy(archive,output,new Dictionary<string,byte[]>{{"test.ntd",replacement}});
            var tree=RpfArchive.Read(File.ReadAllBytes(output));var node=tree.Children.Single(n=>n.Name=="test.ntd");Must(TextureDictionary.Inspect(ResourceContainer.Read(node.Data)).Single().Width==256&&RpfFileEditor.ReadBinary(output,"common/data/dlclist.xml").SequenceEqual(list));
        });
        check("Unknown replacement is rejected before output exists",()=>{string output=Path.Combine(dir,"missing.rpf");Reject(()=>RpfFileEditor.WriteCopy(archive,output,new Dictionary<string,byte[]>{{"missing.ntd",texture}}));Must(!File.Exists(output));});
        check("Current archive cannot be overwritten",()=>Reject(()=>RpfFileEditor.WriteCopy(archive,archive,new Dictionary<string,byte[]>{{"test.ntd",texture}})));
        check("Source change after list reading rejects stale merge",()=>Reject(()=>RpfFileEditor.WriteCopy(archive,Path.Combine(dir,"stale.rpf"),new Dictionary<string,byte[]>{{"common/data/dlclist.xml",list}},expected:new Dictionary<string,byte[]>{{"common/data/dlclist.xml",Encoding.UTF8.GetBytes("wrong")}})));
        check("Cancellation removes partial archive and preserves source",()=>{
            string output=Path.Combine(dir,"cancelled.rpf");using var c=new CancellationTokenSource();bool cancelled=false;
            try{RpfFileEditor.WriteCopy(archive,output,new Dictionary<string,byte[]>{{"test.ntd",texture}},c.Token,_=>c.Cancel());}catch(OperationCanceledException){cancelled=true;}
            Must(cancelled&&!File.Exists(output)&&!Directory.EnumerateFiles(dir,"*.partial").Any()&&File.ReadAllBytes(archive).SequenceEqual(original));
        });
        check("Encrypted archive table gives actionable rejection",()=>{string bad=Path.Combine(dir,"encrypted.rpf");var bytes=(byte[])original.Clone();BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(12),0x12345678);File.WriteAllBytes(bad,bytes);Reject(()=>RpfFileEditor.List(bad));});
        check("Truncated archive is rejected",()=>{string bad=Path.Combine(dir,"truncated.rpf");File.WriteAllBytes(bad,original[..30]);try{RpfFileEditor.List(bad);}catch(EndOfStreamException){return;}throw new Exception("Truncated table accepted");});
        string sd=Path.Combine(dir,"SD"),romfs=Path.Combine(sd,"atmosphere/contents",GameAssetUpdater.TitleId,"romfs");Directory.CreateDirectory(Path.Combine(romfs,"update"));File.WriteAllBytes(Path.Combine(romfs,"update/update.rpf"),original);
        string dlc=Path.Combine(dir,"dlc.rpf");File.WriteAllBytes(dlc,RpfArchive.Write(Dir("",Data("content.xml",Encoding.UTF8.GetBytes("<root/>")),Data("setup2.xml",Encoding.UTF8.GetBytes("<root/>")),Data("test.ntd",texture,true))));
        check("SD root and local archive resolution",()=>Must(GameAssetUpdater.ResolveUpdate(sd)==Path.GetFullPath(Path.Combine(romfs,"update/update.rpf"))&&GameAssetUpdater.ResolveUpdate(archive)==archive));
        check("Ambiguous game folder requires an exact archive",()=>{File.WriteAllBytes(Path.Combine(sd,"update.rpf"),original);Reject(()=>GameAssetUpdater.ResolveUpdate(sd));File.Delete(Path.Combine(sd,"update.rpf"));});
        check("Add-on update preserves current list and prepares SD-root files",()=>{
            var result=GameAssetUpdater.AddDlc(dlc,sd,"new-pack",Path.Combine(dir,"DLC output"));
            var merged=XDocument.Parse(Encoding.UTF8.GetString(RpfFileEditor.ReadBinary(result.UpdatedArchive,"common/data/dlclist.xml")));
            Must(merged.Descendants("Item").Select(n=>n.Value).SequenceEqual(new[]{"dlcpacks:/existing/","dlcpacks:/new-pack/"}));
            Must(File.ReadAllBytes(Path.Combine(result.Directory,"atmosphere/contents",GameAssetUpdater.TitleId,"romfs/update/switch/dlcpacks/new-pack/dlc.rpf")).SequenceEqual(File.ReadAllBytes(dlc))&&File.ReadAllBytes(archive).SequenceEqual(original));
            Must(!Directory.Exists(Path.Combine(result.Directory,"atmosphere/contents",GameAssetUpdater.TitleId,"romfs/dlcpacks")));
            Must(JsonDocument.Parse(File.ReadAllText(result.Manifest)).RootElement.GetProperty("complete").GetBoolean());
        });
        check("Add-on updater refuses existing output",()=>Reject(()=>GameAssetUpdater.AddDlc(dlc,sd,"new-pack",Path.Combine(dir,"DLC output"))));
        check("Add-on updater rejects unsafe IDs",()=>Reject(()=>GameAssetUpdater.AddDlc(dlc,sd,"../bad",Path.Combine(dir,"bad"))));
        check("Replacement workflow validates exact target and writes copied archive",()=>{string file=Path.Combine(dir,"test.ntd");File.WriteAllBytes(file,TextureDictionary.Downscale(texture,256).Data);var result=GameAssetUpdater.ReplaceAsset(file,archive,"test.ntd",Path.Combine(dir,"Replacement output"));Must(File.Exists(result.UpdatedArchive)&&File.ReadAllBytes(archive).SequenceEqual(original));});
        check("Executable replacement cannot enter the asset workflow",()=>{string file=Path.Combine(dir,"evil.nsc");File.WriteAllBytes(file,[1,2,3]);Reject(()=>GameAssetUpdater.ReplaceAsset(file,archive,"evil.nsc",Path.Combine(dir,"invalid output")));});
    }
}
