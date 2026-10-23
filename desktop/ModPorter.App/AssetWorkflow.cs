using Microsoft.Win32;
using ModPorter.Core.Assets;
using System.IO;
using System.Windows;
using System.Windows.Controls;

namespace ModPorter.App;

public partial class MainWindow {
    string? assetDirectory;
    AssetOptions AssetSettings => new(TextureCapCombo.SelectedItem is ComboBoxItem cap && int.TryParse(cap.Content?.ToString(), out int size) ? size : 0,
        (AssetKindCombo.SelectedItem as ComboBoxItem)?.Content?.ToString() ?? "Auto");
    private void AssetFile_Click(object sender, RoutedEventArgs e) {
        var dialog = new OpenFileDialog { Title = "Choose a mod asset or DLC archive", Filter = "Mod assets|*.zip;*.rpf;*.ytd;*.ntd;*.yft;*.nft;*.ydr;*.ndr;*.ydd;*.ndd;*.ymt;*.nmt;*.ybn;*.nbn|All files|*.*" };
        if (dialog.ShowDialog(this) == true) AssetInputPath.Text = dialog.FileName;
    }
    private void AssetFolder_Click(object sender, RoutedEventArgs e) { string? path = PickFolder("Choose the mod's asset/DLC folder"); if (path != null) AssetInputPath.Text = path; }
    private void AssetOutput_Click(object sender, RoutedEventArgs e) { string? path = PickFolder("Choose a parent folder for converted assets"); if (path != null) AssetOutputPath.Text = Path.Combine(path, "Switch-assets-" + DateTime.Now.ToString("yyyyMMdd-HHmmss")); }
    private async void PedTemplate_Click(object sender, RoutedEventArgs e) {
        var dialog = new OpenFileDialog { Title = "Choose current-game peds.meta or its containing OPEN RPF", Filter = "PED metadata or game archive|*.meta;*.rpf" };
        if (dialog.ShowDialog(this) != true) return;
        PedTemplatePath.Text = dialog.FileName;
        await RunBusy("Reading target PED definitions…", async cancel => {
            var names = await Task.Run(() => AddonBuilder.PedNames(dialog.FileName, cancel), cancel);
            PedBaseName.ItemsSource = names; StatusText.Text = names.Count + " PED definitions available · Choose the matching base";
        });
    }
    private async void UpdateGameBatch_Click(object sender, RoutedEventArgs e) {
        string game = CurrentGamePath.Text.Trim();
        if (game.Length == 0) { StatusText.Text = "Choose the current update.rpf or its SD/game folder first."; return; }
        string? folder = PickFolder("Choose the collection containing pack-id/dlc.rpf subfolders"); if (folder == null) return;
        string? parent = PickFolder("Choose where to save the updated game files"); if (parent == null) return;
        string output = Path.Combine(parent, "Switch-addons-" + DateTime.Now.ToString("yyyyMMdd-HHmmss"));
        await RunBusy("Checking all DLC packs and merging the current game list…", async cancel => {
            var result = await Task.Run(() => AddonBatch.Prepare(AddonBatch.Discover(folder), game, output, cancel), cancel);
            assetDirectory = result.Directory; AssetOutputLabel.Text = result.Directory;
            AssetLog.Text = "All selected DLC packs were prepared together. Existing registrations were retained; the source game files were not changed.\nManifest: " + result.Manifest;
            ShowAssetReport("DLC collection prepared · Switch gameplay testing required"); StatusText.Text = "Updated game files verified";
        });
    }
    private async void BuildPedAddon_Click(object sender, RoutedEventArgs e) {
        string path = AssetInputPath.Text.Trim(), output = AssetOutputPath.Text.Trim();
        var settings = new PedAddonOptions(PedPackId.Text.Trim(), PedModelName.Text.Trim(), PedTemplatePath.Text.Trim(), PedBaseName.Text.Trim(), AssetSettings.MaximumTextureDimension);
        await RunBusy("Building PED resources, registration and verified DLC…", async cancel => {
            var result = await Task.Run(() => AddonBuilder.Ped(path, output, settings, cancel), cancel);
            assetDirectory = result.Directory; PreparedAssetPath.Text = result.DlcArchive; DlcId.Text = settings.Id;
            AssetLog.Text = result.Audit.Text() + "\n\nSpawn model: " + settings.Model + "\nZIP: " + result.Archive;
            AssetOutputLabel.Text = result.Directory; ShowAssetReport("PED add-on prepared · Add to game to register it");
            StatusText.Text = "PED DLC built · Appearance and gameplay testing required";
        });
    }
    private async void AnalyzeAssets_Click(object sender, RoutedEventArgs e) {
        string path = AssetInputPath.Text.Trim(); var settings = AssetSettings;
        await RunBusy("Inspecting asset resources…", async cancel => {
            var result = await Task.Run(() => AssetService.Analyze(path, settings, cancel), cancel);
            AssetLog.Text = result.Text(); ShowAssetReport($"Inspected · {result.Files.Count} files · {result.Notes.Count} notes"); StatusText.Text = "Assets inspected · Review required metadata and texture limits";
        });
    }
    private async void ConvertAssets_Click(object sender, RoutedEventArgs e) {
        string path = AssetInputPath.Text.Trim(), output = AssetOutputPath.Text.Trim(); var settings = AssetSettings;
        await RunBusy("Preparing copied Switch assets and textures…", async cancel => {
            var result = await Task.Run(() => AssetService.Convert(path, output, settings, cancel), cancel);
            assetDirectory = result.Directory;
            string prepared = Path.Combine(result.Directory,"converted","dlc.rpf");
            if(File.Exists(prepared)) PreparedAssetPath.Text=prepared;
            else { var files = Directory.EnumerateFiles(Path.Combine(result.Directory,"converted"),"*",SearchOption.AllDirectories).Take(2).ToArray(); if(files.Length==1)PreparedAssetPath.Text=files[0]; }
            AssetLog.Text = result.Report.Text() + "\n\nZIP: " + result.Archive;
            AssetOutputLabel.Text = result.Directory;
            ShowAssetReport($"Prepared · {result.Report.Files.Count} files · {result.Report.GraphicsBytesSaved / 1048576.0:N1} MiB of texture allocation removed");
            StatusText.Text = "Assets prepared · Switch gameplay testing required";
        });
    }
    private void OpenAssets_Click(object sender, RoutedEventArgs e) => OpenFolder(assetDirectory);
    void ShowAssetReport(string summary) { AssetReportSummary.Text = summary; AssetTabs.SelectedIndex = 2; }
    private void UsePreparedAssets_Click(object sender, RoutedEventArgs e) => AssetTabs.SelectedIndex = 1;
    private async void PrepareDlc_Click(object sender, RoutedEventArgs e) {
        var dlc = new OpenFileDialog { Title = "Choose the converted complete dlc.rpf", Filter = "DLC archive|*.rpf" }; if (dlc.ShowDialog(this) != true) return;
        var list = new OpenFileDialog { Title = "Choose dlclist.xml extracted from your actual installation", Filter = "DLC list|dlclist.xml|XML|*.xml" }; if (list.ShowDialog(this) != true) return;
        string? parent = PickFolder("Choose where to save the merged DLC import package"); if (parent == null) return;
        string id = DlcId.Text.Trim(), output = Path.Combine(parent, id + "-import-" + DateTime.Now.ToString("yyyyMMdd-HHmmss"));
        await RunBusy("Preparing DLC registration and import files…", async cancel => {
            string zip = await Task.Run(() => DlcPackageService.Prepare(dlc.FileName, list.FileName, id, output, cancel), cancel);
            assetDirectory = output; AssetLog.Text = "DLC import ZIP: " + zip + "\nThe selected list's existing registrations were retained. Import the files into the game archives and test on Switch.";
            AssetOutputLabel.Text = output; ShowAssetReport("DLC import package prepared");
            StatusText.Text = "DLC registration package prepared";
        });
    }
}
