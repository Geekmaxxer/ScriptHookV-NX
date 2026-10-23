# Runtime completion

The Switch runtime now covers the native-plugin feature set used by source ports:
typed/raw native calls, custom NSC natives and persistence, multiple cooperative
scripts, world pools, borrowed global/entity pointers, PNG textures, controller
callbacks, HUD callbacks and independent plugin management.

SDK Parity Test 1 passed its recorded console checks, including changing entity
counts during gameplay. Controller input is the Switch release requirement.
USB keyboard compatibility is optional and does not block release.

v1.3 adds diagnostics and lifecycle hardening to that baseline.
[Matched console acceptance](../evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json) covers the multi-script worker/render example, optional diagnostics
and simultaneous gameplay mods without observed regressions. The exact accepted
runtime is shipped unchanged.

## Diagnose costly plugins

The runtime records calls, total elapsed microseconds and worst invocation for
each plugin's script resumes, callbacks, cleanup and initialization. Sleeping
time in WAIT is excluded. Counters survive Kill/Load within the same GTA process.
There is no periodic profiling log writer or profiling worker thread.

Use the optional [diagnostics mod](../mods/runtime-diagnostics/README.md) to save
a report with **L + R + Nintendo X**, or query it from C++:

```cpp
#include "plugin_manager.h"
ScriptHook::Plugins::Timing timing{};
if (ScriptHook::Plugins::GetTiming(0,
        ScriptHook::Plugins::TimingKind::Script, timing)) {
    // timing.calls, timing.totalMicroseconds, timing.worstMicroseconds
}
```

Call `TimingAvailable()` to detect older runtimes. The custom native is
`SHVNX_GET_PLUGIN_TIMING(slot, kind, &calls, &totalUs, &worstUs)` at
`0x5348564e58504d31`; kinds are Script=0, Callback=1, Cleanup=2, Initialize=3.
It uses three distinct writable eight-byte outputs and rejects invalid indices,
overlapping buffers and malformed contexts. The SanScript header declares the
same call; ordinary SanScript INT arithmetic sees the low 32 bits, while C++
preserves the complete 64-bit counters.

These are elapsed dispatch timings, including native calls, I/O and OS
preemption. They do not measure the whole game, isolated CPU usage, peak RAM,
GPU usage or FPS. Custom natives invoked independently by an NSC are outside
this attribution. Use snapshots to compare the same gameplay workload.

## Callback isolation

A callback that attempts WAIT or leaves an unfinished raw native call is disabled
after its first diagnosed failure. Other callbacks continue. Its failure count
appears in the loader report. Explicit re-registration or a plugin restart can
retry it. This detects API contract violations; it cannot recover from a crash,
arbitrary memory corruption or a function that never returns.

## Port multi-script initialization

`SHV_NX_PLUGIN_WITH_INIT(id, Main, Initialize)` binds the API and calls a bounded
`bool Initialize()` that may stage workers and callbacks. Add cleanup with
`SHV_NX_PLUGIN_WITH_LIFECYCLE(id, Main, Initialize, Stop)`.

Mod Porter recognizes registration-only DllMain bodies using supported if/switch
branches. It preserves workers and callbacks in an NX initializer, checks
required capabilities, and uses checked registration calls. Runtime-owned stop
and restart retain the registrations for Load Now. Custom calls, assignments,
unsupported conditions and thread-notification work remain review blockers.
It does not lift registration out of an arbitrary PC condition.

Try the [multi-script PC source example](../desktop/examples/multi-script-pc/README.md).
Build NRO and Build NRO + Install ZIP use the same generated project. Identical
plugin inputs now produce identical NRO bytes across build-output locations;
compiler changes can still change the binary.

## Remaining boundaries

Windows ASI binaries, DirectX/Win32, managed runtimes, PC-specific offsets and
automatic support for future game builds still require separate platform work.
Game asset conversion has its own [workflow and compatibility scope](GAME-ASSETS.md).
These cannot be guaranteed by native SDK API completeness.

The local capacity suite runs eight distinct NRO images with sixteen cooperative
entries and eight render callbacks, rejects a ninth resident/seventeenth script,
and checks selective stop/restart without mapping growth. Confirm gameplay on
the target Switch before applying that acceptance to a new runtime revision.
