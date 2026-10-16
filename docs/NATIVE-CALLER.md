# Native caller ABI 1

The caller implements typed C++ invocation and the raw ScriptHook-style C API
for the pinned Switch executable. Candidate 1 passes 116 local checks. The
2026-10-15 console report confirms seven getter checks passed with zero failures
or native errors, plus one successful save and two successful loads. The installed
module hash matches the candidate. [Preserved console evidence](../evidence/native-caller-research/hardware-20261015/console-acceptance.json)
records the tested scope; the earlier persistence Test 3 remains available for rollback.

## Calling a native

```cpp
#include "natives.h"

void ReadScreenOnGameScriptThread() {
    int32_t width = 0, height = 0;
    ScriptHook::Natives::GRAPHICS::GET_SCREEN_RESOLUTION(&width, &height);
    if (getNativeCallStatus() != NativeCall_Ok) {
        const uint64_t failedHash = getLastNativeHash();
        (void)failedHash; // Report or disable the affected feature.
        return;
    }
}
```

`ScriptHook::invoke<Result>(switchHash, arguments...)` is the direct typed API.
Each call owns its context, including nested calls. Generated wrappers use
this API with hashes from the target's dev_ng declarations. Available C entry
points are exported from the NSO; the typed helpers use the sized C interface
so the plugin and runtime can check their shared context ABI.

```cpp
nativeInit(0x320d1840b6daa1ccULL); // Target GET_GAME_TIMER hash, not a PC hash.
const uint32_t timer = uint32_t(nativeCall()[0]);
if (getNativeCallStatus() != NativeCall_Ok) {
    // The returned slots are zero on failure. Handle the status explicitly.
}
```

Check `getNativeCallerAbiVersion() == SHV_NX_CALLER_ABI_VERSION` when binding a
plugin to a runtime. Raw results contain three eight-byte slots. Their pointer
lasts until another call reuses that nesting depth; copy results immediately.
`nativePush64` takes an already-encoded slot, so encode FLOAT bit patterns rather
than casting their numeric value to uint64_t. Prefer typed invocation for floats.

## Types and limits

| Value | Contract |
| --- | --- |
| INT, handle, Hash, enum | Low 32 bits of an eight-byte slot; signed small integers promote to INT |
| Any, pointer, explicit 64-bit integer | All 64 bits retained |
| FLOAT | 32-bit float bit pattern; use `1.0f`, not a double literal |
| BOOL value | Four-byte numeric boolean; nonzero converts to true |
| BOOL output reference | `BOOL*` (int32_t), never `bool*` |
| STRING input | `const char*`, nullable; bounded validation through 16 KiB |
| STRING output reference | Storage for a full string pointer |
| Vector3 value | Three FLOAT argument slots |
| Vector3 return | Three padded eight-byte result slots, decoded into clean padding |
| Vector3 output reference | 24-byte writable Vector3; copied back from the engine's 16-byte temporary |
| NativeNavData value | Three FLOAT slots, as verified in the target task handler |
| Opaque/STRUCT reference | Caller-defined storage via void*; the caller supplies the correct layout and capacity |

There are 64 argument slots, four vector output temporaries and eight nested raw
call frames. A vector cannot partially overflow an argument list. Invalid
handler addresses, missing hashes, invalid typed pointers, bad output-vector
counts and ABI-size mismatches return errors with cleared results. A handler
leaving an unfinished nested raw call fails and releases the abandoned frame.

Typed pointer checks cover the declared pointee; they cannot infer variable
buffer lengths or native-specific argument rules. The raw API carries no
pointer type metadata. Null arguments are permitted for natives whose contracts
allow them. Native handlers are trusted game/plugin code; these checks do not
recover from arbitrary faults inside a handler.

## Game thread and scheduling

Invocation requires the target VM's active script TLS flag, its script pointer,
and the corresponding global active-script pointer to agree. Worker-thread and
out-of-context calls fail. The raw stack also enforces thread ownership. Native
calls do not establish a game script context on the caller's behalf.

Candidate 1 keeps scheduling disabled. The subsequent [cooperative runtime
test](COOPERATIVE-RUNTIME.md) adds stackful wait/resume and once-per-frame
dispatch through the paired menu thread. That Test 1 runtime passed all eight console checks. The [public loader
test](PUBLIC-SDK.md) now adds external SD NRO plugins and general host selection;
its new paths remain pending console acceptance. Finish all raw native
arguments/calls before waiting; yielding inside a native handler is rejected.

## Lookup and wrapper coverage

The bounded engine resolver understands the target's obfuscated registration
nodes. A 128-entry positive cache checks the current bucket head and main base;
misses are not cached, so later registrations remain visible. Custom handlers
are checked first. If another component directly edits existing registry
handlers without changing the bucket head, call `invalidateNativeLookupCache()`.

The generator emits 8,540 wrappers across 49 namespaces without guessing unknown
types. 6,419 wrappers match the 6,445 static registrations extracted from this
target. This is static availability evidence, not 8,540 runtime compatibility
claims. Query `nativeIsAvailable(hash)` and handle missing natives. The raw API
uses Switch hashes and does not automatically translate arbitrary PC hashes.
Compatible PC plugin sources need rebuilding against this SDK and review of
their platform-specific dependencies and native contracts.

## Verification and console check

Build with a fresh output directory, then run these tools with that directory:

```powershell
python ScriptHookV-NX/tools/build.py --output ScriptHookV-NX/build/my-caller
python ScriptHookV-NX/tools/build_native_fixture.py --build ScriptHookV-NX/build/my-caller
python ScriptHookV-NX/tools/test_native_caller.py --build ScriptHookV-NX/build/my-caller
python ScriptHookV-NX/tools/test_bridge.py --build ScriptHookV-NX/build/my-caller
python ScriptHookV-NX/tools/test_native_sdk.py --build ScriptHookV-NX/build/my-caller
```

The emulator-only C++ adapter forwards into the exact production module, not a
replacement caller. Tests execute actual target handlers for scalar, string,
reference and vector contracts, plus controlled handlers for failure/nesting
cases. OS memory queries, TLS and game state are emulated. The adapter is never
linked into the installable NSO. Compiler checks cover all generated wrappers,
C inclusion, reproducible generation and rejected unsafe types.

On Switch, keep the matching MEGATARD menu, replace only subsdk5, and run Storage
Self-Test. It still tests the separate SD files and now explicitly runs seven
read-only native checks. In `switch/gtav-megatard/bridge-report.txt`, expect:

```text
native_caller_self_test_ran=0x1
native_caller_self_test_passed=0x7
native_caller_self_test_failed=0x0
```

The in-menu storage counters still describe its two SD checks. Native results
are separate report fields. Confirm the existing saved settings also restore.
The getter self-test does not establish every native's gameplay semantics or
the future plugin scheduler's behavior.
