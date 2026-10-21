# ScriptHookV-NX

Native C++ plugins and custom SanScript natives for GTA V on Nintendo Switch.
Target: title `0100b00b51230000`, build `6e3e9b3af5746e74b615bd981080c9271bc226620`.

The current source defaults to **eight resident NROs, sixteen cooperative script
slots and an 8 MiB plugin budget**. Builders and mod authors can prepare custom
limits; players can override each setting in an optional SD configuration file.
See [runtime limits](docs/RUNTIME-LIMITS.md). Switch acceptance of the new
configurable-limits test is pending. Previously published runtimes are unchanged.

Use, editing, redistribution and sale require attribution; see [LICENSE](LICENSE)
and [NOTICE](NOTICE). This is a Public Preview with the limits documented below.
[Downloads](https://github.com/Geekmaxxer/ScriptHookV-NX/releases) are separate
runtime, developer SDK and clean source archives. See [release notes](docs/PUBLIC-PREVIEW.md).

**[Developer quickstart](docs/START-HERE.md)** | **[Install mods](docs/INSTALL.md)** |
**[Tutorials](docs/TUTORIALS.md)** | **[Tools](docs/TOOLS.md)** | **[NSC modding](docs/NSC-MODDING.md)**

## For players

Install a compatible runtime with GTA closed, then put native mods in
`switch/gtav-scripthook/plugins/`. `.nro` starts automatically, `.nro.manual`
waits for Load Now and `.nro.disabled` stays disabled. Use MEGATARD's plugin page,
the optional standalone manager, or choose filenames yourself. No particular
NSC menu is required. Keep saved settings when updating.

[Installation and troubleshooting](docs/INSTALL.md) covers startup modes, binding
files, process controls and resident-module limits. Windows ASI/DLL mods
need a source port and ARM64 rebuild.

## For developers

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
Exact reports and hashes are retained separately:

- [First public loader](evidence/public-loader-test1-20261015/hardware-20261015/console-acceptance.json).
- [Refinement Test 2](evidence/refinement-test2-20261015/hardware-20261015/console-acceptance.json).
- [Four-plugin showcase](evidence/safe-manager-test1-20261015/hardware-20261016/console-acceptance.json).
- [Manager Test 2 restart](evidence/safe-manager-test2-20261016/hardware-restart-20261016/console-acceptance.json).

SDK Refinement 1 adds optional headers/tools/tutorials without changing ABI 1,
subsdk5, the menu or saved settings. New tutorials have local ARM64/NSC regression
checks; they still need their own Switch tests. The latest menu description
renderer also has separate pending visual acceptance. Local emulation mocks game,
OS and RO services and cannot establish console FPS, peak memory or arbitrary
third-party plugin compatibility. See [remaining work](docs/COMPATIBILITY.md).

Plugins cooperate: they must yield. A 2 ms scheduler budget cannot interrupt a
runaway loop. Up to eight NROs remain mapped until GTA exits in Capacity Test 1,
even after Kill Process or
normal completion; process cancellation retains native-handler pointers. Direct
NSC loading from SD, hot code replacement and Windows platform APIs are outside
the current API. See the linked guides for the complete limits and workflows.
