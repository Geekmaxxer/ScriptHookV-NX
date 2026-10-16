# External plugin API 1

Include `script.h` for the common interface. Use `plugin.h` directly only when
supplying an explicit descriptor and initialization callback. One NRO exports
one `shvPluginQuery`, with a side-effect-free query, short initialize and one
cooperative run entry. Global constructors/destructors and unresolved imports
are rejected by the builder/loader.

| API | Contract |
| --- | --- |
| `SHV_NX_PLUGIN("id", ScriptMain)` | Define an entry with a compile-time checked 1-31 character ID |
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
