# Capacity performance diagnostics

Test2 adds measurements to the bounded Test1 capacity workload while retaining
the exact Test1 runtime. It prepares investigation of the 78.6 ms Explicit and
96.4 ms Automatic owner dispatch maxima. Neither value identifies a failing
operation, and the earlier runs are not a controlled performance comparison.

The [physical Test1 results](CAPACITY-ACCEPTANCE1.md) remain immutable. Test2 now
has scoped physical acceptance of both workloads, measurements and final cleanup.
Its diagnostic overhead is measured below. No production runtime, ABI or target
adapter changes.

## Implementation and local checks

- [x] Measure non-yielding operations with the runtime's existing Horizon clock boundary.
- [x] Separate per-run elapsed time and dispatch deltas from lifetime owner maxima.
- [x] Record exclusive operation-duration and callback-interval histograms.
- [x] Preserve up to eight final run reports and capture them before profile changes.
- [x] Keep diagnostic report text in static storage and retain compiler frame evidence.
- [x] Attribute a modeled 60 ms filesystem stall to the report-write region locally.
- [x] Execute worker progress, allocator verification, nested cleanup and restart locally.
- [x] Validate actual compiled report output with the portable reviewer.
- [x] Reject incomplete, duplicated, inconsistent and oversized report fixtures.
- [x] Verify the Test2 workload and measurements on physical Switch hardware.
- [x] Identify report saves as the long measured region matching reproduced dispatch-stall magnitudes.
- [x] Reduce periodic status writes and admission reports in the separate Test3 source revision.
- [ ] Record a controlled baseline, extended soak and remaining transition checks.

The [local record](../../evidence/capacity-diagnostics2-local-20261030/verification.json)
identifies exact artifacts and completed checks. ARM64 execution models the OS
clock, allocation, filesystem, RO, game services and outer scheduling. Injected
latency demonstrates attribution logic, not an explanation of the hardware spikes.

## Measurement boundaries

Nine named regions cover setup, large extent access, each worker admission,
each block verification/allocation, report formatting/writing, parent cleanup and
Trim. Their timers end before cooperative waits. Samples include synchronous
service stalls and OS descheduling. Callback intervals reflect runtime callback
dispatch, not GPU frame presentation. Untimed work remains outside these regions.

Reports snapshot previous format/write samples. Their own samples appear later;
the last history write has no subsequent snapshot. Owner lifetime maxima remain
separate from per-run completed-call/total-time differences. A prior maximum cannot
be subtracted to derive a new run's maximum.

The [kit instructions](../../examples/capacity-acceptance/README.md) specify a
ten-minute diagnostic run, Kill/Load, a second two-minute run and restoration.
History uses an eight-slot ring; actual run numbers and a process-local session
start tick distinguish retained records. No account names or saves are collected.

Peak process/GPU memory, whole-stack use, presentation frame time, long-session
stability and multiple resident owners remain independent gates. Further major
runtime expansion and Menyoo porting remain gated by the
[platform acceptance sequence](PLATFORM-CAPACITY1.md#development-gate).

## Physical Test2 result - 2026-10-30

The [hardware record](../../evidence/capacity-diagnostics2-console-20261030/acceptance.json)
matches the installed Test1 runtime, Test2 NRO and Explicit profile. Both history
reports belong to the same process-local session and identify completed runs 1
and 2. Each run admitted all 36 workers, verified 513 image and BSS pages, drained
both children before their parent and finished with zero owner allocations,
charged bytes or registrations. Allocation, release, report, clock and cleanup
errors are zero. Disarm and Footprints remain running.

| Instrumented observation | Run 1 | Run 2 |
| --- | --- | --- |
| Wall-clock lifetime | 570.359 s | 151.580 s |
| Completed allocator cycles | 366 | 95 |
| Worst report-save region | 122.480 ms | 72.693 ms |
| Worst other measured region | 0.109 ms | 0.102 ms |
| Owner lifetime worst dispatch | 122.542 ms | 122.542 ms, retained from run 1 |

The operator reports sustained driving, combat, explosions and a five-star wanted
level, successful Kill/Load, no crash and good overall playability with slowdown
during heavy activity. Approximate reported durations are at least ten and two
minutes; the instrumented lifetimes above are authoritative for recorded elapsed
time. This scoped gameplay result does not establish a controlled FPS benchmark.

All 377/106 measured report saves exceeded 2 ms; 21/2 exceeded 50 ms. Non-report
measured regions stayed below 0.11 ms. Synchronous report saves therefore account
for the reproduced stall magnitude. The individual dispatch and write maxima do
not carry a shared event ID, so exact event pairing and attribution of older
Test1 maxima remain unavailable. Callback gaps include gameplay/scheduling pauses
and are not presentation FPS.

The separate Test3 source retains the first active report, then saves periodic
status at most once per 30 seconds and records only one admission snapshot.
Immediate failure/final cleanup reports remain. Test2 artifacts and reports are
preserved. The quieter revision has local checks only and can join a later combined
workload; repeating this long capacity run immediately is not required.
