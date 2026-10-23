# Public plugin and SanScript SDK

The current loader targets GTA V Switch build
`6e3e9b3af5746e74b615bd981080c9271bc22662`. Interfaces are shared by developers;
they do not depend on the MEGATARD menu name, code length or native indices.
The first public loader passed on Switch: one plugin loaded and all seven sample
checks passed. Refinement Test 2 retains ABI 1 and also passed those seven console
sample checks with zero loader errors. Later console tests cover simultaneous
showcase mods and startup-policy restoration after restart; exact scope is linked
from the repository README. Actual peak memory/FPS measurements remain pending.
SDK Parity Test 1 also passed its [scoped console checks](EXTENSIONS.md) for the
combined probe, PNG rendering, controller input, coexistence and restart.
Start with [the quickstart](START-HERE.md) and [tutorials](TUTORIALS.md), or see [porting guide](PORTING.md) for familiar ScriptMain/WAIT
syntax and efficient plugin design.

## C++ plugins

Use devkitPro/devkitA64 and Python 3.10+. The developer ZIP is self-contained for
building freestanding plugins; it does not include game binaries or the compiler.

```powershell
python tools/build_plugin.py --devkitpro C:/devkitPro --source examples/public_plugin.cpp --output out/my-plugin
```

For a minimal `ScriptMain` entry, build `examples/scriptmain_plugin.cpp` instead.
Include `script.h` for native namespaces, `WAIT` and `SHV_NX_PLUGIN`. Use repeated
`--include DIR` options for additional header roots; selected headers are copied
into the build snapshot before compilation. Windows and POSIX executable names
are recognized; the current toolchain was tested on Windows.

Copy `out/my-plugin/plugin.nro` to
`sd:/switch/gtav-scripthook/plugins/my-plugin.nro`, then restart GTA.
The loader scans directory pages, retains a bounded 16-entry catalog and allows
eight resident NROs in Capacity Test 1 (four in earlier published runtimes).
Auto candidates load one per frame after obtaining a valid
game script context. `.nro.manual` waits for Load Now; `.nro.disabled` cannot load
until its policy changes. Suffixes are case-insensitive. Catalog order is stable;
duplicate policy filenames for one canonical module are reported as conflicts.
Discovery is bounded to 4,096 entries per directory and reports limit/read errors
instead of loading a partial selection. Filenames must use
ASCII letters, digits, underscores, hyphens and periods, with at most 63 characters
including the policy suffix. The canonical filename including `.nro` is limited
to 54 characters so policy suffixes fit; do not use leading/repeated periods.
It uses the target SDK's NRO/NRR interfaces. Existing RO initialization is reused;
the current game permissions already include `ldr:ro`. It changes no executable
text, NPDM permissions or game archives. NRR acceptance depends on the console's
CFW environment. It passed in the reported first-loader console test.

Each plugin exports `shvPluginQuery`, returning a `ShvNxPlugin` descriptor:

```cpp
#include "plugin.h"
#include "natives.h"

static bool Initialize(const ShvNxApi* api) { return shvBindApi(api); }
static void Run() {
    for (;;) {
        (void)ScriptHook::Natives::MISC::GET_GAME_TIMER();
        scriptWait(1000);
    }
}
static const ShvNxPlugin plugin = {sizeof(ShvNxPlugin), 1, 1, 1,
                                  "my-plugin", Initialize, Run};
extern "C" SHV_NX_API const ShvNxPlugin* shvPluginQuery() { return &plugin; }
```

`initialize` binds the versioned API and returns without waiting. Custom handlers
registered there are staged until initialization succeeds. `run` gets its own
cooperative stack, may call `scriptWait`, and ends when it returns. The sample
registers custom hash `0x5348564558414d31`, checks real game natives and waits,
and writes a seven-check result. A different NSC can call that custom hash.

Use `scriptSaveFile` / `scriptReadFile` with flat filenames of at most 63
characters. The SDK places them under `settings/<plugin-id>/<filename>` in
`switch/gtav-scripthook`. IDs are unique ASCII letters/digits/underscore/hyphen,
at most 31 characters. File writes are limited to 64 KiB. These raw file APIs
are single-file writes; use your own journaling if needed. The SDK prevents
crossing directories through these filenames. Native invocation and I/O belong
to the game script thread. Initialize game/player-dependent state only when the
corresponding game objects are ready.

