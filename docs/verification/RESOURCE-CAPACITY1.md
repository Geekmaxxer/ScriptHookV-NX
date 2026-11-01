# Growing resource registrations

Date: 2026-11-01. Unreleased runtime candidate. Stable resource pages replace the
fixed 64-registration admission limit. Public ABI 1 layouts, target bindings and
compiled configuration defaults remain unchanged. The
[ownership contract](../reference/OWNED-RESOURCES.md) defines pinning, retained
metadata, reverse cleanup and bounded nested callback execution.

## Implementation and local verification

- [x] Admit stable per-owner resource records through shared-budget metadata pages.
- [x] Preserve unique handles, stale-ticket refusal and independent owner lifetimes.
- [x] Reuse consumed records and retain reverse registration order after reuse.
- [x] Return failed metadata reservations and recover after budget/OS refusal.
- [x] Track pins in stable heap records without scanning all registrations on free/realloc.
- [x] Drain the newest live entry directly and tolerate nested child consumption.
- [x] Preserve a 64-call active cleanup nesting bound before consuming a deferred child.
- [x] Exercise 1,024 individually pinned contexts and 256 independent peer registrations locally.
- [x] Reclaim 192 registrations after rejected initialization before module release.
- [x] Recheck existing heap, ownership, scheduling and PNG behavior locally.
- [x] Execute repeated completion and cancellation of the exact packaged 512-object probe locally.
- [x] Reproduce the kernel and kit from an isolated source ZIP.
- [ ] Accept the growing-resource candidate on physical Switch hardware.
- [ ] Measure substantial nested workloads, cleanup time, stack/RAM use and long sessions.

The [local record](../../evidence/resource-capacity1-local-20261101/verification.json)
identifies sources, compiled artifacts, regressions and reproduction results.
Production ARM64 registry, pin, heap, callback and loader instructions execute;
OS, RO, filesystem, GPU and outer game scheduling remain modeled. Local instruction
allowances establish no physical frame-time or stack bound.

Seven runtime suites pass 813 local checks; the portable source suite passes
39 tests. An isolated source build reproduces the kernel, probe NRO and all
16 kit files byte for byte. These results exclude physical acceptance.

The kernel SHA-256 is
`fdbbeba40b2f04e123e8a2c5f3cd5ae4bdef1ade765ba803ba5b6955fc4e19fa`.
[Batch 7 acceptance](SERVICE-BATCH7.md) applies to the preceding kernel; it does
not automatically accept this modification. No new native signatures, offsets,
Windows/hosted C++ support or Menyoo gameplay claims are introduced.

## One later hardware batch

[Service batch 8](../../examples/service-acceptance/README.md) contains one manual
[resource-registry probe](../../examples/resource-registry/README.md). It registers
512 individually owned objects plus an audit, checks each handle/pin and verifies
reverse destruction with zero final owner allocations or registrations.

Two natural runs, optional cancellation/restart, existing-mod coexistence,
transitions and longer gameplay observations are grouped in one later session.
Existing configuration, settings, bindings, saves and game files are preserved.
The runtime/NPDM candidate is separate from the probe tree and has explicit
preservation/rollback instructions. Accepted XML Batch 4 and the four Batch 7
workloads require no separate repeat installations.
