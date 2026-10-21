using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using ModPorter.Core;

namespace ModPorter.App;

public partial class MainWindow {
    public async Task RunUiTestAsync(string output, string sdk, string devkit, string python, string assetInput = "") {
        var checks = new List<string>();
        void Check(bool ok, string name) { if (!ok) throw new InvalidOperationException(name + " — " + StatusText.Text + "\n" + BuildLog.Text); checks.Add(name); }
        async Task Idle() {
            var until = DateTime.UtcNow.AddMinutes(2);
            while (Busy && DateTime.UtcNow < until) await Task.Delay(25);
            if (Busy) { operation?.Cancel(); throw new TimeoutException("UI operation did not finish."); }
        }
        void Screenshot(string name, double width = 1210, double height = 850) {
            var root = (FrameworkElement)Content;
            root.Measure(new Size(width, height)); root.Arrange(new Rect(0, 0, width, height)); root.UpdateLayout();
            UpdateResponsiveLayout(); root.UpdateLayout();
            var bitmap = new RenderTargetBitmap((int)width, (int)height, 96, 96, PixelFormats.Pbgra32); bitmap.Render(root);
            var png = new PngBitmapEncoder(); png.Frames.Add(BitmapFrame.Create(bitmap));
            using var file = File.Create(Path.Combine(output, name + ".png")); png.Save(file);
        }
        void Inside(FrameworkElement control, string name) {
            var root=(FrameworkElement)Content;
            var box=control.TransformToAncestor(root).TransformBounds(new Rect(control.RenderSize));
            bool visible=box.Width>0&&box.Height>0&&box.Left>=0&&box.Top>=0&&box.Right<=root.ActualWidth+1&&box.Bottom<=root.ActualHeight+1;
            for(DependencyObject? p=VisualTreeHelper.GetParent(control);p!=null&&p!=root;p=VisualTreeHelper.GetParent(p))
                if(p is ScrollContentPresenter viewport){var v=viewport.TransformToAncestor(root).TransformBounds(new Rect(viewport.RenderSize));visible&=v.Contains(box);}
            Check(visible,name);
        }
        SdkPath.Text = Path.GetFullPath(sdk); DevkitPath.Text = Path.GetFullPath(devkit); PythonPath.Text = python;
        Check(!BuildButton.IsEnabled && !BuildNroButton.IsEnabled && !InstallNroButton.IsEnabled && !CreateButton.IsEnabled,"Unavailable project actions disabled on startup");
        Tabs.SelectedIndex = 0; Screenshot("01-import");
        var example = new[] { Path.Combine(AppContext.BaseDirectory, "examples/pc-native-loop"), Path.Combine(sdk, "desktop/examples/pc-native-loop") }.First(Directory.Exists);
        InputPath.Text = example;
        AnalyzeButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
        Check(input != null && report != null && report.Blockers == 0 && Tabs.SelectedIndex == 1, "Import and analyze through the UI");
        Check(choices.Count == 1 && choices[0].Selected, "Source-file selection populated");
        FindingsList.SelectedIndex = 0; Check(FindingAction.Text != "Select a finding to view its next step.", "Finding action updates on selection");
        Screenshot("02-review");
        Screenshot("17-review-small",920,680);Inside(ReviewNextButton,"Review action remains visible at minimum window size");
        FindingFilter.Text = "DLL_ENTRY"; Check(FindingsList.Items.Count == 1, "Finding filter"); FindingFilter.Text = "";
        SeverityFilter.SelectedIndex = 2; Check(FindingsList.Items.Cast<Finding>().All(f => f.Severity == Severity.Review), "Review severity filter");
        FindingFilter.Text = "NO_SUCH_FINDING"; Check(FindingsEmpty.Visibility == Visibility.Visible && FindingsList.Items.Count == 0,"Empty search state is visible");
        FindingFilter.Text = ""; SeverityFilter.SelectedIndex = 0;
        GoProject_Click(this, new RoutedEventArgs());
        PluginId.Text = "ui-test-example"; OutputPath.Text = Path.Combine(output, "Generated project with spaces");
        CreateButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
        Check(projectFile != null && File.Exists(projectFile), "Generate project through the UI");
        Check(!ProjectSetup.IsExpanded && PageTitle.Text == "Project & build", "Project setup collapses after creation and navigation title follows selection");
        Check(BuildButton.IsEnabled, "Build becomes available after project generation");
        CheckCppButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
        Check(StatusText.Text.Contains("C++ checks passed"),"Compiler-only checks through the UI");
        Screenshot("03-project");
        BuildButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
        Check(buildDirectory != null && Directory.EnumerateFiles(buildDirectory, "*-Switch-test.zip").Any(), "Build and install ZIP through the UI");
        Check(OpenBuildButton.IsEnabled && InstallNroButton.IsEnabled,"Build output and install actions become available after a successful build");
        Check(BuildLog.Text.Contains("NRO structural checks passed"), "Compiler log and NRO validation shown");
        Check(!BuildLog.Text.Contains("nro_sha256")&&fullBuildLog.ToString().Contains("nro_sha256"),"Build output summarizes machine metadata and retains the complete log");
        DetailedLog.IsChecked=true;Check(BuildLog.Text.Contains("nro_sha256"),"Full compiler output is available on demand");DetailedLog.IsChecked=false;
        string sd=Path.Combine(output,"Mock SD");Directory.CreateDirectory(sd);
        string installRecord=DeploymentService.InstallNro(Path.Combine(buildDirectory!,"plugin.nro"),"ui-test-example",sd);
        Check(File.Exists(Path.Combine(sd,"switch/gtav-scripthook/plugins/ui-test-example.nro.manual")),"SD installation backend used by UI");
        DeploymentService.Restore(installRecord,sd);
        Check(!File.Exists(Path.Combine(sd,"switch/gtav-scripthook/plugins/ui-test-example.nro.manual")),"Restore backend used by UI");
        Screenshot("04-build");
        Screenshot("15-build-small",920,680);
        Inside(BuildButton,"Build ZIP action fits the minimum window size");Inside(BuildNroButton,"Standalone NRO action fits the minimum window size");
        Check(BuildButton.TranslatePoint(new Point(),(UIElement)Content).Y==BuildNroButton.TranslatePoint(new Point(),(UIElement)Content).Y,"Both NRO build actions stay on the same row");
        BuildNroButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
        Check(buildDirectory != null && File.Exists(Path.Combine(buildDirectory, "plugin.nro")) && !Directory.EnumerateFiles(buildDirectory, "*.zip").Any(), "Standalone Build NRO through the UI");
        Tabs.SelectedIndex = 0; RealExample_Click(this, new RoutedEventArgs()); await Idle();
        Check(report != null && report.Blockers == 0 && choices.Count == 2, "Pinned real Footprints mod imported through the UI");
        GoProject_Click(this, new RoutedEventArgs()); PluginId.Text = "footprints-nx"; OutputPath.Text = Path.Combine(output, "Footprints real mod");
        CreateButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
        BuildNroButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
        Check(buildDirectory != null && File.Exists(Path.Combine(buildDirectory, "plugin.nro")) && BuildLog.Text.Contains("NRO structural checks passed"), "Real Footprints AArch64 NRO built through the UI");
        Screenshot("07-real-mod");
        Tabs.SelectedIndex = 3; Doctor_Click(this, new RoutedEventArgs()); await Idle();
        Check(DoctorResult.Text.Contains("OK") && !DoctorResult.Text.Contains("MISSING"), "Toolchain doctor through the UI");
        NativeQuery.Text = "PLAYER_PED_ID"; SearchNative_Click(this, new RoutedEventArgs()); await Idle();
        Check(NativeResults.Text.Contains("PLAYER::PLAYER_PED_ID") && NativeResults.Text.Contains("registration present"), "Native lookup through the UI");
        Screenshot("05-toolchain");
        ToolTabs.SelectedIndex=0;Screenshot("16-tool-setup");
        Screenshot("12-toolchain-small",920,680);
        Tabs.SelectedIndex = 4;
        Check(TextureCapCombo.SelectedIndex == 3 && AssetKindCombo.SelectedIndex == 0, "Asset workflow defaults to Auto and 1024");
        Check(AssetTabs.SelectedIndex==0&&DlcGameControls.Visibility==Visibility.Visible&&ReplacementControls.Visibility==Visibility.Collapsed,"Asset preparation and game update workflows start with separate views");
        if (assetInput.Length > 0) {
            AssetInputPath.Text = assetInput; AssetOutputPath.Text = Path.Combine(output, "Asset output"); TextureCapCombo.SelectedIndex = 1;
            AnalyzeAssets_Click(this, new RoutedEventArgs()); await Idle();
            Check(AssetLog.Text.Contains("Assets inspected"), "Actual asset inspection through the UI");
            ConvertAssetsButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)); await Idle();
            Check(assetDirectory != null && File.Exists(Path.Combine(assetDirectory, "Switch-assets-test.zip")) && AssetLog.Text.Contains("Assets prepared"), "Actual asset conversion and ZIP through the UI");
        }
        Screenshot("08-assets"); Screenshot("09-assets-small", 920, 680);
        AssetTabs.SelectedIndex=0;
        Screenshot("13-assets-setup-small",920,680);
        Inside(ConvertAssetsButton,"Asset conversion action stays visible without scrolling");
        var archiveTest=Path.Combine(output,"Current game test");Directory.CreateDirectory(archiveTest);
        var root=new ModPorter.Core.Assets.AssetNode{Directory=true};
        var common=new ModPorter.Core.Assets.AssetNode{Name="common",Directory=true};var data=new ModPorter.Core.Assets.AssetNode{Name="data",Directory=true};root.Children.Add(common);common.Children.Add(data);
        data.Children.Add(new ModPorter.Core.Assets.AssetNode{Name="dlclist.xml",Data=System.Text.Encoding.UTF8.GetBytes("<SMandatoryPacksData><Paths><Item>dlcpacks:/old/</Item></Paths></SMandatoryPacksData>")});
        string current=Path.Combine(archiveTest,"update.rpf"),replacement=Path.Combine(archiveTest,"dlclist.xml");File.WriteAllBytes(current,ModPorter.Core.Assets.RpfArchive.Write(root));File.WriteAllBytes(replacement,data.Children[0].Data);
        CurrentGamePath.Text=current;PreparedAssetPath.Text=replacement;
        AssetTabs.SelectedIndex=1;
        Check(DlcGameControls.Visibility==Visibility.Visible,"Add-on registration is the default game update mode");
        Screenshot("14-dlc-update-small",920,680);
        Inside(UpdateGameDlcButton,"DLC update action stays visible without scrolling");
        ReplaceAssetMode.IsChecked=true;
        Check(DlcGameControls.Visibility==Visibility.Collapsed&&ReplacementControls.Visibility==Visibility.Visible,"Replacement mode reveals only replacement controls");
        FindArchiveEntryButton.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));await Idle();
        Check(ArchiveEntryPath.Text=="common/data/dlclist.xml","Current RPF entry discovery through UI");
        Check(ArchiveEntryPath.Template.FindName("PART_EditableTextBox",ArchiveEntryPath) is TextBox text && text.Text==ArchiveEntryPath.Text,"Editable archive path template displays the selected entry");
        var archiveResult=ModPorter.Core.Assets.GameAssetUpdater.ReplaceAsset(replacement,current,ArchiveEntryPath.Text,Path.Combine(archiveTest,"Verified output"));
        Check(File.Exists(archiveResult.UpdatedArchive)&&File.Exists(archiveResult.Manifest),"Archive update backend used by UI");
        PreparedAssetPath.BringIntoView();Screenshot("10-current-game-workflow");Screenshot("11-current-game-small",920,680);
        Tabs.SelectedIndex = 0; Screenshot("06-small-window", 920, 680);
        Inside(AnalyzeButton,"Import action remains visible at minimum window size");
        InputPath.Text=new string('a',300);Screenshot("18-long-input-small",920,680);Inside(AnalyzeButton,"Long paths do not push import actions outside the window");
        // Exercise busy state and cancellation without launching a compiler or changing real files.
        var pending=RunBusy("Cancellation test",async token=>await Task.Delay(10000,token));
        Check(!Tabs.IsEnabled&&CancelButton.Visibility==Visibility.Visible,"Long operations disable editing and expose cancellation");Cancel_Click(this,new RoutedEventArgs());await pending;
        Check(!Busy&&Tabs.IsEnabled&&CancelButton.Visibility==Visibility.Collapsed,"Cancelled operations restore navigation and action state");
        JsonFiles.Write(Path.Combine(output, "ui-test.json"), new { passed = checks.Count, failed = 0, checks, original_input_modified = false, switch_runtime_verified = false });
    }
}