Existing ABI 1 NROs use this layout without rebuilding when run on the updated
runtime. On first access, a legacy `<plugin-id>-<filename>` is moved into that
plugin's folder if the destination is absent. Existing files in the new folder
take priority; conflicting legacy copies are retained. A failed migration leaves
the original intact and fails the file operation so defaults cannot overwrite it.

Modules remain mapped until game exit because NSCs can retain their native
handler addresses. Hot unloading/reloading is deliberately unavailable. Images
and BSS are each limited to 2 MiB. The default is sixteen cooperative stacks;
[optional runtime limits](RUNTIME-LIMITS.md) can change that capacity. Duplicate IDs (case-insensitive), mismatched ABIs, malformed layouts,
global constructors/destructors and missing entry points are rejected. Mods are
trusted executable code; faults or loops inside a plugin cannot be recovered by
the loader. The runtime borrows a live game script context and must not use
native WAIT/TERMINATE_THIS_THREAD or assume a separately owned RAGE thread.
Compatible PC plugin sources need rebuilding and adapting their entry point.
Windows DLL/ASI binaries, Windows/DirectX APIs, arbitrary PC hashes and a hosted
PC C++ standard library are not supported. The provided shim supports typed/raw
native calls, availability/handler queries, `scriptWait`, custom registration,
and namespaced file I/O. [Extensions ABI 1](EXTENSIONS.md) adds owned additional
scripts, all four entity pools, globals/entity pointers, custom PNG textures and
controller/keyboard/rendering callbacks. The base API and plugin descriptor
stay unchanged. Core-only scheduler-wide control/self-tests and cache management
are not external shim functions. Other APIs need an explicit port; unresolved
imports fail at link time. The SDK versions its caller, runtime and plugin interfaces.

## SanScript / NSC scripts

`sdk/commands_scripthook_nx.sch` declares:

| Function | Purpose |
| --- | --- |
| `SHVNX_SAVE(buffer[0], count, namespace=0)` | Save up to 256 INTs using alternating CRC-protected records |
| `SHVNX_LOAD(buffer[0], count, namespace=0)` | Restore INTs; returns the number read, or zero |
| `SHVNX_AVAILABLE(hashLow, hashHigh)` | Check a native handler without invoking it |

Counts are INT counts, not byte lengths. Namespace zero uses the calling
script's name hash; distinct scripts get distinct files. An explicit 32-bit
namespace intentionally shares settings. INTs retain their low 32-bit values
and restored negative values are sign-extended into eight-byte VM slots.
Example sources are `shvnx_public.sc` and `external_native.sc`. The latter also
calls the native provided by the example NRO. Both examples compile and adapt
against stock Switch headers. NSCs still use GTA's script loader: insert raw
adapted outputs into `script_rel.rpf` as normal compressed file entries and
start them with the existing game/menu script-loading workflow. This NRO loader
does not inject arbitrary `.nsc` files from SD into the game's resource system.

The engine replaces hashes with cached handlers when it binds a script. To
recover a custom call cached before its provider loaded, generate a small
per-script binding file from the exact compiled NSC:

```powershell
python tools/native_bindings.py --script mymod.nsc --hash 0x5348564558414d31 --output mymod.snb
```

Drop it into `sd:/switch/gtav-scripthook/bindings/`. Multiple `.snb` files merge
without overwriting other mods. Built-in persistence, capability and manager hashes are included
automatically; pass `--hash` for each additional provider hash. Uncompressed raw
and deflated RSC7 scripts are accepted. Rebuild the binding file whenever that
script is rebuilt. The loader verifies name, code length, native count, entry
fingerprint and indices before replacing only registered custom-native traps.
Scripts with declared missing dependencies wait for registration rather than
execute the missing-handler trap. Invalid/conflicting metadata is rejected;
regenerate it instead of forcing an index. At most 64 identities and 64 KiB of
metadata are retained. The older single `native-bindings.bin` form is also read.

## Plugin manager API (version 3)

`include/plugin_manager.h` offers typed C++ helpers in `ScriptHook::Plugins`.
Check `Available()` before using them. SanScript declarations are in
`sdk/commands_scripthook_nx.sch`. The manager works without a particular menu:
Auto files start from normal game scripts, and the optional standalone manager
provides the same manual loading controls as MEGATARD.

