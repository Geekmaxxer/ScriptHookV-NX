# Recoverable file publication candidate

This candidate adds isolated file mutations and an application-validated,
backup-preserving publication helper. It is unreleased. Local execution models
filesystem, OS, RO and outer game scheduling; the accepted runtime and installed
release artifacts remain unchanged.

## Implementation

- [x] Bind a separate mutation ABI 1 table before explicit initialization.
- [x] Preserve existing file/catalog and base plugin/extension table layouts.
- [x] Validate owner-thread access, bounded paths and the 64 MiB extent cap.
- [x] Provide rename without replacement and explicit checked file deletion.
- [x] Report postcondition failures separately from confirmed success.
- [x] Define all eight current/prepared/backup presence states.
- [x] Preserve backups and conflicts until an explicit application decision.
- [x] Provide explicit restoration without replacing an existing current file.
- [x] Demonstrate cooperative preparation, validation and publication of 256 KiB.
- [ ] Complete physical storage, interruption and performance acceptance.

No new target offset, native signature or filesystem import was introduced.
The service reuses the existing audited RenameFile/DeleteFile bindings and
metadata/read operations. The original low-level policy-rename path retains its
existing behavior.

## Local verification

The compiled fixture executes production runtime/shim/helper ARM64 instructions.
Independent file models test every presence combination, byte-preserving large
candidate movement, conflicting destinations, wrong namespaces/threads, invalid
paths, extent limits, deletion and optional-service availability.

Fault cases include failures before mutation, successful no-op responses,
completed mutations with lost acknowledgements, failed postchecks, an unreadable
backup and an interrupted two-rename publication. Kill/Load recovery and the
complete cooperative example check retained data and explicit recovery behavior.
The example also preserves an interrupted preparation on restart.

The retained [verification summary](../../evidence/plugin-publication1-local-20261029/verification.json)
identifies the exact runtime and completed checks. Temporary handles/mounts are
checked after fixture calls. No SD, game archive or installed-runtime write is
performed by these local models.

## Physical acceptance

- [x] Verify the exact runtime and publication-example identity on the supported Switch.
- [x] Accept normal publication and retained backup contents on SD storage.
- [x] Accept explicit backup restoration on SD storage through the batch 4 XML workflow.
- [ ] Exercise error/interruption recovery without overwriting retained data.
- [ ] Test pause/loading/death/character transitions and concurrent plugins.
- [ ] Measure preparation, rename/delete latency and extended-session behavior.
- [ ] Document actual media failures and durability limits separately.

This workflow does not establish atomic replacement, content identity from size
alone, filesystem transaction isolation or power-loss durability. See the
[public contract](../reference/FILE-PUBLICATION.md) for required application
validation and the single-writer policy.

The [physical service batch 1](SESSION-LOCAL1.md#physical-batch-1-result---2026-10-30)
verifies the full 256 KiB published byte pattern, exact `old` backup, absent
prepared file and successful repeated-load checklist. This covers normal
publication and backup retention, not injected mutation failures or durability.
The later [batch 4 result](CATALOG-SCAN1.md#physical-batch-4-result---2026-10-30)
adds verified 98,376-byte XML publication, exact original restoration and retained
replacement verification on the second natural run. Mutation failures and
interrupted restoration remain separate physical checks.
