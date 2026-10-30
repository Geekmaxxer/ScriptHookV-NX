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
    private void GoToCollection_Click(object sender,RoutedEventArgs e){AssetTabs.SelectedIndex=3;AssetPageScroll.ScrollToTop();StatusText.Text="DLC collection · Choose a collection, add models, then save a new revision";}
    private void AssetDragOver(object sender, DragEventArgs e) { e.Handled = true; e.Effects = !Busy && e.Data.GetDataPresent(DataFormats.FileDrop) ? DragDropEffects.Copy : DragDropEffects.None; }
    private void AssetDrop(object sender, DragEventArgs e) {
        e.Handled = true;
        if (Busy || e.Data.GetData(DataFormats.FileDrop) is not string[] paths) return;
        SelectDroppedAssets(paths);
    }
    void SelectDroppedAssets(string[] paths) {
        if (paths.Length == 0) return;
        if (paths.Length == 1) { AssetInputPath.Text = paths[0]; return; }
        var parents = paths.Select(Path.GetDirectoryName).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
        var extensions = paths.Select(p => AssetService.SwitchName(Path.GetFileName(p))).Select(Path.GetExtension).ToHashSet(StringComparer.OrdinalIgnoreCase);
        if (paths.Length != 4 || parents.Length != 1 || !extensions.SetEquals(new[] { ".ndd", ".nft", ".nmt", ".ntd" }) || paths.Select(Path.GetFileNameWithoutExtension).Distinct(StringComparer.OrdinalIgnoreCase).Count() != 1) {
            StatusText.Text = "Drop one folder/ZIP, or four matching PED files from the same folder."; return;
        }
        AssetInputPath.Text = parents[0]; AssetKindCombo.SelectedIndex = 4; PedAddonExpander.IsExpanded = true;
        StatusText.Text = "Four PED assets selected · Build PED add-on generates names and metadata automatically";
    }
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
        PedBaseName.ItemsSource = null; PedBaseName.Text = "";
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
    private async void BuildPedAddon_Click(object sender, RoutedEventArgs e) => await BuildPedAddonAsync(createZip: true);
    private async void BuildPedAddonFolder_Click(object sender, RoutedEventArgs e) => await BuildPedAddonAsync(createZip: false);
    private async void BuildPedGameFolder_Click(object sender, RoutedEventArgs e) => await BuildPedAddonAsync(createZip: false, withGame: true);
    private async void BuildPedGameZip_Click(object sender, RoutedEventArgs e) => await BuildPedAddonAsync(createZip: true, withGame: true);
    private async Task BuildPedAddonAsync(bool createZip, bool withGame = false) {
        if (Busy) return;
        string path = AssetInputPath.Text.Trim(), output = AssetOutputPath.Text.Trim();
        if (path.Length == 0) { StatusText.Text = "Choose or drop the folder/ZIP with the four PED assets first."; return; }
        bool custom = UseCustomPedMetadata.IsChecked == true;
        if (custom && string.IsNullOrWhiteSpace(PedTemplatePath.Text)) { StatusText.Text = "Choose custom metadata, or turn that option off to use the built-in human profile."; return; }
        string game = CurrentGamePath.Text.Trim();
        if (withGame && game.Length == 0) {
            string? folder = PickFolder("Choose your SD/game folder (current update.rpf is found automatically)");
            if (folder == null) return; game = folder; CurrentGamePath.Text = game;
        }
        if (withGame) SavePreferences();
        var settings = new PedAddonOptions(PedPackId.Text.Trim(), PedModelName.Text.Trim(), custom ? PedTemplatePath.Text.Trim() : "", custom ? PedBaseName.Text.Trim() : "", AssetSettings.MaximumTextureDimension,
            (PedProfile.SelectedItem as ComboBoxItem)?.Tag?.ToString() ?? "male");
        await RunBusy("Building PED resources, registration and verified DLC…", async cancel => {
            AddonBuild result; string folder, archive;
            if (withGame) {
                var install = await Task.Run(() => AddonBuilder.PedForGame(path, output, settings, game, cancel, createZip, ArchiveProgress), cancel);
                result = install.Ped; folder = install.GameFiles.Directory; archive = install.Archive;
            } else {
                result = await Task.Run(() => AddonBuilder.Ped(path, output, settings, cancel, createZip: createZip), cancel); folder = result.Directory; archive = result.Archive;
            }
            assetDirectory = folder; PreparedAssetPath.Text = result.DlcArchive; DlcId.Text = result.Id;
            AssetLog.Text = result.Audit.Text() + "\n\nSpawn model: " + result.Model + "\nMetadata: " + result.MetadataSource + "\nFolder: " + folder + (createZip ? "\nZIP: " + archive : "")
                + (withGame ? "\n\nDLC registered. Close GTA and copy the output's atmosphere folder to the SD root after backing up replaced files." : "\n\nDLC built. Use Add to game or Build with game files to register it in an existing installation.");
            AssetOutputLabel.Text = folder; ShowAssetReport(withGame ? "PED and registered game files ready" : "PED add-on prepared · Add to game to register it");
            StatusText.Text = "PED built · Spawn: " + result.Model + " · Test appearance and animations on Switch";
        });
    }
    private async void AnalyzeAssets_Click(object sender, RoutedEventArgs e) {
        string path = AssetInputPath.Text.Trim(); var settings = AssetSettings;
        await RunBusy("Inspecting asset resources…", async cancel => {
            var result = await Task.Run(() => AssetService.Analyze(path, settings, cancel), cancel);
            AssetLog.Text = result.Text(); ShowAssetReport($"Inspected · {result.Files.Count} files · {result.Notes.Count} notes"); StatusText.Text = "Assets inspected · Review required metadata and texture limits";
        });
    }
    private async void ConvertAssets_Click(object sender, RoutedEventArgs e) => await ConvertAssetsAsync(createZip: true);
    private async void ConvertAssetsFolder_Click(object sender, RoutedEventArgs e) => await ConvertAssetsAsync(createZip: false);
    private async Task ConvertAssetsAsync(bool createZip) {
        string path = AssetInputPath.Text.Trim(), output = AssetOutputPath.Text.Trim(); var settings = AssetSettings;
        await RunBusy("Preparing copied Switch assets and textures…", async cancel => {
            var result = await Task.Run(() => AssetService.Convert(path, output, settings, cancel, createZip: createZip), cancel);
            assetDirectory = result.Directory;
            PreparedAssetPath.Text = "";
            string prepared = Path.Combine(result.Directory,"converted","dlc.rpf");
            if(File.Exists(prepared)) PreparedAssetPath.Text=prepared;
            else { var files = Directory.EnumerateFiles(Path.Combine(result.Directory,"converted"),"*",SearchOption.AllDirectories).Take(2).ToArray(); if(files.Length==1)PreparedAssetPath.Text=files[0]; }
            AssetLog.Text = result.Report.Text() + "\n\nFolder: " + result.Directory + (createZip ? "\nZIP: " + result.Archive : "");
            AssetOutputLabel.Text = result.Directory;
            ShowAssetReport($"Prepared · {result.Report.Files.Count} files · {result.Report.GraphicsBytesSaved / 1048576.0:N1} MiB of texture allocation removed");
            StatusText.Text = "Assets prepared · Switch gameplay testing required";
        });
    }
    private void OpenAssets_Click(object sender, RoutedEventArgs e) => OpenFolder(assetDirectory);
    void ShowAssetReport(string summary) { AssetReportSummary.Text = summary; AssetTabs.SelectedIndex = 2; }
    private void UsePreparedAssets_Click(object sender, RoutedEventArgs e) => AssetTabs.SelectedIndex = 1;
    private async void PrepareDlc_Click(object sender, RoutedEventArgs e) => await PrepareDlcAsync(createZip: true);
    private async void PrepareDlcFolder_Click(object sender, RoutedEventArgs e) => await PrepareDlcAsync(createZip: false);
    private async Task PrepareDlcAsync(bool createZip) {
        var dlc = new OpenFileDialog { Title = "Choose the converted complete dlc.rpf", Filter = "DLC archive|*.rpf" }; if (dlc.ShowDialog(this) != true) return;
        var list = new OpenFileDialog { Title = "Choose dlclist.xml extracted from your actual installation", Filter = "DLC list|dlclist.xml|XML|*.xml" }; if (list.ShowDialog(this) != true) return;
        string? parent = PickFolder("Choose where to save the merged DLC import package"); if (parent == null) return;
        string id = DlcId.Text.Trim(), output = Path.Combine(parent, id + "-import-" + DateTime.Now.ToString("yyyyMMdd-HHmmss"));
        await PrepareDlcFilesAsync(dlc.FileName, list.FileName, id, output, createZip);
    }
    private async Task PrepareDlcFilesAsync(string dlc, string list, string id, string output, bool createZip) {
        await RunBusy("Preparing DLC registration and import files…", async cancel => {
            string result = await Task.Run(() => DlcPackageService.Prepare(dlc, list, id, output, cancel, createZip: createZip), cancel);
            assetDirectory = output; AssetLog.Text = "DLC import folder: " + output + (createZip ? "\nZIP: " + result : "") + "\nThe selected list's existing registrations were retained. Import the files into the game archives and test on Switch.";
            AssetOutputLabel.Text = output; ShowAssetReport("DLC import package prepared");
            StatusText.Text = "DLC registration package prepared";
        });
    }
}