| Operation | Behavior |
| --- | --- |
| `SHVNX_PLUGIN_MANAGER_VERSION()` | Returns 3 for this API |
| `SHVNX_GET_PLUGIN_COUNT/NAME/ID/STATUS/STATUS_TEXT` | Inspect stable catalog slots, status, accounted KiB and load step |
| `SHVNX_LOAD_PLUGIN(slot)` | Queue a Manual/Auto plugin; restart a stopped/finished resident run without initialization |
| `SHVNX_GET_PLUGIN_POLICY(slot)` | Disabled=0, Manual=1, Auto=2 |
| `SHVNX_SET_PLUGIN_POLICY(slot, policy)` | Persist mode by a checked rename; no implicit unload or load |
| `SHVNX_GET_PLUGIN_ACTION_ERROR()` | Read the last action's result code; zero means accepted |
| `SHVNX_RELOAD_PLUGINS()` | Queue a merge rescan; retain existing mappings and published natives |
| `SHVNX_IS_PLUGIN_RUNNING(slot)` | True for an actual Ready/Running/Sleeping cooperative script |
| `SHVNX_KILL_PLUGIN(slot)` | Cancel that script at the next scheduler boundary; preserve startup mode and mapped code |

Statuses are Disabled=0, Resident=1, Error=2, Queued=3, Ready=4 and
ResidentPolicyChanged=5, Stopped=6 and Finished=7. Ready=4 describes runtime
readiness and does not mean the startup policy is Manual. A successful Load Now means queued, so refresh status
after the scheduler has run. The legacy `SHVNX_TOGGLE_PLUGIN` cycles startup
policies only. Kill Process stops a plugin's scheduled entry and releases its
cooperative stack. It keeps mapped code and custom handlers alive; it does not
unwind destructors or undo previous game changes. Restart GTA to replace resident code. Eight resident modules in Capacity Test 1
share an 8 MiB accounting budget by default; the catalog contains up to 16
entries. The current source supports [startup overrides](RUNTIME-LIMITS.md).

Status outputs require three distinct writable eight-byte VM slots. Names and
status strings belong to the core; copy names if retaining them across rescans.
Match `.snb` metadata to the rebuilt NSC. Capability checks inside an NSC do not
make a declared missing native safe on an older core: use the matched module.

A single-script `.snb` contains a 16-byte file header, a 48-byte identity record
and 16 bytes per custom-native entry. Two entries give 96 bytes; thirteen give
272 bytes; fifteen give 304 bytes. Explorer rounds these up to 1 KB. The loader
accepts at most 64 KiB of merged metadata, so a 96 KB manifest is oversized.

## Existing MEGATARD menu

Keep the current `ragemenu.sc`, installed `ragemenu.nsc`, update2.rpf and saved
settings. `MEGASAVE` / `MEGALOAD` and their files under `switch/gtav-megatard`
remain compatibility APIs. The core no longer compiles their native indices
into the module. The EASY test supplies `bindings/ragemenu.snb` for the currently
tested menu; a rebuilt menu only needs regenerated metadata, not a new core.
New mods should use the generic functions to avoid sharing the legacy menu files.

## Reports and validation

The earlier Refinement Test 2 EASY package contains subsdk5, the sample NRO, the menu binding
file and a short guide. Boot GTA and check `switch/gtav-scripthook/loader-report.txt`:
`plugins_loaded=0x1`, `loader_error=0x0`. Readable step/error names accompany the
raw result codes in the refinement test. The sample writes
`settings/public-sample/sample-result.txt` with `passed=7` / `failed=0`.
The existing menu's Storage Self-Test can still check its own bridge/runtime.

Local tests execute the production ARM64 loader, real NRO instructions, native
caller, target native binding and custom dispatch. OS/SDK RO services, mappings,
filesystem and the outer VM tick are mocked. They establish source/ABI/layout
behavior, not actual console NRR acceptance or every native's semantics. The
console-confirmed first-loader, runtime and earlier caller packages are preserved
for rollback. Acceptance records identify exact module and plugin hashes.

The loader flow was checked against [Skyline's loader](https://github.com/skyline-dev/skyline/blob/master/source/skyline/plugin/PluginManager.cpp)
and its [RO declarations](https://github.com/skyline-dev/skyline/blob/master/include/nn/ro.h),
then verified against this target SDK's exports and instructions. No Skyline
implementation is copied into the runtime.
