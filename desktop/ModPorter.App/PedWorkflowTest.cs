using System.IO;
using System.Windows;
using System.Windows.Controls;
using ModPorter.Core;
using ModPorter.Core.Assets;

namespace ModPorter.App;
public partial class MainWindow {
    public async Task RunPedUiTestAsync(string output, string input, string template, string basePed, string model) {
        var checks = new List<string>();
        void Check(bool ok, string name) { if (!ok) throw new IOException(name + ": " + StatusText.Text); checks.Add(name); }
        async Task Idle() {
            var until = DateTime.UtcNow.AddMinutes(2);
            while (Busy && DateTime.UtcNow < until) await Task.Delay(25);
            if (Busy) { operation?.Cancel(); throw new TimeoutException("Asset UI operation did not finish."); }
        }
        bool NoZip(string folder) => !Directory.EnumerateFiles(folder, "*.zip", SearchOption.AllDirectories).Any();
        try {
            var sources = Directory.EnumerateFiles(input, "*", SearchOption.AllDirectories).Append(template).Distinct().ToDictionary(f => f, JsonFiles.HashFile);
            Tabs.SelectedIndex = 4; AssetInputPath.Text = input;
            Check(!PedAdvancedExpander.IsExpanded && UseCustomPedMetadata.IsChecked != true && string.IsNullOrEmpty(PedPackId.Text) && string.IsNullOrEmpty(PedModelName.Text), "Default PED form requires no names or custom metadata");
            var dropped = Directory.GetFiles(input).Where(p => AssetService.ResourceExtensions.Contains(Path.GetExtension(p))).ToArray();
            SelectDroppedAssets(dropped);
            Check(AssetInputPath.Text == input && AssetKindCombo.SelectedIndex == 4 && PedAddonExpander.IsExpanded, "Dropping four files selects their folder and opens the PED builder");
            SelectDroppedAssets(dropped[..3]);
            Check(StatusText.Text.Contains("four matching PED files") && AssetInputPath.Text == input, "Incomplete drops explain the requirement and preserve selected assets");
            PedProfile.SelectedIndex = 1;
            foreach (bool zip in new[] { false, true }) {
                string suffix = zip ? "zip" : "folder";
                AssetOutputPath.Text = Path.Combine(output, "automatic-" + suffix);
                (zip ? BuildPedAddonZipButton : BuildPedAddonFolderButton).RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
                Check(File.Exists(PreparedAssetPath.Text) && DlcId.Text == "nx_" + model && AssetLog.Text.Contains("built-in human female"), "Automatic " + suffix + " button builds from four files without game metadata");
                Check(AddonAudit.Inspect(File.ReadAllBytes(PreparedAssetPath.Text)).ReadyForTest, "Automatic " + suffix + " DLC passes registration audit");
                Check(zip ? Directory.EnumerateFiles(assetDirectory!, "*.zip", SearchOption.AllDirectories).Count() == 1 : NoZip(assetDirectory!), "Automatic " + suffix + " respects ZIP selection");
            }
            AssetTabs.SelectedIndex = 0; AssetOutputPath.Text = "";
            BuildPedAddonFolderButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
            Check(assetDirectory != null && File.Exists(PreparedAssetPath.Text), "Blank output path automatically creates a usable new output folder");
            string automaticPath = Path.GetFullPath(assetDirectory!);
            string automaticRoot = Path.GetFullPath(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "ScriptHookV-NX Projects")) + Path.DirectorySeparatorChar;
            if (!automaticPath.StartsWith(automaticRoot, StringComparison.OrdinalIgnoreCase) || !Path.GetFileName(automaticPath).StartsWith("PED-" + model + "-", StringComparison.Ordinal)) throw new IOException("Unexpected automatic output location; preserved for inspection.");
            string retained = Path.GetFullPath(Path.Combine(output, "automatic-default-output"));
            if (!retained.StartsWith(Path.GetFullPath(output) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)) throw new IOException("Test output is outside its evidence folder.");
            Directory.Move(automaticPath, retained);
            assetDirectory = null;
            AssetTabs.SelectedIndex = 0; AssetInputPath.Text = "";
            BuildPedAddonFolderButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
            Check(StatusText.Text.Contains("four PED assets"), "Missing assets produce a useful inline message");
            AssetInputPath.Text = input;
            AssetNode Dir(string name, params AssetNode[] children) { var n = new AssetNode { Name = name, Directory = true }; n.Children.AddRange(children); return n; }
            var dlcList = new AssetNode { Name = "dlclist.xml", Data = System.Text.Encoding.UTF8.GetBytes("<SMandatoryPacksData><Paths><Item>dlcpacks:/existing/</Item></Paths></SMandatoryPacksData>") };
            string game = Path.Combine(output, "current-update.rpf"); File.WriteAllBytes(game, RpfArchive.Write(Dir("", Dir("common", Dir("data", dlcList)))));
            string gameHash = JsonFiles.HashFile(game); CurrentGamePath.Text = game;
            foreach (bool zip in new[] { false, true }) {
                AssetTabs.SelectedIndex = 0;
                AssetOutputPath.Text = Path.Combine(output, "game-" + (zip ? "zip" : "folder"));
                (zip ? BuildPedGameZipButton : BuildPedGameFolderButton).RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
                string updated = Path.Combine(assetDirectory!, "atmosphere/contents/" + GameAssetUpdater.TitleId + "/romfs/update/update.rpf");
                Check(File.Exists(updated) && AssetLog.Text.Contains("DLC registered"), "Build with game files " + zip + " creates an SD-root package");
                var merged = System.Xml.Linq.XDocument.Parse(System.Text.Encoding.UTF8.GetString(RpfFileEditor.ReadBinary(updated, "common/data/dlclist.xml")));
                Check(merged.Descendants("Item").Select(e => e.Value).SequenceEqual(new[] { "dlcpacks:/existing/", "dlcpacks:/nx_" + model + "/" }), "Game build " + zip + " retains existing DLC registrations");
                Check(JsonFiles.HashFile(game) == gameHash, "Game build " + zip + " preserves original game files");
                Check(Directory.EnumerateFiles(AssetOutputPath.Text, "*.zip", SearchOption.AllDirectories).Count() == (zip ? 1 : 0), "Game build " + zip + " creates only the requested install ZIP");
            }
            UseCustomPedMetadata.IsChecked = true; PedAdvancedExpander.IsExpanded = true;
            PedPackId.Text = "nx_" + model; PedModelName.Text = model; PedTemplatePath.Text = template;
            PedBaseName.ItemsSource = AddonBuilder.PedNames(template); PedBaseName.Text = basePed;
            Check(PedBaseName.Items.Count > 0, "Target PED metadata populates the selector");
            string list = Path.Combine(output, "dlclist.xml");
            File.WriteAllText(list, "<SMandatoryPacksData><Paths><Item>dlcpacks:/existing/</Item></Paths></SMandatoryPacksData>");
            string? folderDlcHash = null;
            foreach (bool zip in new[] { false, true }) {
                string suffix = zip ? "zip" : "folder";
                AssetTabs.SelectedIndex = 0; AssetOutputPath.Text = Path.Combine(output, "assets-" + suffix);
                (zip ? ConvertAssetsButton : ConvertAssetsFolderButton).RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
                Check(assetDirectory == AssetOutputPath.Text && File.Exists(Path.Combine(assetDirectory!, "asset-report.json")), "Prepare assets " + suffix + " produces files and reports");
                Check(zip ? File.Exists(Path.Combine(assetDirectory!, "Switch-assets-test.zip")) : NoZip(assetDirectory!), "Prepare assets " + suffix + " respects ZIP choice");
                Check(AssetLog.Text.Contains("ZIP:") == zip, "Asset report identifies the selected output type");
                AssetTabs.SelectedIndex = 0; AssetOutputPath.Text = Path.Combine(output, "ped-" + suffix);
                (zip ? BuildPedAddonZipButton : BuildPedAddonFolderButton).RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
                Check(File.Exists(PreparedAssetPath.Text), "PED " + suffix + " produces a DLC");
                var audit = AddonAudit.Inspect(File.ReadAllBytes(PreparedAssetPath.Text));
                Check(audit.ReadyForTest && audit.Models.Contains(model) && DlcId.Text == "nx_" + model, "PED " + suffix + " retains model and registration");
                Check(zip ? Directory.EnumerateFiles(assetDirectory!, "*.zip", SearchOption.AllDirectories).Count() == 1 : NoZip(assetDirectory!), "PED " + suffix + " creates only requested ZIPs");
                string hash = JsonFiles.HashFile(PreparedAssetPath.Text);
                if (zip) Check(hash == folderDlcHash, "Folder and ZIP buttons produce identical DLC bytes"); else folderDlcHash = hash;
                string dlc = PreparedAssetPath.Text, registration = Path.Combine(output, "registration-" + suffix);
                await PrepareDlcFilesAsync(dlc, list, "nx_" + model, registration, zip);
                Check(File.Exists(Path.Combine(registration, "dlc-package.json")), "Registration " + suffix + " produces its manifest");
                Check(zip ? Directory.EnumerateFiles(registration, "*.zip").Count() == 1 : NoZip(registration), "Registration " + suffix + " respects ZIP choice");
                var entries = System.Xml.Linq.XDocument.Load(Path.Combine(registration, "dlclist.xml")).Descendants("Item").Select(e => e.Value);
                Check(entries.SequenceEqual(new[] { "dlcpacks:/existing/", "dlcpacks:/nx_" + model + "/" }), "Registration " + suffix + " retains existing entries");
                Check(AssetLog.Text.Contains("ZIP:") == zip, "Registration report identifies the selected output type");
            }
            Check(sources.All(f => JsonFiles.HashFile(f.Key) == f.Value), "Asset inputs and target metadata remain unchanged");
            JsonFiles.Write(Path.Combine(output, "ped-ui-test.json"), new { passed = checks.Count, failed = 0, checks, target_metadata_dropdown_populated = PedBaseName.Items.Count > 0,
                selected_model = model, generated_dlc_sha256 = JsonFiles.HashFile(PreparedAssetPath.Text), source_files_modified = false, switch_runtime_verified = false });
        } finally { Close(); }
    }
}
