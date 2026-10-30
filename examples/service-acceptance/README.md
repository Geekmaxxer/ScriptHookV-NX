# Service acceptance batch 1

This kit groups three manual probes for the existing Test1 runtime: shared C++
session state, backup-preserving file publication and nested file/catalog access.
It requires no shared configuration change or new runtime installation. The probes
use only their own settings directories and do not change saves or game entities.

## Installation

With GTA closed, copy `INSTALL/switch` to the SD root, preserving all existing
settings. Keep the installed Test1 runtime identified in `manifest.json` and the
working game/DLC layout. All three NROs fit the default image/BSS limits.

If capacity testing left an Explicit/Automatic profile selected, the previous
capacity kit can restore the original configuration with GTA closed. Do not load
the oversized capacity probe during this batch. A repeated long capacity test is
not required for these service checks.

## One combined gameplay session

1. Load Now on **session-state**. Play normally for approximately 30 seconds,
   Kill Process, Load Now again, play approximately 30 seconds and Kill Process.
   A pause/resume or an ordinary gameplay transition can be included in this run;
   record any transition exercised. There is no visible overlay.
2. Load Now on **recoverable-publication** and allow approximately 30 seconds to
   finish. Load it once more to verify the retained published file and backup.
   It completes naturally; an idle/finished state is expected.
3. Load Now on **streamed-catalog** and allow approximately 30 seconds to finish.
   Load it once more to verify the existing sample and its catalog metadata.
4. Close GTA and retain `loader-report.txt` plus these settings directories:

   - `session-state-example/`: `cleanup.txt` should show `run=2`, `destructors=2`,
     `worker_progressed=1` and `owner_cleanup_complete=1`.
   - `publication-example/`: `result.txt` should show `verified_bytes=262144`,
     `backup_retained=1` and `failed=0`. Keep `current.bin` and `backup.bin`.
   - `streamed-catalog-example/`: `result.txt` should show `verified_bytes=262144`,
     `catalog_verified=1` and `failed=0`. Keep its `catalog/` directory.

The display labels can include each NRO's filename suffix; select the corresponding
manual module. Preserve results if a report shows a failure or inspection request.
The publication sample intentionally retains interrupted/unfamiliar file states
for inspection; it does not overwrite or delete them automatically.

This batch verifies bounded service behavior, not deliberate media removal,
power-loss durability, peak process RAM, full-capacity owner growth or whole-stack
use. Those remaining checks can be grouped with later workload milestones. The
[session-state record](../../docs/verification/SESSION-LOCAL1.md) distinguishes
local verification from physical acceptance.
