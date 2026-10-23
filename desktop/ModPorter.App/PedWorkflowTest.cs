using System.IO;
using System.Windows;
using ModPorter.Core;
using ModPorter.Core.Assets;

namespace ModPorter.App;
public partial class MainWindow {
    public async Task RunPedUiTestAsync(string output, string input, string template, string basePed, string model) {
        try {
            AssetInputPath.Text = input; AssetOutputPath.Text = Path.Combine(output, "ped-output");
            PedPackId.Text = "nx_" + model; PedModelName.Text = model; PedTemplatePath.Text = template;
            PedBaseName.ItemsSource = AddonBuilder.PedNames(template); PedBaseName.Text = basePed;
            BuildPedAddon_Click(this, new RoutedEventArgs());
            var until = DateTime.UtcNow.AddMinutes(2);
            while (Busy && DateTime.UtcNow < until) await Task.Delay(25);
            if (Busy) { operation?.Cancel(); throw new TimeoutException("PED UI operation did not finish."); }
            if (!File.Exists(PreparedAssetPath.Text)) throw new IOException("PED UI did not produce a DLC: " + StatusText.Text);
            var audit = AddonAudit.Inspect(File.ReadAllBytes(PreparedAssetPath.Text));
            if (!audit.ReadyForTest || !audit.Models.Contains(model) || DlcId.Text != "nx_" + model) throw new InvalidDataException("PED UI result did not retain the selected model/registration.");
            JsonFiles.Write(Path.Combine(output, "ped-ui-test.json"), new { passed = 1, target_metadata_dropdown_populated = PedBaseName.Items.Count > 0,
                selected_model = model, generated_dlc_sha256 = JsonFiles.HashFile(PreparedAssetPath.Text), source_files_modified = false, switch_runtime_verified = false });
        } finally { Close(); }
    }
}
