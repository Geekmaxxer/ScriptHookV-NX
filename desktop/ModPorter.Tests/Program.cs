using System.Buffers.Binary;
using System.IO.Compression;
using System.Text;
using ModPorter.Core;

int passed = 0, failed = 0;
string scratch = Path.Combine(Path.GetTempPath(), "shvnx-porter-tests-" + Guid.NewGuid().ToString("N")); Directory.CreateDirectory(scratch);
string sdk = Path.GetFullPath(args.Length > 0 ? args[0] : Path.Combine(AppContext.BaseDirectory, "../../../../.."));
var catalog = NativeCatalog.Load(sdk);
string compilerName = OperatingSystem.IsWindows() ? "aarch64-none-elf-g++.exe" : "aarch64-none-elf-g++";
string devkit = new[] { Environment.GetEnvironmentVariable("DEVKITPRO"), Path.GetFullPath(Path.Combine(sdk, "../devkitPro")), "C:/devkitPro", "/opt/devkitpro" }
    .FirstOrDefault(p => p != null && File.Exists(Path.Combine(p, "devkitA64", "bin", compilerName))) ?? "C:/devkitPro";
void Check(string name, Action test) { try { test(); passed++; Console.WriteLine("PASS " + name); } catch (Exception ex) { failed++; Console.WriteLine("FAIL " + name + ": " + ex.Message); } }
void Must(bool condition, string detail = "Assertion failed") { if (!condition) throw new Exception(detail); }
void Reject(Action action) { try { action(); } catch (Exception ex) when (ex is InvalidDataException or IOException or ArgumentException or InvalidOperationException) { return; } throw new Exception("Expected rejection"); }
string NewDir() { string p = Path.Combine(scratch, Guid.NewGuid().ToString("N")); Directory.CreateDirectory(p); return p; }
string Source(string code, string name = "main.cpp") { string p = NewDir(); File.WriteAllText(Path.Combine(p, name), code); return p; }
string Zip(params (string Name, string Text)[] items) { string p = Path.Combine(scratch, Guid.NewGuid().ToString("N") + ".zip"); using var z = ZipFile.Open(p, ZipArchiveMode.Create); foreach (var item in items) { using var w = new StreamWriter(z.CreateEntry(item.Name).Open()); w.Write(item.Text); } return p; }
AnalysisReport Analyze(string code) => SourceAnalyzer.Analyze(InputLoader.Load(Source(code)), catalog);
string Entry(string extra = "") => "#include \"script.h\"\nvoid ScriptMain() { " + extra + " WAIT(0); }\n";
byte[] Pe() {
    var b = new byte[0x600];
    void U16(int p, ushort v) => BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(p), v);
    void U32(int p, uint v) => BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(p), v);
    U16(0, 0x5a4d); U32(0x3c, 0x80); U32(0x80, 0x4550); U16(0x84, 0x8664); U16(0x86, 1); U16(0x94, 0xf0); U16(0x96, 0x2000);
    U16(0x98, 0x20b); U32(0x98 + 60, 0x200); U32(0x98 + 108, 16); U32(0x98 + 120, 0x1000); U32(0x98 + 124, 40);
    int s = 0x188; U32(s + 8, 0x400); U32(s + 12, 0x1000); U32(s + 16, 0x400); U32(s + 20, 0x200);
    U32(0x200, 0x1060); U32(0x20c, 0x1040); U32(0x210, 0x1060);
    Encoding.ASCII.GetBytes("ScriptHookV.dll\0").CopyTo(b, 0x240); U32(0x260, 0x1080);
    Encoding.ASCII.GetBytes("scriptRegister\0").CopyTo(b, 0x282);
    return b;
}
try {
    AssetTests.Run(scratch, Check);
    RefinementTests.Assets(scratch, Check);
    GameArchiveTests.Run(scratch, Check);
    Check("Matched native index", () => Must(catalog.Entries.Count == 8540 && catalog.Entries["PLAYER::PLAYER_PED_ID"].Registered));
    Check("Native search by hash", () => Must(catalog.Search(catalog.Entries["PLAYER::PLAYER_PED_ID"].Hash).Single().Name == "PLAYER::PLAYER_PED_ID"));
    Check("Reject stale native index", () => { string p = NewDir(); Directory.CreateDirectory(Path.Combine(p, "include")); Directory.CreateDirectory(Path.Combine(p, "sdk")); File.Copy(Path.Combine(sdk, "include/natives.h"), Path.Combine(p, "include/natives.h")); File.WriteAllText(Path.Combine(p, "sdk/native-index.json"), "{\"format\":1,\"header_sha256\":\"bad\"}"); Reject(() => NativeCatalog.Load(p)); });
    foreach (string bad in new[] { "../escape.cpp", "/root.cpp", "C:/evil.cpp", "a/../../evil.cpp", "a:stream.cpp", "a/CON.cpp", "x/NUL", "x./a.cpp", "x /a.cpp", "a//b.cpp" }) Check("Reject unsafe path " + bad, () => Reject(() => InputLoader.SafeRelative(bad)));
    Check("Preserve nested header paths", () => { var i = InputLoader.Load(Zip(("mod/main.cpp", Entry()), ("mod/inc/common.h", "#pragma once"))); Must(i.Files.Count == 2); });
    Check("Reject ZIP traversal before extraction", () => Reject(() => InputLoader.Load(Zip(("../evil.cpp", Entry())))));
    Check("Reject case collisions", () => Reject(() => InputLoader.Load(Zip(("main.cpp", Entry()), ("MAIN.cpp", Entry())))));
    Check("Reject invalid source encoding", () => { string p = NewDir(); File.WriteAllBytes(Path.Combine(p, "main.cpp"), [0xff, 0x80]); Reject(() => InputLoader.Load(p)); });
    Check("Reject binary content masquerading as source", () => Reject(() => InputLoader.Load(Source("abc\0def"))));
    Check("Reject inputs without source", () => Reject(() => InputLoader.Load(Zip(("readme.txt", "hello")))));
    Check("Ignore build output trees", () => { string p = Source(Entry()); Directory.CreateDirectory(Path.Combine(p, "build")); File.WriteAllText(Path.Combine(p, "build", "other.cpp"), Entry()); Must(InputLoader.Load(p).Files.Count == 1); });
    Check("Comments and strings do not create findings", () => { var a = Analyze(Entry("// VirtualProtect(); GAMEPLAY::NO_SUCH_NATIVE();\nconst char* s = \"GetAsyncKeyState()\"; (void)s;")); Must(a.Blockers == 0); });
    Check("Raw string contents ignored", () => { var a = Analyze(Entry("const char* s = R\"tag(VirtualProtect(); PLAYER::DOES_NOT_EXIST())tag\"; (void)s;")); Must(a.Blockers == 0); });
    Check("Detect missing entry", () => Must(Analyze("void Other() {}").Findings.Any(f => f.Code == "ENTRY_MISSING")));
    Check("Entry declaration is not a definition", () => Must(Analyze("void ScriptMain();").Findings.Any(f => f.Code == "ENTRY_MISSING")));
    Check("Detect multiple entries", () => { var i = InputLoader.Load(Zip(("a.cpp", Entry()), ("b.cpp", Entry()))); Must(SourceAnalyzer.Analyze(i, catalog).Findings.Any(f => f.Code == "ENTRY_MULTIPLE")); });
    Check("Translation-unit selection", () => { var i = InputLoader.Load(Zip(("a.cpp", Entry()), ("b.cpp", "void ScriptMain() { VirtualProtect(); }"))); Must(SourceAnalyzer.Analyze(i, catalog, ["a.cpp"]).Blockers == 0); });
    Check("Detect unknown native", () => Must(Analyze(Entry("PLAYER::NOT_A_REAL_NATIVE();")).Findings.Any(f => f.Code == "NATIVE_UNKNOWN")));
    Check("Detect missing target registration", () => Must(Analyze(Entry("APPS::APPS_CAR_UNLOCK(\"a\");")).Findings.Any(f => f.Code == "NATIVE_UNREGISTERED")));
    Check("Namespace alias verified per function", () => { var a = Analyze(Entry("GAMEPLAY::GET_GAME_TIMER();")); Must(a.Natives.Single().Name == "MISC::GET_GAME_TIMER" && a.Blockers == 0); });
    Check("PC cheat native needs controller adaptation", () => Must(Analyze(Entry("GAMEPLAY::_HAS_CHEAT_STRING_JUST_BEEN_ENTERED(0);")).Findings.Any(f => f.Code == "PC_CHEAT_INPUT")));
    Check("PC version guard is not faked", () => Must(Analyze(Entry("getGameVersion();")).Blockers > 0));
    Check("Full world enumeration uses the NX extension", () => Must(Analyze(Entry("int p[8]; worldGetAllPeds(p,8); worldGetAllVehicles(p,8); worldGetAllObjects(p,8); worldGetAllPickups(p,8);")).Blockers == 0));
    Check("Additional scripts use the NX extension", () => Must(Analyze(Entry("scriptRegisterAdditionalThread(Worker); scriptUnregister(Worker);")).Blockers == 0));
    Check("Global and entity pointers require layout review", () => { var r=Analyze(Entry("getGlobalPtr(10); getScriptHandleBaseAddress(1);")); Must(r.Blockers==0 && r.Findings.Count(f=>f.Code=="NX_MEMORY_LAYOUT")==2); });
    Check("Rendering and texture support keep DirectX limitations explicit", () => { var r=Analyze(Entry("renderCallbackRegister(Render); presentCallbackRegister(Present); createTexture(\"test.png\");")); Must(r.Blockers==0 && r.Findings.Any(f=>f.Code=="NX_RENDER_CONTEXT") && r.Findings.Any(f=>f.Code=="NX_TEXTURE_ASSET")); Must(Analyze(Entry("IDXGISwapChain* p;")).Blockers>0); });
    Check("USB keyboard callback no longer mistaken for Win32 polling", () => { var r=Analyze(Entry("keyboardHandlerRegister(Keys);")); Must(r.Blockers==0 && r.Findings.Any(f=>f.Code=="NX_KEYBOARD")); });
    Check("Legacy names use verified target declarations", () => { var a = Analyze(Entry("UI::_SET_TEXT_ENTRY_2(\"STRING\"); GRAPHICS::_0xBA3D194057C79A7B(\"\");")); Must(a.Blockers == 0 && a.Natives.All(n => n.Registered)); });
    Check("Custom registered entry is discovered and adapted", () => {
        var input = InputLoader.Load(Source("#include \"script.h\"\nvoid ScriptInit(){ WAIT(0); }\nBOOL DllMain(HMODULE m, DWORD r, LPVOID p) { if(r == DLL_PROCESS_ATTACH) scriptRegister(m, ScriptInit); return TRUE; }"));
        var p = PortProject.Create(input, catalog, sdk, Path.Combine(scratch, "custom-entry"), new("custom-entry"));
        Must(p.Report.Blockers == 0 && File.ReadAllText(Path.Combine(p.Directory, "src/main.cpp")).Contains("SHV_NX_PLUGIN(\"custom-entry\", ScriptInit)"));
    });
    Check("Raw PC hash calls blocked", () => Must(Analyze(Entry("nativeInit(0x12345678); nativeCall();")).Blockers >= 2));
    Check("Win32 memory hooks blocked", () => Must(Analyze(Entry("VirtualProtect();")).Findings.Any(f => f.Code == "API_VIRTUALPROTECT")));
    Check("Keyboard polling blocked", () => Must(Analyze(Entry("GetAsyncKeyState(0x70);")).Findings.Any(f => f.Code == "API_GETASYNCKEYSTATE")));
    Check("Unsupported hosted includes blocked", () => Must(Analyze("#include <vector>\n" + Entry()).Findings.Any(f => f.Code == "DEPENDENCY")));
    Check("Custom types header is preserved", () => Must(!SourceAnalyzer.IsVendorHeader(new("types.h", "struct MySettings { int a; };", ""))));
    Check("Standard DllMain recognized", () => { var f = new SourceFile("main.cpp", Entry() + "BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) { if(reason == DLL_PROCESS_ATTACH) scriptRegister(module, ScriptMain); return TRUE; }", ""); Must(SourceAnalyzer.DllEntries(f).Single().Standard); });
    Check("Custom DllMain initialization not removed", () => { var f = new SourceFile("main.cpp", "BOOL DllMain(HMODULE m, DWORD r, LPVOID p) { configureMod(); return TRUE; }", ""); Must(!SourceAnalyzer.DllEntries(f).Single().Standard); });
    Check("DllMain assignment not treated as boilerplate", () => { var f = new SourceFile("main.cpp", "BOOL DllMain(HMODULE m, DWORD r, LPVOID p) { counter = 5; return TRUE; }", ""); Must(!SourceAnalyzer.DllEntries(f).Single().Standard); });
    Check("Second PC script registration rejected", () => { var f = new SourceFile("main.cpp", "BOOL DllMain(HMODULE m, DWORD r, LPVOID p) { scriptRegister(m, OtherScript); return TRUE; }", ""); Must(!SourceAnalyzer.DllEntries(f).Single().Standard); });
    string multiRegistration = "#include \"script.h\"\nvoid Worker(){for(;;)WAIT(0);}\nvoid Present(void*){}\nvoid ScriptMain(){for(;;)WAIT(0);}\nBOOL DllMain(HMODULE m,DWORD r,LPVOID p){switch(r){case DLL_PROCESS_ATTACH:scriptRegister(m,ScriptMain);scriptRegisterAdditionalThread(m,Worker);presentCallbackRegister(Present);break;case DLL_PROCESS_DETACH:scriptUnregister(m);presentCallbackUnregister(Present);break;}return TRUE;}";
    Check("Multi-script registration-only DllMain has an explicit lifecycle plan", () => { var d=SourceAnalyzer.DllEntries(new("main.cpp",multiRegistration,"")).Single(); Must(d.Standard&&d.Registration is {Extended:true} &&d.Registration.Initialize.Count==2&&d.Registration.Stop.Count==1); });
    foreach(string body in new[]{"if(FALSE)scriptRegister(m,ScriptMain);return TRUE;","if(r==DLL_PROCESS_ATTACH){counter=1;scriptRegister(m,ScriptMain);}return TRUE;","if(r==DLL_PROCESS_ATTACH){scriptRegister(m,ScriptMain);configureMod();}return TRUE;","if(r==DLL_THREAD_ATTACH)scriptRegister(m,ScriptMain);return TRUE;","if(r!=DLL_PROCESS_DETACH)scriptRegister(m,ScriptMain);return TRUE;"})
        Check("Unsafe DllMain is preserved: "+body,()=>Must(!SourceAnalyzer.DllEntries(new("main.cpp","BOOL DllMain(HMODULE m,DWORD r,LPVOID p){"+body+"}","")).Single().Standard));
    Check("Multi-script PC lifecycle adaptation preserves worker and callback work", () => {
        var p=PortProject.Create(InputLoader.Load(Source(multiRegistration)),catalog,sdk,Path.Combine(scratch,"multi-lifecycle"),new("multi-lifecycle"));
        string text=File.ReadAllText(Path.Combine(p.Directory,"src/main.cpp"));
        Must(p.Report.Blockers==0&&text.Contains("SHV_NX_PLUGIN_WITH_LIFECYCLE")&&text.Contains("if (!scriptRegisterAdditionalThread(Worker)) return false;")&&text.Contains("presentCallbackRegisterChecked(Present)")&&text.Contains("retaining Load Now support")&&!text.Contains("\n    presentCallbackUnregister("));
        var built=BuildService.BuildAsync(p.ProjectFile,new(sdk,"python",devkit),_=>{},createInstallZip:false).GetAwaiter().GetResult();Must(built.Nro.QueryExportPresent);
    });
    Check("Conditional callbacks are not silently lifted out of their conditions",()=>{
        var f=new SourceFile("main.cpp",multiRegistration.Replace("presentCallbackRegister(Present);","if(enabled)presentCallbackRegister(Present);"),"");Must(!SourceAnalyzer.DllEntries(f).Single().Standard);
    });
    Check("Over-nested DllMain cannot overflow the lifecycle parser stack",()=>{
        var f=new SourceFile("main.cpp","BOOL DllMain(HMODULE m,DWORD r,LPVOID p){if(r==DLL_PROCESS_ATTACH)"+new string('{',100)+"scriptRegister(m,ScriptMain);"+new string('}',100)+"return TRUE;}","");Must(!SourceAnalyzer.DllEntries(f).Single().Standard);
    });
    Check("Initialization macros preserve the descriptor plugin ID",()=>{
        var files=new[]{new SourceFile("main.cpp","void Entry(){} SHV_NX_PLUGIN_WITH_INIT(\"my-id\",Entry,Initialize)","")};Must(SourceAnalyzer.DescriptorId(files)=="my-id"&&SourceAnalyzer.DiscoverEntry(files)=="Entry");
    });
    Check("Plugin ID restrictions", () => { Must(PortProject.ValidId("hello-world_1")); Must(!PortProject.ValidId("hello.world") && !PortProject.ValidId("bad\"id") && !PortProject.ValidId(new string('a', 32))); });
    Check("Port transformations are recorded and isolated", () => {
        string p = Source(Entry("GAMEPLAY::GET_GAME_TIMER(); const char* text = \"GAMEPLAY::GET_GAME_TIMER()\"; (void)text;")); string original = File.ReadAllText(Path.Combine(p, "main.cpp"));
        string output = Path.Combine(scratch, "port-isolation"); var result = PortProject.Create(InputLoader.Load(p), catalog, sdk, output, new("test-isolation"));
        string changed = File.ReadAllText(Path.Combine(output, "src/main.cpp"));
        Must(changed.Contains("MISC::GET_GAME_TIMER()") && changed.Contains("\"GAMEPLAY::GET_GAME_TIMER()\"") && changed.Contains("SHV_NX_PLUGIN(\"test-isolation\", ScriptMain)"));
        Must(original == File.ReadAllText(Path.Combine(p, "main.cpp")) && result.Report.Blockers == 0 && result.Changes.Count >= 2);
    });
    Check("Existing output is preserved", () => { string p = Source(Entry()); string output = NewDir(); File.WriteAllText(Path.Combine(output, "keep.txt"), "keep"); Reject(() => PortProject.Create(InputLoader.Load(p), catalog, sdk, output, new("test"))); Must(File.ReadAllText(Path.Combine(output, "keep.txt")) == "keep"); });
    Check("Output inside source rejected", () => { string p = Source(Entry()); Reject(() => PortProject.Create(InputLoader.Load(p), catalog, sdk, Path.Combine(p, "child"), new("test"))); });
    Check("Sleep adapter is explicit", () => { string p = Source(Entry("Sleep(100);")); var a = PortProject.Create(InputLoader.Load(p), catalog, sdk, Path.Combine(scratch, "sleep-off"), new("sleep-off")); var b = PortProject.Create(InputLoader.Load(p), catalog, sdk, Path.Combine(scratch, "sleep-on"), new("sleep-on", AdaptSleep: true)); Must(a.Report.Blockers > 0 && b.Report.Blockers == 0 && File.ReadAllText(Path.Combine(b.Directory, "src/main.cpp")).Contains("WAIT(100)")); });
    Check("Local header copied and found", () => {
        var i = InputLoader.Load(Zip(("main.cpp", "#include \"inc/helper.h\"\n" + Entry("helper();")), ("inc/helper.h", "inline void helper() {}")));
        var p = PortProject.Create(i, catalog, sdk, Path.Combine(scratch, "nested"), new("nested")); Must(p.Report.Blockers == 0 && File.Exists(Path.Combine(p.Directory, "src/inc/helper.h")));
    });
    Check("Unreachable headers do not block selected sources", () => {
        var i = InputLoader.Load(Zip(("main.cpp", Entry()), ("unused/windows_only.h", "#include <Windows.h>\ninline void unused(){ VirtualProtect(); }")));
        Must(SourceAnalyzer.Analyze(i, catalog, ["main.cpp"]).Blockers == 0);
    });
    Check("Custom SDK-named header survives adaptation", () => {
        var i = InputLoader.Load(Zip(("main.cpp", "#include \"types.h\"\n" + Entry("MySettings s{}; (void)s;")), ("types.h", "#pragma once\nstruct MySettings { int value; };")));
        var p = PortProject.Create(i, catalog, sdk, Path.Combine(scratch, "custom-types"), new("custom-types"));
        Must(File.Exists(Path.Combine(p.Directory, "src/nx_user_headers/types.h")) && File.ReadAllText(Path.Combine(p.Directory, "src/main.cpp")).Contains("nx_user_headers/types.h") && p.Report.Blockers == 0);
    });
    Check("Original UTF-16 bytes preserved", () => {
        string p = NewDir(), f = Path.Combine(p, "main.cpp"); File.WriteAllText(f, Entry(), Encoding.Unicode); var raw = File.ReadAllBytes(f);
        var port = PortProject.Create(InputLoader.Load(p), catalog, sdk, Path.Combine(scratch, "encoding"), new("encoding"));
        Must(raw.SequenceEqual(File.ReadAllBytes(Path.Combine(port.Directory, "original/main.cpp"))));
    });
    Check("Existing descriptor ID cannot silently change", () => {
        var i = InputLoader.Load(Source(Entry() + "SHV_NX_PLUGIN(\"stable-id\", ScriptMain)"));
        Reject(() => PortProject.Create(i, catalog, sdk, Path.Combine(scratch, "wrong-id"), new("different-id")));
        var p = PortProject.Create(i, catalog, sdk, Path.Combine(scratch, "stable-id"), new("stable-id")); Must(File.ReadAllText(Path.Combine(p.Directory, "src/main.cpp")).Split("SHV_NX_PLUGIN").Length == 2);
    });
    Check("Manifest traversal rejected", () => { string p = NewDir(); string f = Path.Combine(p, "project.json"); File.WriteAllText(f, "{\"format\":1,\"kind\":\"nro\",\"id\":\"evil\",\"sources\":[\"../outside.cpp\"],\"includes\":[]}"); Reject(() => PortProject.Read(f)); });
    Check("Cleanup descriptor entry and identity preserved",()=>{
        var i=InputLoader.Load(Source("#include \"script.h\"\nvoid Run(){WAIT(0);}\nvoid Stop(){}\nSHV_NX_PLUGIN_WITH_STOP(\"stop-test\",Run,Stop)\n"));
        Must(SourceAnalyzer.DiscoverEntry(i.Files)=="Run"&&SourceAnalyzer.DescriptorId(i.Files)=="stop-test");
        var p=PortProject.Create(i,catalog,sdk,Path.Combine(scratch,"stop-test"),new("stop-test"));
        Must(p.Report.Blockers==0&&File.ReadAllText(Path.Combine(p.Directory,"src/main.cpp")).Split("SHV_NX_PLUGIN").Length==2);
    });
    Check("Valid ASI imports enumerated", () => { string p = Path.Combine(scratch, "valid.asi"); File.WriteAllBytes(p, Pe()); var r = AsiAnalyzer.Analyze(p); Must(r.Status == "Binary conversion unavailable" && r.Details["Imports: ScriptHookV.dll"] == "scriptRegister" && r.Blockers == 1); });
    Check("Malformed PE rejected", () => { string p = Path.Combine(scratch, "bad.asi"); File.WriteAllBytes(p, [1, 2, 3]); Reject(() => AsiAnalyzer.Analyze(p)); });
    Check("PE import RVA bounds checked", () => { var b = Pe(); BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(0x200), 0xfffffffe); string p = Path.Combine(scratch, "rva.asi"); File.WriteAllBytes(p, b); Reject(() => AsiAnalyzer.Analyze(p)); });
    Check("PE oversized directory count rejected", () => { var b = Pe(); BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(0x98 + 108), 100); string p = Path.Combine(scratch, "dirs.asi"); File.WriteAllBytes(p, b); Reject(() => AsiAnalyzer.Analyze(p)); });
    Check("PE unterminated import name rejected", () => { var b = Pe(); Array.Fill(b, (byte)'a', 0x240, b.Length - 0x240); string p = Path.Combine(scratch, "name.asi"); File.WriteAllBytes(p, b); Reject(() => AsiAnalyzer.Analyze(p)); });
    Check("Invalid NRO header rejected", () => { string p = Path.Combine(scratch, "bad.nro"); File.WriteAllBytes(p, new byte[4096]); Reject(() => NroInspector.Inspect(p)); });
    Check("Cancellation before input read", () => { using var c = new CancellationTokenSource(); c.Cancel(); bool cancelled = false; try { InputLoader.Load(Source(Entry()), c.Token); } catch (OperationCanceledException) { cancelled = true; } Must(cancelled); });
    Check("Markdown and JSON keep testing status explicit", () => { var r = Analyze(Entry()); Must(!r.SwitchRuntimeVerified && r.ToMarkdown().Contains("do not establish Switch runtime compatibility")); });
    var checks = await BuildService.DoctorAsync(new(sdk, "python", devkit));
    Check("Toolchain doctor detects local prerequisites", () => Must(checks.All(c => c.Passed), string.Join("; ", checks.Where(c => !c.Passed).Select(c => c.Name + ": " + c.Detail))));
    string fixture = Source(Entry("(void)PLAYER::PLAYER_PED_ID();")); File.WriteAllText(Path.Combine(fixture,"config.ini"),"[Controls]\nToggleButton=Y\n"); string portOutput = Path.Combine(scratch, "Real compiler with spaces");
    var real = PortProject.Create(InputLoader.Load(fixture), catalog, sdk, portOutput, new("real-compiler"));
    try {
        var built = await BuildService.BuildAsync(real.ProjectFile, new(sdk, "python", devkit), _ => { });
        Check("Real AArch64 compile and NRO validation", () => Must(built.Nro.QueryExportPresent && built.Nro.ImageBytes >= 4096));
        RefinementTests.Deployment(scratch, built.NroPath, Check);
        string syntax = await CompilerCheckService.CheckAsync(real.ProjectFile,new(sdk,"python",devkit),_=>{});
        Check("Compiler syntax and types accepted",()=>Must(syntax.Contains("checks passed")));
        string unit = Path.Combine(real.Directory,"src/main.cpp"); string originalUnit = File.ReadAllText(unit);
        File.WriteAllText(unit,originalUnit.Replace("(void)PLAYER::PLAYER_PED_ID();","(void)PLAYER::PLAYER_PED_ID(123);"));
        syntax = await CompilerCheckService.CheckAsync(real.ProjectFile,new(sdk,"python",devkit),_=>{});
        Check("Compiler rejects wrong native arity",()=>Must(syntax.Contains("checks failed")));
        File.WriteAllText(unit,originalUnit+"\n#if 0\nthis is invalid C++\n#endif\n");
        syntax = await CompilerCheckService.CheckAsync(real.ProjectFile,new(sdk,"python",devkit),_=>{});
        Check("Compiler respects inactive conditional code",()=>Must(syntax.Contains("checks passed")));
        File.WriteAllText(unit,originalUnit);
        var nroOnly = await BuildService.BuildAsync(real.ProjectFile, new(sdk, "python", devkit), _ => { }, createInstallZip: false);
        Check("Standalone NRO build produces no install ZIP", () => Must(nroOnly.InstallZip.Length == 0 && File.Exists(nroOnly.NroPath) && !Directory.EnumerateFiles(nroOnly.Directory, "*.zip").Any()));
        Check("Install ZIP contains mod, namespaced defaults, instructions and attribution", () => { using var z = ZipFile.OpenRead(built.InstallZip); Must(z.Entries.Count == 5 && z.Entries.Any(e => e.FullName == "switch/gtav-scripthook/plugins/real-compiler.nro") && z.Entries.Any(e => e.FullName == "LICENSE-ScriptHookV-NX.txt") && z.Entries.Any(e=>e.FullName=="switch/gtav-scripthook/settings/real-compiler/config.ini")); });
        Check("NRO tampering detected", () => { string p = Path.Combine(scratch, "tampered.nro"); var b = File.ReadAllBytes(built.NroPath); BinaryPrimitives.WriteUInt32LittleEndian(b.AsSpan(0x18), 0xffffffff); File.WriteAllBytes(p, b); Reject(() => NroInspector.Inspect(p)); });
    } catch (Exception ex) { failed++; Console.WriteLine("FAIL real compiler integration: " + ex.Message); }
    try {
        var input = InputLoader.Load(Zip(("source/entry.cpp", "#include \"../inc/helper.h\"\n" + Entry("helper();")), ("source/helper.cpp", "#include \"../inc/helper.h\"\nint helper(){ return MISC::GET_GAME_TIMER(); }"), ("inc/helper.h", "#pragma once\nint helper();")));
        var p = PortProject.Create(input, catalog, sdk, Path.Combine(scratch, "Multi unit build"), new("multi-unit"));
        var built = await BuildService.BuildAsync(p.ProjectFile, new(sdk, "python", devkit), _ => { });
        Check("Multi-unit and nested include compiler integration", () => Must(built.Nro.QueryExportPresent));
    } catch (Exception ex) { failed++; Console.WriteLine("FAIL nested/multi-unit compiler integration: " + ex.Message); }
    using (var c = new CancellationTokenSource(TimeSpan.FromMilliseconds(200))) {
        bool cancelled = false;
        try { await BuildService.RunAsync("python", ["-c", "import time; time.sleep(30)"], scratch, _ => { }, c.Token); } catch (OperationCanceledException) { cancelled = true; }
        Check("Cancel terminates the child process", () => Must(cancelled));
    }
} finally {
    if (failed == 0 && scratch.StartsWith(Path.Combine(Path.GetTempPath(), "shvnx-porter-tests-"), StringComparison.OrdinalIgnoreCase)) Directory.Delete(scratch, true);
    else Console.WriteLine("Failure artifacts: " + scratch);
}
Console.WriteLine($"\n{passed} passed; {failed} failed. Switch runtime testing was not performed.");
return failed == 0 ? 0 : 1;
