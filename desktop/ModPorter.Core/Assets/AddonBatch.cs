using System.Text;

namespace ModPorter.Core.Assets;

public sealed record PreparedAddon(string Id, string Archive);
public static class AddonBatch {
    public static IReadOnlyList<PreparedAddon> Discover(string folder) {
        folder = Path.GetFullPath(folder);
        if ((File.GetAttributes(folder) & FileAttributes.ReparsePoint) != 0) throw new IOException("Choose an ordinary add-on folder.");
        var result = new List<PreparedAddon>(); int scanned = 0;
        foreach (string directory in Directory.EnumerateDirectories(folder).Order(StringComparer.OrdinalIgnoreCase)) {
            if (++scanned > 256) throw new InvalidDataException("Choose a smaller DLC collection folder.");
            if ((File.GetAttributes(directory) & FileAttributes.ReparsePoint) != 0) throw new IOException("Linked DLC folders are unsupported.");
            string file = Path.Combine(directory, "dlc.rpf"); if (File.Exists(file)) result.Add(new(Path.GetFileName(directory), file));
        }
        if (result.Count is < 1 or > 32) throw new InvalidDataException("Choose a folder containing 1-32 pack-id/dlc.rpf subfolders.");
        return result;
    }
    // Validate every pack before touching output, merge one current DLC list and
    // copy the current update.rpf once. Never write to the selected installation.
    public static GameAssetUpdate Prepare(IReadOnlyList<PreparedAddon> addons, string selectedGame, string output,
        CancellationToken cancel = default, Action<string>? progress = null) {
        if (addons.Count is < 1 or > 32) throw new ArgumentException("Select 1-32 prepared DLC packs.");
        output = Path.GetFullPath(output);
        if (Directory.Exists(output) || File.Exists(output)) throw new IOException("Choose a new output folder.");
        var ids = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var models = new Dictionary<uint, string>();
        var modkits = new Dictionary<uint, string>();
        var sources = new List<(PreparedAddon Addon, FileStream Stream, string Hash)>();
        try {
            foreach (var item in addons) {
                cancel.ThrowIfCancellationRequested();
                if (!PortProject.ValidId(item.Id) || item.Id != item.Id.ToLowerInvariant() || !ids.Add(item.Id)) throw new ArgumentException("DLC folder IDs must be unique lowercase identifiers.");
                string path = Path.GetFullPath(item.Archive);
                if (Directory.Exists(path) || (File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0) throw new IOException("Choose ordinary prepared DLC files.");
                var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
                sources.Add((new(item.Id, path), stream, ""));
                if (stream.Length > ResourceContainer.MaxBytes) throw new InvalidDataException("A prepared DLC exceeds 256 MiB.");
                byte[] data = new byte[stream.Length]; stream.ReadExactly(data);
                var audit = AddonAudit.Inspect(data, cancel); if (!audit.ReadyForTest) throw new InvalidDataException(item.Id + ": " + audit.Text());
                foreach (string model in audit.Models) {
                    uint hash = AddonAudit.ModelHash(model);
                    if (!models.TryAdd(hash, item.Id + "/" + model)) throw new InvalidDataException("Model collision between " + models[hash] + " and " + item.Id + "/" + model + ". Select one variant or rename it correctly in the source pack.");
                }
                foreach (var kit in audit.ModKits) if (!modkits.TryAdd(kit.Id, item.Id + "/" + kit.Name)) throw new InvalidDataException("Mod-kit ID collision between " + modkits[kit.Id] + " and " + item.Id + "/" + kit.Name + ".");
                var analysis = AssetService.Analyze(path, new(0), cancel); if (analysis.ConvertedNames != 0) throw new InvalidDataException(item.Id + ": use Prepare assets before adding this pack.");
                sources[^1] = (new(item.Id, path), stream, JsonFiles.Hash(data));
            }
            string current = GameAssetUpdater.ResolveUpdate(selectedGame), entry = "common/data/dlclist.xml";
            byte[] original = RpfFileEditor.ReadBinary(current, entry, cancel: cancel), merged = original;
            foreach (var item in sources) merged = DlcPackageService.MergeList(merged, item.Addon.Id);
            cancel.ThrowIfCancellationRequested();
            string romfs = Path.Combine(output, "atmosphere/contents", GameAssetUpdater.TitleId, "romfs");
            string updated = Path.Combine(romfs, "update/update.rpf");
            var edit = RpfFileEditor.WriteCopy(current, updated, new Dictionary<string, byte[]> { { entry, merged } }, cancel, progress,
                expected: new Dictionary<string, byte[]> { { entry, original } });
            foreach (var item in sources) {
                progress?.Invoke("Copying add-on " + item.Addon.Id); cancel.ThrowIfCancellationRequested();
                string target = Path.Combine(romfs, "update/switch/dlcpacks", item.Addon.Id, "dlc.rpf"); Directory.CreateDirectory(Path.GetDirectoryName(target)!);
                using (var destination = new FileStream(target, FileMode.CreateNew, FileAccess.Write, FileShare.None)) {
                    item.Stream.Position = 0; byte[] buffer = new byte[128 * 1024]; int n;
                    while ((n = item.Stream.Read(buffer)) > 0) { cancel.ThrowIfCancellationRequested(); destination.Write(buffer, 0, n); }
                    destination.Flush(true);
                }
                if (RpfFileEditor.HashFile(target, cancel) != item.Hash) throw new IOException("DLC source changed or copy verification failed: " + item.Addon.Id);
            }
            string manifest = Path.Combine(output, "game-update.json");
            JsonFiles.Write(manifest, new { format = 1, operation = "add_dlc_batch", source_archive = current, source_archive_sha256 = edit.SourceSha256,
                updated_archive = Path.GetRelativePath(output, updated), updated_archive_sha256 = edit.OutputSha256,
                addons = sources.Select(s => new { id = s.Addon.Id, dlc_sha256 = s.Hash }), source_files_modified = false, complete = true, switch_runtime_verified = false });
            File.WriteAllText(Path.Combine(output, "READ-ME-FIRST.md"), "# Prepared add-ons\n\nClose GTA. Back up update.rpf and the affected DLC folders, then copy atmosphere to the SD root. These files were prepared together from your selected installation. Keep unrelated mods and settings.\n\nTest models: " + string.Join(", ", addons.Select(a => a.Id)) + ". Check each pack's model manifest for spawn names. Restore the original archive and DLC folders to undo installation. Do not install a partial output without a complete game-update.json.\n", new UTF8Encoding(false));
            return new(output, updated, manifest, edit);
        } finally { foreach (var item in sources) item.Stream.Dispose(); }
    }
}
