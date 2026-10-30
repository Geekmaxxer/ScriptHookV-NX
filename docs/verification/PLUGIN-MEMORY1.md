# Plugin Memory Test 1

Date: 2026-10-29. Unreleased runtime/SDK candidate. No SD installation, public
release replacement or new Switch acceptance is performed.

`plugin_memory.h` adds plugin-owned allocation, calloc, realloc, alignment,
free/size queries, statistics and explicit release of unused pools. The separate
sized `ShvNxMemory` table has memory ABI 1; base plugin, caller and extension ABI 1
layouts are unchanged. Loader binding occurs before initialization through optional
`shvPluginBindMemory`. Older plugins may omit that export. Older runtimes supply
no table: requiring plugins must check `scriptMemoryAvailable()` before proceeding.
`SHV_NX_CAP_PLUGIN_MEMORY` identifies runtime support.

## Public contract

| API | Behavior |
| --- | --- |
| `scriptAllocate(bytes, alignment, flags)` | Nonzero payload; power-of-two alignment 1-4096, returned alignment at least 16; flags 0 or `SHV_NX_MEMORY_ZERO` |
| `scriptCalloc(count, elementBytes)` | Overflow-checked multiplication and zeroed payload; zero size returns null |
| `scriptReallocate(pointer, bytes)` | Preserves alignment/prefix; growth failure preserves the old block; null allocates; zero size frees and returns null |
| `scriptFree(pointer)` | Exact live owned pointer; null succeeds with supported service/thread; interior/foreign/already-freed pointers fail |
| `scriptAllocationSize(pointer)` | Requested payload bytes, or zero for an unrecognized pointer |
| `scriptGetMemoryStats(out)` | Full writable output: current/peak requested and charged bytes, allocations/arenas and failure counters |
| `scriptTrimMemory()` | Release empty owned arenas; live allocations remain valid |

All operations use the module's engine/OS owner thread, including bounded
initialization, scripts, callbacks, cleanup and resident native handlers. Worker
allocation/free is unsupported; reentry fails without acquiring the allocator
again. These are trusted native plugins, not memory-isolated processes.

```cpp
#include "script.h"
namespace {
void* buffer{};
bool Initialize() {
    if (!scriptMemoryAvailable()) return false;
    buffer = scriptCalloc(1024, sizeof(float));
    return buffer != nullptr;
}
void Main() { while (true) WAIT(0); }
}
SHV_NX_PLUGIN_WITH_INIT("memory-example", Main, Initialize)
```

This example retains module-owned data across Kill/Load until game exit. Real
cleanup can explicitly free unneeded data and Trim. Initialization runs once;
restarted Main must recreate anything cleanup freed. malloc/free, new/delete and
`std::allocator` are not silently redirected. The subsequent [library profile](PLUGIN-LIBRARIES1.md)
explicitly opts into a C allocation/string subset and pinned XML memory DOM.
The subsequent [C++ profile](PLUGIN-CXX1.md) adds explicit nothrow allocation and
fallible containers; it does not redirect ordinary throwing allocation.
Full CRT/STL, exceptions and constructors remain separate
[roadmap](../PC-RUNTIME-ROADMAP.md) work.

## Capacity and lifetime

Small allocations share reusable 64 KiB arenas; large requests use page-rounded
arenas. Charge each arena once to the existing shared plugin budget alongside
images/textures. Heap pages have no extra mapped-copy charge. Requested payload
and charged capacity differ; Free retires blocks, Trim releases empty pools.
Global `plugin_memory_accounted` includes heaps. Per-plugin heap statistics remain
separate from the manager's image/BSS figure.

Bounds are 16 owners, 128 arenas and 2,048 live allocations shared across plugins.
A bounded 4,096-slot pointer index and linked block metadata avoid repeated full
scans for ordinary lookup/removal; freed records are reused. Fragmentation and
metadata/arena/OS/budget exhaustion can still refuse allocation. Core metadata
memory is separate from plugin accounting. Large zero/copy operations remain
synchronous; these bounds do not promise a measured frame-time cost.

Kill/Load and normal completion retain live allocations because resident code and
published native handlers may still reference them. Cancellation does not unwind
suspended C++ stack objects. Cross-plugin free/realloc/size queries fail. As with C
pointers, a pointer becomes invalid after free/realloc and must not be reused if
its address later belongs to a new allocation.

Realloc may temporarily need both old and new blocks. Failed growth preserves old
data. Failed initialization before publication/scheduling permits loader discard
before code unload; failed cleanup retains ownership/accounting and may leave that
unpublished module resident until exit. Published handlers forbid blanket discard.

## SDK release and verification

SDK release has two result-bearing phases: FreeMemoryPages, then FreeAddressRegion.
After successful page release, address-only retries never release the pages again.
Failed arenas are not reused and remain charged until both phases succeed.
Bounded IDA 9.5 analysis verifies initialized SDK segments, public wrappers and
VammManager bodies. Assembly confirms returned results where inferred pseudocode
displays void. The private database closes unsaved with its hash unchanged. No
original source prototype/error constants are invented; full decomp and RAGECOOPV-NX
feature work remain deferred.

528 heap checks execute two real NROs, initialization calloc/Trim, alignments,
mixed allocation sequences, moved/in-place realloc, metadata/arena exhaustion,
foreign/wrong-thread refusal, Kill/Load, bounded reentry, OS failure and both release
phases. Existing ABI 1 binaries, manager, extension, capacity, lifecycle, large-
module, configuration, scheduler and native suites are checked separately against
the same candidate. Production ARM64 heap/shim/loader/plugin code executes locally;
OS allocation/release, filesystem and RO boundaries remain modeled.

Retained authored source/hash/local reports:
[`plugin-memory1-local-20261029/verification.json`](../../evidence/plugin-memory1-local-20261029/verification.json).
No game modules, raw target disassembly/pseudocode, IDA databases or installable
binaries are included. The accepted runtime reference remains unchanged.

Physical acceptance must cover actual Horizon allocation/release, NSC/NRO
coexistence, initialization/cleanup, sustained churn, low memory/capacity refusal,
Kill/Load, pause/loading/death/character transitions and a measured long soak.
Local tests establish neither multicore safety nor STL/XML or arbitrary PC-mod
compatibility. No installed runtime or public release is changed.
