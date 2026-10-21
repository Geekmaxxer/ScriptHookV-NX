using System.Text.Json;

namespace ModPorter.Core;

public sealed record DeploymentRecord(string PluginId, string Destination, string InstalledSha256, string? Backup, string? BackupSha256, DateTimeOffset Created);
public static class DeploymentService {
    static void RegularPath(string path) {
        for (DirectoryInfo? p = new(Path.GetFullPath(path)); p != null; p = p.Parent)
            if (p.Exists && (p.Attributes & FileAttributes.ReparsePoint) != 0) throw new IOException("Installation paths cannot contain links/junctions.");
    }
    public static string InstallNro(string nro, string pluginId, string sdRoot, bool manual = true) {
        if (!PortProject.ValidId(pluginId)) throw new ArgumentException("Invalid plugin ID.");
        _ = NroInspector.Inspect(nro);
        sdRoot = Path.GetFullPath(sdRoot); RegularPath(sdRoot);
        if (!Directory.Exists(sdRoot)) throw new DirectoryNotFoundException(sdRoot);
        string directory = Path.Combine(sdRoot, "switch", "gtav-scripthook", "plugins"); RegularPath(directory);
        Directory.CreateDirectory(directory);
        string[] candidates = [Path.Combine(directory, pluginId + ".nro"), Path.Combine(directory, pluginId + ".nro.manual"), Path.Combine(directory, pluginId + ".nro.disabled")];
        var existing = Directory.EnumerateFiles(directory).Where(f => candidates.Any(c => c.Equals(f, StringComparison.OrdinalIgnoreCase))).ToList();
        if (existing.Count > 1) throw new IOException("Conflicting Auto/Manual/Disabled copies exist. Resolve them before installing.");
        // Preserve an existing startup policy. The selected default only affects a new install.
        string destination = existing.SingleOrDefault() ?? candidates[manual ? 1 : 0];
        if (File.Exists(destination) && (File.GetAttributes(destination) & FileAttributes.ReparsePoint) != 0) throw new IOException("Plugin destination is a link.");
        string history = Path.Combine(sdRoot, "switch", "gtav-scripthook", "install-backups", DateTime.UtcNow.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N")[..6]);
        RegularPath(history); Directory.CreateDirectory(history);
        string? backup = null, backupHash = null;
        if (File.Exists(destination)) { backup = Path.Combine(history, Path.GetFileName(destination)); File.Copy(destination, backup); backupHash = JsonFiles.HashFile(backup); }
        string temporary = Path.Combine(directory, ".porter-" + Guid.NewGuid().ToString("N") + ".tmp");
        string inputHash = JsonFiles.HashFile(nro);
        try {
            File.Copy(nro, temporary);
            if (JsonFiles.HashFile(temporary) != inputHash) throw new IOException("NRO copy verification failed.");
            var record = new DeploymentRecord(pluginId, Path.GetRelativePath(sdRoot, destination), inputHash,
                backup == null ? null : Path.GetRelativePath(sdRoot, backup), backupHash, DateTimeOffset.UtcNow);
            JsonFiles.Write(Path.Combine(history, "installation.json"), record);
            File.Move(temporary, destination, true);
        } finally { if (File.Exists(temporary)) File.Delete(temporary); }
        return Path.Combine(history, "installation.json");
    }
    public static void Restore(string installationRecord, string sdRoot) {
        sdRoot = Path.GetFullPath(sdRoot); RegularPath(sdRoot); RegularPath(Path.GetDirectoryName(installationRecord)!);
        if (new FileInfo(installationRecord).Length > 65536) throw new InvalidDataException("Installation record too large.");
        var r = JsonSerializer.Deserialize<DeploymentRecord>(File.ReadAllText(installationRecord), JsonFiles.Options) ?? throw new InvalidDataException("Invalid installation record.");
        if (!PortProject.ValidId(r.PluginId)) throw new InvalidDataException("Invalid plugin identity.");
        string Resolve(string relative) { InputLoader.SafeRelative(relative); string p = Path.GetFullPath(Path.Combine(sdRoot, relative)); if (!InputLoader.Inside(p, sdRoot)) throw new InvalidDataException("Record path escapes SD root."); RegularPath(Path.GetDirectoryName(p)!); if (File.Exists(p) && (File.GetAttributes(p) & FileAttributes.ReparsePoint) != 0) throw new IOException("Record target is a link."); return p; }
        string dest = Resolve(r.Destination);
        string expected = Path.Combine(sdRoot, "switch", "gtav-scripthook", "plugins", r.PluginId + ".nro");
        if (!new[] { expected, expected + ".manual", expected + ".disabled" }.Contains(dest, StringComparer.OrdinalIgnoreCase)) throw new InvalidDataException("Record is not a plugin destination.");
        if (!File.Exists(dest) || JsonFiles.HashFile(dest) != r.InstalledSha256) throw new IOException("Installed file changed after installation; preserve it before restoring.");
        if (r.Backup == null) { File.Delete(dest); return; }
        string backup = Resolve(r.Backup);
        string recordDirectory = Path.GetDirectoryName(Path.GetFullPath(installationRecord))!;
        if (!InputLoader.Inside(backup, recordDirectory) || !InputLoader.Inside(recordDirectory, Path.Combine(sdRoot, "switch", "gtav-scripthook", "install-backups")) || JsonFiles.HashFile(backup) != r.BackupSha256) throw new IOException("Backup path/hash validation failed.");
        string temp = dest + ".restore-" + Guid.NewGuid().ToString("N");
        try { File.Copy(backup, temp); if (JsonFiles.HashFile(temp) != r.BackupSha256) throw new IOException("Restore copy differs."); File.Move(temp, dest, true); }
        finally { if (File.Exists(temp)) File.Delete(temp); }
    }
}
