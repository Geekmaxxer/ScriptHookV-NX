# Buffered file-transfer verification

The SDK adds cooperative read-buffer and prepared-write tasks on the unchanged
Test1 runtime. This connects bounded file services to larger XML/configuration
source ports while preserving explicit allocation, ownership and error handling.
No new engine binding, native signature, runtime ABI or target offset is introduced.

## Implementation and local coverage

- [x] Allocate a caller-bounded read buffer and withhold partial data until completion.
- [x] Preserve complete binary data and an appended NUL, including empty files.
- [x] Advance at most one chunk per read/write call and validate final file length.
- [x] Refuse invalid paths, bounds and chunks through explicit statuses.
- [x] Preserve active task state when a second Begin or foreign reset is refused.
- [x] Exercise allocation refusal, short reads, I/O failures and changed final length.
- [x] Exclusively create prepared outputs and preserve existing or partial files.
- [x] Clean registered read storage after Kill and recreate fresh task state on Load.
- [x] Execute a complete 256 KiB XML transfer, parse, cleanup and restart locally.
- [x] Preserve malformed existing XML and report the parse failure without leaking.
- [x] Execute all four packaged service probes together under compiled defaults and repeat loading without remapping.
- [ ] Accept the new transfer/XML example on physical Switch hardware.

The [local record](../../evidence/file-transfer1-local-20261030/verification.json)
identifies exact artifacts and completed tests. Tests execute production runtime,
SDK, XML library and probe ARM64 instructions with modeled OS, RO, filesystem,
game boundaries and outer scheduling. The complete XML parse uses a bounded
50-million-instruction emulator call budget; this is not a CPU timing measurement.

## Next grouped check

Service batch 1 already accepts shared state, normal publication/backup retention
and nested catalog access on the supported Switch. The current
[batch 4 checklist](../../examples/service-acceptance/README.md) combines the pending
large XML transfer check with preset browsing, growable serialization, verification
and backup restoration. Hardware acceptance remains deferred; repeating previously
accepted probes is optional. The [current record](CATALOG-SCAN1.md) identifies the
consolidated scope; the [batch 3 record](XML-WORKFLOW1.md) retains earlier evidence.

Transfer steps do not establish a maximum storage delay. DOM parsing remains a
separate complete operation, input-copy/node allocations count against the owner
heap, and same-size concurrent modifications are outside the contract. Media-error
recovery, power-loss durability, process RAM and presentation frame time remain
independent acceptance items. See the [transfer contract](../reference/FILE-TRANSFERS.md).
