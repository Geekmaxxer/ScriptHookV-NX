using Microsoft.Win32;
using ModPorter.Core;
using System.IO;
using System.Windows;

namespace ModPorter.App;
public partial class MainWindow {
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
        var file = new OpenFileDialog { Title = "Choose a Switch loader, runtime or mod status report", Filter = "Runtime reports|*.txt|All files|*.*" };
        if (file.ShowDialog(this) != true) return;
        await RunBusy("Reading runtime diagnostics…", async cancel => {
            var report = await Task.Run(() => RuntimeReportService.Read(file.FileName), cancel);
            DiagnosticReport.Text = report.Title + "\n\n" + string.Join("\n", report.Findings); ToolTabs.SelectedIndex = 2;
            StatusText.Text = "Runtime report read · Recorded failures and counters shown";
        });
    }
}
