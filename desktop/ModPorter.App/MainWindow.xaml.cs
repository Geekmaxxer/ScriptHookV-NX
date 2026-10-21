using Microsoft.Win32;
using ModPorter.Core;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace ModPorter.App;

public sealed class SourceChoice {
    public string Path { get; set; } = "";
    public bool Selected { get; set; } = true;
}
public sealed class Preferences {
    public string Sdk { get; set; } = "";
    public string Python { get; set; } = "python";
    public string Devkit { get; set; } = "";
    public string ProjectParent { get; set; } = "";
}
public partial class MainWindow : Window {
    ModInput? input;
    AnalysisReport? report;
    string? projectFile, buildDirectory;
    CancellationTokenSource? operation;
    readonly ObservableCollection<SourceChoice> choices = [];
    readonly StringBuilder fullBuildLog = new();
    readonly string preferencesPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "ScriptHookV-NX", "ModPorter", "settings.json");
    string projectParent = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "ScriptHookV-NX Projects");
    bool Busy => operation != null;
    bool TestMode => Environment.GetCommandLineArgs().Any(a => a is "--ui-test" or "--layout-test");
    ToolSettings Settings => new(SdkPath.Text.Trim(), PythonPath.Text.Trim(), DevkitPath.Text.Trim());
    public MainWindow() {
        InitializeComponent(); InitializeResponsiveLayout(); SourceList.ItemsSource = choices;
        SdkPath.Text = FindSdk(); PythonPath.Text = FindPython(); DevkitPath.Text = FindDevkit();
        try {
            if (!TestMode && File.Exists(preferencesPath)) {
                var p = JsonSerializer.Deserialize<Preferences>(File.ReadAllText(preferencesPath), JsonFiles.Options);
                if (p != null) {
                    if (Directory.Exists(p.Sdk)) SdkPath.Text = p.Sdk;
                    if (!string.IsNullOrWhiteSpace(p.Python)) PythonPath.Text = p.Python;
                    if (!string.IsNullOrWhiteSpace(p.Devkit)) DevkitPath.Text = p.Devkit;
                    if (!string.IsNullOrWhiteSpace(p.ProjectParent)) projectParent = p.ProjectParent;
                }
            }
        } catch (Exception ex) when (ex is IOException or JsonException or UnauthorizedAccessException) { StatusText.Text = "Saved paths could not be read; using detected paths."; }
        SuggestOutput();
        AssetOutputPath.Text = Path.Combine(projectParent, "Switch-assets-" + DateTime.Now.ToString("yyyyMMdd-HHmmss"));
        UpdateWorkspaceControls();
        Loaded += async (_, _) => {
            var args = Environment.GetCommandLineArgs();
            if (args.Length > 1 && !args[1].StartsWith("--", StringComparison.Ordinal)) {
                if (Path.GetFileName(args[1]).Equals("project.json", StringComparison.OrdinalIgnoreCase)) await OpenProjectAsync(args[1]);
                else { InputPath.Text = args[1]; await AnalyzeAsync(); }
            }
        };
    }
    static string FindSdk() {
        if (File.Exists(Path.Combine(AppContext.BaseDirectory, "SDK", "include", "script.h"))) return Path.Combine(AppContext.BaseDirectory, "SDK");
        for (DirectoryInfo? d = new(AppContext.BaseDirectory); d != null; d = d.Parent) if (File.Exists(Path.Combine(d.FullName, "sdk", "native-index.json"))) return d.FullName;
        return "";
    }
    static string FindPython() {
        var paths = (Environment.GetEnvironmentVariable("PATH") ?? "").Split(Path.PathSeparator);
        foreach (string p in paths) { try { string file = Path.Combine(p, "python.exe"); if (File.Exists(file) && !file.Contains("WindowsApps", StringComparison.OrdinalIgnoreCase)) return file; } catch (ArgumentException) { } }
        return "python";
    }
    static string FindDevkit() {
        string? env = Environment.GetEnvironmentVariable("DEVKITPRO"); if (!string.IsNullOrWhiteSpace(env) && File.Exists(Path.Combine(env, "devkitA64", "bin", "aarch64-none-elf-g++.exe"))) return env;
        for (DirectoryInfo? d = new(AppContext.BaseDirectory); d != null; d = d.Parent) if (Directory.Exists(Path.Combine(d.FullName, "devkitPro", "devkitA64"))) return Path.Combine(d.FullName, "devkitPro");
        return "C:\\devkitPro";
    }
    void SavePreferences() {
        if (TestMode) return;
        Directory.CreateDirectory(Path.GetDirectoryName(preferencesPath)!);
        JsonFiles.Write(preferencesPath, new Preferences { Sdk = SdkPath.Text.Trim(), Python = PythonPath.Text.Trim(), Devkit = DevkitPath.Text.Trim(), ProjectParent = projectParent });
    }
    void SuggestOutput() => OutputPath.Text = Path.Combine(projectParent, PortProject.SuggestId(PluginId.Text) + "-" + DateTime.Now.ToString("yyyyMMdd-HHmmss"));
    async Task RunBusy(string status, Func<CancellationToken, Task> work) {
        if (Busy) return;
        operation = new(); var token = operation.Token;
        Tabs.IsEnabled = false; BusyBar.Visibility = Visibility.Visible; CancelButton.Visibility = Visibility.Visible; StatusText.Text = status;
        try { await work(token); }
        catch (OperationCanceledException) { StatusText.Text = "Cancelled · Partial outputs are preserved"; AppendLog("Cancelled."); }
        catch (Exception ex) when (ex is not OutOfMemoryException) {
            StatusText.Text = "Action could not complete · " + ex.Message; AppendLog(ex.Message);
            if (!TestMode) MessageBox.Show(this, ex.Message, "ScriptHookV-NX Mod Porter", MessageBoxButton.OK, MessageBoxImage.Warning);
        } finally {
            operation.Dispose(); operation = null; Tabs.IsEnabled = true; BusyBar.Visibility = Visibility.Collapsed; CancelButton.Visibility = Visibility.Collapsed;
            UpdateWorkspaceControls();
        }
    }
    void UpdateWorkspaceControls() {
        BuildButton.IsEnabled = BuildNroButton.IsEnabled = CheckCppButton.IsEnabled = OpenProjectFolderButton.IsEnabled = projectFile != null;
        OpenBuildButton.IsEnabled = InstallNroButton.IsEnabled = buildDirectory != null;
        OpenAssetOutputButton.IsEnabled = assetDirectory != null;
        CreateButton.IsEnabled = input is { IsBinary: false } && choices.Any(c => c.Selected);
        ReviewNextButton.IsEnabled = input is { IsBinary: false } && choices.Any(c => c.Selected);
    }
    private void Navigation_SelectionChanged(object sender, SelectionChangedEventArgs e) {
        if (e.OriginalSource != Tabs || PageTitle == null) return;
        var pages = new[] {
            ("Import a mod", "Open source code, a ZIP, or a compiled plugin."),
            ("Findings", "Compatibility, source selection, and next steps."),
            ("Project & build", "Compile and package a Switch plugin."),
            ("Tools", "Build tools, native lookup, and Switch reports."),
            ("Game assets", "Convert resources or update a copy of your game archives.")
        };
        if (Tabs.SelectedIndex is >= 0 and < 5) (PageTitle.Text, PageSubtitle.Text) = pages[Tabs.SelectedIndex];
        QueueResponsiveLayout();
    }
    private void GameAction_Changed(object sender, RoutedEventArgs e) {
        if (ReplacementControls == null || DlcGameControls == null) return;
        bool replace = ReplaceAssetMode.IsChecked == true;
        ReplacementControls.Visibility = replace ? Visibility.Visible : Visibility.Collapsed;
        DlcGameControls.Visibility = replace ? Visibility.Collapsed : Visibility.Visible;
        if (DlcFooterActions != null) DlcFooterActions.Visibility = replace ? Visibility.Collapsed : Visibility.Visible;
        if (ReplacementFooterActions != null) ReplacementFooterActions.Visibility = replace ? Visibility.Visible : Visibility.Collapsed;
    }
    private void GoAssets_Click(object sender, RoutedEventArgs e) => Tabs.SelectedIndex = 4;
    private void NativeQuery_KeyDown(object sender, KeyEventArgs e) {
        if (e.Key == Key.Enter && !Busy) { e.Handled = true; SearchNative_Click(sender, new RoutedEventArgs()); }
    }
    private async void Window_PreviewKeyDown(object sender, KeyEventArgs e) {
        if (e.Key == Key.Escape && Busy) { e.Handled = true; Cancel_Click(sender, new RoutedEventArgs()); return; }
        if (Busy) return;
        if (Keyboard.Modifiers == (ModifierKeys.Control | ModifierKeys.Shift) && e.Key == Key.O) {
            e.Handled = true; OpenProject_Click(sender, new RoutedEventArgs());
        } else if (Keyboard.Modifiers == ModifierKeys.Control) {
            if (e.Key == Key.O) { e.Handled = true; ChooseInput_Click(sender, new RoutedEventArgs()); }
            else if (e.Key == Key.F && Tabs.SelectedIndex == 1) { e.Handled = true; FindingFilter.Focus(); FindingFilter.SelectAll(); }
            else if (e.Key == Key.Enter) {
                if (Tabs.SelectedIndex == 0) { e.Handled = true; await AnalyzeAsync(); }
                else if (Tabs.SelectedIndex == 2 && projectFile != null) { e.Handled = true; await BuildProjectAsync(false); }
                else if (Tabs.SelectedIndex == 4 && AssetTabs.SelectedIndex == 0) { e.Handled = true; AnalyzeAssets_Click(sender, new RoutedEventArgs()); }
            }
        }
    }
    void DisplayReport(AnalysisReport value) {
        report = value; ReviewInput.Text = input?.Name ?? (projectFile == null ? "Analysis" : Path.GetFileName(Path.GetDirectoryName(projectFile))); ReviewTitle.Text = value.Status;
        ReviewSummary.Text = $"{value.Blockers} required changes · {value.Reviews} review items · {value.Natives.Count} native references";
        if (value.Kind != "source") ReviewSummary.Text = string.Join(" · ", value.Details.Take(3).Select(p => p.Key + ": " + p.Value));
        FindingAction.Text = "Select a finding to view its next step.";
        FindingFilter.Text = ""; SeverityFilter.SelectedIndex = 0; ApplyFindingFilter();
        ReviewNextButton.IsEnabled = input is { IsBinary: false } && choices.Any(c => c.Selected);
        SourceColumn.Visibility = value.Kind == "source" ? Visibility.Visible : Visibility.Collapsed;
        BinaryColumn.Visibility = value.Kind == "source" ? Visibility.Collapsed : Visibility.Visible;
        BinaryDetails.Text = string.Join("\n\n", value.Details.Select(p => p.Key + "\n" + p.Value));
    }
    async Task AnalyzeAsync() {
        string path = InputPath.Text.Trim(), sdk = SdkPath.Text.Trim();
        await RunBusy("Analyzing mod…", async token => {
            var loaded = await Task.Run(() => InputLoader.Load(path, token), token);
            var analyzed = await Task.Run(() => loaded.IsBinary ? (Path.GetExtension(loaded.Path).Equals(".nro", StringComparison.OrdinalIgnoreCase) ? NroInspector.Report(loaded.Path) : AsiAnalyzer.Analyze(loaded.Path, token)) : SourceAnalyzer.Analyze(loaded, NativeCatalog.Load(sdk)), token);
            input = loaded; projectFile = null; buildDirectory = null; choices.Clear();
            foreach (var f in loaded.Files.Where(f => f.IsTranslationUnit)) choices.Add(new() { Path = f.RelativePath, Selected = true });
            PluginId.Text = analyzed.Details.GetValueOrDefault("Embedded plugin ID", PortProject.SuggestId(loaded.Name)); SuggestOutput(); ProjectLabel.Text = "No project open"; BuildSummary.Text = "Review findings, then create a copied project.";
            ProjectSetup.IsExpanded = true;
            DisplayReport(analyzed); Tabs.SelectedIndex = 1; StatusText.Text = "Analysis complete · " + analyzed.Status;
        });
    }
    private async void Analyze_Click(object sender, RoutedEventArgs e) => await AnalyzeAsync();
    private void InputPath_Changed(object sender, TextChangedEventArgs e) { if (AnalyzeButton != null) AnalyzeButton.IsEnabled = !string.IsNullOrWhiteSpace(InputPath.Text); }
    private void ChooseInput_Click(object sender, RoutedEventArgs e) {
        var dialog = new OpenFileDialog { Filter = "Mod inputs|*.zip;*.asi;*.dll;*.nro;*.cpp;*.cc;*.cxx;*.c|All files|*.*", Title = "Choose a mod or source archive" };
        if (dialog.ShowDialog(this) == true) InputPath.Text = dialog.FileName;
    }
    private void ChooseFolder_Click(object sender, RoutedEventArgs e) { string? p = PickFolder("Choose the mod source folder"); if (p != null) InputPath.Text = p; }
    string? PickFolder(string title) { var d = new OpenFolderDialog { Title = title, Multiselect = false }; return d.ShowDialog(this) == true ? d.FolderName : null; }
    private async void Example_Click(object sender, RoutedEventArgs e) {
        await LoadExampleAsync("pc-native-loop");
    }
    private async void RealExample_Click(object sender, RoutedEventArgs e) => await LoadExampleAsync("real-mod-footprints");
    private async Task LoadExampleAsync(string name) {
        var candidates = new[] { Path.Combine(AppContext.BaseDirectory, "examples", name), Path.Combine(SdkPath.Text, "desktop", "examples", name) };
        var found = candidates.FirstOrDefault(Directory.Exists);
        if (found == null) { MessageBox.Show(this, "The example folder is missing. Extract the complete app ZIP first."); return; }
        InputPath.Text = found; await AnalyzeAsync();
    }
    private async void Reanalyze_Click(object sender, RoutedEventArgs e) {
        if (input == null || input.IsBinary) return;
        var current = input; var selected = choices.Where(c => c.Selected).Select(c => c.Path).ToList(); string sdk = SdkPath.Text.Trim();
        await RunBusy("Analyzing selected source files…", async token => {
            var analyzed = await Task.Run(() => SourceAnalyzer.Analyze(current, NativeCatalog.Load(sdk), selected), token);
            DisplayReport(analyzed); StatusText.Text = "Selection analyzed · " + analyzed.Status;
        });
    }
    private void SourceSelection_Changed(object sender, RoutedEventArgs e) {
        if (Busy || report == null) return;
        // Checked is raised before the two-way source binding commits.
        Dispatcher.BeginInvoke(() => { UpdateWorkspaceControls(); StatusText.Text = $"{choices.Count(c => c.Selected)} source files selected · Analyze selection to refresh findings"; });
    }
    private void FindingFilter_TextChanged(object sender, TextChangedEventArgs e) => ApplyFindingFilter();
    private void SeverityFilter_Changed(object sender, SelectionChangedEventArgs e) => ApplyFindingFilter();
    void ApplyFindingFilter() {
        if (report == null || SeverityFilter == null || FindingsEmpty == null) return;
        string q = FindingFilter.Text;
        var results = report.Findings.Where(f => (SeverityFilter.SelectedIndex switch { 1 => f.Severity == Severity.Blocker, 2 => f.Severity == Severity.Review, 3 => f.Severity == Severity.Info, _ => true }) &&
            $"{f.Severity} {f.Code} {f.File} {f.Message} {f.Action}".Contains(q, StringComparison.OrdinalIgnoreCase)).ToList();
        FindingsList.ItemsSource = results;
        FindingsEmpty.Text = report.Findings.Count == 0 ? "No findings in this report." : "No findings match these filters.";
        FindingsEmpty.Visibility = results.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
        FindingAction.Text = "Select a finding to view its next step.";
    }
    private void Findings_SelectionChanged(object sender, SelectionChangedEventArgs e) { if (FindingsList.SelectedItem is Finding f) FindingAction.Text = f.Action; }
    private void ExportReport_Click(object sender, RoutedEventArgs e) {
        if (report == null) return;
        var d = new SaveFileDialog { Filter = "Markdown report|*.md|JSON report|*.json", FileName = "porting-report.md" };
        if (d.ShowDialog(this) != true) return;
        try { if (Path.GetExtension(d.FileName).Equals(".json", StringComparison.OrdinalIgnoreCase)) JsonFiles.Write(d.FileName, report); else File.WriteAllText(d.FileName, report.ToMarkdown(), new UTF8Encoding(false)); StatusText.Text = "Report saved · " + d.FileName; }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { MessageBox.Show(this, ex.Message); }
    }
    private void GoProject_Click(object sender, RoutedEventArgs e) { ProjectSetup.IsExpanded = projectFile == null; Tabs.SelectedIndex = 2; }
    private void GoTools_Click(object sender, RoutedEventArgs e) { ToolTabs.SelectedIndex = 0; Tabs.SelectedIndex = 3; }
    private void OutputBrowse_Click(object sender, RoutedEventArgs e) { var p = PickFolder("Choose a parent folder for the new project"); if (p != null) { projectParent = p; SuggestOutput(); } }
    private async void CreateProject_Click(object sender, RoutedEventArgs e) {
        if (input is not { IsBinary: false }) return;
        var current = input; var selected = choices.Where(c => c.Selected).Select(c => c.Path).ToList();
        var options = new PortOptions(PluginId.Text.Trim(), AdaptDll.IsChecked == true, AdaptSleep.IsChecked == true);
        string sdk = SdkPath.Text.Trim(), output = OutputPath.Text.Trim();
        await RunBusy("Creating copied project…", async token => {
            var port = await Task.Run(() => PortProject.Create(current, NativeCatalog.Load(sdk), sdk, output, options, selected, token), token);
            projectFile = port.ProjectFile; buildDirectory = null; ProjectLabel.Text = PluginId.Text.Trim() + " · " + projectFile; DisplayReport(port.Report); ProjectSetup.IsExpanded = false;
            SetBuildLog($"Project: {port.Directory}\nRecorded edits: {port.Changes.Count}\nRequired changes remaining: {port.Report.Blockers}\n\nEdit src files and inspect changes.json. Build preflight reads your current files again.\n");
            BuildSummary.Text = port.Report.Blockers > 0 ? "Changes required before build. Open the project folder and read PORTING-REPORT.md." : "Project ready for a build attempt. Configure the toolchain if needed.";
            SavePreferences(); StatusText.Text = "Project created · " + port.Report.Status;
        });
    }
    private async Task OpenProjectAsync(string path) {
        await RunBusy("Opening NX project…", token => {
            token.ThrowIfCancellationRequested(); var data = PortProject.Read(path); projectFile = Path.GetFullPath(path); buildDirectory = null;
            PluginId.Text = data.Id; ProjectLabel.Text = data.Id + " · " + projectFile; SetBuildLog("Opened " + projectFile + "\nCurrent sources will be checked before compilation.\n");
            BuildSummary.Text = "Edits to the project are picked up on each build."; input = null; choices.Clear(); ProjectSetup.IsExpanded = false; Tabs.SelectedIndex = 2;
            StatusText.Text = "Project opened · " + data.Id; return Task.CompletedTask;
        });
    }
    private async void OpenProject_Click(object sender, RoutedEventArgs e) { var d = new OpenFileDialog { Filter = "NX project|project.json|JSON files|*.json", Title = "Open an NX project" }; if (d.ShowDialog(this) == true) await OpenProjectAsync(d.FileName); }
    private async void Build_Click(object sender, RoutedEventArgs e) => await BuildProjectAsync(true);
    private async void BuildNro_Click(object sender, RoutedEventArgs e) => await BuildProjectAsync(false);
    private async Task BuildProjectAsync(bool installZip) {
        if (projectFile == null) return; string p = projectFile; var settings = Settings;
        await RunBusy("Building Switch NRO…", async token => {
            SavePreferences(); SetBuildLog(""); buildDirectory = null;
            var checks = await BuildService.DoctorAsync(settings, token);
            foreach (var c in checks) AppendLog($"{(c.Passed ? "OK" : "MISSING")} {c.Name}: {c.Detail}");
            if (checks.Any(c => !c.Passed)) throw new InvalidOperationException("Build tools are missing. Open Tools > Build tools and check the configured paths.");
            var built = await Task.Run(() => BuildService.BuildAsync(p, settings, AppendLog, token, createInstallZip: installZip), token);
            buildDirectory = built.Directory; BuildSummary.Text = $"Built · NRO {built.Nro.ImageBytes / 1024.0:N0} KiB · Loader memory {built.Nro.AccountedBytes / 1024.0:N0} KiB · Console test required";
            StatusText.Text = installZip ? "Build complete · Install ZIP is in the build output" : "Build complete · plugin.nro is in the build output";
        });
    }
    void AppendLog(string line) {
        if (!Dispatcher.CheckAccess()) { Dispatcher.BeginInvoke(() => AppendLog(line)); return; }
        if (fullBuildLog.Length > 500000) fullBuildLog.Remove(0, fullBuildLog.Length - 250000);
        fullBuildLog.AppendLine(line);
        string visible = DetailedLog.IsChecked == true ? line : ConciseLogLine(line);
        if (visible.Length > 0) { if (BuildLog.Text.Length > 500000) BuildLog.Text = BuildLog.Text[^250000..]; BuildLog.AppendText(visible + Environment.NewLine); BuildLog.ScrollToEnd(); }
    }
    static string ConciseLogLine(string line) {
        if (line.StartsWith('{')) {
            try { using var json = JsonDocument.Parse(line); if (json.RootElement.TryGetProperty("nro_sha256", out _)) return "ARM64 NRO created. Build metadata saved with the output."; }
            catch (JsonException) { }
        }
        if (line.StartsWith("OK ") && line.IndexOf(':') is int colon && colon > 0) return line[..colon];
        return line;
    }
    void SetBuildLog(string text) { fullBuildLog.Clear(); fullBuildLog.Append(text); RefreshBuildLog(); }
    void RefreshBuildLog() { BuildLog.Text = DetailedLog.IsChecked == true ? fullBuildLog.ToString() : string.Join(Environment.NewLine, fullBuildLog.ToString().Split('\n').Select(l => ConciseLogLine(l.TrimEnd('\r')))); BuildLog.ScrollToEnd(); }
    private void DetailedLog_Changed(object sender, RoutedEventArgs e) { if (BuildLog != null) RefreshBuildLog(); }
    private void CopyBuildLog_Click(object sender, RoutedEventArgs e) {
        try { Clipboard.SetText(fullBuildLog.Length > 0 ? fullBuildLog.ToString() : BuildLog.Text); StatusText.Text = "Full build log copied"; }
        catch (System.Runtime.InteropServices.COMException) { StatusText.Text = "Clipboard is busy. Select and copy text from the output instead."; }
    }
    void OpenFolder(string? folder) {
        if (folder == null || !Directory.Exists(folder)) { StatusText.Text = "Create or build a project first."; return; }
        try { Process.Start(new ProcessStartInfo("explorer.exe") { UseShellExecute = true, ArgumentList = { folder } }); }
        catch (Exception ex) when (ex is IOException or System.ComponentModel.Win32Exception) { MessageBox.Show(this, ex.Message); }
    }
    private void OpenProjectFolder_Click(object sender, RoutedEventArgs e) => OpenFolder(projectFile == null ? null : Path.GetDirectoryName(projectFile));
    private void OpenBuild_Click(object sender, RoutedEventArgs e) => OpenFolder(buildDirectory);
    private void SdkBrowse_Click(object sender, RoutedEventArgs e) { string? p = PickFolder("Choose the matched ScriptHookV-NX SDK"); if (p != null) SdkPath.Text = p; }
    private void DevkitBrowse_Click(object sender, RoutedEventArgs e) { string? p = PickFolder("Choose devkitPro root"); if (p != null) DevkitPath.Text = p; }
    private void PythonBrowse_Click(object sender, RoutedEventArgs e) { var d = new OpenFileDialog { Filter = "Python executable|python.exe|Executable|*.exe" }; if (d.ShowDialog(this) == true) PythonPath.Text = d.FileName; }
    private async void Doctor_Click(object sender, RoutedEventArgs e) {
        var settings = Settings;
        await RunBusy("Checking build tools…", async token => {
            SavePreferences(); var checks = await BuildService.DoctorAsync(settings, token);
            DoctorResult.Text = string.Join("\n\n", checks.Select(c => $"{(c.Passed ? "OK" : "MISSING")}  {c.Name}\n{c.Detail}"));
            ToolchainSummary.Text = checks.All(c => c.Passed) ? "Ready to build · All tools found" : $"{checks.Count(c => !c.Passed)} missing items · Expand Tool details";
            StatusText.Text = checks.All(c => c.Passed) ? "Toolchain ready" : "Toolchain needs attention · See missing items";
        });
    }
    private async void SearchNative_Click(object sender, RoutedEventArgs e) {
        ToolTabs.SelectedIndex = 1;
        string sdk = SdkPath.Text.Trim(), query = NativeQuery.Text.Trim();
        await RunBusy("Searching native index…", async token => {
            var matches = await Task.Run(() => NativeCatalog.Load(sdk).Search(query).ToList(), token);
            NativeResults.Text = matches.Count == 0 ? "No matching declarations." : string.Join("\n\n", matches.Select(n => n.Signature + "\n" + n.Hash + " · " + (n.Registered ? "target registration present" : "registration not found")));
            StatusText.Text = $"Native search · {matches.Count} results (maximum 100)";
        });
    }
    void Link(string uri) { try { Process.Start(new ProcessStartInfo(uri) { UseShellExecute = true }); } catch (System.ComponentModel.Win32Exception ex) { MessageBox.Show(this, ex.Message); } }
    private void PythonLink_Click(object sender, RoutedEventArgs e) => Link("https://www.python.org/downloads/windows/");
    private void DevkitLink_Click(object sender, RoutedEventArgs e) => Link("https://devkitpro.org/wiki/Getting_Started");
    private void Cancel_Click(object sender, RoutedEventArgs e) { operation?.Cancel(); StatusText.Text = "Cancelling…"; }
    private void Window_DragOver(object sender, DragEventArgs e) {
        e.Handled = true;
        e.Effects = !Busy && e.Data.GetData(DataFormats.FileDrop) is string[] { Length: 1 } ? DragDropEffects.Copy : DragDropEffects.None;
    }
    private async void Window_Drop(object sender, DragEventArgs e) {
        e.Handled = true;
        if (Busy || !e.Data.GetDataPresent(DataFormats.FileDrop)) return;
        if (e.Data.GetData(DataFormats.FileDrop) is not string[] paths || paths.Length != 1) { StatusText.Text = "Drop one mod or project at a time."; return; }
        if (Path.GetFileName(paths[0]).Equals("project.json", StringComparison.OrdinalIgnoreCase)) await OpenProjectAsync(paths[0]);
        else if (Tabs.SelectedIndex == 4 || Path.GetExtension(paths[0]).Equals(".rpf", StringComparison.OrdinalIgnoreCase) || ModPorter.Core.Assets.AssetService.ResourceExtensions.Contains(Path.GetExtension(paths[0]))) { AssetInputPath.Text = paths[0]; Tabs.SelectedIndex = 4; AssetTabs.SelectedIndex = 0; }
        else { InputPath.Text = paths[0]; await AnalyzeAsync(); }
    }
    private void Window_Closing(object? sender, CancelEventArgs e) {
        if (Busy) { operation?.Cancel(); e.Cancel = true; StatusText.Text = "Cancelling · Close again after the operation stops"; return; }
        try { SavePreferences(); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { /* Closing does not depend on preference storage. */ }
    }
}
