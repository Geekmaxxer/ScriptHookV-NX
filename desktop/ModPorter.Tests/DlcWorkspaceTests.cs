using System.Text;
using System.Xml.Linq;
using System.IO.Compression;
using ModPorter.Core.Assets;

static class DlcWorkspaceTests {
    static AssetNode D(string name,params AssetNode[] children){var n=new AssetNode{Name=name,Directory=true};n.Children.AddRange(children);return n;}
    static AssetNode F(string name,byte[] bytes)=>new(){Name=name,Data=bytes,Resource=bytes.AsSpan().StartsWith("RSC7"u8)};
    static byte[] B(string s)=>Encoding.UTF8.GetBytes(s);
    static void Must(bool b){if(!b)throw new Exception("DLC workspace assertion failed");}
    static void Reject(Action a){try{a();}catch(Exception e)when(e is IOException or InvalidDataException or ArgumentException or InvalidOperationException){return;}throw new Exception("Expected editor rejection");}
    static byte[] Resource(uint v)=>new ResourceContainer(v,ResourceContainer.SinglePageFlags(512,v>>4),ResourceContainer.SinglePageFlags(0,v&15),new byte[512],[]).Write();
    static string Ped(string parent,string model){string p=Path.Combine(parent,model);Directory.CreateDirectory(p);File.WriteAllBytes(Path.Combine(p,model+".ydd"),Resource(165));File.WriteAllBytes(Path.Combine(p,model+".yft"),Resource(162));File.WriteAllBytes(Path.Combine(p,model+".ymt"),"PSINtest"u8.ToArray());File.WriteAllBytes(Path.Combine(p,model+".ytd"),AssetTests.Dictionary());return p;}
    public static void Run(string scratch,Action<string,Action> check){
        string dir=Path.Combine(scratch,"DLC workspace");Directory.CreateDirectory(dir);string peds=Path.Combine(dir,"peds");Directory.CreateDirectory(peds);Ped(peds,"alice");Ped(peds,"bob");
        var ws=DlcWorkspace.Create("my-mods");
        check("DLC workspace bundles several loose PEDs in one pack",()=>{var result=ws.Import(new[]{peds},256);Must(result.Peds==2&&result.Vehicles==0&&ws.Models().Count==2&&ws.Audit().ReadyForTest);});
        check("Standard PED builder automatically bundles multiple models",()=>{var built=AddonBuilder.Ped(peds,Path.Combine(dir,"automatic-batch"),new(TextureCap:256),createZip:false);Must(built.Id=="my-mods"&&built.Audit.Models.Count==2&&built.MetadataSource.Contains("multi-model"));});
        check("Batch PED import preserves distinct metadata headers within one DLC",()=>{
            string sources=Path.Combine(dir,"custom-headers");Directory.CreateDirectory(sources);
            foreach(string name in new[]{"header_alice","header_bob"}) {
                string path=Ped(sources,name);var metadata=PedMetadata.Human(name,"female");
                metadata.Root!.SetAttributeValue("customHeader",name);
                metadata.Root.Element("residentTxd")!.Value="txd_"+name;
                metadata.Root.Element("residentAnims")!.Add(new XElement("Item","anim_"+name));
                metadata.Root.AddFirst(new XElement("FutureRootField",new XAttribute("value",name)));
                File.WriteAllText(Path.Combine(path,"peds.meta"),metadata.ToString());
            }
            var collection=DlcWorkspace.Create("headers");collection.Import(new[]{sources},256);
            Must(collection.Models().Count==2&&collection.Audit().ReadyForTest);
            foreach(var model in collection.Models()) {
                var metadata=XDocument.Parse(collection.ReadMetadata(model.MetadataPath));
                Must((string?)metadata.Root!.Attribute("customHeader")==model.Name&&metadata.Root.Element("residentTxd")!.Value=="txd_"+model.Name);
                Must(metadata.Root.Element("residentAnims")!.Element("Item")!.Value=="anim_"+model.Name&&(string?)metadata.Root.Element("FutureRootField")!.Attribute("value")==model.Name);
            }
            var saved=collection.Export(Path.Combine(dir,"header-export"));Must(DlcWorkspace.Open(saved.DlcArchive).Models().Count==2);
        });
        check("Mod covers and JSON sidecars need no manual removal before import",()=>{
            string cover=Path.Combine(peds,"preview.png"),json=Path.Combine(peds,"description.json");File.WriteAllBytes(cover,new byte[]{1,2,3});File.WriteAllText(json,"{\"description\":\"mod\"}");
            try{var batch=DlcWorkspace.Create("covers");batch.Import(new[]{peds},256);Must(batch.Models().Count==2&&batch.Audit().ReadyForTest);}finally{File.Delete(cover);File.Delete(json);}
        });
        var alice=ws.Models().Single(m=>m.Name=="alice");
        check("PED field editor exposes nested fields and attributes and retains unknown XML",()=>{
            var item=XDocument.Parse(ws.DefinitionXml(alice));item.Root!.Add(new XElement("CustomFutureField",new XAttribute("enabled","true"),new XElement("Child","preserve")));ws.SetDefinition(alice,item.ToString());
            var fields=ws.Fields(alice);Must(fields.Any(f=>f.Path.EndsWith("/@enabled"))&&fields.Any(f=>f.Value=="preserve"));
            ws.ApplyFields(alice,fields.Select(f=>f.Path.EndsWith("/@enabled")?f with{Value="false"}:f));Must(ws.DefinitionXml(alice).Contains("enabled=\"false\""));
            ws.SetHumanProfile(alice,"female");Must(ws.DefinitionXml(alice).Contains("CIVFEMALE")&&ws.DefinitionXml(alice).Contains("preserve"));
        });
        check("Model rename moves all PED resources and metadata together with Undo and Redo",()=>{
            var before=ws.Assets(alice).ToDictionary(a=>Path.GetExtension(a.Name),a=>ws.ReadAsset(a.Path));ws.RenameModel(alice,"alice_nx");
            var renamed=ws.Models().Single(m=>m.Name=="alice_nx");Must(ws.Assets(renamed).Count==4&&ws.Assets(renamed).All(a=>a.Name.StartsWith("alice_nx.")));
            Must(ws.Assets(renamed).All(a=>ws.ReadAsset(a.Path).SequenceEqual(before[Path.GetExtension(a.Name)]))&&ws.Audit().ReadyForTest);
            ws.Undo();Must(ws.Models().Any(m=>m.Name=="alice"));ws.Redo();Must(ws.Models().Any(m=>m.Name=="alice_nx"));alice=renamed;
        });
        check("Conflicting model names are rejected atomically",()=>{var before=ws.DefinitionXml(alice);Reject(()=>ws.RenameModel(alice,"bob"));Must(ws.DefinitionXml(alice)==before);Reject(()=>ws.RenameModel(alice,"../bad"));});
        check("Raw peds.meta editor preserves custom fields and rejects uncoordinated renames",()=>{
            string xml=ws.ReadMetadata(alice.MetadataPath);ws.SetMetadata(alice.MetadataPath,xml.Replace("preserve","edited"));Must(ws.DefinitionXml(alice).Contains("edited"));
            Reject(()=>ws.SetMetadata(alice.MetadataPath,xml.Replace("alice_nx","orphan")));Reject(()=>ws.SetMetadata(alice.MetadataPath,"<!DOCTYPE x [<!ENTITY a SYSTEM 'file:///secret'>]><x/>"));
        });
        check("Structural metadata errors roll back and leave the collection index usable",()=>{
            string before=ws.ReadMetadata("setup2.xml");var setup=XDocument.Parse(before);setup.Root!.Element("datFile")!.Value="missing.xml";Reject(()=>ws.SetMetadata("setup2.xml",setup.ToString()));
            setup=XDocument.Parse(before);setup.Root!.Element("deviceName")!.Value="../invalid";Reject(()=>ws.SetMetadata("setup2.xml",setup.ToString()));
            string content=ws.ReadMetadata("content.xml");var broken=XDocument.Parse(content);broken.Root!.Element("dataFiles")!.Remove();Reject(()=>ws.SetMetadata("content.xml",broken.ToString()));
            var ped=XDocument.Parse(ws.ReadMetadata(alice.MetadataPath));ped.Root!.Element("InitDatas")!.Name="wrong";Reject(()=>ws.SetMetadata(alice.MetadataPath,ped.ToString()));
            Must(ws.ReadMetadata("setup2.xml")==before&&ws.ReadMetadata("content.xml")==content&&ws.Models().Count==2&&ws.Audit().ReadyForTest);
        });
        check("Deeply nested definition edits return an error instead of exhausting the editor stack",()=>{
            string xml="<Item>"+string.Concat(Enumerable.Repeat("<Nested>",200))+"value"+string.Concat(Enumerable.Repeat("</Nested>",200))+"</Item>";Reject(()=>DlcWorkspace.DefinitionFields(xml));
        });
        check("Resource removal blocks saving until fixed and can be undone",()=>{
            var a=ws.Assets(alice).Single(a=>a.Name.EndsWith(".nft"));ws.RemoveAsset(a.Path);Must(!ws.Audit().ReadyForTest);string output=Path.Combine(dir,"invalid-save");Reject(()=>ws.Export(output));Must(!Directory.Exists(output));ws.Undo();Must(ws.Audit().ReadyForTest);
        });
        check("Asset replacement validates its type and retains the original model name",()=>{
            var texture=ws.Assets(alice).Single(a=>a.Name.EndsWith(".ntd"));string file=Path.Combine(dir,"new-texture.ytd");File.WriteAllBytes(file,TextureDictionary.Downscale(AssetTests.Dictionary(),256).Data);ws.ReplaceAsset(texture.Path,file);
            Must(TextureDictionary.Inspect(ResourceContainer.Read(ws.ReadAsset(texture.Path))).Single().Width==256);Reject(()=>ws.ReplaceAsset(texture.Path,Path.Combine(peds,"alice/alice.yft")));
        });
        check("Assets can be removed and restored in a selected nested RPF",()=>{
            var bob=ws.Models().Single(m=>m.Name=="bob");var texture=ws.Assets(bob).Single(a=>a.Name.EndsWith(".ntd"));string container=texture.Path.Split("::")[0];ws.RemoveAsset(texture.Path);ws.AddAsset(container,Path.Combine(peds,"bob/bob.ytd"));Must(ws.Audit().ReadyForTest);
        });
        string car=Path.Combine(dir,"car.rpf");
        check("Vehicle DLC import retains handling and tuning registration alongside PEDs",()=>{
            var tree=AddonTests.ValidPack(AssetTests.Dictionary());
            var content=XDocument.Parse(Encoding.UTF8.GetString(tree.Children.Single(n=>n.Name=="content.xml").Data));
            void Register(string file,string type){content.Root!.Element("dataFiles")!.Add(new XElement("Item",new XElement("filename","dlc_test:/"+file),new XElement("fileType",type)));}
            tree.Children.Add(F("cars.rpf",RpfArchive.Write(D("",F("testcar.nft",Resource(162)),F("testcar_hi.nft",Resource(162)),F("testcar.ntd",AssetTests.Dictionary())))));
            Register("cars.rpf","RPF_FILE");Register("vehicles.meta","VEHICLE_METADATA_FILE");Register("handling.meta","HANDLING_FILE");
            tree.Children.Add(F("vehicles.meta",B("<CVehicleModelInfo__InitDataList><InitDatas><Item><modelName>testcar</modelName><txdName>testcar</txdName><handlingId>MY_HANDLING</handlingId><CustomVehicleField>retained</CustomVehicleField></Item></InitDatas></CVehicleModelInfo__InitDataList>")));
            tree.Children.Add(F("handling.meta",B("<CHandlingDataMgr><HandlingData><Item><handlingName>MY_HANDLING</handlingName></Item></HandlingData></CHandlingDataMgr>")));
            tree.Children.Add(F("unrelated.bin",new byte[]{1,7,9,2}));tree.Children.Single(n=>n.Name=="content.xml").Data=B(content.ToString());File.WriteAllBytes(car,RpfArchive.Write(tree));
            var imported=ws.Import(new[]{car},256);Must(imported.Vehicles==1&&ws.Models().Count==3&&ws.Audit().ReadyForTest);
            var model=ws.Models().Single(m=>m.Kind=="Vehicle");Must(ws.DefinitionXml(model).Contains("retained")&&ws.Assets(model).Any(a=>a.Name=="testcar_hi.nft"));
            var untouched=ws.MetadataFiles().Single(p=>p.EndsWith("handling.meta"));Must(ws.ReadMetadata(untouched).Contains("MY_HANDLING"));
        });
        check("Repeated or colliding batch imports preserve the working collection",()=>{int count=ws.Models().Count;string original=ws.DefinitionXml(alice);Reject(()=>ws.Import(new[]{peds}));Must(ws.Models().Count==count&&ws.DefinitionXml(alice)==original);});
        check("Vehicle renames retain coordinated texture references inside the edited definition",()=>{
            var carModel=ws.Models().Single(m=>m.Kind=="Vehicle");ws.RenameModel(carModel,"NEW_CAR");var renamed=ws.Models().Single(m=>m.Name=="new_car");
            Must(ws.DefinitionXml(renamed).Contains("<txdName>new_car</txdName>")&&ws.Assets(renamed).Any(a=>a.Name=="new_car_hi.nft")&&ws.Audit().ReadyForTest);ws.Undo();
        });
        check("Mixed-case existing vehicle names can be edited without manual normalization",()=>{
            var carModel=ws.Models().Single(m=>m.Kind=="Vehicle");string xml=ws.ReadMetadata(carModel.MetadataPath);ws.SetMetadata(carModel.MetadataPath,xml.Replace("testcar","TestCar",StringComparison.Ordinal));
            carModel=ws.Models().Single(m=>m.Kind=="Vehicle");ws.SetDefinition(carModel,ws.DefinitionXml(carModel).Replace("retained","edited",StringComparison.Ordinal));
            Must(ws.Models().Single(m=>m.Kind=="Vehicle").Name=="testcar"&&ws.Audit().ReadyForTest);ws.Undo();ws.Undo();
        });
        DlcWorkspaceOutput? exported=null;
        check("One DLC export reopens with all PEDs, vehicles and unknown source payloads",()=>{
            exported=ws.Export(Path.Combine(dir,"export"),true);Must(File.Exists(exported.Archive)&&!ws.IsDirty);
            var reopened=DlcWorkspace.Open(exported.DlcArchive);Must(reopened.Id=="my-mods"&&reopened.Models().Count==3&&reopened.Audit().ReadyForTest);
            Must(reopened.ReadAsset("imports/part0002/unrelated.bin").SequenceEqual(new byte[]{1,7,9,2}));
            using var zip=ZipFile.OpenRead(exported.Archive);Must(zip.Entries.Count(e=>e.FullName.EndsWith("dlc.rpf"))==1&&!zip.Entries.Any(e=>e.FullName.EndsWith("update.rpf")));
        });
        check("Editing an existing DLC never overwrites its original archive",()=>{
            Must(exported!=null);var before=File.ReadAllBytes(exported!.DlcArchive);var existing=DlcWorkspace.Open(exported.DlcArchive);string extra=Ped(dir,"charlie");existing.Import(new[]{extra});var edited=existing.Export(Path.Combine(dir,"updated"));Must(existing.Models().Count==4&&File.ReadAllBytes(exported.DlcArchive).SequenceEqual(before)&&File.Exists(edited.DlcArchive));
        });
        check("First install registers once; subsequent collection updates omit update.rpf",()=>{
            string current=Path.Combine(dir,"current.rpf");File.WriteAllBytes(current,RpfArchive.Write(D("",D("common",D("data",F("dlclist.xml",B("<SMandatoryPacksData><Paths><Item>dlcpacks:/keep/</Item></Paths></SMandatoryPacksData>")))))));
            var original=File.ReadAllBytes(current);var first=ws.PrepareGame(current,Path.Combine(dir,"first-install"));Must(first.UpdateArchiveWritten);
            string updated=Directory.GetFiles(first.Directory,"update.rpf",SearchOption.AllDirectories).Single();var before=File.ReadAllBytes(updated);var second=ws.PrepareGame(updated,Path.Combine(dir,"second-install"),true);
            Must(!second.UpdateArchiveWritten&&!Directory.EnumerateFiles(second.Directory,"update.rpf",SearchOption.AllDirectories).Any()&&File.ReadAllBytes(updated).SequenceEqual(before)&&File.ReadAllBytes(current).SequenceEqual(original));
            Must(DlcPackageService.Contains(RpfFileEditor.ReadBinary(updated,"common/data/dlclist.xml"),"my-mods"));
        });
        check("Removing a PED removes its owned resources and preserves other models",()=>{ws.RemoveModel(alice);Must(ws.Models().Count==2&&ws.AllAssets().All(a=>!a.Name.StartsWith("alice_nx."))&&ws.Audit().ReadyForTest);ws.Undo();Must(ws.Models().Count==3);});
        check("Cancelled imports leave no partial collection edits",()=>{using var c=new CancellationTokenSource();c.Cancel();int count=ws.Models().Count;try{ws.Import(new[]{peds},cancel:c.Token);}catch(OperationCanceledException){Must(ws.Models().Count==count);return;}throw new Exception("Expected cancellation");});
        check("Filtered PED profile batches preserve vehicles and custom fields with one Undo",()=>{
            var selected=ws.Models().Where(m=>m.Kind=="PED").ToArray();var before=selected.ToDictionary(m=>m.Key,ws.DefinitionXml);var carModel=ws.Models().Single(m=>m.Kind=="Vehicle");string vehicle=ws.DefinitionXml(carModel);
            Must(ws.SetHumanProfiles(selected,"female")==2&&selected.All(m=>ws.DefinitionXml(m).Contains("CIVFEMALE"))&&ws.DefinitionXml(alice).Contains("edited"));
            Must(ws.DefinitionXml(carModel)==vehicle);ws.Undo();Must(selected.All(m=>ws.DefinitionXml(m)==before[m.Key]));ws.Redo();Must(selected.All(m=>ws.DefinitionXml(m).Contains("CIVFEMALE")));ws.Undo();
        });
        check("Invalid or cancelled profile batches leave definitions unchanged",()=>{
            var models=ws.Models().ToArray();string before=ws.DefinitionXml(alice);Reject(()=>ws.SetHumanProfiles(models,"female"));Reject(()=>ws.SetHumanProfiles(new[]{alice},"unknown"));
            using var cancel=new CancellationTokenSource();cancel.Cancel();try{ws.SetHumanProfiles(new[]{alice},"male",cancel.Token);}catch(OperationCanceledException){}Must(ws.DefinitionXml(alice)==before);
        });
        string replacement=Ped(dir,"replacement");
        check("Four-file PED replacement preserves identity, metadata and other models",()=>{
            var bob=ws.Models().Single(m=>m.Name=="bob");var before=ws.Assets(alice).ToDictionary(a=>a.Path,a=>ws.ReadAsset(a.Path));string metadata=ws.DefinitionXml(alice);var other=ws.Assets(bob).ToDictionary(a=>a.Path,a=>ws.ReadAsset(a.Path));
            File.WriteAllBytes(Path.Combine(replacement,"replacement.ymt"),"PSINnew-variation"u8.ToArray());
            Must(ws.ReplacePedAssets(alice,Directory.GetFiles(replacement),256)==4&&ws.DefinitionXml(alice)==metadata&&ws.Audit().ReadyForTest);
            Must(ws.Assets(alice).All(a=>a.Name.StartsWith(alice.Name+"."))&&ws.Assets(alice).Any(a=>a.Name.EndsWith(".nmt")&&ws.ReadAsset(a.Path).SequenceEqual("PSINnew-variation"u8.ToArray())));
            Must(other.All(p=>ws.ReadAsset(p.Key).SequenceEqual(p.Value)));ws.Undo();Must(before.All(p=>ws.ReadAsset(p.Key).SequenceEqual(p.Value)));
        });
        check("Replacement folders and ZIPs are converted as complete sets",()=>{
            Must(ws.ReplacePedAssets(alice,new[]{replacement},256)==4);ws.Undo();string zip=Path.Combine(dir,"replacement.zip");ZipFile.CreateFromDirectory(replacement,zip);Must(ws.ReplacePedAssets(alice,new[]{zip},256)==4);ws.Undo();
        });
        check("Incomplete or ambiguous replacement sets roll back without changing assets",()=>{
            var asset=ws.Assets(alice).First();byte[] before=ws.ReadAsset(asset.Path);Reject(()=>ws.ReplacePedAssets(alice,new[]{Path.Combine(replacement,"replacement.ydd")}));Reject(()=>ws.ReplacePedAssets(alice,new[]{peds}));
            using var c=new CancellationTokenSource();c.Cancel();try{ws.ReplacePedAssets(alice,new[]{replacement},cancel:c.Token);}catch(OperationCanceledException){}Must(ws.ReadAsset(asset.Path).SequenceEqual(before));
        });
        check("Complete PED replacement repairs a previously removed required resource",()=>{
            var texture=ws.Assets(alice).Single(a=>a.Name.EndsWith(".ntd"));ws.RemoveAsset(texture.Path);Must(ws.ReplacePedAssets(alice,new[]{replacement},256)==4&&ws.Audit().ReadyForTest);ws.Undo();Must(!ws.Audit().ReadyForTest);ws.Undo();Must(ws.Audit().ReadyForTest);
        });
        check("Complete PED replacement preserves existing subdirectories inside model archives",()=>{
            var nested=DlcWorkspace.Create("nested-ped");nested.Import(new[]{replacement},256);var saved=nested.Export(Path.Combine(dir,"nested-source"));var tree=RpfArchive.Read(File.ReadAllBytes(saved.DlcArchive));
            AssetNode? resource=null;void Find(AssetNode node){foreach(var child in node.Children)if(child.Directory)Find(child);else if(child.Name.EndsWith(".rpf",StringComparison.OrdinalIgnoreCase))resource=child;}Find(tree);
            var inner=RpfArchive.Read(resource!.Data);var sub=D("models",inner.Children.ToArray());inner.Children.Clear();inner.Children.Add(sub);resource.Data=RpfArchive.Write(inner);
            string file=Path.Combine(dir,"nested.rpf");File.WriteAllBytes(file,RpfArchive.Write(tree));var opened=DlcWorkspace.Open(file);var model=opened.Models().Single();
            Must(opened.ReplacePedAssets(model,new[]{replacement},256)==4&&opened.Assets(model).All(a=>a.Path.Contains("::models/"))&&opened.Audit().ReadyForTest);
        });
        check("Installed DLC browser distinguishes local, registered and unavailable archives",()=>{
            string game=Path.Combine(dir,"library","update");Directory.CreateDirectory(game);string archive=Path.Combine(game,"update.rpf");
            File.WriteAllBytes(archive,RpfArchive.Write(D("",D("common",D("data",F("dlclist.xml",B("<SMandatoryPacksData><Paths><Item>DLCPACKS:/MY-MODS/</Item><Item>dlcpacks:/basepack/</Item></Paths></SMandatoryPacksData>")))))));
            foreach(string id in new[]{"my-mods","not-registered"}){string target=Path.Combine(game,"switch","dlcpacks",id);Directory.CreateDirectory(target);File.WriteAllBytes(Path.Combine(target,"dlc.rpf"),new byte[]{1,2,3});}
            var result=DlcLibrary.Scan(Path.GetDirectoryName(game)!);Must(result.Packs.Count==3&&result.Packs.Single(p=>p.Id.Equals("my-mods",StringComparison.OrdinalIgnoreCase)) is{Registered:true,Exists:true});
            Must(result.Packs.Single(p=>p.Id=="not-registered") is{Registered:false,Exists:true}&&result.Packs.Single(p=>p.Id=="basepack") is{Registered:true,Exists:false});
            var before=File.ReadAllBytes(archive);DlcLibrary.Scan(archive);Must(before.SequenceEqual(File.ReadAllBytes(archive)));
        });
        check("Installed DLC browser rejects unsafe registrations and supports cancellation",()=>{
            string archive=Path.Combine(dir,"unsafe-list.rpf");File.WriteAllBytes(archive,RpfArchive.Write(D("",D("common",D("data",F("dlclist.xml",B("<SMandatoryPacksData><Paths><Item>dlcpacks:/../escape/</Item></Paths></SMandatoryPacksData>")))))));
            Reject(()=>DlcLibrary.Scan(archive));using var c=new CancellationTokenSource();c.Cancel();try{DlcLibrary.Scan(archive,c.Token);}catch(OperationCanceledException){return;}throw new Exception("Expected cancellation");
        });
    }
}
