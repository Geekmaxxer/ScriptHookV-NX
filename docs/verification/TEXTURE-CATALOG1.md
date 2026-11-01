# Growing texture cache

This candidate removes fixed PNG cache counts while preserving the extension ABI,
owner checks, audited renderer calls and existing image limits. It addresses a
concrete resource obstacle for sprite-heavy source ports; it does not establish
Menyoo feature parity or hosted graphics compatibility.

## Implementation and local verification

- [x] Admit stable cache records through shared-budget metadata pages.
- [x] Preserve cached IDs and uploads through catalogue growth and Kill/Load.
- [x] Cache 160 PNGs in one plugin and render initial/high IDs in layer order.
- [x] Refuse cross-owner, unpublished and invalid IDs.
- [x] Preserve reservations and prior records across allocation failures.
- [x] Retain and account reusable metadata after failed GPU upload.
- [x] Verify budget exhaustion, cached lookup and subsequent recovery.
- [x] Recheck PNG, callback, scheduling, module-capacity and catalogue behavior.
- [x] Build an 80-image example with cancellation and repeated completion checks.
- [x] Prepare a combined graphics/library candidate with manual startup policies.
- [x] Reproduce the runtime, probe and all 80 PNGs from the source ZIP outside the workspace.
- [x] Accept the bounded 80-image allocation/display workload and visual output in physical Batch 7.
- [ ] Accept broader transitions, capacity and measured performance on Switch.

The [local record](../../evidence/texture-catalog1-local-20261101/verification.json)
identifies the runtime, probes, source hashes, regression results and candidate
ZIP. Production ARM64 code performs real PNG decoding, metadata admission and
sprite argument submission. Horizon, RO, filesystem, GPU operations and outer
game ticks are modeled. No new target offsets or GPU destructor contracts are
introduced. Historical runtime acceptance does not transfer to this rebuilt module.

## Contracts and remaining limits

Sixteen records are inline. Each additional stable page holds 32 records and is
charged at its page-aligned extent to the shared plugin memory budget. IDs are
positive signed integers; an existing ID never moves when pages grow. Invalid or
foreign IDs are refused. Failed create attempts return zero without publishing
a record. Successfully admitted empty metadata pages remain reusable after upload
failure and are included in diagnostic accounting.

The existing flat filenames, 1024x1024 dimensions, 4 MiB encoded PNG size, 64
per-texture instance indices and 128 simultaneous draw instances remain in effect.
Cached texture resources stay resident until GTA exits, including after Kill.
Conservative GPU accounting is not a measurement of the actual graphics footprint.
Decode/upload work is synchronous; cooperative loading does not make each PNG
operation preemptible or guarantee a frame-time bound.

See the [extension contract](../reference/EXTENSIONS.md) for ownership, queue and
report-field details. Substantial mods still require native/memory/asset audits.

## Physical Batch 7 result

[Batch 7 acceptance](SERVICE-BATCH7.md) verifies all 80 installed assets,
run=2, failed=0, 360 frames and both retained-ID checks. Visual acceptance covers
five pages, rotation/transparency and no stale sprites. Broader transitions and
measured GPU/RAM/frame-time behavior remain open. No standalone graphics run
is required.

## Original consolidated hardware sequence

The pending installation check is now included in
[service batch 7](../../examples/service-acceptance/README.md), alongside library,
hash-registry and growing-heap probes. Graphics candidate 1 and batch 6 remain
archived; separate runs of those kits are not required. The visual probe and
assets remain byte-identical; batch 7 uses the separately verified
[growing-heap runtime](HEAP-CAPACITY1.md).

The candidate kit groups the [80-image probe](../../examples/texture-catalog/README.md)
and the existing [library-services workload](../../examples/library-services/README.md).
It includes a separately labelled runtime candidate, manual NROs, generated PNG
tiles, attribution and an explicit rollback guide. Shared settings, runtime.ini,
game archives and the MG Menu distribution are excluded. The original batch 5
package and published runtime remain unchanged.

Use one later session for all four probes. Before replacing the runtime, preserve the
installed module and NPDM with GTA closed. Check two natural runs, texture
rotation/transparency and five pages, then cancellation/restart. Preserve both
probe result files and the loader report against exact artifact identities.

Further graphics expansion and stable promotion should wait for this candidate's
physical allocation/rendering, transition and performance results. The runtime
still needs its broader capacity/soak and concrete substantial-port acceptance
gates; these checks do not require repeating already accepted XML batch 4 work.
