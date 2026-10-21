using System.IO.Compression;
using System.Text;

namespace ModPorter.Core;

public static class InputLoader {
    public static readonly HashSet<string> SourceExtensions = new(StringComparer.OrdinalIgnoreCase) { ".cpp", ".cc", ".cxx", ".c" };
    public static readonly HashSet<string> TextExtensions = new(StringComparer.OrdinalIgnoreCase) {
        ".cpp", ".cc", ".cxx", ".c", ".h", ".hh", ".hpp", ".hxx", ".inl", ".inc", ".ipp", ".tpp", ".json", ".ini", ".md", ".txt", ".vcxproj", ".props"
    };
    static readonly HashSet<string> IgnoredDirectories = new(StringComparer.OrdinalIgnoreCase) { ".git", ".vs", ".idea", "bin", "obj", "node_modules", "__pycache__", "build", "out", "releases" };
    const int MaxEntries = 8192, MaxFiles = 2048, MaxFileBytes = 8 * 1024 * 1024;
    const long MaxTotalBytes = 64 * 1024 * 1024;
    public static bool IsLegalNotice(string path) {
        string stem = Path.GetFileNameWithoutExtension(path);
        return new[] { "LICENSE", "LICENCE", "COPYING", "NOTICE", "AUTHORS" }.Any(s => stem.Equals(s, StringComparison.OrdinalIgnoreCase) || stem.StartsWith(s + "-", StringComparison.OrdinalIgnoreCase));
    }
    public static bool Inside(string child, string parent) {
        var a = Path.GetFullPath(child).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        var b = Path.GetFullPath(parent).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        return a.Equals(b, StringComparison.OrdinalIgnoreCase) || a.StartsWith(b + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    }
    public static string SafeRelative(string path) {
        var parts = path.Replace('\\', '/').Split('/');
        if (parts.Length == 0 || parts.Any(p => p is "" or "." or ".." || p.EndsWith('.') || p.EndsWith(' ') || p.Length > 180 ||
            p.Any(c => c < 32 || "<>:\"|?*".Contains(c)) || IsDevice(p))) throw new InvalidDataException("Unsafe file path: " + path);
        return string.Join('/', parts);
    }
    static bool IsDevice(string p) {
        var stem = p.Split('.')[0].ToUpperInvariant();
        return stem is "CON" or "PRN" or "AUX" or "NUL" || (stem.Length == 4 && (stem.StartsWith("COM") || stem.StartsWith("LPT")) && stem[3] is >= '0' and <= '9');
    }
    public static ModInput Load(string path, CancellationToken cancel = default) {
        cancel.ThrowIfCancellationRequested();
        path = Path.GetFullPath(path);
        if (File.Exists(path) && Path.GetExtension(path).ToLowerInvariant() is ".asi" or ".dll" or ".nro")
            return new ModInput { Path = path, Name = Path.GetFileNameWithoutExtension(path), IsBinary = true };
        var files = new List<SourceFile>(); var skipped = new List<string>(); var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        long total = 0; int entries = 0;
        void Add(string relative, long length, Func<Stream> open) {
            cancel.ThrowIfCancellationRequested();
            relative = SafeRelative(relative);
            if (!seen.Add(relative)) throw new InvalidDataException("Duplicate or case-colliding path: " + relative);
            if (relative.Split('/').Any(IgnoredDirectories.Contains) || !TextExtensions.Contains(Path.GetExtension(relative)) && !IsLegalNotice(relative)) { if (skipped.Count < 256) skipped.Add(relative); return; }
            if (files.Count >= MaxFiles || length < 0 || length > MaxFileBytes || (total += length) > MaxTotalBytes) throw new InvalidDataException("Source input exceeds the file/size limits (2,048 files, 8 MiB each, 64 MiB total).");
            using var input = open(); using var buffer = new MemoryStream();
            var chunk = new byte[65536]; int count;
            while ((count = input.Read(chunk, 0, chunk.Length)) != 0) {
                cancel.ThrowIfCancellationRequested();
                if (buffer.Length + count > length || buffer.Length + count > MaxFileBytes) throw new InvalidDataException("File expanded beyond its declared size: " + relative);
                buffer.Write(chunk, 0, count);
            }
            if (buffer.Length != length) throw new InvalidDataException("Source changed while reading: " + relative);
            var data = buffer.ToArray();
            string text;
            if (data.Length >= 2 && (data[0] == 0xff && data[1] == 0xfe || data[0] == 0xfe && data[1] == 0xff)) {
                using var reader = new StreamReader(new MemoryStream(data), Encoding.UTF8, true); text = reader.ReadToEnd();
            } else {
                try { text = new UTF8Encoding(false, true).GetString(data).TrimStart('\ufeff'); }
                catch (DecoderFallbackException) { throw new InvalidDataException("Source is not UTF-8/UTF-16; save it as UTF-8: " + relative); }
            }
            if (text.Contains('\0')) throw new InvalidDataException("Binary data in a source file: " + relative);
            files.Add(new(relative, text, JsonFiles.Hash(data)) { OriginalBytes = data });
        }
        if (Directory.Exists(path)) {
            if ((File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Choose a regular directory, not a symbolic link.");
            void Walk(string dir) {
                foreach (var item in Directory.EnumerateFileSystemEntries(dir).Order(StringComparer.OrdinalIgnoreCase)) {
                    cancel.ThrowIfCancellationRequested();
                    if (++entries > MaxEntries) throw new InvalidDataException("Too many directory entries; choose the mod's source folder.");
                    var attr = File.GetAttributes(item);
                    if ((attr & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Symbolic links are unsupported: " + item);
                    if ((attr & FileAttributes.Directory) != 0) { if (!IgnoredDirectories.Contains(Path.GetFileName(item))) Walk(item); }
                    else Add(Path.GetRelativePath(path, item), new FileInfo(item).Length, () => File.OpenRead(item));
                }
            }
            Walk(path);
        } else if (File.Exists(path) && Path.GetExtension(path).Equals(".zip", StringComparison.OrdinalIgnoreCase)) {
            using var zip = ZipFile.OpenRead(path);
            if (zip.Entries.Count > MaxEntries) throw new InvalidDataException("ZIP contains too many entries.");
            foreach (var entry in zip.Entries) {
                cancel.ThrowIfCancellationRequested();
                if (((entry.ExternalAttributes >> 16) & 0xf000) == 0xa000 || (entry.ExternalAttributes & 0x400) != 0) throw new InvalidDataException("ZIP symbolic links are unsupported.");
                if (entry.FullName.EndsWith('/') || entry.FullName.EndsWith('\\')) { SafeRelative(entry.FullName.TrimEnd('/', '\\')); continue; }
                Add(entry.FullName, entry.Length, entry.Open);
            }
        } else if (File.Exists(path) && SourceExtensions.Contains(Path.GetExtension(path))) {
            // A single file imports its parent so sibling headers remain available.
            return Load(Path.GetDirectoryName(path)!, cancel);
        } else throw new FileNotFoundException("Choose a source folder, source ZIP, C++ file, ASI, DLL or NRO.", path);
        if (!files.Any(f => f.IsTranslationUnit)) throw new InvalidDataException("No C/C++ source files were found. An ASI binary alone cannot be rebuilt as an NRO.");
        return new ModInput { Path = path, Name = Directory.Exists(path) ? new DirectoryInfo(path).Name : Path.GetFileNameWithoutExtension(path), Files = files.OrderBy(f => f.RelativePath, StringComparer.OrdinalIgnoreCase).ToList(), SkippedFiles = skipped };
    }
}
