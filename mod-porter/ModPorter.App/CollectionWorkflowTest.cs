using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Xml.Linq;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using ModPorter.Core;
using ModPorter.Core.Assets;

namespace ModPorter.App;
public partial class MainWindow {
    public async Task RunCollectionUiTestAsync(string output,string[] inputs){
        var checks=new List<string>();
        void Check(bool ok,string text){if(!ok)throw new IOException(text+": "+StatusText.Text+" / "+CollectionEditor.CollectionStatus.Text);checks.Add(text);}
        async Task Idle(){var until=DateTime.UtcNow.AddMinutes(3);while(Busy&&DateTime.UtcNow<until)await Task.Delay(25);if(Busy)throw new TimeoutException("Collection UI operation did not finish.");}
        async Task Click(Button b){b.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));await Idle();}
        void Capture(FrameworkElement element,string name){element.UpdateLayout();var image=new RenderTargetBitmap((int)element.ActualWidth,(int)element.ActualHeight,96,96,PixelFormats.Pbgra32);image.Render(element);var png=new PngBitmapEncoder();png.Frames.Add(BitmapFrame.Create(image));using var file=File.Create(Path.Combine(output,name+".png"));png.Save(file);}
        ShowActivated=false;ShowInTaskbar=false;Opacity=0;Show();await Task.Delay(100);
        try{
            var sources=inputs.SelectMany(p=>Directory.Exists(p)?Directory.EnumerateFiles(p,"*",SearchOption.AllDirectories):new[]{p}).Distinct().ToDictionary(p=>p,JsonFiles.HashFile);
            Tabs.SelectedIndex=4;AssetTabs.SelectedIndex=3;var editor=CollectionEditor;
            Check(!editor.ImportOptions.IsExpanded&&!editor.DefinitionExpander.IsExpanded&&!editor.FullMetadataExpander.IsExpanded&&!editor.GameRegistrationExpander.IsExpanded,"Collection workflow starts with optional editors collapsed and three clear steps");
            AssetPageScroll.ScrollToTop();UpdateLayout();Capture(AppShell,"collection-start");
            await Click(editor.NewButton);await editor.ImportPathsAsync(inputs);
            var ws=editor.Workspace??throw new IOException("No collection created.");
            Check(ws.Models().Count>=4&&ws.Models().Count(m=>m.Kind=="PED")>=2&&ws.Models().Count(m=>m.Kind=="Vehicle")>=2,"Real PED and vehicle sources are bundled into one collection");
            Check(ws.Audit().ReadyForTest,"Mixed collection passes asset and registration audit");
            editor.DefinitionExpander.IsExpanded=true;
            var vehicle=ws.Models().First(m=>m.Kind=="Vehicle"&&m.Name.Any(char.IsUpper));editor.ModelList.SelectedItem=vehicle;
            editor.DefinitionXml.Text+="\n";await Click(editor.ApplyDefinitionButton);
            Check(editor.ModelList.SelectedItem is DlcModel active&&active.Kind=="Vehicle"&&active.Name==vehicle.Name.ToLowerInvariant(),"Existing mixed-case vehicle definitions remain selectable after editing");
            var selected=ws.Models().First(m=>m.Kind=="PED");string old=selected.Name;
            editor.ModelList.SelectedItem=selected;editor.ModelName.Text="nx_"+old;await Click(editor.ApplyDefinitionButton);
            selected=ws.Models().Single(m=>m.Name=="nx_"+old);
            string validDefinition=editor.DefinitionXml.Text;editor.DefinitionXml.Text=validDefinition.Replace("</Item>","<Name>duplicate</Name></Item>",StringComparison.Ordinal);editor.ModelName.Text="invalid_duplicate";
            Check(editor.CollectionStatus.Text.Contains("duplicate"),"Duplicate XML identity fields do not crash simple-field editing");
            await Click(editor.RevertDefinitionButton);Check(editor.DefinitionXml.Text==validDefinition,"Revert unapplied edits restores a usable definition after invalid XML");
            Check(ws.Assets(selected).Count>=4&&ws.Assets(selected).All(a=>a.Name.StartsWith("nx_"+old,StringComparison.OrdinalIgnoreCase)),"UI model rename updates all four assets and metadata");
            await Click(editor.FemaleButton);Check(ws.DefinitionXml(selected).Contains("CIVFEMALE")&&ws.DefinitionXml(selected).Contains("move_f@generic"),"Female profile updates PED movement and type");
            var otherPed=ws.Models().First(m=>m.Kind=="PED"&&m.Key!=selected.Key);string otherDefinition=ws.DefinitionXml(otherPed);
            editor.ModelFilter.Text=selected.Name;editor.BatchProfile.SelectedIndex=0;await Click(editor.BatchProfileButton);
            Check(ws.DefinitionXml(selected).Contains("CIVMALE")&&ws.DefinitionXml(otherPed)==otherDefinition,"Batch profile action changes only the PEDs shown by the filter");
            await Click(editor.UndoButton);Check(ws.DefinitionXml(selected).Contains("CIVFEMALE"),"One Undo restores the profile batch");editor.ModelFilter.Clear();
            string retained=ws.DefinitionXml(selected);var originalAssets=ws.Assets(selected).ToDictionary(a=>a.Path,a=>JsonFiles.Hash(ws.ReadAsset(a.Path)));
            await editor.ReplacePedSetAsync(new[]{inputs[0]});
            Check(ws.DefinitionXml(selected)==retained&&ws.Assets(selected).Count==4&&ws.Audit().ReadyForTest,"Complete replacement folder retains model name, metadata and valid registration");
            await Click(editor.UndoButton);Check(originalAssets.All(p=>JsonFiles.Hash(ws.ReadAsset(p.Key))==p.Value),"One Undo restores all four original PED resources");
            string incomplete=Directory.EnumerateFiles(inputs[0],"*.ydd",SearchOption.AllDirectories).First();await editor.ReplacePedSetAsync(new[]{incomplete});
            Check(StatusText.Text.Contains("exactly one matching")&&originalAssets.All(p=>JsonFiles.Hash(ws.ReadAsset(p.Key))==p.Value),"Incomplete replacement produces a useful message and leaves the PED intact");
            var item=XDocument.Parse(editor.DefinitionXml.Text);item.Root!.Add(new XElement("WorkspaceCustomField",new XAttribute("enabled","true"),"alpha"));editor.DefinitionXml.Text=item.ToString();await Click(editor.ApplyDefinitionButton);
            Check(ws.DefinitionXml(selected).Contains("WorkspaceCustomField"),"Definition XML editor retains added fields and attributes");
            var row=((IEnumerable<DlcFieldRow>)editor.FieldGrid.ItemsSource).Single(r=>r.Path.EndsWith("WorkspaceCustomField[1]/@enabled"));row.Value="false";await Click(editor.ApplyDefinitionButton);
            Check(ws.DefinitionXml(selected).Contains("enabled=\"false\""),"Field grid applies arbitrary XML attributes");
            var full=new MetadataEditorWindow(ws,selected.MetadataPath){Owner=this,ShowActivated=false,ShowInTaskbar=false,Opacity=0,Width=650,Height=450};full.Show();await Task.Delay(100);full.UpdateLayout();
            Check(full.XmlText.ActualHeight>100&&full.ApplyButton.TranslatePoint(new Point(0,full.ApplyButton.ActualHeight),(UIElement)full.Content).Y<=((FrameworkElement)full.Content).ActualHeight,"Metadata editor retains its XML area and visible actions at minimum size");Capture((FrameworkElement)full.Content,"metadata-editor-minimum");
            full.XmlText.Text=full.XmlText.Text.Replace("alpha","beta",StringComparison.Ordinal);await full.ApplyAsync();
            Check(ws.DefinitionXml(selected).Contains("beta"),"Built-in full peds.meta editor changes the working DLC");
            string validMetadata=full.XmlText.Text;full.XmlText.Text=validMetadata.Replace("InitDatas","InvalidSection",StringComparison.Ordinal);await full.ApplyAsync();
            Check(full.Status.Text.Contains("rename/add/remove")&&ws.Models().Count==4&&ws.DefinitionXml(selected).Contains("beta"),"Invalid metadata structure is rejected while every working model remains available");
            full.XmlText.Text=validMetadata;await full.ApplyAsync();full.Close();editor.Refresh(selected.Key);
            // The recovery applies the same file again; undo it before checking
            // the original metadata edit's undo/redo behavior.
            await Click(editor.UndoButton);
            await Click(editor.UndoButton);Check(ws.DefinitionXml(selected).Contains("alpha"),"Undo restores an entire metadata-file edit");await Click(editor.RedoButton);Check(ws.DefinitionXml(selected).Contains("beta"),"Redo restores metadata changes");
            editor.ShowAllAssets.IsChecked=true;Check(editor.AssetList.Items.Count>4&&editor.ResourceArchive.Items.Count>=1,"Every DLC resource and resource archive is available in the asset editor");
            AssetPageScroll.ScrollToTop();UpdateLayout();Capture(AppShell,"collection-loaded");
            editor.OutputParent.Text=Path.Combine(output,"saves");await Click(editor.SaveFolderButton);
            string archive=PreparedAssetPath.Text;Check(File.Exists(archive)&&!ws.IsDirty,"Save DLC writes one verified archive and clears the unsaved state");
            editor.CollectionId.Text="unsaved-id";Check(!editor.ConfirmClose()&&editor.Summary.Text.Contains("Unsaved"),"Changing only the DLC ID is tracked before Save and prevents silent discard");editor.CollectionId.Text=ws.Id;
            Check(!Directory.EnumerateFiles(assetDirectory!,"update.rpf",SearchOption.AllDirectories).Any(),"Save DLC never generates update.rpf");
            await editor.OpenPathAsync(archive);ws=editor.Workspace!;Check(ws.Models().Count>=4&&ws.Models().Any(m=>m.Name==selected.Name),"An exported mixed DLC reopens with all editor changes");
            editor.ModelList.SelectedItem=ws.Models().Single(m=>m.Name==selected.Name);int count=ws.Models().Count;await Click(editor.DeleteModelButton);Check(ws.Models().Count==count-1,"Remove model updates the PED list and owned assets");await Click(editor.UndoButton);Check(ws.Models().Count==count,"Undo restores removed models and resources");
            await Click(editor.SaveZipButton);Check(Directory.EnumerateFiles(assetDirectory!,"*.zip").Count()==1,"Save DLC + ZIP creates exactly one ZIP");
            AssetNode Dir(string name,params AssetNode[] children){var n=new AssetNode{Name=name,Directory=true};n.Children.AddRange(children);return n;}
            string game=Path.Combine(output,"current-update.rpf");var list=new AssetNode{Name="dlclist.xml",Data=System.Text.Encoding.UTF8.GetBytes("<SMandatoryPacksData><Paths><Item>dlcpacks:/existing/</Item></Paths></SMandatoryPacksData>")};File.WriteAllBytes(game,RpfArchive.Write(Dir("",Dir("common",Dir("data",list)))));
            string original=JsonFiles.HashFile(game);CollectionGamePath=game;await Click(editor.GameFolderButton);
            string updated=Directory.EnumerateFiles(assetDirectory!,"update.rpf",SearchOption.AllDirectories).Single();Check(DlcPackageService.Contains(RpfFileEditor.ReadBinary(updated,"common/data/dlclist.xml"),ws.Id),"First installation registers the collection once");
            string registeredHash=JsonFiles.HashFile(updated);CollectionGamePath=updated;await Click(editor.GameZipButton);
            Check(!Directory.EnumerateFiles(assetDirectory!,"update.rpf",SearchOption.AllDirectories).Any(),"Already registered collection exports only its DLC");
            Check(Directory.EnumerateFiles(Path.GetDirectoryName(assetDirectory!)!,"*.zip").Count()==1,"Registered update creates only the requested install ZIP");
            Check(JsonFiles.HashFile(updated)==registeredHash&&JsonFiles.HashFile(game)==original,"Game-file builds preserve the original and selected current archives");
            string library=Path.Combine(output,"selected-game","update");Directory.CreateDirectory(library);string current=Path.Combine(library,"update.rpf");File.Copy(updated,current);
            foreach(string id in new[]{ws.Id,"local-unregistered"}){string folder=Path.Combine(library,"switch","dlcpacks",id);Directory.CreateDirectory(folder);File.Copy(archive,Path.Combine(folder,"dlc.rpf"));}
            string currentHash=JsonFiles.HashFile(current);await editor.ScanInstalledAsync(Path.GetDirectoryName(library)!);
            Check(editor.InstalledList.Items.Count==3&&editor.InstalledExpander.IsExpanded,"Installed DLC browser discovers local and registered packs without loading every archive");
            editor.InstalledList.SelectedItem=editor.InstalledList.Items.Cast<InstalledDlc>().Single(p=>!p.Exists);Check(!editor.OpenInstalledButton.IsEnabled,"Unavailable base-game packs cannot be opened as loose files");
            editor.InstalledFilter.Text=ws.Id;Check(editor.InstalledList.Items.Count==1&&editor.OpenInstalledButton.IsEnabled,"Installed DLC filter finds the editable registered collection");
            await Click(editor.OpenInstalledButton);ws=editor.Workspace!;Check(ws.Id=="my-mods"&&ws.Models().Count==4&&!ws.IsDirty,"Opening an installed collection retains its identity and saved state");
            Check(JsonFiles.HashFile(current)==currentHash,"Installed DLC discovery and opening preserve the selected game archive");
            int beforeRemoval=ws.Models().Count;foreach(var entry in ws.Models().ToArray())ws.RemoveModel(entry);editor.Refresh();
            Check(editor.ModelList.Items.Count==0&&editor.FullMetadataButton.IsEnabled&&editor.UndoButton.IsEnabled&&!editor.ConfirmClose(),"An emptied collection retains metadata access, Undo and unsaved-change protection");
            for(int i=0;i<beforeRemoval;++i)ws.Undo();editor.Refresh();Check(ws.Models().Count==beforeRemoval&&!ws.IsDirty,"Undo restores every removed model and the saved collection state");
            editor.InstalledExpander.IsExpanded=true;AssetPageScroll.ScrollToTop();UpdateLayout();Capture(AppShell,"installed-library");
            Check(sources.All(p=>JsonFiles.HashFile(p.Key)==p.Value),"All real source assets and original DLC files remain unchanged");
            JsonFiles.Write(Path.Combine(output,"collection-ui-test.json"),new{passed=checks.Count,failed=0,checks,models=ws.Models(),source_files_modified=false,installed_game_files_modified=false,switch_runtime_verified=false});
        }finally{Close();}
    }
}
