# ScriptHookV-NX

Native C++ plugins and custom SanScript natives for GTA V on Nintendo Switch.
Target: title `0100b00b51230000`, build `6e3e9b3af5746e74b615bd981080c9271bc22662`.

The current source defaults to **eight resident NROs, sixteen cooperative script
slots and an 8 MiB plugin budget**. Builders and mod authors can prepare custom
limits; players can override each setting in an optional SD configuration file.
See [runtime limits](docs/RUNTIME-LIMITS.md). **v1.4 Public Preview** ships the unchanged v1.3 runtime with
[matched console acceptance](evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json). Earlier runtime revisions passed
console plugin lifecycle tests with MEGATARD's NSC menu. The current SDK Parity
Test 1 adds [world enumeration, multiple scripts, globals/entity pointers, PNG
textures and input/render callbacks](docs/EXTENSIONS.md). These additions have
local ARM64 coverage and [matched Switch gameplay acceptance](evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json)
for the combined probe, rendering, controller input and NSC/NRO coexistence.
Dynamic entity counts are confirmed during gameplay. Controller input is the
Switch release target; USB keyboard support is optional. Per-plugin timing,
callback isolation and multi-script source-port lifecycle support complete this
release. The tested runtime is shipped unchanged.

Created by MEGATARD (@Geekmaxver; GitHub: Geekmaxxer).
Original source: https://github.com/Geekmaxxer/ScriptHookV-NX
Use, editing, redistribution and sale require attribution; see [LICENSE](LICENSE)
and [NOTICE](NOTICE). This is a Public Preview with the limits documented below.
[Downloads](https://github.com/Geekmaxxer/ScriptHookV-NX/releases) are separate
runtime, developer SDK and clean source archives. See [release notes](docs/PUBLIC-PREVIEW.md).

**[Developer quickstart](docs/START-HERE.md)** | **[Install mods](docs/INSTALL.md)** |
**[Tutorials](docs/TUTORIALS.md)** | **[Tools](docs/TOOLS.md)** | **[NSC modding](docs/NSC-MODDING.md)**

This release adds [reusable controller menus](docs/CONTROLLER-UI.md),
[complete loose-PED DLC creation and batch add-on checks](docs/GAME-ASSETS.md),
and [repeatable maintenance checks](docs/MAINTENANCE.md). These SDK/tool additions
have their own test scope. Mod Porter **0.6.0** adds folder-only asset, PED and
DLC registration outputs alongside ZIP packaging.

## For players

The [Mod Porter desktop tool](desktop/README.md) builds supported C++ source ports,
checks compiler signatures, prepares Legacy assets and merges DLC registration.
It includes verified SD NRO backup/restore, report import, and an
[archive workflow](docs/GAME-ASSETS.md) using your own current game files. Binary-only ASI
translation remains research work. See [completion status](docs/COMPLETION-ROADMAP.md)
for the supported operations and outstanding work.

Install a compatible runtime with GTA closed, then put native mods in
`switch/gtav-scripthook/plugins/`. `.nro` starts automatically, `.nro.manual`
waits for Load Now and `.nro.disabled` stays disabled. Use MEGATARD's plugin page,
the optional standalone manager, or choose filenames yourself. No particular
NSC menu is required. Keep saved settings when updating.

Each native mod stores its files under `switch/gtav-scripthook/settings/<plugin-id>/`.
The updated runtime migrates old prefixed filenames on access; existing files in
the new folder take priority. ABI 1 plugins do not need rebuilding for this change.

[Installation and troubleshooting](docs/INSTALL.md) covers startup modes, binding
files, process controls and resident-module limits. Windows ASI/DLL mods
need a source port and ARM64 rebuild.

## For developers

[Mod Porter desktop app](desktop/README.md) prepares compatible PC C++ source
projects, analyzes ASI dependencies, prepares Legacy assets and builds NRO test packages. Compiled ASI
translation is not included; generated ports require Switch testing.

The standalone SDK builds freestanding C++17 NROs using devkitA64 and Python 3.10+.
The core also builds from a clean checkout using the included audited target
profile; see [core builds](docs/DEVELOPING.md). No game extraction is required for
that normal build. External paths/toolchain locations are configurable.
Start with a generated project, or use familiar ScriptMain/native/WAIT syntax:

```cpp
#include "script.h"
void ScriptMain() {
    while (true) {
        (void)MISC::GET_GAME_TIMER();
        WAIT(1000);
    }
}
SHV_NX_PLUGIN("my-plugin", ScriptMain)
```

```powershell
python tools/doctor.py --devkitpro C:/devkitPro
python tools/create_mod.py --template hello --id my-plugin --output projects/my-plugin
python tools/build_mod.py --project projects/my-plugin/project.json --output out/my-plugin-test1 --devkitpro C:/devkitPro
```

Run those commands from this project/SDK directory. The developer ZIP contains
headers, shim, examples, source mods, guides and tools; no game files or compiler.
Native namespaces use this Switch target's declarations. Offline search covers
8,540 wrappers, including 6,419 static registrations; availability is not proof
of every native's behavior on console.

- [Public API](docs/PUBLIC-SDK.md), [native caller](docs/NATIVE-CALLER.md) and [runtime](docs/COOPERATIVE-RUNTIME.md).
- [PC source porting](docs/PORTING.md), [optional helpers](docs/SDK-HELPERS.md) and [compatibility limits](docs/COMPATIBILITY.md).
- [Showcase sources](mods/showcase-test1/README.md): Drive HUD, Pit Stop and Stunt Journal.
- [Standalone manager](mods/plugin-manager/README.md): startup policies and manual loading.
- [Maintaining/contributing](docs/DEVELOPING.md) and [contribution guidance](docs/CONTRIBUTING.md).

## Tested scope

Console tests established the native caller/runtime samples, external NRO loading,
simultaneous gameplay mods, manager use and startup-mode restoration after restart.
Completion Test 1 also passed Disarm arm-hit gameplay and Footprints cleanup,
with NSC menu Kill Process/Load Now controls. Mod Porter 0.3.0 passed its desktop
workflow. The newer 0.4.0 game-file updater has separate local validation and
requires asset gameplay testing. Exact records are retained separately:

- [Runtime Completion Test 1 gameplay, multi-script source port and diagnostics](evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json).
- [SDK Parity Test 1 gameplay and restart](evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json).
- [Earlier runtime and mod acceptance](evidence/completion-test1-20261021/console-acceptance.json).

- [First public loader](evidence/public-loader-test1-20261015/hardware-20261015/console-acceptance.json).
- [Refinement Test 2](evidence/refinement-test2-20261015/hardware-20261015/console-acceptance.json).
- [Four-plugin showcase](evidence/safe-manager-test1-20261015/hardware-20261016/console-acceptance.json).
- [Manager Test 2 restart](evidence/safe-manager-test2-20261016/hardware-restart-20261016/console-acceptance.json).

The SDK preserves caller/runtime/plugin ABI 1. Optional helpers and examples do
not inherit console acceptance automatically. Local emulation cannot establish
Switch FPS, peak memory, long-duration stability or arbitrary third-party mod
compatibility. See [the roadmap](docs/COMPLETION-ROADMAP.md) and
[changelog](CHANGELOG.md) for the current scope.

Plugins cooperate: they must yield. A 2 ms scheduler budget cannot interrupt a
runaway loop. Resident NROs remain mapped until GTA exits, even after Kill Process or normal
completion; process cancellation retains native-handler pointers. Direct
NSC loading from SD, hot code replacement and Windows platform APIs are outside
the current API. See the linked guides for the complete limits and workflows.

## Development disclosure

AI was used throughout the entirety of this project; it was solely to get it to a working state and to even make this possible in any way.
