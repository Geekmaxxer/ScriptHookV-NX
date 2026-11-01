# Heap capacity and accounting

The unreleased growing-heap runtime expands allocation records, arena records
and the pointer index on demand. The public memory ABI and pointer-ownership
rules are unchanged. Older runtime artifacts retain their original capacity.
See the [verification record](../verification/HEAP-CAPACITY1.md) for exact coverage.

## Growth and admission

| Structure | Initial storage | Growth |
| --- | --- | --- |
| Owner records | 16 records | Stable pages of 16 records |
| Allocation records | 2,048 records | Stable pages of 256 records |
| Arena records | 128 records | Stable pages of 16 records |
| Pointer index | 4,096 entries | Doubles before live load exceeds three quarters |

Initial extents do not cap later allocation counts. New metadata pages require
available shared plugin budget and successful platform allocation. Record pages
preserve addresses and owner tokens. Index growth rebuilds each live record's
lookup slot without moving its payload. Freed records and arenas are reused;
excess tombstones trigger a rebuild of the current index.

Admission validates size arithmetic and the existing 32-bit index encodings.
The index reserves zero and UINT32_MAX, so representable record IDs remain a
separate bound. Alignment remains a power of two through 4,096 bytes, with at
least 16-byte returned alignment. Fragmentation, budget, address-space or OS
allocation failure can refuse a request before representational limits are reached.

## Accounting and retained storage

Each dynamic metadata mapping is page-rounded and reserved against
`plugin_memory_mib` before allocation. A failed mapping returns its reservation.
A successful mapping remains resident and accounted until game exit, including
retired pointer-index tables and reusable pages admitted before a later step fails.
Repeated attempts can reuse those pages without charging them again.

The loader reports these quantities in hexadecimal:

| Field | Meaning |
| --- | --- |
| `heap_metadata_bytes` | Retained additional heap-record and index mappings |
| `heap_block_capacity` | Admitted allocation-record capacity, including inline records |
| `heap_arena_capacity` | Admitted arena-record capacity, including inline records |
| `heap_lookup_capacity` | Current pointer-index entry count |

`heap_metadata_bytes` is included in both the shared `plugin_memory_accounted`
total and `runtime_metadata_bytes`. These are overlapping views; do not add them
together. Inline metadata resides in the kernel image and is excluded from the
additional-mapping counter. Per-owner `ShvNxMemoryStats.charged_bytes` describes
owned payload arenas, excluding shared metadata.

Free releases a payload record. Trim releases empty owned arenas after both
platform release phases succeed. Neither operation unmaps shared metadata.
Arena occupancy is tracked in 16-byte units to skip full arenas before searching
their gaps; this does not guarantee space for a fragmented or aligned request.

## Ownership and execution

Kill/Load and natural completion retain unregistered allocations for resident
handlers. Use [registered ownership](OWNED-RESOURCES.md) for objects that must
be reclaimed on whole-plugin cancellation. Rejected initialization can discard
unpublished allocations across every admitted record page. Release failures
retain their ownership and charges for retry.

Growth, rehashing, zeroing and copying execute synchronously on the supported
owner thread. Yield between groups of allocations. A count accepted by local
emulation does not establish physical frame time, RAM use or fragmentation
behavior. The [combined probe](../../examples/heap-records/README.md) exercises
record/index growth with the default budget; larger arena configurations and
measured long sessions remain separate hardware gates.
