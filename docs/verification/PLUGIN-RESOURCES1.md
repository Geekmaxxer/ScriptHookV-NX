# Owned-resource lifecycle candidate

This unreleased candidate adds explicit cleanup registrations and managed heap
objects that survive cooperative suspension. Whole-plugin cancellation drains
registered ownership without resuming abandoned C++ stacks. The published and
installed runtime remains unchanged.

## Implementation

- [x] Bind an independent resource ABI 1 table before explicit initialization.
- [x] Preserve base plugin, extension, memory and filesystem ABI layouts.
- [x] Validate owner thread, callback text, exact heap context and per-owner capacity.
- [x] Pin registered allocations against free, realloc and checked typed deletion.
- [x] Consume each registration before invoking its cleanup callback.
- [x] Support explicit release, ownership transfer and stale-handle rejection.
- [x] Drain in reverse order after user stop and on failed initialization/setup.
- [x] Diagnose attempted waits and unfinished raw calls without duplicate cleanup.
- [x] Provide move-only nothrow managed ownership and a complete yielding example.
- [ ] Complete physical Switch transition, performance and extended-session acceptance.

No new native signature, target offset or platform import was introduced. The
registry uses fixed metadata with up to 64 registrations for each of 16 owners;
the configured resident-module and heap budgets remain independent limits.

## Local verification

Compiled fixtures execute production runtime, SDK shim and C++ instructions.
Cases cover exact context validation, pinning, exhaustion, two independent owners,
explicit release/forget, stale handles, nested release and callback violations.
Lifecycle cases cover normal scope exit, waits, natural completion, failed
initialization, Kill/Load and repeated cancellation. Optional bindings and a
null-context plugin without a memory-table binding are tested independently.

The complete managed example is compiled and executed. Its registered session
owns a dynamic string and sample vector; cancellation invokes its destructor and
returns all three live payloads to the owner heap. Restart creates a fresh
lifetime. Pool capacity can remain charged until Trim under the memory contract.

The [verification summary](../../evidence/plugin-resources1-local-20261030/verification.json)
records exact artifacts, source hashes, focused regressions and public export
checks. OS allocation, RO, filesystem and outer scheduling are modeled. Instruction
execution does not establish Horizon behavior, gameplay, frame cost or stack
high-water measurements.

## Physical acceptance

The [initial physical Test1 run](PLATFORM-CAPACITY1.md#physical-test1-demo-run)
verifies the exact managed example on the candidate runtime. Its session remained
active with one registered resource, no resource failures and a reported run of
at least 30 seconds. Stop/restart was not exercised in that initial run.

The later [combined probe](COMBINED-ACCEPTANCE1.md#physical-test1-result---2026-10-30)
verifies a managed string/vector session on the same candidate with a completed
Load/Kill/Load/Kill checklist. Final-run reports agree on run 2, one stop callback,
one session destructor, zero remaining registrations and zero live/requested heap
allocations. Trim succeeds without release errors. Disarm and Footprints remain
running. Only the final run's detailed cleanup survives; the first run's report
was overwritten. Broader nested ownership and transition failures remain open.

- [x] Verify exact runtime, example and configuration identities on Switch.
- [x] Cancel and restart the combined managed workload across cooperative waits.
- [x] Check final-run destructor/auditor results and absence of live registrations.
- [x] Record coexistence with running Disarm and Footprints.
- [ ] Exercise game transitions, nested registrations and broader failure cleanup.
- [ ] Measure callback latency, peak RAM, whole-stack use and extended-session behavior.
- [ ] Accept updated runtime installation and rollback separately.

The [public contract](../reference/OWNED-RESOURCES.md) defines retained registrations
when invocation preconditions fail. General C++ unwinding, global initialization,
exceptions and arbitrary callback-failure recovery remain outside this candidate.
