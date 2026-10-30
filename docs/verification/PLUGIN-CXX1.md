# Plugin C++ Test 1

Date: 2026-10-29. Unreleased SDK candidate using the unchanged
[owner-heap core](PLUGIN-MEMORY1.md). No installation, public release replacement
or new physical Switch acceptance is performed.

This milestone supplies owner-backed nothrow C++ object allocation and fallible
growable vectors, strings and sorted maps. It exercises real object lifetimes,
move-only records and numeric XML registries. Full hosted STL and exception/static
initialization services remain separate work.

## Building

```powershell
python tools/build_plugin.py --source path/to/main.cpp --c-runtime --cxx-runtime --output out/cxx-mod --devkitpro C:/devkitPro
python tools/build_mod.py --project examples/container-registry/project.json --output out/container-registry --devkitpro C:/devkitPro
```

NRO project options are `"c_runtime": true` and `"cxx_runtime": true`.
The C++ option must be a JSON boolean, requires the C profile and is invalid for
an NSC project. Add `numeric_runtime: true` separately for floating XML data.
Defaults remain disabled. The Porter's general hosted-library blockers are not
automatically resolved by these options.

Build reports record `cxx_runtime_enabled`, source/dependency hashes, required
memory support and compiler stack frames. An absent memory service refuses entry.
Base runtime/plugin/extension ABI 1 layouts are unchanged. Accepted public cores
lack this candidate heap service. Allocation operators and bindings belong to
each NRO; objects must be destroyed by their owning module.

## Object allocation

Include `<new>` for explicit `new(std::nothrow) T` and array forms. The profile
provides global scalar/array nothrow allocation, sized/unsized delete and aligned forms.
Allocation failure returns null without constructing the object. Zero-byte
requests use at least one backing byte on success. Alignment is limited to 4,096
bytes. Compiler-managed array cookies mean an element pointer is aligned for its
type, and need not retain the backing block's minimum 16-byte alignment.

