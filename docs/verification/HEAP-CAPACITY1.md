# Growing heap metadata and service batch 7

Date: 2026-11-01. Unreleased runtime candidate. This change replaces the initial
allocation-record, arena-record and pointer-index ceilings with budget-accounted
growth. Public ABI layouts, target bindings, defaults and published runtime
artifacts remain unchanged. The [capacity contract](../reference/HEAP-CAPACITY.md)
defines admission, retained metadata and diagnostic accounting.

## Implementation and local verification

- [x] Admit stable allocation, arena and owner pages through checked shared-budget reservations.
- [x] Grow and rebuild the pointer index while preserving live pointers and owner isolation.
- [x] Return failed mapping reservations and reuse successfully admitted metadata.
- [x] Reclaim allocations beyond the initial record extent after rejected initialization.
- [x] Verify 8,000 simultaneous allocations across two owners and 160 arenas under an explicit 16 MiB budget.
- [x] Exercise record-page, index and arena-page failure/retry, realloc, record reuse and tombstone rebuilding.
- [x] Preserve unregistered resident data through Kill/Load and verify independent owner Trim.
- [x] Recheck baseline heap, scheduler, resource, hash-map and texture behavior on the new kernel.
- [x] Execute the packaged probes through two complete lifetimes and managed cancellation/restart locally.
- [x] Consolidate all pending library, registry and texture checks with the new heap probe in one kit.
- [x] Reproduce the runtime and every combined-kit file from an isolated source ZIP.
- [x] Accept the combined workload on physical Switch hardware. See [Batch 7 scope](SERVICE-BATCH7.md).
- [ ] Measure larger arena configurations, fragmentation, frame time and extended coexistence on hardware.

The [local record](../../evidence/heap-capacity1-local-20261101/verification.json)
identifies source hashes, kernel, probes, test results and packaging evidence.
Production ARM64 code executes locally; OS allocation/release, filesystem, RO,
GPU and outer game scheduling remain modeled. The stress suite uses a larger
emulator instruction allowance, which establishes no physical timing bound.
No original game source, private databases or target dumps are distributed.

Eight local runtime suites pass 1,462 checks; the portable source suite passes
39 tests. An isolated build reproduces the kernel and all 99 kit files, including
four NROs and 80 PNGs. These counts exclude physical acceptance.

The kernel SHA-256 is
`e65509d418d98a687049fff452ad3e863455b114cdec821eb674e4165f76abfc`.
The earlier memory and hash-map records retain their original artifact scope.
Their local or physical results do not automatically accept this modified kernel.

## Physical Batch 7 result

[Batch 7 acceptance](SERVICE-BATCH7.md) verifies the completed record/index
workload on this exact kernel under an explicit 16 MiB budget. The final report
has run=3, failed=0, peak_allocations=4097 and complete owner cleanup. The four
probes need no separate repeat installations. Cancellation, broader arena
profiles and measured performance remain open.

## Original combined hardware batch

[Service batch 7](../../examples/service-acceptance/README.md) contains four manual
probes: texture-catalog, library-services, hash-registry and heap-records. It
supersedes pending installation checks for graphics candidate 1 and batches 5/6.
Historical kits remain archived. No separate hardware session is requested for
those earlier probes or for this implementation milestone.

The runtime candidate is separate from the probe/assets tree. Existing settings,
bindings, game files and saves are preserved; no runtime.ini is included. The
guide defines report preservation, repeated completion, optional cancellation,
normal gameplay/transitions and rollback in the same consolidated workflow.

The [heap example](../../examples/heap-records/README.md) verifies 4,096 child
buffers plus their registered root, then frees all owned data. This accepts
record/index growth only when run on hardware. The local 160-arena profile uses
an explicit 16 MiB budget and is not represented by that default-budget probe.
Broader profiles, physical low-memory behavior, GPU correctness, media failures,
stack measurements and long sessions remain independently tracked.
