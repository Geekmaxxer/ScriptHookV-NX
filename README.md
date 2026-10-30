# ScriptHookV-NX

ScriptHookV-NX is a native scripting runtime and development SDK for GTA V on
Nintendo Switch. ARM64 C++ plugins can call game natives, run cooperative scripts,
respond to controller input and render overlays alongside Story Mode.

The project includes a standalone SDK, example mods, a Windows Mod Porter and
audited bindings for the supported executable. Compatible PC mods can be adapted
from source; Windows ASI/DLL binaries require a separate port.

[Downloads](https://github.com/Geekmaxxer/ScriptHookV-NX/releases) |
[Documentation](docs/README.md) | [Roadmap](docs/COMPLETION-ROADMAP.md) |
[Changelog](CHANGELOG.md)

## Getting started

**Players:** follow the [installation guide](docs/guides/INSTALL.md) to install
the runtime and choose each mod's startup policy. Close GTA before replacing
runtime or plugin files, and preserve existing settings.
Before deleting DLC archives to save space, read the [DLC removal and recovery guide](docs/guides/DLC-RECOVERY.md).

**Mod developers:** start with the [plugin quickstart](docs/guides/START-HERE.md).
The SDK requires Python 3.10+, devkitA64 and switch-tools. Plugin builds do not
require extracted game files or the runtime's private research inputs.

```powershell
python tools/doctor.py --devkitpro C:/devkitPro
python tools/create_mod.py --template hello --id my-plugin --output projects/my-plugin
python tools/build_mod.py --project projects/my-plugin/project.json --output out/my-plugin --devkitpro C:/devkitPro
```

**PC mod authors:** use the [porting guide](docs/guides/PORTING.md) and
[Mod Porter](mod-porter/README.md) for source analysis, ARM64 builds and supported
Legacy asset preparation. Each resulting port requires its own gameplay testing.

## Capabilities

- Typed and raw game-native invocation with availability and failure reporting.
- Cooperative `ScriptMain`/`WAIT` execution, additional scripts and bounded cleanup.
- Entity pools, borrowed entity/global access, controller callbacks, HUD rendering
  and PNG textures through versioned SDK extensions.
- Isolated settings, startup policies, diagnostics and optional process controls.
- Unreleased profiles for owner-accounted memory, C/XML libraries, decimal
  conversion and fallible C++ containers.
- Unreleased chunked file access, isolated nested directories and paged catalogs.
- Unreleased registered resource cleanup and managed heap objects across cancellation.
- Unreleased explicitly initialized C++ session state shared across source files.
- Unreleased active-profile nickname access and growing runtime capacity tables.

See the [API reference](docs/reference/API-REFERENCE.md) for contracts and
[compatibility](docs/reference/COMPATIBILITY.md) for platform limitations.

## Project status

The published baseline is **v1.4.1 Preview**, with **Mod Porter 0.9.1**. Its runtime
has [scoped Switch gameplay acceptance](evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json).
Current development adds substantial source-port services; those candidates have
local verification and scoped physical Switch acceptance; broader validation remains open.
The latest [physical Test1 record](docs/verification/PLATFORM-CAPACITY1.md#physical-test1-demo-run)
verifies startup, manual demo loading, a nickname matching one selected Switch
account and a reported managed run of at least 30 seconds. The later
[combined workload](docs/verification/COMBINED-ACCEPTANCE1.md#physical-test1-result---2026-10-30)
verifies storage/XML, worker/render callbacks, borrowed entities and final-run
managed cleanup after restart. The [Explicit capacity result](docs/verification/CAPACITY-ACCEPTANCE1.md)
extends this to large image/BSS extents, 36 workers and nested child/parent cleanup;
the same bounded workload is accepted under Automatic settings. Original
configuration restoration and fresh startup are verified. Additional profiles,
resident-module growth, transitions and
measured performance/soak acceptance remain pending.
The [Test2 hardware result](docs/verification/CAPACITY-DIAGNOSTICS2.md) accepts both
instrumented workloads and cleanup, identifying synchronous report-save stalls.
The next [service batch](examples/service-acceptance/README.md) groups new shared
C++ state with publication and catalog checks on the unchanged runtime.

The supported target is title `0100b00b51230000`, executable build
`6e3e9b3af5746e74b615bd981080c9271bc22662`. A changed executable requires a renewed
audit. Windows APIs, arbitrary PC memory patches, managed scripts and unlimited
plugin workloads are outside the current compatibility contract.

The [project roadmap](docs/COMPLETION-ROADMAP.md) tracks delivered functionality.
The [runtime roadmap](docs/PC-RUNTIME-ROADMAP.md) tracks active implementation and
acceptance gates. Full Menyoo porting follows those gates.

## Repository structure

| Directory | Responsibility |
| --- | --- |
| `runtime/` | Engine bridge, scheduler, loader and platform services |
| `include/` | Public plugin ABI, natives and reusable SDK headers |
| `sdk/` | Plugin startup/linker support, libraries and dependency provenance |
| `examples/` | Tutorials, complete projects and example gameplay mods |
| `tools/` | Build, configuration, verification and packaging commands |
| `tests/` | Automated regressions and compiled fixtures |
| `targets/` | Audited build profiles and runtime references |
| `mod-porter/` | Windows Mod Porter application and tests |
| `docs/` | Guides, reference material, plans and verification summaries |
| `packaging/` | Shared SD-access metadata and preservation installer |

Generated builds, release staging and maintainer research are excluded from
curated public exports. The [repository guide](docs/development/REPOSITORY.md)
defines source ownership and export rules.

## Contributing and licensing

Read the [contribution guide](docs/development/CONTRIBUTING.md) and
[build instructions](docs/development/DEVELOPING.md). Changes require appropriate
source/build checks; local emulation and physical gameplay are separate evidence.

Created by **MEGATARD** (@Geekmaxver; GitHub: Geekmaxxer).
Use, modification, redistribution and sale require attribution under the
[project license](LICENSE) and [notice](NOTICE). Dependency licenses are retained
in [third-party notices](THIRD-PARTY-NOTICES.txt). AI-assisted development is used
throughout the project; acceptance claims refer to recorded verification scope.
