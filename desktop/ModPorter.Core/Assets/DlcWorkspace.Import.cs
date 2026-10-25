using System.Text;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public sealed record DlcImportResult(int Peds, int Vehicles, int Packs, IReadOnlyList<string> Notes);
public sealed partial class DlcWorkspace {
    static bool Complete(AssetNode node) => node.Directory && node.Children.Any(n=>n.Name.Equals("setup2.xml",StringComparison.OrdinalIgnoreCase));
    static AssetNode LoosePack(string name, IEnumerable<AssetNode> resources, IEnumerable<(string Name,string Type,XDocument Doc)> metadata) {
        var resource = DlcTree.Dir("");
        foreach (var node in resources) {
            var old = resource.Children.FirstOrDefault(n=>n.Name.Equals(node.Name,StringComparison.OrdinalIgnoreCase));
            if (old != null) { if (!old.Data.SequenceEqual(node.Data)) throw new InvalidDataException("Different resources use the same filename: "+node.Name+". Select one variant or rename it before bundling."); continue; }
            resource.Children.Add(DlcTree.Clone(node));
        }
        var pack = Create("import").root; var files = new List<(string Path,string Type)> { ("switch/models/"+name+".rpf","RPF_FILE") };
        DlcTree.Add(pack,files[0].Path,RpfArchive.Write(resource));
        foreach (var (filename,type,doc) in metadata) { string path="common/data/"+filename; DlcTree.Add(pack,path,DlcTree.Xml(doc)); files.Add((path,type)); }
        var content = AddonAudit.Xml(pack.Children.Single(n=>n.Name=="content.xml").Data);
        foreach (var f in files) content.Root!.Element("dataFiles")!.Add(new XElement("Item",new XElement("filename","dlc_import:/"+f.Path),new XElement("fileType",f.Type),new XElement("disabled",new XAttribute("value","true")),new XElement("persistent",new XAttribute("value","false"))));
        content.Root!.Element("contentChangeSets")!.Add(new XElement("Item",new XElement("changeSetName","import_STARTUP"),new XElement("filesToEnable",files.Select(f=>new XElement("Item","dlc_import:/"+f.Path)))));
        DlcTree.Replace(pack,"content.xml",DlcTree.Xml(content));
        var setup = AddonAudit.Xml(pack.Children.Single(n=>n.Name=="setup2.xml").Data);
        setup.Descendants("ContentChangeSets").Single().Add(new XElement("Item","import_STARTUP")); DlcTree.Replace(pack,"setup2.xml",DlcTree.Xml(setup)); return pack;
    }
    static XDocument MergeDocuments(IEnumerable<XDocument> documents) {
        var list = documents.ToArray(); var merged = new XDocument(list[0]);
        foreach (var doc in list[1..]) {
            if (doc.Root!.Name != merged.Root!.Name) throw new InvalidDataException("Metadata roots differ; import each complete vehicle DLC instead.");
            foreach (var child in doc.Root.Elements()) {
                var existing = merged.Root.Elements(child.Name).SingleOrDefault();
                if (existing == null) merged.Root.Add(new XElement(child));
                else if (existing.HasElements && child.HasElements) existing.Add(child.Elements().Select(e=>new XElement(e)));
                else if (existing.IsEmpty && child.HasElements) existing.Add(child.Elements().Select(e=>new XElement(e)));
                else if (!XNode.DeepEquals(existing,child) && !child.IsEmpty) throw new InvalidDataException("Conflicting metadata field: "+child.Name+". Import complete DLC packs to retain separate settings.");
            }
        }
        return merged;
    }
    static void Loose(AssetNode tree, List<AssetNode> packs, string profile, List<string> notes, CancellationToken cancel) {
        var files = DlcTree.Files(tree,cancel);
        var resources = files.Values.Where(n=>AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(n.Name))).ToArray();
        var peds = resources.Where(n=>System.IO.Path.GetExtension(n.Name).ToLowerInvariant() is ".ndd" or ".nmt")
            .Select(n=>System.IO.Path.GetFileNameWithoutExtension(n.Name)).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        if (peds.Length > 0) {
            var groups = new Dictionary<string,(XDocument Metadata,List<AssetNode> Resources)>();
            foreach (string name in peds) {
                string model = name.ToLowerInvariant(); CheckModel(model);
                var selected = new List<AssetNode>();
                foreach (string ext in new[]{".ndd",".nft",".nmt",".ntd"}) {
                    var matching = resources.Where(n=>n.Name.Equals(name+ext,StringComparison.OrdinalIgnoreCase)).ToArray();
                    if (matching.Length != 1) throw new InvalidDataException("PED '"+name+"' needs exactly one "+name+ext+". Several variants must be imported separately.");
                    selected.Add(matching[0]);
                }
                var matches = new List<(XDocument Doc,XElement Item)>();
                foreach (var meta in files.Values.Where(n=>n.Name.Equals("peds.meta",StringComparison.OrdinalIgnoreCase))) {
                    var doc = PedMetadata.Parse(meta.Data,"peds.meta in batch input");
                    matches.AddRange(doc.Root!.Elements().Where(e=>e.Name.LocalName=="InitDatas").Elements().Where(e=>DlcTree.Value(e,"Name").Equals(name,StringComparison.OrdinalIgnoreCase)).Select(e=>(doc,e)));
                }
                if (matches.Count > 1) throw new InvalidDataException("Duplicate PED definitions for "+name+". Choose one variant.");
                var metadata = new XDocument(matches.Count==1 ? matches[0].Doc : PedMetadata.Human(model,profile));
                var definitions = metadata.Root!.Elements().Single(e=>e.Name.LocalName=="InitDatas");
                var item = new XElement(definitions.Elements().Single(e=>DlcTree.Value(e,"Name").Equals(name,StringComparison.OrdinalIgnoreCase)));
                item.Elements().Single(e=>e.Name.LocalName=="Name").Value=model;
                definitions.RemoveNodes();
                // Keep each source header intact. Distinct resident TXDs, animation
                // dictionaries and unknown root fields need separate metadata files,
                // registered inside the same collection rather than discarded.
                string header = JsonFiles.Hash(DlcTree.Xml(metadata));
                if (!groups.TryGetValue(header,out var group)) { group=(metadata,new()); groups.Add(header,group); }
                group.Metadata.Root!.Elements().Single(e=>e.Name.LocalName=="InitDatas").Add(item);
                selected.AddRange(resources.Where(n=>System.IO.Path.GetFileNameWithoutExtension(n.Name).Equals(name+"_p",StringComparison.OrdinalIgnoreCase)));
                group.Resources.AddRange(selected);
            }
            foreach (var group in groups.Values) packs.Add(LoosePack("peds",group.Resources,new[]{("peds.meta","PED_METADATA_FILE",group.Metadata)}));
            notes.Add(peds.Length+" loose PEDs detected; matching mod metadata retained, otherwise human "+profile+" defaults used.");
        }
        var vehicleMeta = files.Values.Where(n=>n.Name.Equals("vehicles.meta",StringComparison.OrdinalIgnoreCase)).ToArray();
        if (vehicleMeta.Length > 0) {
            var typeMap = new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase) { ["vehicles.meta"]="VEHICLE_METADATA_FILE",["handling.meta"]="HANDLING_FILE",["carcols.meta"]="CARCOLS_FILE",["carvariations.meta"]="VEHICLE_VARIATION_FILE",["vehiclelayouts.meta"]="VEHICLE_LAYOUTS_FILE" };
            var metadata = files.Values.Where(n=>typeMap.ContainsKey(n.Name)).GroupBy(n=>n.Name,StringComparer.OrdinalIgnoreCase)
                .Select(g=>(g.Key.ToLowerInvariant(),typeMap[g.Key],MergeDocuments(g.Select(n=>AddonAudit.Xml(n.Data))))).ToArray();
            var pedNames = peds.SelectMany(n=>new[]{n,n+"_p"}).ToHashSet(StringComparer.OrdinalIgnoreCase);
            var vehicleResources = resources.Where(n=>!pedNames.Contains(System.IO.Path.GetFileNameWithoutExtension(n.Name))).ToArray();
            packs.Add(LoosePack("vehicles",vehicleResources,metadata)); notes.Add("Vehicle handling, tuning, variations and layouts imported from supplied metadata.");
        } else if (resources.Any(n=>!peds.Any(p=>System.IO.Path.GetFileNameWithoutExtension(n.Name).Equals(p,StringComparison.OrdinalIgnoreCase)||System.IO.Path.GetFileNameWithoutExtension(n.Name).Equals(p+"_p",StringComparison.OrdinalIgnoreCase))))
            throw new InvalidDataException("Loose vehicle resources need their author's vehicles.meta and related handling/tuning metadata, or a complete DLC archive. Physical settings cannot be inferred safely from a model file.");
    }
    static AssetNode? Harvest(AssetNode node, List<AssetNode> packs, CancellationToken cancel) {
        cancel.ThrowIfCancellationRequested();
        if (Complete(node)) { packs.Add(node); return null; }
        if (!node.Directory && System.IO.Path.GetExtension(node.Name).Equals(".rpf",StringComparison.OrdinalIgnoreCase)) {
            var child=RpfArchive.Read(node.Data,cancel); if(Complete(child)){packs.Add(child);return null;}
            return node;
        }
        if (!node.Directory) return node;
        var result=DlcTree.Dir(node.Name); foreach(var child in node.Children){var remaining=Harvest(child,packs,cancel);if(remaining!=null)result.Children.Add(remaining);}return result;
    }
    static void MergeSection(XElement target, XElement source) {
        foreach(var child in source.Elements()) {
            var existing=target.Elements(child.Name).SingleOrDefault();
            if(existing==null){target.Add(new XElement(child));continue;}
            if(child.HasElements){
                if(existing.Attributes().Any()&&!existing.Attributes().Select(a=>a.ToString()).SequenceEqual(child.Attributes().Select(a=>a.ToString())))throw new InvalidDataException("Conflicting content section attributes: "+child.Name);
                existing.Add(child.Elements().Select(e=>new XElement(e)));
            } else if((!string.IsNullOrWhiteSpace(child.Value)||child.HasAttributes)&&!XNode.DeepEquals(existing,child)) throw new InvalidDataException("Conflicting content setting: "+child.Name);
        }
    }
    static void MergePack(AssetNode destination, AssetNode source, CancellationToken cancel) {
        RequireReady(source,cancel);
        var sourceSetup=AddonAudit.Xml(source.Children.Single(n=>n.Name.Equals("setup2.xml",StringComparison.OrdinalIgnoreCase)).Data);
        var targetSetupNode=destination.Children.Single(n=>n.Name.Equals("setup2.xml",StringComparison.OrdinalIgnoreCase));var targetSetup=AddonAudit.Xml(targetSetupNode.Data);
        string oldDevice=DlcTree.Value(sourceSetup.Root,"deviceName"),device=DlcTree.Value(targetSetup.Root,"deviceName"),dat=DlcTree.Value(sourceSetup.Root,"datFile"),targetDat=DlcTree.Value(targetSetup.Root,"datFile");
        var sourceFiles=DlcTree.Files(source,cancel);var content=AddonAudit.Xml(sourceFiles[dat].Data);var targetFiles=DlcTree.TopFiles(destination);var targetContent=AddonAudit.Xml(targetFiles[targetDat].Data);
        var imports=destination.Children.FirstOrDefault(n=>n.Directory&&n.Name.Equals("imports",StringComparison.OrdinalIgnoreCase));int number=1;
        while(imports?.Children.Any(n=>n.Name.Equals("part"+number.ToString("D4"),StringComparison.OrdinalIgnoreCase))==true)number++;
        if(number>1024)throw new InvalidDataException("This collection already contains 1,024 imported groups. Split it into another DLC.");
        string prefix="imports/part"+number.ToString("D4")+"/";
        var sets=content.Descendants().Where(e=>e.Name.LocalName=="changeSetName").Select(e=>e.Value.Trim()).Distinct(StringComparer.OrdinalIgnoreCase)
            .ToDictionary(s=>s,s=>"nx"+number+"_"+s[..Math.Min(48,s.Length)],StringComparer.OrdinalIgnoreCase);
        string Rebase(string value) {
            if(sets.TryGetValue(value.Trim(),out var set))return set;
            string normalized=value.Trim().Replace('\\','/').Replace("%PLATFORM%","switch",StringComparison.OrdinalIgnoreCase);
            if(normalized.StartsWith(oldDevice+":/",StringComparison.OrdinalIgnoreCase))return device+":/"+prefix+InputLoader.SafeRelative(normalized[(oldDevice.Length+2)..]);
            if(sourceFiles.ContainsKey(normalized))return device+":/"+prefix+normalized;
            return value;
        }
        void RebaseXml(XDocument doc) {
            foreach(var e in doc.Descendants().Where(e=>!e.HasElements)){string value=Rebase(e.Value);if(value!=e.Value)e.Value=value;}
            foreach(var a in doc.Descendants().Attributes()){string value=Rebase(a.Value);if(value!=a.Value)a.Value=value;}
        }
        void ConvertFiles(AssetNode dir,int depth) {
            if(depth>4)throw new InvalidDataException("Too many nested archives.");
            foreach(var node in dir.Children) {
                cancel.ThrowIfCancellationRequested();if(node.Directory){ConvertFiles(node,depth);continue;}
                string ext=System.IO.Path.GetExtension(node.Name).ToLowerInvariant();
                if(ext==".rpf"){var nested=RpfArchive.Read(node.Data,cancel);ConvertFiles(nested,depth+1);node.Data=RpfArchive.Write(nested,cancel);}
                else if(ext is ".meta" or ".xml"){var doc=AddonAudit.Xml(node.Data);RebaseXml(doc);node.Data=DlcTree.Xml(doc);}
                else if(node.Data.AsSpan().IndexOf(Encoding.ASCII.GetBytes(oldDevice+":/"))>=0)throw new InvalidDataException("A compiled asset embeds its old DLC device path: "+node.Name+". This reference needs a format-specific conversion before bundling.");
            }
        }
        var copied=DlcTree.Clone(source);ConvertFiles(copied,0);
        copied.Children.RemoveAll(n=>n.Name.Equals("setup2.xml",StringComparison.OrdinalIgnoreCase));
        // content is folded into the existing root; all other source payloads stay.
        DlcTree.Remove(copied,dat);
        if(imports==null){imports=DlcTree.Dir("imports");destination.Children.Add(imports);}var container=DlcTree.Dir("part"+number.ToString("D4"));container.Children.AddRange(copied.Children);imports.Children.Add(container);
        RebaseXml(content);MergeSection(targetContent.Root!,content.Root!);
        var targetGroups=targetSetup.Root!.Elements().SingleOrDefault(e=>e.Name.LocalName=="contentChangeSetGroups");
        if(targetGroups==null){targetGroups=new XElement("contentChangeSetGroups");targetSetup.Root.Add(targetGroups);}
        foreach(var group in sourceSetup.Root!.Elements().Where(e=>e.Name.LocalName=="contentChangeSetGroups").Elements()) {
            var clone=new XElement(group);RebaseXml(new XDocument(clone));string name=DlcTree.Value(clone,"NameHash");
            var existing=targetGroups.Elements().SingleOrDefault(e=>DlcTree.Value(e,"NameHash").Equals(name,StringComparison.OrdinalIgnoreCase));
            if(existing==null)targetGroups.Add(clone);else {
                var list=existing.Elements().SingleOrDefault(e=>e.Name.LocalName=="ContentChangeSets");if(list==null){list=new XElement("ContentChangeSets");existing.Add(list);}
                list.Add(clone.Elements().Where(e=>e.Name.LocalName=="ContentChangeSets").Elements().Select(e=>new XElement(e)));
            }
        }
        foreach(var child in sourceSetup.Root.Elements().Where(e=>e.Name.LocalName is not ("deviceName" or "datFile" or "nameHash" or "timeStamp" or "order" or "type" or "contentChangeSetGroups"))) {
            var existing=targetSetup.Root.Elements(child.Name).SingleOrDefault();
            if(existing==null)targetSetup.Root.Add(new XElement(child));else if(!XNode.DeepEquals(existing,child))throw new InvalidDataException("Conflicting DLC setup setting: "+child.Name);
        }
        DlcTree.Replace(destination,targetDat,DlcTree.Xml(targetContent));DlcTree.Replace(destination,"setup2.xml",DlcTree.Xml(targetSetup));
    }
    public DlcImportResult Import(IEnumerable<string> inputs, int textureCap = 1024, string profile = "male", CancellationToken cancel = default) {
        var paths=inputs.Where(p=>!string.IsNullOrWhiteSpace(p)).Select(System.IO.Path.GetFullPath).Select(p=>AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(p))?System.IO.Path.GetDirectoryName(p)!:p).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        if(paths.Length==0)throw new ArgumentException("Choose folders, ZIPs, complete DLCs or matching PED assets.");
        if(paths.Length>256)throw new InvalidDataException("Import at most 256 sources per operation.");
        return ImportTrees(paths.Select(path=>(AssetService.Load(path,cancel),path)),textureCap,profile,cancel);
    }
    internal DlcImportResult ImportLoaded(AssetNode source,string label,int textureCap,string profile,CancellationToken cancel) => ImportTrees(new[]{(source,label)},textureCap,profile,cancel);
    DlcImportResult ImportTrees(IEnumerable<(AssetNode Tree,string Path)> inputs,int textureCap,string profile,CancellationToken cancel) {
        var packs=new List<AssetNode>();var notes=new List<string>();
        foreach(var (tree,path) in inputs) {
            var report=AssetService.Process(tree,path,new(textureCap),true,cancel);notes.AddRange(report.Notes);
            var loose=Harvest(tree,packs,cancel);if(loose!=null)Loose(loose,packs,profile,notes,cancel);
        }
        if(packs.Count==0)throw new InvalidDataException("No complete PED sets or vehicle DLCs found in the selected sources.");
        var before=Models();Commit(next=>{foreach(var pack in packs)MergePack(next,pack,cancel);},requireReady:true,cancel:cancel);
        var after=Models();return new(after.Count(m=>m.Kind=="PED")-before.Count(m=>m.Kind=="PED"),after.Count(m=>m.Kind=="Vehicle")-before.Count(m=>m.Kind=="Vehicle"),packs.Count,notes);
    }
}
