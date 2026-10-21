# External plugin API 1

Include `script.h` for the common interface. Use `plugin.h` directly only when
supplying an explicit descriptor and initialization callback. One NRO exports
one `shvPluginQuery`, with a side-effect-free query, short initialize and one
cooperative run entry. Global constructors/destructors and unresolved imports
are rejected by the builder/loader.

| API | Contract |
| --- | --- |
| `SHV_NX_PLUGIN("id", ScriptMain)` | Define an entry with a compile-time checked 1-31 character ID |
| `SHV_NX_PLUGIN_WITH_STOP("id", ScriptMain, Stop)` | Same ABI 1 descriptor with an optional `void Stop()` export; see lifecycle requirements below |
| `WAIT(ms)` / `scriptWait(ms)` | Yield from the run entry/callees; zero means next game frame |
| `ScriptHook::invoke<R>(hash, args...)` | Per-call typed context; zero result on failure, inspect status |
| `nativeInit`, `nativePush64`, `nativeCall` | Raw encoded argument slots; finish before yielding |
| `nativeIsAvailable(hash)` | Check a handler exists without calling it |
| `getNativeCallStatus`, `getLastNativeHash` | Copy failure/status before another call replaces it |
| `getNativeHandler(hash)` | Lookup a checked handler; its presence does not validate semantics |
| `registerCustomNative(hash, handler)` | Register a public callback; during initialize it is staged until success |
| `scriptSaveFile(name, data, bytes)` | Namespaced raw write, at most 64 KiB; bool result |
| `scriptReadFile(name, buffer, capacity, &got)` | Namespaced read; validate returned length and record contents |
| `getNativeCallerAbiVersion`, `getScriptRuntimeAbiVersion` | Version checks supplied by the bound core |

`shvBindApi` validates descriptor size and all three ABI versions before binding.
Each plugin owns its shim state. IDs are case-insensitively unique and namespace
settings; names supplied to file APIs are flat ASCII filenames, at most 63
characters, with no leading/repeated periods or directory separators.

`main.h` also declares core-only APIs for in-core registration, scheduler control,
self-tests and diagnostics. External ABI 1 does **not** supply `scriptRegister`,
`scriptRegisterChecked`, `scriptUnregister`, `scriptStopAll`,
`getScriptRuntimeInfo`, `getScriptRuntimeStatus`, `scriptRuntimeSelfTest`,
`nativeCallerSelfTest`, `invalidateNativeLookupCache` or `getScriptHookVersion`.
Use the one scheduled run entry and optional manager native helpers externally.
Undefined imports fail at link time instead of becoming a promised API.

## Bounded nearby-ped queries

`<shv/world.h>` provides `ScriptHook::World::NearbyPeds<Capacity>` (default 32,
range 1–64). `Refresh(anchorPed, excludedPedType = -1)` returns whether the query
succeeded. Read `Size()` and `list[index]`; a failed query clears old handles and
an out-of-range index returns zero. Revalidate handles before using them.

The helper owns the native's length header and eight-byte element slots and
validates the complete writable output range. A normal `Ped[]` is not that buffer.
Results come from the anchor's nearby scanner, not the global pedestrian pool.
Call from the scheduled plugin entry and bound work between `WAIT(0)` calls.
This is a header-only helper using existing ABI 1 calls; runtime installation is unchanged.

## Optional cleanup and portable helpers

Lifecycle Test 1 resolves `shvPluginStop` in the owning module's executable text.
It runs once after the scheduled entry is stopped or returns, and once again if
a restarted entry later stops. Run bounded cleanup that releases owned entities
or effects; use ordinary native calls and namespaced file I/O. Do not call WAIT,
run loading loops or leave an unfinished native call. The runtime diagnoses those
last two call-state violations; it cannot preempt an infinite callback or recover
from arbitrary invalid memory access. Keep cleanup idempotent.

Existing ABI 1 plugins without the export remain supported. Older runtimes ignore
the export. Cleanup does not unmap the NRO or remove its custom-native handlers,
and is not guaranteed on a crash or process exit.

`<shv/input.h>` provides Nintendo-labeled GTA frontend controls and
`ScriptHook::Input::ShoulderChord`, defaulting to L + R + Y. Poll once per frame;
the chord fires once per complete press. These are GTA control actions rather
than raw Joy-Con button reads. Keyboard behavior needs explicit adaptation.

`<shv/ini.h>` provides `ScriptHook::Util::Ini<Bytes, Entries>` with no heap allocation.
Call `Load("config.ini")` once, then `Get(section,key,fallback)`,
`Int(section,key,fallback,min,max)` or `Bool(section,key,fallback)`. Defaults are
4096 bytes and 64 entries; maximums are 16384 bytes and 128 entries. It accepts
UTF-8, comments, sections and duplicate keys with last-value precedence. Malformed
input fails as a whole so callers retain defaults. Quoted/escaped values and
automatic writes are not supported. File reads use the plugin's existing namespace.

## Native call status

| Value | NativeCallStatus |
| --- | --- |
| 0 | Ok |
| 1 | NotInitialized |
| 2 | NoScriptContext |
| 3 | WrongThread |
| 4 | NoActiveCall |
| 5 | ArgumentOverflow |
| 6 | MissingHandler |
| 7 | InvalidHandler |
| 8 | InvalidPointer |
| 9 | InvalidVectorCount |
| 10 | DepthExceeded |
| 11 | HashMismatch |
| 12 | InvalidContext |
| 13 | ReentrantContext |
| 14 | StringTooLong |

Native BOOL outputs require four-byte BOOL storage. INTs occupy the low 32 bits
of eight-byte slots. Vector3 uses padded slots, not a packed three-float PC
structure. Opaque reference storage needs an independently verified contract.
See [native caller](NATIVE-CALLER.md), [porting](PORTING.md) and [public SDK](PUBLIC-SDK.md)
for limits, manager policies, NSC bindings and provider lifetime.
