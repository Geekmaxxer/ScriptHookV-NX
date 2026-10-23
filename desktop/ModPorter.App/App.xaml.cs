using System.Windows;
using ModPorter.Core;
using System.IO;

namespace ModPorter.App;
public partial class App : Application {
    protected override async void OnStartup(StartupEventArgs e) {
        base.OnStartup(e);
        if (e.Args.Length > 1 && e.Args[0] is "--ui-test" or "--layout-test" or "--ped-ui-test") {
            ShutdownMode = ShutdownMode.OnExplicitShutdown;
            string output = Path.GetFullPath(e.Args[1]);
            try {
                if (Directory.Exists(output)) throw new IOException("UI test output already exists.");
                Directory.CreateDirectory(output);
                string Arg(string key, string fallback) { int i = Array.IndexOf(e.Args, key); return i >= 0 && i + 1 < e.Args.Length ? e.Args[i + 1] : fallback; }
                var window = new MainWindow();
                if (e.Args[0] == "--layout-test") await window.RunLayoutTestAsync(output);
                else if (e.Args[0] == "--ped-ui-test") await window.RunPedUiTestAsync(output, Arg("--input", ""), Arg("--template", ""), Arg("--base-ped", ""), Arg("--model", ""));
                else await window.RunUiTestAsync(output, Arg("--sdk", window.SdkPath.Text), Arg("--devkitpro", window.DevkitPath.Text), Arg("--python", window.PythonPath.Text), Arg("--asset-input", ""));
                Shutdown(0);
            } catch (Exception error) when (error is not OutOfMemoryException) {
                if (Directory.Exists(output)) JsonFiles.Write(Path.Combine(output, "ui-test-failure.json"), new { error = error.ToString() });
                Shutdown(1);
            }
            return;
        }
        MainWindow = new MainWindow(); MainWindow.Show();
    }
}
