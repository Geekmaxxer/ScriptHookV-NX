# Service acceptance batch 4

This deferred batch combines preset browsing with the pending large XML
load/save/recovery checks. Its hardware run remains deferred while implementation
continues. Use the sequence below when the next consolidated acceptance session
is scheduled; standalone batch 2 and batch 3 runs are unnecessary. The existing
Test1 runtime and shared configuration remain unchanged.

## One combined check

1. With GTA closed, copy `INSTALL/switch` to the SD root. Preserve existing settings,
   the working runtime and DLC layout. No capacity profile change is required.
2. Restart GTA and Load Now on **xml-workflow**. Let it complete naturally,
   allowing approximately 30 seconds. There is no visible overlay. The script
   indexes 13 XML presets and one nested directory across four pages, filters
   one text file, verifies sorted selections and parses one selected preset. It
   then loads a 256 KiB XML input, adds a 96 KiB text payload, serializes it, writes
   and verifies the prepared output, publishes it, then restores the original
   while retaining the verified replacement separately.
3. Load **xml-workflow** once more and allow completion. This run verifies the
   retained files without replacing them. No Kill or long gameplay soak is needed
   for these normal-completion checks.
4. Close GTA. Retain `loader-report.txt` and `settings/xml-workflow-example/`:

   - `result.txt`: `run=2`, `failed=0`, `input_bytes=262144`,
     `serialized_bytes` greater than 65,536, `publication_verified=1`,
     `backup_restored=1`, `catalog_verified=1`, `catalog_count=14`,
     `catalog_pages=4`, `catalog_skipped=1`.
   - `cleanup.txt`: `run=2`, `owner_cleanup_complete=1`.
   - `input.xml`, `current.xml` and `published.xml`: retain complete files for
     independent verification. `current.xml` should contain the original small
     document. `published.xml` retains the larger verified replacement.
   - `presets/`: retain all 13 XML samples, `notes.txt` and the nested directory
     for independent catalog/content verification.

A completed run leaves no `prepared.xml` or `backup.xml`: publication consumed
prepared output, and explicit restoration moved the backup to current. The
replacement remains as `published.xml`; no file deletion is used by the workflow.

If a run is interrupted or reports a failure, preserve its files for inspection.
Unknown, incomplete or conflicting publication states are refused on restart;
the probe does not repair or discard them automatically. Do not rerun old batches
or delete these files merely to obtain a passing result.

## Previously accepted probes

The kit retains **session-state**, **recoverable-publication** and
**streamed-catalog** for optional concurrent checks. Their batch 1 acceptance
remains attached to the original artifacts; repeating them is not required.
The untested standalone **large-xml-file** example remains available in the SDK,
but its pending transfer behaviors are included in the new combined workload.

The kit contains no runtime module, game archives, NPDM or shared `runtime.ini`.
`manifest.json` identifies all NROs and the required existing runtime. No repeat
of the long capacity test is required. All four packaged probes fit the compiled
default resident/image/BSS limits in the local model.

## Scope

Catalog traversal, sorting and validation yield between steps. Pages are not a
snapshot; concurrent directory edits can still skip entries. Transfer and
verification steps are bounded by bytes and yield between file calls.
XML parsing/serialization remain complete synchronous operations; storage can
also stall the game thread. The workflow uses only its dedicated settings namespace
and changes no save data, accounts or game entities.

This batch does not establish power-loss durability, media-error recovery,
whole-process RAM, full-capacity owner growth or presentation frame time. New
features still require physical acceptance. See the
[catalog/workflow record](../../docs/verification/CATALOG-SCAN1.md),
[earlier XML workflow record](../../docs/verification/XML-WORKFLOW1.md) and
[earlier file-transfer record](../../docs/verification/FILE-TRANSFERS1.md).
