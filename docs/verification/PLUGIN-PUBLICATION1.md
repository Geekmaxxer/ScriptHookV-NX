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

- [ ] Verify the exact runtime, SDK and plugin identities on the supported Switch.
- [ ] Exercise publication, retained backups and explicit restoration on SD storage.
- [ ] Exercise error/interruption recovery without overwriting retained data.
- [ ] Test pause/loading/death/character transitions and concurrent plugins.
- [ ] Measure preparation, rename/delete latency and extended-session behavior.
- [ ] Document actual media failures and durability limits separately.

This workflow does not establish atomic replacement, content identity from size
alone, filesystem transaction isolation or power-loss durability. See the
[public contract](../reference/FILE-PUBLICATION.md) for required application
validation and the single-writer policy.
