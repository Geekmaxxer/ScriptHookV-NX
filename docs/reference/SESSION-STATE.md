# Shared C++ session state

`shv/session.h` supplies `ScriptHook::Memory::SessionLocal<T, Tag>` for source
ports that need one shared object across several source files and cooperative
entries. The first explicit `TryGet` constructs it after service binding. Owned
resource cleanup destroys it on Kill, natural completion or rejected initialization;
a later Load can construct a fresh object. No production runtime or ABI change is
required beyond the existing memory and owned-resource services.

## Availability and access

Enable `c_runtime` and `cxx_runtime`, include the same complete `T` and `Tag`
declarations in each translation unit, and call on the runtime owner thread.
Construction/destruction must be `noexcept`, bounded and non-yielding. The
allocation alignment limit remains 4,096 bytes. Missing services or refused
allocation/registration return null. A C-runtime module without its required
memory binding can be rejected before entry.

```cpp
#include "shv/session.h"
#include "shv/containers.h"

struct SharedState {
    ScriptHook::Containers::String label;
    explicit SharedState(uint32_t initial) noexcept : revision(initial) {}
    uint32_t revision;
};
using CurrentSession = ScriptHook::Memory::SessionLocal<SharedState>;

void Example() {
    auto* state = CurrentSession::TryGet(1);
    if (!state || !state->label.TryAssign("session")) return;
    // Reacquire CurrentSession::Get() after any WAIT before using the object.
}
```

| Operation | Contract |
| --- | --- |
| `TryGet(args...)` | Construct and register once for this active type/tag, or return its existing borrow |
| `Get()` | Return a borrow only while the current owner has an active registration |
| `Reset()` | Release through registered cleanup; return false if preconditions or callback rules fail |

The default tag is `void`. Different tags create independent slots for the same
type. Slots are shared within one NRO, including across translation units; each
NRO retains its own storage. They have no dynamic module constructor/destructor.
Arguments are used only for the first successful construction in a lifetime.
Function argument expressions still follow normal C++ evaluation rules.

## Lifetime and failure behavior

Each live object consumes one owned-resource registration and an exact owner-heap
allocation, plus any nested allocations. The original runtime admits 64
registrations per owner; the [growing-resource candidate](OWNED-RESOURCES.md)
admits additional stable pages under the shared budget.
Registration failure destroys the unpublished object and releases its allocation.
Later access can retry after a recoverable allocation failure. Fallible setup of
the object's contents remains explicit and must check its own results.

Recursive access through `TryGet` is refused while that same slot is constructing
or destroying. Cleanup clears the published pointer before invoking the destructor.
An unfinished raw native call can defer `Reset`; the slot remains intact until
cleanup is actually invoked. A refused owner/thread check also preserves it.
The helper provides no thread synchronization; concurrent access from arbitrary
threads is unsupported.

Pointers returned by `Get`/`TryGet` are borrowed. Do not `delete`, free, register,
transfer or retain them across a reset, cancellation or game exit. Reacquire after
cooperative waits. The [owned-resource rules](OWNED-RESOURCES.md) govern callback
ordering, non-yielding destructors and consumed callback failures. A false Reset
result does not establish that cleanup was never invoked.

Objects are ordered by explicit construction and reverse resource cleanup, not
by source-file order. Group mutually dependent members in one state object when
their construction/destruction ordering matters. No automatic retries occur after
callback contract violations or a crash.

This facility is an explicit alternative for adapted session state. It does not
enable compiler-managed global constructors, `atexit`, static-local destructor
registration, exceptions, stack unwinding or a hosted STL. Ordinary global
constructor/destructor sections remain rejected. The complete
[two-source example](../../examples/session-state/README.md) and
[verification record](../verification/SESSION-LOCAL1.md) retain the supported scope.
