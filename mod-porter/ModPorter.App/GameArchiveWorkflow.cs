using Microsoft.Win32;
using ModPorter.Core.Assets;
using System.IO;
using System.Windows;

namespace ModPorter.App;
public partial class MainWindow {
    private void PreparedAsset_Click(object sender,RoutedEventArgs e) {
        var file=new OpenFileDialog{Title="Choose the prepared Switch DLC or replacement asset",Filter="Prepared assets|*.rpf;*.ntd;*.nft;*.ndr;*.ndd;*.nbn;*.nmt;*.ncd;*.nmap;*.ntyp;*.nld;*.npt;*.ned;*.nmf;*.xml;*.meta|All files|*.*"};
        string? dir=assetDirectory==null?null:Path.Combine(assetDirectory,"converted");if(dir!=null&&Directory.Exists(dir))file.InitialDirectory=dir;
        if(file.ShowDialog(this)==true)PreparedAssetPath.Text=file.FileName;
    }
    private void CurrentArchive_Click(object sender,RoutedEventArgs e) {
        var file=new OpenFileDialog{Title="Choose the current game RPF (from the SD drive or a local copy)",Filter="Game archives|*.rpf"};
        if(file.ShowDialog(this)==true)CurrentGamePath.Text=file.FileName;
    }
    private void CurrentGameFolder_Click(object sender,RoutedEventArgs e) {string? path=PickFolder("Choose the mounted SD root, GTA game folder or extracted romfs folder");if(path!=null)CurrentGamePath.Text=path;}
    string SelectedArchive()=>GameAssetUpdater.ResolveUpdate(CurrentGamePath.Text.Trim());
    private async void ListArchiveEntries_Click(object sender,RoutedEventArgs e) {
        string archive;try{archive=SelectedArchive();}catch(Exception error)when(error is IOException or ArgumentException){StatusText.Text=error.Message;return;}
        string filename=Path.GetFileName(PreparedAssetPath.Text.Trim());
        await RunBusy("Reading current archive entries…",async cancel=>{
            var entries=await Task.Run(()=>RpfFileEditor.List(archive,cancel),cancel);
            var matching=entries.Where(n=>filename.Length==0||Path.GetFileName(n.Path).Equals(filename,StringComparison.OrdinalIgnoreCase)).Select(n=>n.Path).Order(StringComparer.OrdinalIgnoreCase).ToList();
            ArchiveEntryPath.ItemsSource=matching.Take(512).ToList();if(matching.Count==1)ArchiveEntryPath.Text=matching[0];
            StatusText.Text=matching.Count==0?"No matching asset in this archive. Choose its containing RPF, or enter the exact path.":matching.Count+" matching entries · Choose the exact replacement path";
        });
    }
    void ArchiveProgress(string text)=>Dispatcher.BeginInvoke(()=>{StatusText.Text=text;AssetLog.AppendText(text+"\n");AssetLog.ScrollToEnd();});
    private async void UpdateGameDlc_Click(object sender,RoutedEventArgs e) {
        if(!File.Exists(PreparedAssetPath.Text.Trim())){StatusText.Text="Choose a prepared DLC archive first.";return;}
        if(CurrentGamePath.Text.Trim().Length==0){StatusText.Text="Choose the current update.rpf or its SD/game folder first.";return;}
        string? parent=PickFolder("Choose where to save the new game files (the originals will remain unchanged)");if(parent==null)return;
        string id=DlcId.Text.Trim(),prepared=PreparedAssetPath.Text.Trim(),game=CurrentGamePath.Text.Trim();
        string output=Path.Combine(parent,"GTA-DLC-update-"+DateTime.Now.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N")[..6]);
        await RunBusy("Preparing updated game archive and DLC…",async cancel=>{
            AssetLog.Text="Preparing from the selected current game files.\n"; ShowAssetReport("Building updated game files…");
            var result=await Task.Run(()=>GameAssetUpdater.AddDlc(prepared,game,id,output,cancel,ArchiveProgress),cancel);
            assetDirectory=result.Directory;AssetOutputLabel.Text=result.Directory;AssetReportSummary.Text="Updated game files ready · Source archive preserved";AssetLog.AppendText("\nReady: "+result.Directory+"\nCopy its atmosphere folder to the SD root with GTA closed. Keep the selected original archive as the backup.\n");StatusText.Text="Updated game files ready · Original files preserved";
        });
    }
    private async void ReplaceGameAsset_Click(object sender,RoutedEventArgs e) {
        if(!File.Exists(PreparedAssetPath.Text.Trim())||ArchiveEntryPath.Text.Trim().Length==0){StatusText.Text="Choose a prepared replacement and its exact archive entry first.";return;}
        string archive;try{archive=SelectedArchive();}catch(Exception error)when(error is IOException or ArgumentException){StatusText.Text=error.Message;return;}
        string? parent=PickFolder("Choose where to save the updated archive copy");if(parent==null)return;
        string prepared=PreparedAssetPath.Text.Trim(),entry=ArchiveEntryPath.Text.Trim();
        string output=Path.Combine(parent,"GTA-asset-update-"+DateTime.Now.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N")[..6]);
        await RunBusy("Preparing replacement in an archive copy…",async cancel=>{
            AssetLog.Text="Replacing the selected entry in a new archive copy.\n"; ShowAssetReport("Building replacement archive…");
            var result=await Task.Run(()=>GameAssetUpdater.ReplaceAsset(prepared,archive,entry,output,cancel,ArchiveProgress),cancel);
            assetDirectory=result.Directory;AssetOutputLabel.Text=result.Directory;AssetReportSummary.Text="Replacement archive ready · Source preserved";AssetLog.AppendText("\nReady: "+result.UpdatedArchive+"\nPlace it at the selected archive's original game location with GTA closed.\n");StatusText.Text="Replacement archive ready · Original preserved";
        });
    }
}
