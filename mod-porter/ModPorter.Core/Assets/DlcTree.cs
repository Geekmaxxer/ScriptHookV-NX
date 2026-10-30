using System.Text;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

internal static class DlcTree {
    internal static AssetNode Dir(string name, params AssetNode[] children) { var n = new AssetNode { Name = name, Directory = true }; n.Children.AddRange(children); return n; }
    internal static AssetNode File(string name, byte[] data) => new() { Name = name, Data = data, Resource = data.AsSpan().StartsWith("RSC7"u8) };
    internal static AssetNode Clone(AssetNode node) {
        var copy = new AssetNode { Name = node.Name, Directory = node.Directory, Resource = node.Resource, Data = node.Data, SystemFlags = node.SystemFlags, GraphicsFlags = node.GraphicsFlags };
        copy.Children.AddRange(node.Children.Select(Clone)); return copy;
    }
    internal static byte[] Xml(XDocument doc) {
        using var s = new MemoryStream();
        using (var w = System.Xml.XmlWriter.Create(s, new() { Encoding = new UTF8Encoding(false), Indent = true })) doc.Save(w);
        return s.ToArray();
    }
    internal static Dictionary<string,AssetNode> TopFiles(AssetNode root){
        var result=new Dictionary<string,AssetNode>(StringComparer.OrdinalIgnoreCase);
        void Walk(AssetNode node,string prefix,int depth){if(depth>32)throw new InvalidDataException("DLC directory nesting exceeds 32.");foreach(var n in node.Children){string path=prefix+n.Name;if(n.Directory)Walk(n,path+"/",depth+1);else if(!result.TryAdd(path,n)||result.Count>16384)throw new InvalidDataException("Ambiguous or oversized DLC directory.");}}
        Walk(root,"",0);return result;
    }
    internal static string Value(XElement? item, string name) => item?.Elements().FirstOrDefault(n => n.Name.LocalName == name)?.Value.Trim() ?? "";
    internal static Dictionary<string, AssetNode> Files(AssetNode root, CancellationToken cancel = default) {
        var result = new Dictionary<string, AssetNode>(StringComparer.OrdinalIgnoreCase); long bytes = 0;
        void Walk(AssetNode dir, string prefix, int depth) {
            if (depth > 4) throw new InvalidDataException("DLC nesting exceeds four archive levels.");
            foreach (var n in dir.Children) {
                cancel.ThrowIfCancellationRequested(); string p = prefix + n.Name;
                if (n.Directory) { Walk(n, p + "/", depth); continue; }
                if (!result.TryAdd(p, n) || result.Count > 16384 || (bytes += n.Data.Length) > 1024L * 1024 * 1024) throw new InvalidDataException("DLC exceeds editor limits or has duplicate paths.");
                if (Path.GetExtension(n.Name).Equals(".rpf", StringComparison.OrdinalIgnoreCase)) Walk(RpfArchive.Read(n.Data, cancel), p + "::", depth + 1);
            }
        }
        Walk(root, "", 0); return result;
    }
    static void Change(AssetNode root, string path, Func<AssetNode, AssetNode?> edit) {
        string[] nested = path.Split("::", StringSplitOptions.None);
        if (nested.Length > 5) throw new InvalidDataException("Archive path is too deeply nested.");
        string first = InputLoader.SafeRelative(nested[0]); string[] parts = first.Split('/'); var dir = root;
        foreach (string segment in parts[..^1]) dir = dir.Children.SingleOrDefault(c => c.Directory && c.Name.Equals(segment, StringComparison.OrdinalIgnoreCase)) ?? throw new InvalidDataException("Directory not found: " + first);
        int index = dir.Children.FindIndex(n => !n.Directory && n.Name.Equals(parts[^1], StringComparison.OrdinalIgnoreCase));
        if (index < 0) throw new InvalidDataException("Asset not found: " + path);
        var original = dir.Children[index];
        if (nested.Length > 1) {
            if (!Path.GetExtension(original.Name).Equals(".rpf", StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Path does not name a nested RPF.");
            var archive = RpfArchive.Read(original.Data); Change(archive, string.Join("::", nested[1..]), edit);
            dir.Children[index] = File(original.Name, RpfArchive.Write(archive));
        } else {
            var next = edit(original);
            if (next == null) dir.Children.RemoveAt(index);
            else {
                if (dir.Children.Where((_, i) => i != index).Any(n => n.Name.Equals(next.Name, StringComparison.OrdinalIgnoreCase))) throw new InvalidDataException("Asset name already exists: " + next.Name);
                dir.Children[index] = next;
            }
        }
    }
    internal static void Replace(AssetNode root, string path, byte[] data) => Change(root, path, n => File(n.Name, data));
    internal static void Remove(AssetNode root, string path) => Change(root, path, _ => null);
    internal static void Rename(AssetNode root, string path, string name) {
        InputLoader.SafeRelative(name);
        if (name.Contains('/') || name.Contains('\\')) throw new ArgumentException("Use a filename, without a directory.");
        Change(root, path, n => File(name, n.Data));
    }
    internal static void Add(AssetNode root, string path, byte[] data) {
        path = InputLoader.SafeRelative(path); var parts = path.Split('/'); var dir = root;
        foreach (string segment in parts[..^1]) {
            var child = dir.Children.FirstOrDefault(n => n.Name.Equals(segment, StringComparison.OrdinalIgnoreCase));
            if (child == null) { child = Dir(segment); dir.Children.Add(child); }
            if (!child.Directory) throw new InvalidDataException("File blocks asset directory: " + segment);
            dir = child;
        }
        if (dir.Children.Any(n => n.Name.Equals(parts[^1], StringComparison.OrdinalIgnoreCase))) throw new InvalidDataException("Asset path already exists: " + path);
        dir.Children.Add(File(parts[^1], data));
    }
}
