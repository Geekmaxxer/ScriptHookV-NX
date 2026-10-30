using Microsoft.Win32;
using ModPorter.Core;
using System.IO;
using System.Windows;

namespace ModPorter.App;
public partial class MainWindow {
    string? checkedLayoutSd;
    private async void CheckGameLayout_Click(object sender, RoutedEventArgs e) {
        checkedLayoutSd = null; RecoverGameLayoutButton.IsEnabled = false;
        string? sd = PickFolder("Choose the mounted SD root to check GTA's DLC layout");
        if (sd == null) return;
        var settings = Settings;
        await RunBusy("Checking known DLC paths without changing files…", async cancel => {
            var layout = await GameLayoutService.RunAsync(settings.SdkPath, sd, cancel: cancel);
            checkedLayoutSd = sd; RecoverGameLayoutButton.IsEnabled = layout.RecoveryAllowed;
            DiagnosticReport.Text = layout.Text(); ToolTabs.SelectedIndex = 2;
            StatusText.Text = layout.RecoveryAllowed ? "Missing DLC paths found · Review recovery guidance" : "Game layout checked · Review the reported scope and findings";
        });
    }
    private async void RecoverGameLayout_Click(object sender, RoutedEventArgs e) {
        if (checkedLayoutSd == null) return;
        if (MessageBox.Show(this,
            "Use this recovery for the build-9b485eb8 loose-RomFS profile only. Close GTA first.\n\nCreate blank placeholders at the known optional DLC paths that are still missing? Existing archives, base packs, settings and saves will be preserved. Placeholders provide path coverage, not playable DLC content. The additions will be recorded on the SD card.",
            "Recover missing DLC paths", MessageBoxButton.OKCancel, MessageBoxImage.Information) != MessageBoxResult.OK) return;
        string sd = checkedLayoutSd; var settings = Settings;
        await RunBusy("Recovering missing DLC path coverage…", async cancel => {
            var layout = await GameLayoutService.RunAsync(settings.SdkPath, sd, recover: true, cancel: cancel);
            RecoverGameLayoutButton.IsEnabled = layout.RecoveryAllowed; DiagnosticReport.Text = layout.Text();
            StatusText.Text = "DLC paths checked · Review the result, safely eject and test startup";
        });
    }
    private async void CheckCpp_Click(object sender, RoutedEventArgs e) {
        if (projectFile == null) { StatusText.Text = "Create or open a project first."; return; }
        string project = projectFile; var settings = Settings;
        await RunBusy("Checking C++ types, signatures and active conditional code…", async cancel => {
            SetBuildLog("");
            string result = await Task.Run(() => CompilerCheckService.CheckAsync(project,settings,AppendLog,cancel),cancel);
            StatusText.Text = result.Contains("checks failed") ? "C++ check failed · Review compiler diagnostics" : "C++ checks passed · No NRO built";
        });
    }
    private async void InstallNro_Click(object sender, RoutedEventArgs e) {
        if (buildDirectory == null || projectFile == null) { StatusText.Text = "Build a plugin first."; return; }
        string? sd = PickFolder("With GTA closed, choose the SD root for installation (new plugins use Manual startup)");
        if (sd == null) return;
        string nro = Path.Combine(buildDirectory, "plugin.nro"), id = PortProject.Read(projectFile).Id;
        await RunBusy("Installing plugin with backup and hash verification…", async cancel => {
            cancel.ThrowIfCancellationRequested();
            string record = await Task.Run(() => DeploymentService.InstallNro(nro, id, sd), cancel);
            AppendLog("Installed. Backup/restore record: " + record);
            StatusText.Text = "Installed · Restart GTA, then load the plugin in its manager";
        });
    }
    private async void RestoreInstall_Click(object sender, RoutedEventArgs e) {
        var file = new OpenFileDialog { Title = "Choose installation.json from the SD install-backups folder", Filter = "Installation record|installation.json" };
        if (file.ShowDialog(this) != true) return;
        string? sd = PickFolder("With GTA closed, choose that SD root for restoration"); if (sd == null) return;
        await RunBusy("Restoring the verified previous plugin…", async cancel => {
            await Task.Run(() => DeploymentService.Restore(file.FileName, sd), cancel);
            StatusText.Text = "Previous installation restored";
        });
    }
    private async void ImportRuntimeReport_Click(object sender, RoutedEventArgs e) {
        var file = new OpenFileDialog { Title = "Choose an Atmosphere fatal or Switch runtime report", Filter = "Switch reports|*.txt;*.bin|Runtime text report|*.txt|Atmosphere AFE2 fatal report|*.bin|All files|*.*" };
        if (file.ShowDialog(this) != true) return;
        await RunBusy("Reading runtime diagnostics…", async cancel => {
            var report = await Task.Run(() => RuntimeReportService.Read(file.FileName), cancel);
            DiagnosticReport.Text = report.Title + "\n\n" + string.Join("\n", report.Findings); ToolTabs.SelectedIndex = 2;
            StatusText.Text = "Runtime report read · Recorded failures and counters shown";
        });
    }
}