Ordinary throwing new remains unprovided. Returning null from that signature
would violate its [required behavior](https://eel.is/c++draft/new.delete.single).
No exception runtime, new-handler loop or general `std::allocator` replacement is
supplied. Unsupported dependencies still fail compilation or strict linking.
Global constructors/destructors remain rejected, including under section GC.

For checked single-object helpers, include `shv/memory.h` and run after binding,
for example from Main:

```cpp
struct Cache { int value; explicit Cache(int n) noexcept : value(n) {} };
void Example() {
    Cache* cache = ScriptHook::Memory::TryNew<Cache>(2699);
    if (cache) (void)ScriptHook::Memory::Delete(cache); // Clears cache on success.
}
```

`TryNew<T>` explicitly selects the global owner-backed allocator even when T has
class-specific allocation operators. It requires noexcept construction/destruction.
Ordinary new expressions retain C++ class-specific allocator lookup rules.
`Memory::Delete` requires
the complete allocated type, an exact live owned block of matching size, and
refuses foreign storage before invoking its destructor. Arrays, borrowed entities
and runtime type inspection are outside this helper. Stale pointers remain invalid
even if an address is later reused. A normal C++ delete expression invokes its
destructor before the allocator can check ownership; invalid deletion is not an
ownership test. Nothrow allocation does not make arbitrary initialization safe;
type-specific fallible setup still needs an explicit result.

## Fallible containers

Include `shv/containers.h` and use `ScriptHook::Containers`.

| Type | Supported behavior |
| --- | --- |
| `Vector<T>` | Growth, reserve/resize, copy/rvalue push and insertion, erase, checked `At`, explicit copy, move/swap, clear/reset |
| `String` | Byte spans or C strings, assignment/append, self-spans, reserve, explicit copy, move, NUL-terminated `CStr` |
| `FlatMap<K,V,Compare>` | Sorted contiguous entries, logarithmic lookup, insertion/assignment/erase, reserve, explicit copy and indexed access |

Mutations return results. Failed allocation/overflow preserves existing elements,
string bytes/terminator or map entries. `TryCopyFrom` builds a temporary buffer
before replacing the destination, so both allocations may coexist. Map insertion
returns `InsertResult::Inserted`, `Exists` or `NoMemory`. Existing-value assignment
can avoid allocating a new map entry.

Vector elements require noexcept move construction/destruction. Copy methods also
require noexcept copying; resize requires noexcept default construction. Elements
need not support assignment. Self-references are rebound after growth; overloaded
address-of operators do not redirect storage operations. Moves transfer buffers
without allocation. Copy construction/assignment is disabled in favor of explicit
fallible copying. Check returned pointers and results before using data.

String lengths/capacities exclude the terminator. Explicit spans can contain NUL
or UTF-8 bytes; no encoding is validated. C-string overloads require terminated
input. Self-spans beyond initialized storage fail. Map keys require strict weak
ordering and a noexcept comparator. This initial map requires noexcept-copyable
keys/values for insertion/copy, and a stateless comparator for explicit copying.
An ID-to-index map can reference move-only records owned by a vector.

These helpers are not aliases for standard containers. FlatMap insertion/erasure
is linear; growth and shifts can invalidate addresses. Vector/string growth and
vector insertion/erasure also invalidate affected pointers. Clear destroys elements
and retains capacity; Reset frees the buffer. Empty heap arenas may remain until
Trim. The shared heap still has 2,048 live-block and 128-arena limits across modules,
plus its configured budget; individual dynamic strings consume additional records.
No container promises unbounded size or measured frame-time cost.

## Lifetime and complete workflow

All operations use the permitted owner thread. Constructors, moves, comparisons
and destructors must not throw, yield or reenter a mutation of the same container.
Keep long-lived state behind a trivial module-owned pointer, construct it after
binding, and delete it explicitly in cleanup. Initialization runs once; restarted
Main must recreate state that cleanup freed.

Completed scopes and normal returns run destructors. Kill does not unwind a
suspended stack. Do not leave sole ownership in a local RAII object across a yield
and expect cancellation to free it. Resident native handlers may retain data;
cleanup must follow the mod's publication/liveness policy.

The [registry example](../../examples/container-registry/README.md) combines all three
profiles. It imports numeric XML into a temporary vector of move-only records,
dynamic labels and an ID map, commits the complete in-memory registry, exports
settings, then destroys module state. It changes no game world state. In-memory
commit does not make raw SD writes transactional. Flat paths and the 64 KiB write
limit remain unchanged.

## Verification and remaining acceptance

540 C++ checks execute two real ARM64 NROs with scalar/array cookies, alignment,
lifetime counts, ownership refusal, overflow, allocation failure, 220 mixed vector
steps, 90 string steps, 190 map steps and a 4,096-entry registry. Cooperative Main
functions exercise move-only records and 256-byte-aligned storage on 64 KiB script
stacks. Negative builds check missing profiles, throwing new, global constructors
and unsupported construction/move traits. An isolated copied SDK also builds.

The combined example passes five workflow/restart/OOM checks; 2,689 numeric
regressions and 34 portable-tool checks also pass. The C++ fixture's largest
compiler frame is 240 bytes. This is not a complete call-chain bound or hardware
stack high-water measurement. Retained source/licenses/reports are in
[`verification.json`](../../evidence/plugin-cxx1-local-20261029-final2/verification.json).
OS allocator, filesystem and RO remain modeled. No game binaries, raw target
disassembly, databases or installable modules are retained in that evidence.

Physical acceptance requires actual Horizon/RO behavior, sustained allocation and
cleanup, low memory/fragmentation, concurrent mods, restart/transitions and measured
RAM/frame-time/long-session results. Hosted STL, exception/static initialization
and bounded streaming/transactional files remain open. No Menyoo port or new game-
native compatibility is certified by this milestone.
