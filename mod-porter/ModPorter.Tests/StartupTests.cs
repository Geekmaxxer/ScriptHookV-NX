using System.Buffers.Binary;
using ModPorter.Core;

static class StartupTests {
    static void Must(bool value) { if (!value) throw new Exception("Startup diagnostic assertion failed"); }
    public static void Run(string scratch, string sdk, Action<string,Action> check) {
        byte[] Fatal(ulong title = 0x010041544D530000UL) {
            byte[] data = new byte[0x450]; "AFE2"u8.CopyTo(data);
            void U64(int at, ulong value) => BinaryPrimitives.WriteUInt64LittleEndian(data.AsSpan(at), value);
            BinaryPrimitives.WriteUInt32LittleEndian(data.AsSpan(4),0xFFE); U64(8,title);
            U64(0x110,0x1400100); U64(0x118,0x1400000); U64(0x140,2); U64(0x148,256);
            U64(0x150,0x1401234); return data;
        }
        string file = Path.Combine(scratch,"fatal.bin");
        check("AFE2 identifies ams.mitm without assigning one assertion", () => {
            File.WriteAllBytes(file,Fatal()); var report=RuntimeReportService.Read(file);
            Must(report.Values["program_id"]=="010041544D530000" && report.Values["pc_module_offset"]=="0x100" && report.Values["trace_0"]=="0x1234");
            Must(report.Findings.Any(f=>f.Contains("not a DLC identifier")) && report.Findings.Any(f=>f.Contains("does not identify the cause")));
        });
        check("Another process does not receive a GTA DLC diagnosis", () => {
            File.WriteAllBytes(file,Fatal(0x0100000000001000)); var report=RuntimeReportService.Read(file);
            Must(report.Findings.Any(f=>f.Contains("another process")) && !report.Findings.Any(f=>f.Contains("check the game layout")));
        });
        check("Malformed AFE2 stack extents and unsupported binaries are rejected", () => {
            foreach(var data in new[]{Fatal()[..0x440],new byte[0x450],Fatal()}) {
                if(data.AsSpan().StartsWith("AFE2"u8) && data.Length==0x450) BinaryPrimitives.WriteUInt64LittleEndian(data.AsSpan(0x140),33);
                File.WriteAllBytes(file,data); bool rejected=false;
                try { RuntimeReportService.Read(file); } catch(InvalidDataException) { rejected=true; }
                Must(rejected);
            }
        });
        if (OperatingSystem.IsWindows()) check("Desktop layout check and recovery use the packaged helper", () => {
            string sd=Path.Combine(scratch,"layout-sd"); string romfs=Path.Combine(sd,"atmosphere/contents/0100b00b51230000/romfs");
            foreach(string name in new[]{"mpbiker","mpchristmas2","mpheist","mppatchesng","patchday1ng","patchday2bng","patchday2ng","patchday3ng"}) {
                string p=Path.Combine(romfs,"update/switch/dlcpacks",name,"dlc.rpf"); Directory.CreateDirectory(Path.GetDirectoryName(p)!); File.WriteAllBytes(p,new byte[1024]);
            }
            var before=GameLayoutService.RunAsync(sdk,sd).GetAwaiter().GetResult();
            Must(before.RecoveryAllowed && before.MissingOptional.Length==17);
            var after=GameLayoutService.RunAsync(sdk,sd,recover:true).GetAwaiter().GetResult();
            Must(after.MissingOptional.Length==0 && after.Placeholders.Length==17 && File.Exists(after.Record));
        });
    }
}
