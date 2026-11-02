# Fallible queues and captured actions

`shv/deque.h` and `shv/function.h` supply owner-backed queues and type-erased
actions for exception-free source ports. They use existing memory/resource
contracts and change no kernel, native binding or public ABI. They are separate
types rather than aliases for hosted `std::deque` or `std::function`.

Enable the C/C++ profiles and construct state explicitly after service binding.
All mutation, invocation, element construction/move/destruction and allocation
belong to the supported owner thread. None may throw, yield or reenter a mutation
of the same instance. Store long-lived state behind registered ownership when
whole-plugin cancellation must reclaim it.

## Deque

`ScriptHook::Containers::Deque<T>` uses a single aligned ring allocation. Both
ends can grow or retire elements without shifting the remaining sequence.

| Operation | Contract |
| --- | --- |
| `TryPushFront`, `TryPushBack` | Move or explicitly copy one element; allocation failure preserves the queue and input |
| `PopFront`, `PopBack` | Destroy one element; return false when empty |
| `TryReserve(n)` | Grow capacity without changing logical order, including wrapped contents |
| `At`, `Front`, `Back` | Borrow an element, or return null out of range/when empty |
| `TryCopyFrom` | Explicit transactional copy; failure preserves source and destination |
| `Size`, `Capacity`, `Empty` | Inspect current logical size and admitted storage |
| `Clear`, `Reset` | Destroy all elements; Clear retains storage, Reset frees it |
| Move/`Swap` | Transfer ownership without allocation; copying through constructors/assignment is disabled |

Elements must be nothrow-movable and nothrow-destructible, aligned through 4,096
bytes. Copy operations additionally require nothrow copy construction. Size and
byte multiplication are checked before allocation. Growth moves elements into
new storage and frees the old allocation; it can temporarily require both buffers.
Self-copy is a no-op.

A push may borrow a live element from the same deque. Its logical index is
resolved before growth and reacquired after relocation. An rvalue self-push
leaves the original element in its type's valid moved-from state. Failure before
growth does not consume that input. Input must designate a live complete element;
unused ring slots and arbitrary interior/overlapping storage are not inputs.

Growth invalidates all element pointers. A pop invalidates the removed element;
other live elements remain in place until later growth/reset. Storage is not
contiguous in logical order after wrapping: iterate through At rather than
performing pointer arithmetic from Front. Deque provides no iterator/container
ABI compatibility with the standard library. Growth/copy/clear are synchronous.

## Captured callables

`ScriptHook::Callbacks::Function<void(Args...)>` is a move-only callable wrapper
with a 64-byte, 16-byte-aligned inline target buffer. Larger or over-aligned
targets allocate from the owner heap; nested allocations inside an inline target
still belong to that target's own lifetime.

| Operation | Contract |
| --- | --- |
| `TryAssign(target)` | Stage a new target, then replace the old binding; allocation failure preserves the old binding and source |
| `TryInvoke(args...)` | Invoke a bound target; return false if empty; no exception on empty invocation |
| `TryCopyFrom` | Explicitly clone a nothrow-copyable target; return false for move-only targets or allocation refusal |
| `Target<T>()` | Borrow the exact stored object type, or null for a different type; no RTTI |
| `UsesHeap`, bool conversion | Inspect wrapper-owned heap storage and binding presence |
| `Reset` | Clear the binding, destroy the target and free wrapper-owned storage |
| Move | Transfer a binding without allocation, leaving the source empty |

Targets must have nothrow construction from the supplied argument, move,
destruction and invocation. Target alignment is limited to 4,096 bytes. Function
references, ordinary function pointers and move-only functors/captures are
supported. A typed null function pointer clears the binding. Wrap member pointers
in a suitable noexcept lambda; automatic member-pointer binding is not provided.
Only void signatures are implemented. Value-producing callables may have their
result discarded when bound to a void signature.

Replacing a target with a direct borrow of that same stored target is refused
before consumption. Query with the exact non-cv stored object type. A target
borrow becomes invalid after replacement/reset/move or owner cleanup. Do not
retain it across cancellation. Captured references and pointers do not acquire
ownership; their referents must outlive every invocation.

Do not mutate, move, reset or destroy a Function while it is invoking its target.
Invocation is synchronous and does not schedule work or create a thread. Target
destructors must follow the same bounded, non-yielding ownership rules as other
registered objects. Clearing the published binding precedes target destruction.
There is no general recovery from callback memory corruption or an infinite loop.

## Queue ownership and cancellation

```cpp
#include "shv/deque.h"
#include "shv/function.h"
#include "shv/managed.h"

struct State {
    ScriptHook::Containers::Deque<ScriptHook::Callbacks::Function<void()>> actions;
};

void Main() {
    auto owner = ScriptHook::Memory::Managed<State>::TryMake();
    if (!owner) return;
    // Build/invoke bounded groups using owner.Get(); release all local owning
    // temporaries before WAIT, then reacquire the registered state after it.
}
```

Managed cleanup destroys the queue, its callable targets and their nested owned
data after Kill. It does not invoke pending actions, unwind abandoned stack
locals, undo game mutations made by already executed actions or roll back external
operations. Plain unregistered allocations retain their existing resident lifetime.

The [complete example](../../examples/task-queue/README.md) combines wrapping,
relocation, inline/heap/string captures, explicit yields and final ownership checks.
The [verification record](../verification/TASK-QUEUES1.md) separates local evidence
from the shared pending hardware batch. These facilities do not establish hosted
STL, exceptions, global constructor policy or full large-mod compatibility.
