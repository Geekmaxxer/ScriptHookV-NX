# Owned resources

The unreleased resource service retains explicit cleanup registrations outside
cooperative stacks. It can destroy a registered heap object after whole-plugin
cancellation, even when its local owner was suspended inside `WAIT`. Normal C++
scope exit remains unchanged; cancellation still discards suspended stack frames.

Bounded ownership workloads have physical acceptance. The subsequent
[growing-resource candidate](../verification/RESOURCE-CAPACITY1.md) has local
ARM64 verification and requires its own physical acceptance. Earlier results
retain their exact [artifact scope](../verification/PLUGIN-RESOURCES1.md).

## Availability and ABI

Include `script.h` or `plugin_resources.h` and check
`scriptResourcesAvailable()`. The independent `ShvNxResources` ABI 1 table is
56 bytes on AArch64 and is bound before explicit initialization. Base plugin,
extension, memory and file contracts retain their existing layouts. Older
runtimes may omit this service; a plugin that requires it must reject initialization.

Each owner starts with 64 inline registrations. The current candidate admits
additional stable pages of 64 records against the shared plugin memory budget;
allocation or budget failure refuses a new registration without changing live
handles or pins. Successfully admitted pages remain accounted and reusable until
game exit. Additional owner pages use the same accounting. Older runtime artifacts
retain their original 64-record limit. `SHV_NX_RESOURCES_PER_OWNER` is retained as
a compatibility spelling for the initial extent, not a runtime capacity query.

Operations belong to the loader thread. A cleanup function must reside in that
plugin's executable text.
Registrations receive nonzero 64-bit handles, are owner-specific and never reuse
a consumed handle during the process lifetime. Exhaustion returns zero.

## Registration and release

| Operation | Contract |
| --- | --- |
| `scriptResourceRegister(cleanup, context)` | Register bounded cleanup; return zero on refusal |
| `scriptResourceRelease(handle)` | Consume a valid registration, unpin its context, then invoke cleanup |
| `scriptResourceForget(handle)` | Remove the registration without invoking cleanup; transfer responsibility to the caller |
| `scriptResourceActive(handle)` | Test an active registration belonging to this plugin |
| `scriptResourcePinned(pointer)` | Test whether an exact context allocation is pinned |
| `scriptResourceCount()` | Return this plugin's active registration count |

A non-null context must be the exact start of a live allocation in the owning
plugin's memory service. Stack, static, interior and foreign allocation pointers
are refused. One allocation can have one registration. Null contexts are allowed
for callbacks that reference explicitly initialized module state, including
plugins without a bound memory table.

Registration pins the context against free and realloc, including realloc to
zero. `ScriptHook::Memory::Delete` checks the pin before invoking a destructor.
Ordinary C++ `delete` invokes the destructor before reaching the allocator and
must not be used on a registered object. Pinning protects the context allocation;
the callback remains responsible for any separately allocated children.

Release checks its execution preconditions before consumption. An unfinished raw
native call defers release and leaves the registration active. Once invoked, the
registration is consumed before the callback so that cleanup can destroy and free
its context. A false result can therefore mean a callback violated its contract
after consumption. Check `scriptResourceActive(handle)` before retrying; an
invoked callback is not repeated automatically.

## Lifecycle and callback rules

Whole-plugin stop cancels owned entries and disables callbacks/textures, invokes
the explicit `shvPluginStop` callback, then drains remaining resource registrations
in reverse registration order. Natural completion follows the same process after
all entries, callbacks and timed draws finish. Rejected initialization and failed
prepublication setup drain registered resources before heap discard and unmapping.
Stopping one entry does not drain resources while other plugin activity remains.

Resource callbacks must be bounded, must not throw and must not call `WAIT` or
leave an unfinished raw native call. Ordinary native calls require a valid game
context. The runtime diagnoses attempted waits and unfinished calls, abandons the
latter and restores the surrounding native status. Callback execution contributes
to cleanup timing diagnostics. It cannot preempt a loop or recover from invalid
memory access.

New registrations are refused while stopping, draining or executing a resource
callback. At most 64 cleanup calls may be active for one owner; a deeper nested
release is refused before consumption and leaves that child's context pinned.
The caller must retain responsibility for deferred work. A callback may release an existing child registration; consumed entries
cannot run twice. Forgetting another registration explicitly transfers its
cleanup responsibility out of the drain.

If callback preconditions prevent invocation, the live registration and pinned
allocation remain retained. Stop processing does not schedule automatic retries.
Resident mappings and raw unregistered allocations continue to follow their
existing lifetimes. Cleanup is not guaranteed after a crash, game exit or power
loss, and at-most-once invocation does not establish success of an external
operation performed by the callback.

Loader reports include `resource_count` and `resource_failures`. The latter counts
registry-level registration refusals and invoked callback violations; it is not
an aggregate of all rejected API calls. Inspect remaining counts when accepting
a workload. The candidate also reports per-owner `resource_capacity` and global
`resource_metadata_bytes`, in hexadecimal. These additional mappings appear in
both `plugin_memory_accounted` and `runtime_metadata_bytes`; the counters overlap
and must not be summed. Per-owner heap charges exclude shared registry metadata.

The registry maintains reverse order through stable links and reuses consumed
records. Heap records track pins directly, avoiding a registry scan on every
free/realloc. Explicit handle lookup scans live registrations. Growth and cleanup
remain synchronous; these data structures establish no measured frame-time bound.

## Managed heap objects

`shv/managed.h` supplies move-only `ScriptHook::Memory::Managed<T>`. Enable the
explicit `c_runtime` and `cxx_runtime` project options and require both memory and
resource services. `T` must satisfy the existing nothrow construction/destruction
requirements and its destructor must follow the callback rules above.

```cpp
#include "script.h"
#include "shv/managed.h"

struct State {
    uint64_t frames = 0;
};

void Main() {
    auto state = ScriptHook::Memory::Managed<State>::TryMake();
    if (!state) return;
    for (;;) {
        {
            State* current = state.Get();
            if (!current) return;
            ++current->frames;
        }
        WAIT(0);
    }
}
```

`TryMake` returns an empty owner on allocation or registration failure and cleans
up a newly constructed object if registration fails. Scope exit and `Reset` release
the ticket. `Get` returns a borrow only while the ticket remains active. Reacquire
it after waits and never retain it across cancellation or external release.
`Detach` forgets the registration and returns the live pointer; the caller must
subsequently destroy/free it. Move construction transfers ownership. Move
assignment first resets the destination and transfers only if reset succeeds.
Complete pending raw native calls before reset, destruction or move assignment.

The [complete example](../../examples/managed-resources/README.md) owns a dynamic
string and vector across waits and releases their nested storage on cancellation.
This facility provides explicit registered ownership, not global constructor
ordering, exception unwinding, hosted STL compatibility or arbitrary stack cleanup.

For an object shared across source files and cooperative entries, use the explicit
[SessionLocal contract](SESSION-STATE.md). It retains one registered object per
type/tag and recreates it after cancellation, without a global dynamic constructor.
