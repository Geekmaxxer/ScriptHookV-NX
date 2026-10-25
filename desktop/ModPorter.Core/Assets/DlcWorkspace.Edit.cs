using System.Text;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

public sealed partial class DlcWorkspace {
    static XDocument ParseText(string xml) {
        try { return AddonAudit.Xml(Encoding.UTF8.GetBytes(xml)); }
        catch(System.Xml.XmlException error){throw new InvalidDataException("Metadata XML is empty or invalid at line "+error.LineNumber+": "+error.Message,error);}
    }
    public IReadOnlyList<DlcField> Fields(DlcModel model) {
        return DefinitionFields(DefinitionXml(model));
    }
    public static IReadOnlyList<DlcField> DefinitionFields(string xml) {
        var doc = ParseText(xml);
        return FieldNodes(doc).Select(p => new DlcField(p.Key, p.Value is XAttribute a ? a.Value : ((XElement)p.Value).Value)).ToArray();
    }
    static Dictionary<string, XObject> FieldNodes(XDocument doc) {
        var result = new Dictionary<string, XObject>(StringComparer.Ordinal);
        void Walk(XElement e, string path) {
            if(result.Count>=8192||e.Ancestors().Take(65).Count()>64)throw new InvalidDataException("Definition exceeds the editor limit of 64 nested levels or 8,192 fields. Simplify this definition before editing.");
            foreach (var a in e.Attributes().Where(a => !a.IsNamespaceDeclaration)) {
                if(result.Count>=8192)throw new InvalidDataException("Definition exceeds the editor limit of 8,192 fields.");
                result.Add(path + "/@" + a.Name, a);
            }
            if (!e.HasElements) result.Add(path, e);
            foreach (var g in e.Elements().GroupBy(n => n.Name)) { int i = 0; foreach (var child in g) Walk(child, path + "/" + child.Name + "[" + ++i + "]"); }
        }
        Walk(doc.Root!, doc.Root!.Name.ToString()); return result;
    }
    public void ApplyFields(DlcModel model, IEnumerable<DlcField> fields) {
        SetDefinition(model,EditFieldsXml(DefinitionXml(model),fields));
    }
    public static string EditFieldsXml(string xml, IEnumerable<DlcField> fields) {
        var doc = ParseText(xml); var map = FieldNodes(doc);
        foreach (var f in fields) {
            if (!map.TryGetValue(f.Path, out var node)) throw new InvalidDataException("The definition changed. Select the PED again before applying fields.");
            if (node is XAttribute a) a.Value = f.Value; else ((XElement)node).Value = f.Value;
        }
        return doc.ToString();
    }
    static void RenameInTree(AssetNode next, DlcModel model, string name) {
        CheckModel(name);
        if (name.Equals(model.Name, StringComparison.OrdinalIgnoreCase)) return;
        if (Models(next).Any(m => AddonAudit.ModelHash(m.Name) == AddonAudit.ModelHash(name))) throw new InvalidDataException("A model with this name/hash already exists: " + name);
        var files = DlcTree.Files(next);
        foreach (var pair in files.Where(p => p.Key.Contains("::",StringComparison.Ordinal) && AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(p.Value.Name))).ToArray()) {
            string stem = System.IO.Path.GetFileNameWithoutExtension(pair.Value.Name), suffix = "";
            if (stem.Equals(model.Name + "_p",StringComparison.OrdinalIgnoreCase)) suffix = "_p";
            else if (stem.Equals(model.Name + "_hi",StringComparison.OrdinalIgnoreCase)) suffix = "_hi";
            else if (!stem.Equals(model.Name,StringComparison.OrdinalIgnoreCase)) continue;
            DlcTree.Rename(next,pair.Key,name+suffix+System.IO.Path.GetExtension(pair.Value.Name).ToLowerInvariant());
        }
        foreach (var pair in files.Where(p => System.IO.Path.GetExtension(p.Key).ToLowerInvariant() is ".meta" or ".xml")) {
            var doc = AddonAudit.Xml(pair.Value.Data); bool changed = false;
            foreach (var e in doc.Descendants().Where(n => !n.HasElements)) {
                if (e.Value.Equals(model.Name,StringComparison.OrdinalIgnoreCase)) { e.Value = name; changed = true; }
                else if (e.Value.Equals(model.Name+"_p",StringComparison.OrdinalIgnoreCase)) { e.Value = name+"_p"; changed = true; }
            }
            if (changed) DlcTree.Replace(next,pair.Key,DlcTree.Xml(doc));
        }
    }
    public void SetDefinition(DlcModel model, string xml) {
        var edited = ParseText(xml);
        if (edited.Root?.Name.LocalName != "Item") throw new InvalidDataException("A model definition must have one Item root. Use the metadata file editor for a whole peds.meta.");
        string keyName=model.Kind=="PED"?"Name":"modelName";
        string name = DlcTree.Value(edited.Root,keyName).Trim().ToLowerInvariant(); CheckModel(name);
        edited.Root.Elements().Single(e=>e.Name.LocalName==keyName).Value=name;
        // Keep the replacement definition's own references coordinated too.
        if(!name.Equals(model.Name,StringComparison.OrdinalIgnoreCase))foreach(var e in edited.Root.Descendants().Where(e=>!e.HasElements)){
            if(e.Value.Equals(model.Name,StringComparison.OrdinalIgnoreCase))e.Value=name;
            else if(e.Value.Equals(model.Name+"_p",StringComparison.OrdinalIgnoreCase))e.Value=name+"_p";
        }
        Commit(next => {
            RenameInTree(next,model,name);
            var file = DlcTree.Files(next)[model.MetadataPath]; var doc = AddonAudit.Xml(file.Data);
            var key = model with { Name = name }; Definition(doc,key).ReplaceWith(new XElement(edited.Root));
            DlcTree.Replace(next,model.MetadataPath,DlcTree.Xml(doc));
        });
    }
    public void RenameModel(DlcModel model, string name) {
        var doc = ParseText(DefinitionXml(model));
        doc.Root!.Elements().Single(e => e.Name.LocalName == (model.Kind=="PED"?"Name":"modelName")).Value = name;
        SetDefinition(model,doc.ToString());
    }
    System.Xml.Linq.XDocument ProfileXml(DlcModel model, string profile) {
        if (model.Kind != "PED") throw new ArgumentException("Human movement profiles apply to PEDs.");
        var doc = ParseText(DefinitionXml(model)); var source = PedMetadata.Human(model.Name,profile).Descendants("Item").Single();
        foreach (string name in new[] { "ClipDictionaryName","ExpressionSetName","Pedtype","MovementClipSet","DefaultGestureClipSet","FacialClipsetGroupName","DefaultVisemeClipSet","PedCapsuleName","RelationshipGroup","Personality" }) {
            var field = doc.Root!.Elements().SingleOrDefault(e => e.Name.LocalName==name);
            if (field == null) doc.Root.Add(new XElement(source.Element(name)!)); else field.Value = source.Element(name)!.Value;
        }
        return doc;
    }
    public void SetHumanProfile(DlcModel model, string profile) => SetDefinition(model,ProfileXml(model,profile).ToString());
    public void SetMetadata(string path, string xml) {
        var doc = ParseText(xml); var previous = AddonAudit.Xml(ReadAsset(path));
        if (doc.Root?.Name != previous.Root?.Name) throw new InvalidDataException("Keep this metadata file's root element: " + previous.Root?.Name);
        // Renames need the model action so resource filenames and references move together.
        if (doc.Root?.Name.LocalName is "CPedModelInfo__InitDataList" or "CVehicleModelInfo__InitDataList") {
            string key = doc.Root.Name.LocalName=="CPedModelInfo__InitDataList"?"Name":"modelName";
            string[] Names(XDocument d) => d.Root!.Elements().Where(e=>e.Name.LocalName=="InitDatas").Elements().Select(e=>DlcTree.Value(e,key)).Order(StringComparer.OrdinalIgnoreCase).ToArray();
            if (!Names(doc).SequenceEqual(Names(previous),StringComparer.OrdinalIgnoreCase)) throw new InvalidDataException("Use the model rename/add/remove actions to update assets and model names together. Other metadata fields are fully editable here.");
        }
        Commit(next=>DlcTree.Replace(next,path,DlcTree.Xml(doc)));
    }
    public void RemoveModel(DlcModel model, bool removeAssets = true) {
        var assets = Assets(model); var used = Models().Where(m=>m.Key!=model.Key).SelectMany(Assets).Select(a=>a.Path).ToHashSet(StringComparer.OrdinalIgnoreCase);
        Commit(next=>{
            var doc = AddonAudit.Xml(DlcTree.Files(next)[model.MetadataPath].Data); Definition(doc,model).Remove(); DlcTree.Replace(next,model.MetadataPath,DlcTree.Xml(doc));
            if (removeAssets) foreach (var asset in assets.Where(a=>!used.Contains(a.Path))) DlcTree.Remove(next,asset.Path);
        });
    }
    public IReadOnlyList<DlcAsset> AllAssets() => Files.Where(p=>AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(p.Value.Name)))
        .Select(p=>new DlcAsset(p.Key,p.Value.Name,p.Value.Data.Length)).OrderBy(p=>p.Path,StringComparer.OrdinalIgnoreCase).ToArray();
    public IReadOnlyList<string> ResourceArchives() => Files.Keys.Where(p=>System.IO.Path.GetExtension(p).Equals(".rpf",StringComparison.OrdinalIgnoreCase)).Order(StringComparer.OrdinalIgnoreCase).ToArray();
    static AssetNode PrepareReplacement(string file, int textureCap, CancellationToken cancel) {
        if (!AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(file))) throw new InvalidDataException("Choose a model, texture, variation or other supported resource file.");
        var tree = AssetService.Load(file,cancel); AssetService.Process(tree,file,new(textureCap),true,cancel);
        return tree.Children.Single(n=>!n.Directory);
    }
    public void ReplaceAsset(string path, string file, int textureCap = 0, CancellationToken cancel = default) {
        var prepared = PrepareReplacement(file,textureCap,cancel);
        if (!System.IO.Path.GetExtension(path).Equals(System.IO.Path.GetExtension(prepared.Name),StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Replacement resource type must match the selected asset.");
        Commit(next=>DlcTree.Replace(next,path,prepared.Data),cancel:cancel);
    }
    public void AddAsset(string container, string file, int textureCap = 0, CancellationToken cancel = default) {
        var prepared = PrepareReplacement(file,textureCap,cancel);
        Commit(next=>{
            var archive = DlcTree.Files(next).TryGetValue(container,out var n) ? RpfArchive.Read(n.Data,cancel) : throw new InvalidDataException("Select a resource archive first.");
            DlcTree.Add(archive,prepared.Name,prepared.Data); DlcTree.Replace(next,container,RpfArchive.Write(archive,cancel));
        },cancel:cancel);
    }
    public void RemoveAsset(string path) {
        if (!AssetService.ResourceExtensions.Contains(System.IO.Path.GetExtension(path))) throw new InvalidDataException("Remove only resources here; metadata has its own editor.");
        Commit(next=>DlcTree.Remove(next,path));
    }
}
