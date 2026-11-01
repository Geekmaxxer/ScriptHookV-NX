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

- Typed/raw native invocation, availability checks and per-call failure reporting.
- Cooperative scripts, controller callbacks, HUD rendering, PNG textures and entity access.
- Isolated settings, startup policies and runtime diagnostics.
- Opt-in owner memory, C/XML libraries, checked numeric parsing, buffer formatting,
  toolchain math and fallible C++ containers.
- Bounded file transfers, sorted preset catalogs and verified save/publication workflows.
- Registered cleanup, explicit shared session state and configurable plugin capacity.
- Growing PNG texture caches with stable IDs and shared memory accounting.

See the [API reference](docs/reference/API-REFERENCE.md) for contracts and
[compatibility](docs/reference/COMPATIBILITY.md) for platform limitations.

## Project status

The published baseline is **v1.4.1 Preview**, with **Mod Porter 0.9.1**.
Current development adds source-port services with separate implementation,
local-verification and hardware-acceptance records. The
[verification matrix](docs/verification/README.md) identifies each tested scope;
[Batch 4](docs/verification/CATALOG-SCAN1.md#physical-batch-4-result---2026-10-30)
accepts the combined preset/XML/save/restoration workload. New parsing, math
and formatter checks are consolidated in the
[library-services example](examples/library-services/README.md); its hardware
run remains deferred while implementation continues.

The [graphics candidate](docs/verification/TEXTURE-CATALOG1.md) removes fixed
texture-cache counts and provides an 80-image probe. It groups its pending
hardware check with the library workload and does not change the published baseline.

The supported target is title `0100b00b51230000`, executable build
`6e3e9b3af5746e74b615bd981080c9271bc22662`. Changed executables require renewed
audits. Compatible PC mods require source adaptation and their own gameplay
acceptance. The [project roadmap](docs/COMPLETION-ROADMAP.md) and
[runtime roadmap](docs/PC-RUNTIME-ROADMAP.md) track open implementation and
hardware gates. Full Menyoo porting follows those gates.

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
