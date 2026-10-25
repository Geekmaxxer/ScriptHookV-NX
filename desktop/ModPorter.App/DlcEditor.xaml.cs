using Microsoft.Win32;
using ModPorter.Core.Assets;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Xml.Linq;

namespace ModPorter.App;

public sealed class DlcFieldRow : INotifyPropertyChanged {
    public string Path { get; }
    string value;
    readonly Action changed;
    public string Value { get=>value; set { if(this.value==value)return;this.value=value;PropertyChanged?.Invoke(this,new(nameof(Value)));changed(); } }
    public DlcFieldRow(DlcField field,Action changed){Path=field.Path;value=field.Value;this.changed=changed;}
    public event PropertyChangedEventHandler? PropertyChanged;
}
public partial class DlcEditor : UserControl {
    MainWindow? host;
    internal DlcWorkspace? Workspace { get; private set; }
    DlcModel? model;
    bool loading, syncing, draft;
    readonly ObservableCollection<DlcFieldRow> rows=[];
    IReadOnlyList<InstalledDlc> installed=Array.Empty<InstalledDlc>();
    bool PendingId => Workspace!=null&&!CollectionId.Text.Trim().Equals(Workspace.Id,StringComparison.Ordinal);
    public DlcEditor(){InitializeComponent();FieldGrid.ItemsSource=rows;}
    internal void Attach(MainWindow window){host=window;Refresh();}
    bool DiscardDraft(){
        if(!draft)return true;
        if(host?.CollectionTestMode==true)return false;
        return MessageBox.Show(host,"Discard the unapplied definition edits? Apply definition first to keep them.","DLC editor",MessageBoxButton.YesNo,MessageBoxImage.Question,MessageBoxResult.No)==MessageBoxResult.Yes;
    }
    bool DiscardCollection(){
        if(!DiscardDraft())return false;
        if(Workspace==null||(!Workspace.IsDirty&&!PendingId))return true;
        if(Workspace.Models().Count==0&&!Workspace.CanUndo&&!Workspace.CanRedo&&!PendingId)return true;
        if(host?.CollectionTestMode==true)return false;
        return MessageBox.Show(host,"Discard the unsaved collection changes? Save DLC first to keep them.","DLC collection",MessageBoxButton.YesNo,MessageBoxImage.Question,MessageBoxResult.No)==MessageBoxResult.Yes;
    }
    string SelectedProfile => (ImportProfile.SelectedItem as ComboBoxItem)?.Tag?.ToString()??"male";
    int TextureCap => int.Parse((ImportCap.SelectedItem as ComboBoxItem)?.Tag?.ToString()??"1024");
    void EnsureWorkspace(){Workspace??=DlcWorkspace.Create(CollectionId.Text.Trim());}
    internal void Refresh(string? selected=null){
        loading=true;
        try{
            string? key=selected??model?.Key;var all=Workspace?.Models()??Array.Empty<DlcModel>();
            string filter=ModelFilter.Text.Trim();var visible=all.Where(m=>filter.Length==0||m.Name.Contains(filter,StringComparison.OrdinalIgnoreCase)||m.Kind.Contains(filter,StringComparison.OrdinalIgnoreCase)).ToArray();
            ModelList.ItemsSource=visible;ModelList.SelectedItem=visible.FirstOrDefault(m=>m.Key==key)??visible.FirstOrDefault();model=ModelList.SelectedItem as DlcModel;
            UpdateSummary();
            UndoButton.IsEnabled=Workspace?.CanUndo==true;RedoButton.IsEnabled=Workspace?.CanRedo==true;DeleteModelButton.IsEnabled=model!=null;AuditButton.IsEnabled=Workspace!=null;
            SaveFolderButton.IsEnabled=SaveZipButton.IsEnabled=GameFolderButton.IsEnabled=GameZipButton.IsEnabled=all.Count>0;
            DefinitionPanel.IsEnabled=model!=null;
            FullMetadataButton.IsEnabled=Workspace!=null;
            syncing=true;DefinitionXml.Text=model==null?"":Workspace!.DefinitionXml(model);syncing=false;PopulateFields();
            DefinitionLocation.Text=model?.MetadataPath??"Select a model to edit its definition.";
            MaleButton.IsEnabled=FemaleButton.IsEnabled=model?.Kind=="PED";
            ReplacePedSetButton.IsEnabled=ReplacePedFolderButton.IsEnabled=model?.Kind=="PED";
            BatchProfileButton.IsEnabled=visible.Any(m=>m.Kind=="PED");
            string? container=ShowAllAssets.IsChecked==true?ResourceArchive.SelectedItem?.ToString():null;ResourceArchive.ItemsSource=Workspace?.ResourceArchives();
            ResourceArchive.SelectedItem=container;
            if(ResourceArchive.SelectedItem==null&&model!=null){string? first=Workspace!.Assets(model).FirstOrDefault()?.Path;ResourceArchive.SelectedItem=first==null?null:first[..first.LastIndexOf("::",StringComparison.Ordinal)];}
            if(ResourceArchive.SelectedItem==null)ResourceArchive.SelectedIndex=0;
            RefreshAssets();draft=false;
        }finally{loading=false;}
    }
    void UpdateSummary(){if(Summary==null)return;var all=Workspace?.Models()??Array.Empty<DlcModel>();Summary.Text=Workspace==null?"No collection yet · Add models to start, or open an existing DLC.":Workspace.Id+" · "+all.Count(m=>m.Kind=="PED")+" PEDs · "+all.Count(m=>m.Kind=="Vehicle")+" vehicles · "+(Workspace.IsDirty||PendingId?"Unsaved changes":"Saved");}
    private void CollectionId_Changed(object sender,TextChangedEventArgs e){if(!loading&&Workspace!=null)UpdateSummary();}
    void PopulateFields(){
        syncing=true;
        try{
            rows.Clear();if(!string.IsNullOrWhiteSpace(DefinitionXml.Text))foreach(var f in DlcWorkspace.DefinitionFields(DefinitionXml.Text))rows.Add(new(f,FieldsChanged));
            var doc=string.IsNullOrWhiteSpace(DefinitionXml.Text)?null:XDocument.Parse(DefinitionXml.Text);
            string key=model?.Kind=="Vehicle"?"modelName":"Name",type=model?.Kind=="Vehicle"?"type":"Pedtype";
            ModelName.Text=doc?.Root?.Elements().FirstOrDefault(e=>e.Name.LocalName==key)?.Value??"";
            ModelType.Text=doc?.Root?.Elements().FirstOrDefault(e=>e.Name.LocalName==type)?.Value??"";
        }catch(Exception e)when(e is IOException or InvalidDataException or System.Xml.XmlException){CollectionStatus.Text=e.Message;}
        finally{syncing=false;}
    }
    void FieldsChanged(){
        if(syncing||loading)return;
        try{syncing=true;DefinitionXml.Text=DlcWorkspace.EditFieldsXml(DefinitionXml.Text,rows.Select(r=>new DlcField(r.Path,r.Value)));syncing=false;
            var doc=XDocument.Parse(DefinitionXml.Text);syncing=true;ModelName.Text=doc.Root!.Elements().FirstOrDefault(e=>e.Name.LocalName==(model?.Kind=="Vehicle"?"modelName":"Name"))?.Value??"";ModelType.Text=doc.Root.Elements().FirstOrDefault(e=>e.Name.LocalName==(model?.Kind=="Vehicle"?"type":"Pedtype"))?.Value??"";
            draft=true;CollectionStatus.Text="Definition edited · Apply definition or Save DLC to keep these changes.";
        }catch(Exception e)when(e is IOException or InvalidDataException or System.Xml.XmlException){CollectionStatus.Text=e.Message;}finally{syncing=false;}
    }
    private void Simple_Changed(object sender,TextChangedEventArgs e){
        if(syncing||loading||model==null)return;
        try{var doc=XDocument.Parse(DefinitionXml.Text);string key=ReferenceEquals(sender,ModelName)?(model.Kind=="PED"?"Name":"modelName"):(model.Kind=="PED"?"Pedtype":"type");
            string value=ReferenceEquals(sender,ModelName)?ModelName.Text:ModelType.Text;var node=doc.Root!.Elements().SingleOrDefault(n=>n.Name.LocalName==key);if(node==null)doc.Root.Add(new XElement(key,value));else node.Value=value;DefinitionXml.Text=doc.ToString();
        }catch(Exception error)when(error is System.Xml.XmlException or InvalidOperationException){CollectionStatus.Text="Fix duplicate or invalid definition fields in XML, or use Revert unapplied edits.";}
    }
    private void DefinitionXml_Changed(object sender,TextChangedEventArgs e){if(syncing||loading)return;PopulateFields();draft=true;CollectionStatus.Text="Definition edited · Apply definition or Save DLC to keep these changes.";}
    private void Model_Changed(object sender,SelectionChangedEventArgs e){
        if(loading)return;var selected=ModelList.SelectedItem as DlcModel;
        if(!DiscardDraft()){loading=true;ModelList.SelectedItem=model;loading=false;return;}
        model=selected;Refresh(selected?.Key);
    }
    private void Filter_Changed(object sender,TextChangedEventArgs e){if(host!=null&&!loading&&DiscardDraft())Refresh();}
    private void AssetsView_Changed(object sender,RoutedEventArgs e){if(host!=null)RefreshAssets();}
    void RefreshAssets(){
        AssetList.ItemsSource=Workspace==null?Array.Empty<DlcAsset>():ShowAllAssets.IsChecked==true?Workspace.AllAssets():model==null?Array.Empty<DlcAsset>():Workspace.Assets(model);
        if(AssetList.Items.Count>0)AssetList.SelectedIndex=0;
        ReplaceAssetButton.IsEnabled=ExportAssetButton.IsEnabled=DeleteAssetButton.IsEnabled=AssetList.Items.Count>0;AddAssetButton.IsEnabled=Workspace!=null;
        if(Workspace!=null){UpdateSummary();UndoButton.IsEnabled=Workspace.CanUndo;RedoButton.IsEnabled=Workspace.CanRedo;}
    }
    private void New_Click(object sender,RoutedEventArgs e){if(!DiscardCollection())return;try{Workspace=DlcWorkspace.Create(CollectionId.Text.Trim());model=null;Refresh();CollectionStatus.Text="New collection · Drop PEDs or vehicle DLCs to add them.";}catch(ArgumentException error){CollectionStatus.Text=error.Message;}}
    private async void Open_Click(object sender,RoutedEventArgs e){var dialog=new OpenFileDialog{Title="Open an existing DLC collection",Filter="DLC archive|*.rpf"};if(dialog.ShowDialog(host)==true)await OpenPathAsync(dialog.FileName);}
    private async void FindInstalled_Click(object sender,RoutedEventArgs e){
        if(host==null)return;string game=host.CollectionGamePath;
        if(string.IsNullOrWhiteSpace(game)){var dialog=new OpenFolderDialog{Title="Choose the current SD/game folder"};if(dialog.ShowDialog(host)!=true)return;game=dialog.FolderName;host.CollectionGamePath=game;}
        await ScanInstalledAsync(game);
    }
    internal async Task ScanInstalledAsync(string game){
        if(host==null)return;
        await host.RunCollectionOperation("Reading the current DLC list and pack folders…",async cancel=>{
            var result=await Task.Run(()=>DlcLibrary.Scan(game,cancel),cancel);installed=result.Packs;InstalledLocation.Text=result.PacksDirectory;
            InstalledExpander.IsExpanded=true;RefreshInstalled();CollectionStatus.Text=installed.Count+" DLCs listed; "+installed.Count(p=>p.Exists)+" available in this folder. Select a pack to edit a copy.";
        });
    }
    private async void ChooseInstalledGame_Click(object sender,RoutedEventArgs e){if(host==null)return;var dialog=new OpenFolderDialog{Title="Choose the current SD/game folder"};if(dialog.ShowDialog(host)==true){host.CollectionGamePath=dialog.FolderName;await ScanInstalledAsync(dialog.FolderName);}}
    void RefreshInstalled(){string filter=InstalledFilter.Text.Trim();InstalledList.ItemsSource=installed.Where(p=>filter.Length==0||p.Id.Contains(filter,StringComparison.OrdinalIgnoreCase)).ToArray();InstalledList.SelectedIndex=InstalledList.Items.Count>0?0:-1;}
    private void InstalledFilter_Changed(object sender,TextChangedEventArgs e){if(InstalledList!=null)RefreshInstalled();}
    private void Installed_Changed(object sender,SelectionChangedEventArgs e){OpenInstalledButton.IsEnabled=InstalledList.SelectedItem is InstalledDlc{Exists:true};}
    private async void Installed_DoubleClick(object sender,System.Windows.Input.MouseButtonEventArgs e){if(InstalledList.SelectedItem is InstalledDlc{Exists:true} selected)await OpenPathAsync(selected.Archive);}
    private async void OpenInstalled_Click(object sender,RoutedEventArgs e){if(InstalledList.SelectedItem is InstalledDlc{Exists:true} selected)await OpenPathAsync(selected.Archive);}
    internal async Task OpenPathAsync(string path){
        if(host==null||!DiscardCollection())return;
        await host.RunCollectionOperation("Opening DLC collection…",async cancel=>{Workspace=await Task.Run(()=>DlcWorkspace.Open(path,cancel),cancel);CollectionId.Text=Workspace.Id;model=null;Refresh();CollectionStatus.Text="DLC opened · Original archive is preserved; save an edited copy.";});
    }
    private async void AddFolders_Click(object sender,RoutedEventArgs e){var dialog=new OpenFolderDialog{Title="Add PED/vehicle folders (several can be selected)",Multiselect=true};if(dialog.ShowDialog(host)==true)await ImportPathsAsync(dialog.FolderNames);}
    private async void AddFiles_Click(object sender,RoutedEventArgs e){var dialog=new OpenFileDialog{Title="Add mod ZIPs, DLC archives or PED files",Filter="Mods and assets|*.zip;*.rpf;*.ydd;*.ndd;*.yft;*.nft;*.ymt;*.nmt;*.ytd;*.ntd",Multiselect=true};if(dialog.ShowDialog(host)==true)await ImportPathsAsync(dialog.FileNames);}
    internal async Task ImportPathsAsync(IEnumerable<string> paths){
        if(host==null||!DiscardDraft())return;string[] selected=paths.ToArray();int cap=TextureCap;string profile=SelectedProfile;
        await host.RunCollectionOperation("Preparing and merging models into one DLC…",async cancel=>{EnsureWorkspace();var result=await Task.Run(()=>Workspace!.Import(selected,cap,profile,cancel),cancel);Refresh();CollectionStatus.Text="Added "+result.Peds+" PEDs and "+result.Vehicles+" vehicles. One DLC; existing models and metadata retained.";});
    }
    async Task ApplyDraft(){
        FieldGrid.CommitEdit(DataGridEditingUnit.Cell,true);FieldGrid.CommitEdit(DataGridEditingUnit.Row,true);
        if(!draft||Workspace==null||model==null)return;var selected=model;string xml=DefinitionXml.Text;
        await Task.Run(()=>Workspace.SetDefinition(selected,xml));
        string name=XDocument.Parse(xml).Root!.Elements().Single(n=>n.Name.LocalName==(selected.Kind=="PED"?"Name":"modelName")).Value.Trim().ToLowerInvariant();
        draft=false;Refresh(selected.MetadataPath+"|"+name);
    }
    private async void ApplyDefinition_Click(object sender,RoutedEventArgs e){if(host!=null)await host.RunCollectionOperation("Applying model definition and resource names…",async _=>{await ApplyDraft();CollectionStatus.Text="Definition applied · Save DLC to export the edited archive.";});}
    private void RevertDefinition_Click(object sender,RoutedEventArgs e){Refresh(model?.Key);CollectionStatus.Text="Unapplied definition edits reverted.";}
    async Task Profile(string profile){if(host==null||Workspace==null||model==null)return;await host.RunCollectionOperation("Applying human movement profile…",async _=>{await ApplyDraft();if(model!=null)await Task.Run(()=>Workspace.SetHumanProfile(model,profile));Refresh();CollectionStatus.Text="Human "+profile+" movement applied; custom fields retained.";});}
    private async void Male_Click(object sender,RoutedEventArgs e)=>await Profile("male");
    private async void Female_Click(object sender,RoutedEventArgs e)=>await Profile("female");
    private async void BatchProfile_Click(object sender,RoutedEventArgs e){
        if(host==null||Workspace==null)return;string profile=(BatchProfile.SelectedItem as ComboBoxItem)?.Tag?.ToString()??"male";
        await host.RunCollectionOperation("Applying movement to shown PEDs…",async cancel=>{
            await ApplyDraft();var selected=ModelList.Items.Cast<DlcModel>().Where(m=>m.Kind=="PED").ToArray();
            int count=await Task.Run(()=>Workspace.SetHumanProfiles(selected,profile,cancel),cancel);Refresh();CollectionStatus.Text="Applied human "+profile+" movement to "+count+" shown PEDs. Undo restores the whole batch.";
        });
    }
    private async void Undo_Click(object sender,RoutedEventArgs e){if(host!=null&&DiscardDraft())await host.RunCollectionOperation("Undoing collection edit…",_=>{Workspace?.Undo();Refresh();return Task.CompletedTask;});}
    private async void Redo_Click(object sender,RoutedEventArgs e){if(host!=null&&DiscardDraft())await host.RunCollectionOperation("Restoring collection edit…",_=>{Workspace?.Redo();Refresh();return Task.CompletedTask;});}
    private async void RemoveModel_Click(object sender,RoutedEventArgs e){if(host==null||Workspace==null||model==null||!DiscardDraft())return;var selected=model;await host.RunCollectionOperation("Removing model and owned resources…",async _=>{await Task.Run(()=>Workspace.RemoveModel(selected));model=null;Refresh();CollectionStatus.Text="Model removed · Undo is available.";});}
    private async void Audit_Click(object sender,RoutedEventArgs e){if(host==null||Workspace==null)return;await host.RunCollectionOperation("Checking registration and assets…",async cancel=>{await ApplyDraft();var report=await Task.Run(()=>Workspace.Audit(cancel),cancel);CollectionStatus.Text=report.Text();});}
    private async void FullMetadata_Click(object sender,RoutedEventArgs e){
        if(host==null||Workspace==null)return;
        await host.RunCollectionOperation("Applying pending model edits…",_=>ApplyDraft());if(draft)return;
        var editor=new MetadataEditorWindow(Workspace,model?.MetadataPath){Owner=host};editor.ShowDialog();Refresh();
    }
    string? AssetFile(string title){var dialog=new OpenFileDialog{Title=title,Filter="Asset resources|*.ydd;*.ndd;*.yft;*.nft;*.ymt;*.nmt;*.ytd;*.ntd;*.ydr;*.ndr;*.ybn;*.nbn;*.ycd;*.ncd|All files|*.*"};return dialog.ShowDialog(host)==true?dialog.FileName:null;}
    private async void ReplaceAsset_Click(object sender,RoutedEventArgs e){if(host==null||Workspace==null||AssetList.SelectedItem is not DlcAsset asset)return;string? file=AssetFile("Replace "+asset.Name+" (type must match)");if(file==null)return;await host.RunCollectionOperation("Replacing selected resource…",async cancel=>{await Task.Run(()=>Workspace.ReplaceAsset(asset.Path,file,0,cancel),cancel);RefreshAssets();UndoButton.IsEnabled=Workspace.CanUndo;CollectionStatus.Text="Resource replaced in the working copy · Save DLC to export.";});}
    private async void ExportAsset_Click(object sender,RoutedEventArgs e){if(host==null||Workspace==null||AssetList.SelectedItem is not DlcAsset asset)return;var dialog=new SaveFileDialog{FileName=asset.Name,Filter="Resource|*.*"};if(dialog.ShowDialog(host)!=true)return;await host.RunCollectionOperation("Exporting resource…",async _=>{byte[] data=Workspace.ReadAsset(asset.Path);await File.WriteAllBytesAsync(dialog.FileName,data);CollectionStatus.Text="Resource exported.";});}
    private async void RemoveAsset_Click(object sender,RoutedEventArgs e){if(host==null||Workspace==null||AssetList.SelectedItem is not DlcAsset asset)return;await host.RunCollectionOperation("Removing selected resource…",_=>{Workspace.RemoveAsset(asset.Path);RefreshAssets();UndoButton.IsEnabled=Workspace.CanUndo;CollectionStatus.Text="Resource removed · Missing required assets block export; Undo restores it.";return Task.CompletedTask;});}
    private async void AddAsset_Click(object sender,RoutedEventArgs e){if(host==null||Workspace==null||ResourceArchive.SelectedItem is not string container)return;string? file=AssetFile("Add a resource to the selected RPF");if(file==null)return;await host.RunCollectionOperation("Adding resource…",async cancel=>{await Task.Run(()=>Workspace.AddAsset(container,file,0,cancel),cancel);RefreshAssets();UndoButton.IsEnabled=Workspace.CanUndo;CollectionStatus.Text="Resource added · Choose Show every resource if it is not named after this model.";});}
    private async void ReplacePedSet_Click(object sender,RoutedEventArgs e){
        if(host==null||model?.Kind!="PED")return;
        var dialog=new OpenFileDialog{Title="Replace the selected PED's four assets",Filter="PED files or ZIP|*.ydd;*.ndd;*.yft;*.nft;*.ymt;*.nmt;*.ytd;*.ntd;*.zip",Multiselect=true};
        if(dialog.ShowDialog(host)==true)await ReplacePedSetAsync(dialog.FileNames);
    }
    private async void ReplacePedFolder_Click(object sender,RoutedEventArgs e){if(host==null||model?.Kind!="PED")return;var dialog=new OpenFolderDialog{Title="Choose a folder containing one complete replacement PED"};if(dialog.ShowDialog(host)==true)await ReplacePedSetAsync(new[]{dialog.FolderName});}
    internal async Task ReplacePedSetAsync(IEnumerable<string> paths){
        if(host==null||Workspace==null||model?.Kind!="PED")return;string[] selected=paths.ToArray();int cap=TextureCap;
        await host.RunCollectionOperation("Preparing and replacing the complete PED asset set…",async cancel=>{
            await ApplyDraft();var current=model!;int count=await Task.Run(()=>Workspace.ReplacePedAssets(current,selected,cap,cancel),cancel);
            Refresh(current.Key);CollectionStatus.Text=count+" PED assets replaced; name and metadata retained. Undo restores the whole set.";
        });
    }
    private async void SaveFolder_Click(object sender,RoutedEventArgs e)=>await SaveAsync(false,false);
    private async void SaveZip_Click(object sender,RoutedEventArgs e)=>await SaveAsync(true,false);
    private async void GameFolder_Click(object sender,RoutedEventArgs e)=>await SaveAsync(false,true);
    private async void GameZip_Click(object sender,RoutedEventArgs e)=>await SaveAsync(true,true);
    internal async Task SaveAsync(bool zip,bool withGame){
        if(host==null||Workspace==null)return;string game=host.CollectionGamePath;
        if(withGame&&string.IsNullOrWhiteSpace(game)){var dialog=new OpenFolderDialog{Title="Choose the current SD/game folder"};if(dialog.ShowDialog(host)!=true)return;game=dialog.FolderName;host.CollectionGamePath=game;}
        string parent=OutputParent.Text.Trim();
        await host.RunCollectionOperation("Checking and saving DLC collection…",async cancel=>{await ApplyDraft();Workspace.SetId(CollectionId.Text.Trim());
            string output=parent.Length==0?"":Path.Combine(Path.GetFullPath(parent),"DLC-"+Workspace.Id+"-"+DateTime.Now.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N")[..6]);
            var result=await Task.Run(()=>withGame?Workspace.PrepareGame(game,output,zip,cancel):Workspace.Export(output,zip,cancel),cancel);
            host.UseCollectionOutput(result,Workspace.Id);Refresh();CollectionStatus.Text="Ready: "+result.Directory+(zip?"\nZIP: "+result.Archive:"")+"\n"+(result.UpdateArchiveWritten?"New DLC registered in a copied update.rpf.":"Only this DLC archive was exported; update.rpf was not written.");
        });
    }
    internal bool ConfirmClose()=>DiscardCollection();
}
