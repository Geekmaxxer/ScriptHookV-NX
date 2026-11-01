# Owner-backed hash maps

`shv/hash_map.h` provides fallible associative storage for adapted C++ registries.
It supports move-only keys/values, checked growth and allocation-free string-view
lookups through the existing owner-memory service. This is an SDK container;
the runtime binary and public ABI layouts are unchanged.

Use the C/C++ project profiles for the SDK string/managed-object workflow and
check the installed memory/resource services. Construct containers explicitly
after binding. Hash maps do not enable ordinary throwing allocation, hosted
`std::unordered_map`, global constructors, exceptions or arbitrary-thread access.

```cpp
#include "shv/hash_map.h"
using namespace ScriptHook::Containers;

void Example() {
    HashMap<String, uint32_t> names;
    String key;
    if (!key.TryAssign("vehicle-preset")) return;
    if (names.TryInsert(std::move(key), uint32_t(2699)) != HashInsertResult::Inserted) return;
    const auto* value = names.FindAs(StringKeyView{"vehicle-preset", 14});
    // Consume this borrow before mutation or a cooperative wait.
    if (value) { /* use *value */ }
}
```

## Operations

| Operation | Contract |
| --- | --- |
| `Size()` | Number of live entries |
| `Capacity()` | Number of entries admitted without growing the bucket table |
| `BucketCount()` | Physical bucket count; zero or a power of two |
| `TryReserve(entries)` | Grow for at least this many entries; preserve contents on refusal |
| `TryInsert(key, value)` | Insert a new key; leave an existing entry unchanged |
| `Find(key)` / `FindAs(query)` | Return a value borrow or null; allocate nothing |
| `Erase(key)` / `EraseAs(query)` | Destroy one matching entry and repair its probe chain |
| `KeyAt(bucket)` / `ValueAt(bucket)` | Inspect occupied buckets; null for empty/out-of-range buckets |
| `TryCopyFrom(other)` | Replace transactionally; available only for nothrow-copyable keys/values |
| Move / `Swap` | Transfer or exchange owned storage without allocation |
| `Clear()` / `Reset()` | Destroy entries, retaining or releasing the bucket table respectively |

Iterate `0..BucketCount()-1`, checking `KeyAt` for null. This order is unspecified,
can change on growth/erasure and is not a sorted or insertion order. Keys are
read-only through the API because changing one would invalidate its stored hash.
Values may be updated through a valid `Find`/`ValueAt` borrow.

`HashInsertResult` distinguishes `Inserted`, `Exists`, `NoMemory` and
`AliasedArgument`. Duplicate insertion neither replaces the value nor consumes
moved arguments. Reserve/insertion allocation failure preserves existing entries
and does not move supplied arguments. Arguments already evaluated at the call
site retain normal C++ evaluation behavior.

New insertion refuses keys or values borrowed from that map's bucket storage,
because growth can relocate it. Copy the needed value to independent storage
first. It also refuses moving the same external object into both fields. Existing
borrowed keys can be used for duplicate lookup and erasure. These checks do not
validate arbitrary pointers or make dangling borrows safe.

## Types, hashing and ownership

Keys/values must have nothrow moves/destructors and alignment at most 4,096 bytes.
Insertion additionally requires nothrow construction from its selected argument
types. Copying requires nothrow copy construction; move-only string keys should
be moved into their entry. No implicit insertion operator is supplied.

Default hashing covers integral and enum keys. The `String` specialization hashes
its explicit bytes, including embedded NULs. `StringKeyView` borrows a valid byte
span for lookup/erasure; zero bytes allow a null pointer. Query strings must remain
readable throughout the call. No allocation, locale folding or Unicode
normalization is performed for lookup.

Custom hash/equality functors must be stateless, nothrow-default-constructible,
bounded and non-yielding. Equal keys must produce equal hashes. A heterogeneous
query must follow the same rule and provide compatible nothrow overloads. Hashes
must remain consistent while entries are stored; callbacks must not reenter or
mutate that map. The byte hash is not designed for hostile-input resistance.

The table uses linear probing with a maximum three-quarter load and backward-shift
erasure. Ordinary lookup is expected constant time with a suitable distribution;
collisions can make operations linear. Growth moves all entries synchronously.
Capacity arithmetic is checked before allocation. No timing/preemption guarantee
is implied by finite probes or cooperative callers.

Structural mutations can invalidate all key/value pointers and bucket indices.
Do not retain borrows across mutation, `WAIT`, cancellation or owner reset.
Containers must be destroyed by their owning plugin. Use
[managed ownership](OWNED-RESOURCES.md) or [session state](SESSION-STATE.md) when
a populated map must survive waits and still be reclaimed on cancellation.
Normal lexical destruction does not unwind suspended frames after Kill.

See the [complete registry example](../../examples/hash-registry/README.md) and
[local verification](../verification/HASH-MAP1.md). Physical acceptance is merged
with pending texture/library and heap-growth checks in service batch 7.
