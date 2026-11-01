# Service batch 7 hardware acceptance

Date: 2026-11-01. The [retained record](../../evidence/service-batch7-console-20261101/acceptance.json)
identifies the installed kernel, NPDM, four exact NROs, all 80 verified PNGs,
configuration bytes, final reports and loader state. Mounted-card inspection
performed no SD writes. The accepted kernel SHA-256 is
`e65509d418d98a687049fff452ad3e863455b114cdec821eb674e4165f76abfc`.

## Accepted workload

- [x] Verify installed runtime, NPDM, NRO and asset identities against the prepared kit.
- [x] Load each manual probe at least twice and allow natural completion.
- [x] Verify integer/decimal parsing, independent errors, math, formatting and truncation/rejection reports.
- [x] Verify 1,365 integer records, 128 named records and 64 deliberately colliding registry records.
- [x] Verify 4,096 separate heap buffers and 4,097 simultaneous owner allocations.
- [x] Observe record capacity 4,352 and pointer-index capacity 8,192 in the loader report.
- [x] Verify zero final owner allocations/registrations in the heap and registry probes.
- [x] Verify 80 PNG loads, 360 completed sprite frames and both retained-ID checks.
- [x] Confirm five tile pages, rotation/transparency and no sprites after completion.
- [x] Verify inactive resident probes with no loader, configuration, callback or resource failures.
- [ ] Accept active cancellation/restart, broader transitions and rollback for this exact kernel.
- [ ] Measure peak RAM, stack use, frame time, fragmentation and extended combined sessions.
- [ ] Accept the default budget and larger arena/capacity profiles independently.

The configuration is 16 residents, 48 script slots, a 16 MiB plugin budget and
3 MiB image/BSS limits. This result does not accept the same workload under the
8 MiB default. The loader retains 180,224 bytes of additional heap metadata and
8,192 bytes of texture metadata; its overlapping accounting views are not total
process/GPU memory measurements. Disarm and Footprints remain running alongside
the four inactive probes.

The retained heap report is run 3; the other three reports are run 2. Earlier
per-run files were overwritten. Repeated loading/completion is an operator
observation; the final reports establish the latest recorded state. Texture
rotation, transparency and cleanup are visual observations, separate from the
scripted PNG/ID counters. No independent per-run timing history is available.

## Promotion and subsequent candidates

The result closes the bounded normal-operation checks for the combined library,
hash registry, growing heap-record/index and 80-image workloads on this artifact.
It does not promote a stable release, accept arbitrary large mods or transfer
hardware acceptance to modified kernels.

The next [growing-resource candidate](RESOURCE-CAPACITY1.md) retains a separate
local record and later combined checklist. The four accepted probes and historical
Batch 7 kit remain archived; separate repeat installations are unnecessary.
