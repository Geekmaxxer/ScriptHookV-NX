# Hash-map registries and consolidated batch 6

This SDK milestone adds owner-backed hash maps for larger source-port registries.
It uses the unchanged growing-texture runtime and existing memory/resource
contracts. No game offsets, native signatures, public ABI layouts or installed
configuration are changed.

## Implementation and local verification

- [x] Provide fallible insertion, reserve, lookup, erasure, copy and move ownership.
- [x] Support move-only string keys/values and allocation-free explicit-byte lookup.
- [x] Verify mixed mutations against independent dictionary models.
- [x] Exercise wrapped collision chains and move-only object destruction.
- [x] Verify 6,144 keys, reserve/insertion/copy failures and overflow refusal.
- [x] Refuse unsafe aliased insertion arguments without consuming them.
- [x] Verify embedded NULs, Unicode bytes, alignment and owner isolation.
- [x] Reclaim nested maps on cancellation and reconstruct state on Load.
- [x] Refuse unsupported hash/move/copy policies through compiler checks.
- [x] Execute two complete registry runs and cancellation/restart locally.
- [x] Package one combined texture/library/registry batch and verify its inventory.
- [x] Reproduce the runtime and every combined-kit file from the exported source outside the workspace.
- [x] Accept the combined workload on physical Switch hardware. See [Batch 7 scope](SERVICE-BATCH7.md).

The [local record](../../evidence/hash-map1-local-20261101/verification.json) retains
source identities, artifacts, checks and acceptance limits. Production ARM64 code
executes the map, allocator, ownership and NRO workloads. Horizon, RO, filesystem,
GPU and outer game scheduling are modeled. These results do not establish physical
latency, fragmentation, stack limits or large-mod gameplay compatibility.

## Physical Batch 7 result

[Batch 7 acceptance](SERVICE-BATCH7.md) verifies run=2, failed=0, completed=1
and complete owner cleanup on its exact growing-heap kernel. Earlier local
artifacts retain their original scope; no standalone Batch 6 run is needed.

## Original combined hardware batch

The pending installation check below is superseded by
[service batch 7](../../examples/service-acceptance/README.md), which adds the
[growing-heap candidate](HEAP-CAPACITY1.md) and its probe. Batch 6 and this local
record retain their original runtime and three-probe scope. The later milestone
rechecks the unchanged probes on the modified runtime; no separate batch 6
hardware session is required.

[Service batch 6](../../examples/service-acceptance/README.md) supersedes the
pending standalone installation checks for graphics candidate 1 and library
batch 5. Their historical artifacts and local evidence remain unchanged. No
additional hardware run is requested during this implementation milestone.

One installation contains three manual probes: texture-catalog, library-services
and hash-registry. Run them in the same game session after installing the exact
candidate runtime once. The texture runtime, visual NRO, PNGs and library NRO are
byte-identical to their prior local checks; the new registry workload adds no
runtime binary change. Explicit rollback and report-preservation instructions are
included. No shared runtime.ini, saved settings or game archives are supplied.

The complete [registry example](../../examples/hash-registry/README.md) verifies
integer, named and deliberately colliding maps, yields between groups and reports
zero owned allocations after cleanup. The [container contract](../reference/HASH-MAP.md)
documents argument aliasing, pointer invalidation, type policies and synchronous
rehashing. Hosted STL and compiler-managed initialization remain separate work.
