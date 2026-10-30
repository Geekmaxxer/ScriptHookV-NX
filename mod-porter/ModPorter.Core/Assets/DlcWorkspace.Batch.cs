namespace ModPorter.Core.Assets;

public sealed partial class DlcWorkspace {
    public int SetHumanProfiles(IEnumerable<DlcModel> models, string profile, CancellationToken cancel = default) {
        var selected=models.DistinctBy(m=>m.Key).ToArray();
        if(selected.Length==0)throw new ArgumentException("Select at least one PED.");
        if(selected.Any(m=>m.Kind!="PED"))throw new ArgumentException("Batch movement profiles apply only to PEDs.");
        var replacement=selected.Select(m=>(Model:m,Xml:ProfileXml(m,profile))).ToArray();
        Commit(next=>{
            var files=DlcTree.TopFiles(next);
            foreach(var group in replacement.GroupBy(p=>p.Model.MetadataPath)) {
                cancel.ThrowIfCancellationRequested(); var doc=AddonAudit.Xml(files[group.Key].Data);
                foreach(var entry in group)Definition(doc,entry.Model).ReplaceWith(entry.Xml.Root!);
                DlcTree.Replace(next,group.Key,DlcTree.Xml(doc));
            }
        },cancel:cancel);
        return selected.Length;
    }

    public int ReplacePedAssets(DlcModel model, IEnumerable<string> inputs, int textureCap = 1024, CancellationToken cancel = default) {
        if(model.Kind!="PED")throw new ArgumentException("Complete four-file replacement applies to PEDs. Vehicle resources use the individual asset controls.");
        var paths=inputs.Where(p=>!string.IsNullOrWhiteSpace(p)).Select(Path.GetFullPath).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        if(paths.Length is < 1 or > 32)throw new ArgumentException("Choose one PED folder/ZIP or its four matching files.");
        var candidates=new List<AssetNode>();long bytes=0;
        foreach(string path in paths) {
            var tree=AssetService.Load(path,cancel);
            foreach(var node in DlcTree.Files(tree,cancel).Values.Where(n=>AssetService.ResourceExtensions.Contains(Path.GetExtension(n.Name)))) {
                if((bytes+=node.Data.Length)>RpfArchive.MaxBytes||candidates.Count>=8192)throw new InvalidDataException("Replacement inputs exceed 512 MiB or 8,192 resources.");
                candidates.Add(node);
            }
        }
        string[] required={".ndd",".nft",".nmt",".ntd"};
        var groups=candidates.GroupBy(n=>Path.GetFileNameWithoutExtension(n.Name),StringComparer.OrdinalIgnoreCase)
            .Where(g=>g.Any(n=>AssetService.SwitchName(Path.GetExtension(n.Name)).Equals(".ndd",StringComparison.OrdinalIgnoreCase))).ToArray();
        if(groups.Length!=1)throw new InvalidDataException("Choose exactly one PED variant with four matching assets; select a narrower folder or the four files themselves.");
        var prepared=new Dictionary<string,AssetNode>(StringComparer.OrdinalIgnoreCase);
        foreach(string ext in required) {
            var matching=groups[0].Where(n=>AssetService.SwitchName(Path.GetExtension(n.Name)).Equals(ext,StringComparison.OrdinalIgnoreCase)).ToArray();
            if(matching.Length!=1)throw new InvalidDataException("The replacement PED needs exactly one matching "+ext+" asset (PC or Switch extension).");
            var tree=DlcTree.Dir("",DlcTree.Clone(matching[0]));
            AssetService.Process(tree,"PED replacement",new(textureCap),true,cancel);prepared.Add(ext,tree.Children.Single());
        }
        var owned=Assets(model).Where(a=>Path.GetFileNameWithoutExtension(a.Name).Equals(model.Name,StringComparison.OrdinalIgnoreCase)).ToArray();
        var containers=owned.Select(a=>a.Path[..a.Path.LastIndexOf("::",StringComparison.Ordinal)]).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        if(containers.Length==0)containers=ResourceArchives().ToArray();
        if(containers.Length!=1)throw new InvalidDataException("PED resources span several archives. Use the individual asset controls to choose each destination explicitly.");
        string container=containers[0];
        var directories=owned.Select(a=>a.Path[(a.Path.LastIndexOf("::",StringComparison.Ordinal)+2)..]).Select(p=>p.Contains('/')?p[..(p.LastIndexOf('/')+1)]:"").Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        if(directories.Length>1)throw new InvalidDataException("PED resources span several directories. Use the individual asset controls to select their destinations.");
        string directory=directories.SingleOrDefault()??"";
        var shared=Models().Where(m=>m.Key!=model.Key).SelectMany(Assets).Select(a=>a.Path).ToHashSet(StringComparer.OrdinalIgnoreCase);
        if(owned.Any(a=>shared.Contains(a.Path)))throw new InvalidDataException("This PED shares assets with another model. Replace those files individually after reviewing the shared dependency.");
        Commit(next=>{
            var archive=RpfArchive.Read(DlcTree.Files(next,cancel)[container].Data,cancel);
            foreach(var pair in prepared) {
                cancel.ThrowIfCancellationRequested();string filename=directory+model.Name+pair.Key;
                if(DlcTree.TopFiles(archive).ContainsKey(filename))DlcTree.Replace(archive,filename,pair.Value.Data);else DlcTree.Add(archive,filename,pair.Value.Data);
            }
            DlcTree.Replace(next,container,RpfArchive.Write(archive,cancel));
        },requireReady:true,cancel:cancel);
        return required.Length;
    }
}
