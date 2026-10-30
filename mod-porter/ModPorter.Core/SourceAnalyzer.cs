using System.Text.RegularExpressions;

namespace ModPorter.Core;

public sealed record EntryPoint(string File, int NameToken, int BodyStart, int BodyEnd);
public sealed record DllEntry(string File, int Start, int End, bool Standard, RegistrationPlan? Registration=null);
public static partial class SourceAnalyzer {
    public static bool IsDescriptorMacro(string name) => name is "SHV_NX_PLUGIN" or "SHV_NX_PLUGIN_WITH_STOP" or "SHV_NX_PLUGIN_WITH_INIT" or "SHV_NX_PLUGIN_WITH_LIFECYCLE";
    // These are namespace spelling aliases only. Every used function is checked against the selected target index.
    public static readonly Dictionary<string, string> NamespaceAliases = new(StringComparer.Ordinal) { ["GAMEPLAY"] = "MISC", ["UI"] = "HUD", ["CONTROLS"] = "PAD", ["TIME"] = "CLOCK", ["_SYSTEM"] = "SYSTEM" };
    // Exact legacy PC symbol aliases: alloc8or native DB names/old_names checked
    // against this target's registered declarations; raw PC hashes are never invoked.
    public static readonly Dictionary<string, string> NativeAliases = new(StringComparer.Ordinal) {
        ["HUD::_SET_TEXT_ENTRY_2"] = "HUD::BEGIN_TEXT_COMMAND_PRINT",
        ["HUD::_DRAW_SUBTITLE_TIMED"] = "HUD::END_TEXT_COMMAND_PRINT",
        ["HUD::_ADD_TEXT_COMPONENT_STRING"] = "HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME",
        ["MISC::_HAS_CHEAT_STRING_JUST_BEEN_ENTERED"] = "MISC::HAS_PC_CHEAT_WITH_HASH_BEEN_ACTIVATED",
        ["GRAPHICS::_0xBA3D194057C79A7B"] = "GRAPHICS::SET_PARTICLE_FX_FOOT_OVERRIDE_NAME"
    };
    public static readonly HashSet<string> SdkHeaders = new(StringComparer.OrdinalIgnoreCase) { "script.h", "main.h", "types.h", "natives.h", "nativecaller.h", "enums.h", "extensions.h", "plugin.h" };
    static readonly HashSet<string> PortableSystemHeaders = new(StringComparer.OrdinalIgnoreCase) { "stdint.h", "stddef.h", "stdbool.h", "limits.h", "float.h", "stdarg.h", "stddef", "cstdint", "cstddef", "type_traits", "utility" };
    [GeneratedRegex(@"(?m)^\s*#\s*include\s*([<""])([^>""\r\n]+)[>""]")]
    public static partial Regex Includes();
    public static bool IsVendorHeader(SourceFile f) {
        string name = Path.GetFileName(f.RelativePath);
        return SdkHeaders.Contains(name) && !f.IsTranslationUnit && (f.Text.Contains("dllimport") || f.Text.Contains("ScriptHookV.dll") ||
            name.Equals("natives.h", StringComparison.OrdinalIgnoreCase) && (f.Text.Contains("namespace PLAYER") || f.Text.Contains("nativeInit(")) ||
            name.Equals("types.h", StringComparison.OrdinalIgnoreCase) && f.Text.Contains("Vector3") && f.Text.Contains("typedef") ||
            name.Equals("nativeCaller.h", StringComparison.OrdinalIgnoreCase) && f.Text.Contains("nativeCall"));
    }
    public static string DiscoverEntry(IEnumerable<SourceFile> files) {
        var names = new HashSet<string>(StringComparer.Ordinal);var descriptors=new HashSet<string>(StringComparer.Ordinal);
        foreach (var file in files.Where(f => f.IsTranslationUnit)) {
            string code = CppLexer.CodeOnly(file.Text);
            foreach (Match m in Regex.Matches(code, @"\bscriptRegister\s*\(\s*[a-zA-Z_]\w*\s*,\s*([a-zA-Z_]\w*)\s*\)")) names.Add(m.Groups[1].Value);
            foreach (var t in CppLexer.Tokens(file.Text).Where(t => IsDescriptorMacro(t.Text))) {
                var m = Regex.Match(file.Text[t.Start..], "^SHV_NX_PLUGIN(?:_WITH_(?:STOP|INIT|LIFECYCLE))?\\s*\\(\\s*\"[A-Za-z0-9_-]+\"\\s*,\\s*([a-zA-Z_]\\w*)\\s*[,)]");
                if (m.Success) descriptors.Add(m.Groups[1].Value);
            }
        }
        return descriptors.Count==1?descriptors.Single():names.Contains("ScriptMain")?"ScriptMain":names.Order(StringComparer.Ordinal).FirstOrDefault()??"ScriptMain";
    }
    public static List<EntryPoint> Entries(SourceFile file, string entryName = "ScriptMain") {
        var t = CppLexer.Tokens(file.Text); var entries = new List<EntryPoint>();
        for (int i = 0; i + 2 < t.Count; i++) if (t[i].Text == entryName && t[i + 1].Text == "(") {
            int args = CppLexer.Match(t, i + 1, "(", ")");
            if (args > 0 && args + 1 < t.Count && t[args + 1].Text == "{") {
                int end = CppLexer.Match(t, args + 1, "{", "}");
                if (end >= 0) entries.Add(new(file.RelativePath, i, args + 1, end));
            }
        }
        return entries;
    }
    public static List<DllEntry> DllEntries(SourceFile file, string entryName = "ScriptMain") {
        var t = CppLexer.Tokens(file.Text); var result = new List<DllEntry>();
        for (int i = 0; i + 1 < t.Count; i++) if (t[i].Text == "DllMain" && t[i + 1].Text == "(") {
            int p = CppLexer.Match(t, i + 1, "(", ")");
            if (p < 0 || p + 1 >= t.Count || t[p + 1].Text != "{") continue;
            int end = CppLexer.Match(t, p + 1, "{", "}"); if (end < 0) continue;
            int start = i;
            while (start > 0 && t[start - 1].Text is not ";" and not "}" and not "{" && t[start - 1].Line >= t[i].Line - 2) start--;
            // Never include preprocessor directives in a function edit.
            while (start < i && t[start].Text == "#") { int line = t[start].Line; while (start < i && t[start].Line == line) start++; }
            var plan=DllRegistration.Parse(t,i+1,p,end,entryName);
            result.Add(new(file.RelativePath,t[start].Start,t[end].Start+1,plan!=null,plan));
        }
        return result;
    }
    static int Line(string text, int at) => 1 + text.AsSpan(0, at).Count('\n');
    public static SourceFile? ResolveInclude(SourceFile file, string include, IReadOnlyList<SourceFile> files) {
        string clean = include.Replace('\\', '/');
        string virtualRoot = Path.Combine(Path.GetTempPath(), "porter-virtual");
        string sibling = Path.GetRelativePath(virtualRoot, Path.GetFullPath(Path.Combine(virtualRoot, Path.GetDirectoryName(file.RelativePath) ?? "", clean))).Replace('\\', '/');
        return files.FirstOrDefault(f => f.RelativePath.Equals(sibling, StringComparison.OrdinalIgnoreCase)) ??
            files.FirstOrDefault(f => f.RelativePath.Equals(clean, StringComparison.OrdinalIgnoreCase)) ??
            files.FirstOrDefault(f => f.RelativePath.EndsWith("/" + clean, StringComparison.OrdinalIgnoreCase));
    }
    public static string? DescriptorId(IEnumerable<SourceFile> files) {
        foreach (var file in files.Where(f => !IsVendorHeader(f))) foreach (var t in CppLexer.Tokens(file.Text).Where(t => IsDescriptorMacro(t.Text))) {
            var m = Regex.Match(file.Text[t.Start..], "^SHV_NX_PLUGIN(?:_WITH_(?:STOP|INIT|LIFECYCLE))?\\s*\\(\\s*\"([A-Za-z0-9_-]{1,31})\"");
            if (m.Success) return m.Groups[1].Value;
        }
        return null;
    }
    public static AnalysisReport Analyze(ModInput input, NativeCatalog catalog, IEnumerable<string>? selected = null) {
        var selectedSet = selected?.ToHashSet(StringComparer.OrdinalIgnoreCase);
        var pending = new Queue<SourceFile>(input.Files.Where(f => f.IsTranslationUnit && (selectedSet == null || selectedSet.Contains(f.RelativePath))));
        var reachable = new Dictionary<string, SourceFile>(StringComparer.OrdinalIgnoreCase);
        while (pending.Count > 0) {
            var f = pending.Dequeue(); if (!reachable.TryAdd(f.RelativePath, f) || IsVendorHeader(f)) continue;
            string code = CppLexer.CodeOnly(f.Text);
            foreach (Match match in Includes().Matches(f.Text)) {
                int hash = f.Text.IndexOf('#', match.Index, match.Length);
                if (hash < 0 || code[hash] != '#') continue;
                string inc = match.Groups[2].Value;
                if (match.Groups[1].Value == "<" && (SdkHeaders.Contains(Path.GetFileName(inc)) || inc.StartsWith("shv/", StringComparison.OrdinalIgnoreCase))) continue;
                var resolved = ResolveInclude(f, inc, input.Files); if (resolved != null) pending.Enqueue(resolved);
            }
        }
        var files = reachable.Values.ToList();
        var report = new AnalysisReport { Input = input.Path, TargetBuild = catalog.TargetBuild };
        report.TranslationUnits = files.Where(f => f.IsTranslationUnit).Select(f => f.RelativePath).ToList();
        report.SourceHashes = files.ToDictionary(f => f.RelativePath, f => f.Sha256);
        string entryName = DiscoverEntry(files);
        var entries = files.Where(f => f.IsTranslationUnit).SelectMany(f => Entries(f, entryName)).ToList();
        void Add(Severity s, string code, string file, int line, string msg, string action) { if (report.Findings.Count < 2048) report.Findings.Add(new(s, code, file, line, msg, action)); }
        if (entries.Count == 0) Add(Severity.Blocker, "ENTRY_MISSING", "", 0, "No " + entryName + " definition found in the selected C++ files.", "Select the C++ entry file and its registration unit, or provide a ScriptMain adapter.");
        if (entries.Count > 1) Add(Severity.Blocker, "ENTRY_MULTIPLE", "", 0, "More than one " + entryName + " definition is selected.", "Select the translation units belonging to one plugin.");
        bool hasDescriptor = files.Where(f => !IsVendorHeader(f)).Any(f => CppLexer.Tokens(f.Text).Any(t => IsDescriptorMacro(t.Text) || t.Text == "shvPluginQuery"));
        if (hasDescriptor) Add(Severity.Info, "EXISTING_DESCRIPTOR", "", 0, "An NX plugin descriptor already exists.", "The existing descriptor is preserved; its embedded plugin ID controls saved-file namespacing.");
        foreach (var file in files) {
            if (IsVendorHeader(file)) { Add(Severity.Info, "PC_SDK_HEADER", file.RelativePath, 1, "Bundled PC SDK declarations are excluded from the generated build.", "The selected Switch SDK supplies native signatures and hashes instead."); continue; }
            if (!file.IsTranslationUnit && !new[] { ".h", ".hpp", ".hh", ".hxx", ".inc", ".inl", ".ipp", ".tpp" }.Contains(Path.GetExtension(file.RelativePath).ToLowerInvariant())) continue;
            var dlls = DllEntries(file, entryName); string code = CppLexer.CodeOnly(file.Text);
            foreach (var dll in dlls) {
                Add(dll.Standard ? Severity.Review : Severity.Blocker, dll.Standard ? "DLL_ENTRY" : "DLL_CUSTOM_INIT", file.RelativePath, Line(file.Text, dll.Start), dll.Standard ? "Windows DllMain registration must be replaced with an NX lifecycle descriptor." : "DllMain contains custom initialization or cleanup.", dll.Standard ? "Enable standard entry-point adaptation to preserve additional scripts and callbacks. Review callback bodies for Switch platform dependencies." : "Move required initialization into an explicit NX initializer and provide a Switch cleanup strategy before building.");
                code = code[..dll.Start] + new string(' ', dll.End - dll.Start) + code[dll.End..];
            }
            foreach (Match match in Includes().Matches(file.Text)) {
                // Ignore include-like text inside comments, strings and raw strings.
                int hashAt = file.Text.IndexOf('#', match.Index, match.Length);
                if (hashAt < 0 || code[hashAt] != '#') continue;
                string inc = match.Groups[2].Value.Replace('\\', '/'), name = inc.Split('/')[^1];
                if (name.Equals("windows.h", StringComparison.OrdinalIgnoreCase)) Add(Severity.Review, "WINDOWS_HEADER", file.RelativePath, Line(file.Text, match.Index), "Windows header dependency.", "The standard adapter removes this include; remaining Windows API uses are reported separately.");
                else if (SdkHeaders.Contains(name)) Add(Severity.Info, "SDK_INCLUDE", file.RelativePath, Line(file.Text, match.Index), "SDK include will use Switch declarations: " + inc, "PC enums are not imported; numeric native arguments must match the Switch target.");
                else if (!PortableSystemHeaders.Contains(inc) && !inc.StartsWith("shv/", StringComparison.OrdinalIgnoreCase)) {
                    bool local = ResolveInclude(file, inc, input.Files) != null;
                    if (!local) Add(Severity.Blocker, "DEPENDENCY", file.RelativePath, Line(file.Text, match.Index), "Unavailable or hosted-library include: " + inc, "Supply compatible local headers or replace the dependency. Arbitrary hosted C++/Windows libraries are not linked by this SDK.");
                }
            }
            var tokens = CppLexer.Tokens(code);
            var ids = tokens.Where(t => char.IsAsciiLetter(t.Text[0]) || t.Text[0] == '_').GroupBy(t => t.Text).ToDictionary(g => g.Key, g => g.First());
            foreach (var (id, tok) in ids) {
                string? reason = id switch {
                    "CreateThread" or "_beginthreadex" or "std::thread" or "CreateFiber" or "SwitchToFiber" or "ConvertThreadToFiber" => "Windows threads/fibers cannot be imported into the cooperative Switch runtime.",
                    "GetAsyncKeyState" or "GetKeyState" or "IsKeyJustUp" or "IsKeyDown" or "IsKeyJustDown" => "Windows keyboard polling needs a Switch input callback or controller binding.",
                    "VirtualProtect" or "VirtualAlloc" or "GetModuleHandle" or "GetModuleHandleA" or "GetProcAddress" or "LoadLibraryA" or "LoadLibrary" or "ReadProcessMemory" or "WriteProcessMemory" or "MH_CreateHook" or "MH_EnableHook" or "FindPattern" or "pattern" => "PC addresses, module lookup or memory hooks require a target-specific port.",
                    "D3D11CreateDevice" or "Direct3DCreate9" or "ImGui" or "IDXGISwapChain" or "ID3D11Device" => "PC rendering dependencies need a GTA HUD/graphics implementation.",
                    "fopen" or "ofstream" or "ifstream" or "filesystem" or "GetPrivateProfileIntA" or "GetPrivateProfileStringA" or "WritePrivateProfileStringA" or "CSimpleIniA" => "File/settings code needs the SDK's namespaced storage helpers.",
                    "vector" or "map" or "unordered_map" or "string" or "thread" or "mutex" or "malloc" or "free" or "new" or "delete" or "throw" or "try" => "Hosted library, heap allocation or exception dependency requires review for the freestanding SDK.",
                    "nativeInit" or "nativePush64" or "nativeCall" or "invoke" => "Raw native calls need verified Switch hashes and argument contracts.",
                    "getGameVersion" or "GetGameVersion" => "PC executable-version guards cannot use a fabricated PC build number on Switch.",
                    "BytePattern" or "HookManager" => "Executable signatures require a separate Switch implementation.",
                    "Sleep" => "Windows Sleep must be replaced by cooperative WAIT inside ScriptMain.",
                    _ => null
                };
                if (reason != null) Add(Severity.Blocker, "API_" + id.ToUpperInvariant(), file.RelativePath, tok.Line, reason, id == "Sleep" ? "Enable the cooperative Sleep adapter, then review its call context." : "Adapt this dependency in the copied project and analyze again.");
                if (id is "getGlobalPtr" or "getScriptHandleBaseAddress") Add(Severity.Review, "NX_MEMORY_LAYOUT", file.RelativePath, tok.Line, "The SDK exposes validated Switch pointers, but PC global indices and entity offsets are not portable.", "Verify every index and offset for the selected Switch build. Reacquire entity pointers after waits or deletion.");
                if (id is "keyboardHandlerRegister" or "keyboardHandlerUnregister") Add(Severity.Review, "NX_KEYBOARD", file.RelativePath, tok.Line, "USB keyboard callbacks are supported; scan codes use HID usages and no Windows message queue exists.", "Add a controller binding for handheld play and review virtual-key assumptions.");
                if (id is "presentCallbackRegister" or "renderCallbackRegister") Add(Severity.Review, "NX_RENDER_CONTEXT", file.RelativePath, tok.Line, "Callbacks run in the game script frame and receive an NX frame descriptor.", "Use GTA HUD natives or drawTexture. Do not cast the argument to a DirectX object or call WAIT from callbacks.");
                if (id == "createTexture") Add(Severity.Review, "NX_TEXTURE_ASSET", file.RelativePath, tok.Line, "Custom textures use PNG files from assets/<plugin-id>/.", "Supply PNG assets up to 1024x1024 and account for the shared plugin memory budget. PC filesystem paths are not converted.");
                if (id is "while" or "for" && !ids.ContainsKey("WAIT") && !ids.ContainsKey("scriptWait")) Add(Severity.Review, "YIELD_REVIEW", file.RelativePath, tok.Line, "This file contains loops without an obvious cooperative wait.", "Check reachable loops. A non-yielding script can stall the game; static analysis cannot prove yield safety.");
            }
            for (int i = 0; i + 3 < tokens.Count; i++) {
                var t = tokens[i];
                if (tokens[i + 1].Text != "::" || tokens[i + 3].Text != "(") continue;
                string ns = NamespaceAliases.GetValueOrDefault(t.Text, t.Text), sourceName = ns + "::" + tokens[i + 2].Text;
                string name = NativeAliases.GetValueOrDefault(sourceName, sourceName);
                if (!catalog.Namespaces.Contains(ns)) continue;
                if (!catalog.Entries.TryGetValue(name, out var native)) Add(Severity.Blocker, "NATIVE_UNKNOWN", file.RelativePath, t.Line, "No exact Switch wrapper for " + t.Text + "::" + tokens[i + 2].Text, "Choose a verified Switch equivalent and audit its signature. No speculative hash replacement is applied.");
                else {
                    if (name != sourceName) Add(Severity.Info, "LEGACY_NATIVE_ALIAS", file.RelativePath, t.Line, sourceName + " maps to " + name, "The selected Switch declaration supplies its own verified target hash and signature.");
                    if (report.Natives.Count < 8192) report.Natives.Add(new(native.Name, native.Hash, native.Signature, native.Registered, file.RelativePath, t.Line));
                    if (!native.Registered) Add(Severity.Blocker, "NATIVE_UNREGISTERED", file.RelativePath, t.Line, "Target registration not found for " + native.Name, "Isolate this call or choose a registered native; wrapper existence alone is insufficient.");
                    if (ns != t.Text) Add(Severity.Info, "NAMESPACE_ALIAS", file.RelativePath, t.Line, t.Text + " maps to " + ns + " for " + native.Name, "Only the namespace spelling is changed. The compiler checks the Switch signature.");
                    if (native.Name is "SYSTEM::WAIT" or "SCRIPT::TERMINATE_THIS_THREAD") Add(Severity.Blocker, "BORROWED_CONTEXT", file.RelativePath, t.Line, "Game script termination/wait cannot control an NRO's borrowed VM context.", "Use cooperative WAIT or return from ScriptMain.");
                    if (native.Name == "MISC::HAS_PC_CHEAT_WITH_HASH_BEEN_ACTIVATED") Add(Severity.Blocker, "PC_CHEAT_INPUT", file.RelativePath, t.Line, "PC cheat-string input is not a Switch controller binding.", "Replace this check with an explicit controller shortcut. Registration alone does not establish usable input on Switch.");
                }
            }
            if (tokens.Any(t => t.Text == "reinterpret_cast")) Add(Severity.Review, "POINTER_LAYOUT", file.RelativePath, 1, "Pointer casts may depend on PC engine layouts.", "Review every cast and structure against the target; no automatic pointer-layout conversion is attempted.");
        }
        report.Details["Source files"] = report.TranslationUnits.Count.ToString();
        report.Details["Input files omitted"] = input.SkippedFiles.Count.ToString();
        report.Details["Native references"] = report.Natives.Count.ToString();
        report.Details["Entry function"] = entryName;
        report.Details["Compatibility basis"] = "Exact Switch build, native/data contracts, dependencies and required assets; release date alone is not a compatibility rule.";
        string? descriptorId = DescriptorId(files);
        if (descriptorId != null) report.Details["Embedded plugin ID"] = descriptorId;
        report.Status = report.Blockers > 0 ? "Changes required" : "Ready for a build attempt";
        return report;
    }
}
