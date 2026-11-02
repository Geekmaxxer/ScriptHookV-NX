# Service batch 8 hardware acceptance

Date: 2026-11-02. The [retained record](../../evidence/service-batch8-console-20261102/acceptance.json)
identifies the exact installed runtime, NPDM, four manual NROs, configuration,
reports and independently checked fixture bytes. Mounted-card inspection made
no SD writes. The accepted kernel SHA-256 is
`fdbbeba40b2f04e123e8a2c5f3cd5ae4bdef1ade765ba803ba5b6955fc4e19fa`.

## Accepted normal workload

- [x] Match all six installed artifacts against the four-probe Batch 8 kit.
- [x] Record at least two loads of each probe and normal gameplay performance.
- [x] Verify 512 pinned objects, 513 peak registrations and reverse destruction.
- [x] Verify buffered binary/text reads, seeking and direct XML loading.
- [x] Verify 768 captured actions, a 512-entry peak queue, wrapping and relocation.
- [x] Verify JSON parsing, serialization, publication and original-file restoration.
- [x] Independently verify all binary/text/XML stream fixtures and restored JSON bytes.
- [x] Confirm no prepared or backup JSON file remains after restoration.
- [x] Verify every latest report has zero failures and zero final owner allocations/resources.
- [ ] Accept active cancellation, broader transitions and rollback for this kernel.
- [ ] Measure peak RAM, stack, frame time, fragmentation and extended combined sessions.
- [ ] Accept default/larger profiles, physical media errors and interrupted publication.

Configuration: 16 residents, 48 script slots, a 16 MiB plugin budget and 3 MiB
image/BSS limits. Resource-registry retains run 3; the other reports retain run 2.
Earlier reports were overwritten. Repeated loading and normal performance are
operator observations, separate from the latest machine-readable completion and
cleanup results. No independent timing history was supplied.

The loader report was saved later than the probe reports and records a startup
with only Disarm and Footprints loaded. It confirms successful startup under the
same configuration, but does not establish final test-session capacities,
simultaneous probe states or cleanup diagnostics. Those claims remain outside
this record; the per-probe final audits establish their own reported ownership.

## Subsequent development

This result closes bounded normal-completion gates for resource growth, streams,
captured actions and JSON presets on the identified artifact. It does not promote
a stable release or transfer hardware acceptance to modified kernels. The Batch 8
archive remains immutable. Accepted normal workloads require no separate repeat
installation; broader observations can accompany the next consolidated candidate.
