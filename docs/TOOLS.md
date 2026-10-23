# SDK tools

Run from the SDK root or use absolute tool paths. These Python tools need only
the standard library; compilation additionally needs the stated toolchain.
The optional ARM64 UI-model runner also needs Unicorn; its host-compiler mode does not.
They do not download compilers, modify your SD card or execute an inspected mod.

| Tool | Purpose |
| --- | --- |
| `doctor.py` | Check SDK files, native-index hash and devkitA64/elf2nro availability |
| `create_mod.py` | Generate an NRO or NSC project without overwriting an existing directory |
| `build_mod.py` | Build a project.json, with matched binding generation for NSCs |
| `build_plugin.py` | Direct multi-source NRO builder, explicit header snapshots and unresolved-import rejection |
| `check_examples.py` | Build and preflight all standalone NRO examples in a fresh output folder |
| `verify_repo.py` | Run full-source profile, portable-tool and optional desktop/NRO checks |
| `run_ui_tests.py` | Test the bounded controller UI model with a host compiler or devkitA64/Unicorn |
| `configure_runtime.py` | Generate an optional runtime.ini with independent resident, script and memory limits |
| `native_query.py` | Search 8,540 target wrapper signatures and 6,419 static registrations offline |
| `inspect_mod.py` | Check NRO segments/export/imports/initializers or binding CRC/identities/bounds |
| `native_bindings.py` | Generate exact-script custom binding metadata |
| `report_summary.py` | Summarize loader reports and common residency/policy errors |

```powershell
python tools/native_query.py GET_ENTITY_COORDS --registered
python tools/native_query.py 0x320d1840b6daa1cc --json
python tools/check_examples.py --output out/example-check1 --devkitpro C:/devkitPro
python tools/inspect_mod.py path/to/mod.nro.manual
python tools/report_summary.py "D:/switch/gtav-scripthook/loader-report.txt"
```

Native lookup checks that the index matches `include/natives.h`. Registered
means a handler was found in this target's static registration evidence; it is
not a guarantee of semantics or a passing console test. No PC hash translation
is implied. Do not use a merely familiar PC name/hash as Switch verification.

Preflight is structural. A valid NRO can still contain bad code, an incompatible
descriptor, duplicate plugin ID or missing natives. Inspecting it does not run
initialization or prove console RO/CFW acceptance. The loader remains the final
runtime validator. Never treat a clean report as proof of arbitrary mod safety.

For multi-source projects edit project.json `sources` and `includes`, using
paths inside the project folder. Add an include directory explicitly even for
headers next to a .cpp: the compiler builds copied sources in its snapshot.
Build artifacts include debug ELF and provenance; distribute the NRO, required
bindings and a short player guide rather than the entire build folder.
Optional `project.json` `runtime_limits` emits a configuration beside the mod;
see [runtime limits](RUNTIME-LIMITS.md) for keys, defaults and installation.
