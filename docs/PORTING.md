# Porting a plugin to Switch

The [Mod Porter desktop app](../desktop/README.md) can analyze a PC source folder
or ZIP, prepare a copied NX project and build a test NRO. ASI input produces a
dependency report; compiled Windows binary translation is not included. The Assets tab
prepares Legacy mod resources and nested OPEN RPF archives, with standalone and
typed embedded texture caps, metadata checks and DLC-list merging. Compiler-only
checking and verified SD NRO backup/restore are available before deployment.
See its [compatibility coverage](../desktop/COMPATIBILITY.md) before publishing a port.

For nearby-ped gameplay, `include/shv/world.h` supplies
`ScriptHook::World::NearbyPeds<32>`. Call `Refresh(playerPed)`, then read `Size()`
and indexed handles. It owns the native's eight-byte VM array slots and validates
the full writable buffer. Capacity is bounded to 1–64. This scanner is not a
full world-pool enumeration. The optional [extension API](EXTENSIONS.md) supplies
all four `worldGetAll*` functions with ordinary handle arrays.
The [Disarm NX example](../desktop/examples/disarm-nx/README.md) demonstrates a
rotating processing budget; hand/weapon disarming passed Switch gameplay testing.
The extension to both arms also passed console gameplay testing.

Start with `examples/scriptmain_plugin.cpp` and build it with `tools/build_plugin.py`.
The SDK is freestanding C++17 for AArch64. It needs devkitA64 and Python 3.10+;
plugin developers do not need extracted game files or the core's private build inputs.

## Entry point and familiar syntax

Include `script.h`, keep your `void ScriptMain()` function, and add
`SHV_NX_PLUGIN("unique-id", ScriptMain)` once after the function. Calls such as
`PLAYER::PLAYER_ID()` and `MISC::GET_GAME_TIMER()` use the supplied target wrappers.
`WAIT(0)` yields until the next game frame; larger values wait in milliseconds.
Waiting is supported from ScriptMain and its callees, not initialization or
custom native handlers.

Replace Windows DLL initialization and `DllMain` with the entry macro. An advanced
plugin can instead export a `ShvNxPlugin` descriptor to register custom natives
in its explicit initialization callback. Each ABI 1 plugin supplies one scheduled
main entry function. [Extensions ABI 1](EXTENSIONS.md) adds owned
`scriptRegisterAdditionalThread` and familiar registration overloads. Move custom
DllMain work into explicit initialization or the main entry. The Porter adapts
recognized registration-only if/switch bodies, preserving additional scripts
and callbacks in a lifecycle initializer. Unsupported conditions and custom
work remain explicit blockers. Scheduler-wide `scriptStopAll`, runtime
self-tests and cache management remain core-only.

## Native calls and data

Recompile against these headers. PC hash constants and PC structure layouts must
be audited; matching a native name alone does not prove Switch behavior. Check
`nativeIsAvailable(hash)` before optional raw calls and `getNativeCallStatus()`
after calls that can fail. Availability confirms a handler exists, not its semantics.
New gameplay natives still need target verification and a console test.

`BOOL` output storage is four bytes. Do not pass a one-byte C++ `bool*` to a BOOL
output. `Vector3` has three floats in padded eight-byte slots (24 bytes total).
Use the typed wrappers for vectors and references. Use `float` literals, not
`double`, for FLOAT arguments. The typed SDK rejects several unsafe argument forms
at compilation. Raw calls require an explicit target ABI audit.

`scriptWait` and `WAIT` are the scheduler's yield points. Do not call the game's
native WAIT or TERMINATE_THIS_THREAD: plugins borrow a game script context.

## Platform work

Windows headers, DLL imports, keyboard hooks, DirectX, and an ordinary hosted C++
standard library are unavailable. Replace input with verified GTA PAD calls and
rendering with verified GTA graphics/HUD calls. Poll on the game script thread.
The linker rejects unresolved platform dependencies. Global constructors and
destructors are rejected; initialize state explicitly. A port is source work,
not a renamed Windows binary.

For additional header folders, pass `--include path/to/headers`. Header snapshots
are compiled and hashed with the plugin. Build outputs go in a fresh directory.

## Settings and script providers

`scriptSaveFile` / `scriptReadFile` use flat filenames and place files under
`switch/gtav-scripthook/settings/<plugin-id>/<filename>`. Keep the ID stable across
updates. Writes are limited to 64 KiB and are raw single-file operations; a mod
that needs crash recovery should maintain its own validated alternating records.

NSC mods use `SHVNX_SAVE` / `SHVNX_LOAD` from `commands_scripthook_nx.sch` for
checksummed alternating settings records. Default namespaces follow script names.
For a custom native provided by another mod, generate binding metadata from the
exact NSC and include the provider hash. The SDK guide describes the complete binding workflow.

## Staying within the Switch budget

Yield regularly. Use WAIT(0) for frame rendering/input, and longer waits for
background work. Cache constant lookups, request models asynchronously, cap
entity counts and release models/entities when no longer needed. Avoid scanning
large entity sets or writing settings every frame. Each plugin has a 64 KiB stack;
large buffers belong in bounded static storage rather than local arrays.

The scheduler has a 2 ms cooperative budget and cannot interrupt a plugin that
never returns or yields. Images and BSS are each capped at 2 MiB, with eight resident
plugins by default; see [runtime limits](RUNTIME-LIMITS.md) for optional overrides. Unloading an NRO during play is unavailable. Disable it and restart
GTA instead. Test menu-open/closed, transitions, death/respawn, vehicles and a soak
on the actual Switch; compilation and emulation are separate evidence.

Those capacities describe the current Capacity Test 1 core. Earlier public
runtimes support four resident NROs. The plugin budget remains 8 MiB; eight
resident images are possible only if their combined accounting fits it.
