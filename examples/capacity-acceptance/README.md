# Capacity diagnostics Test3

This manual probe tests the existing Test1 runtime with a large NRO and one main
script plus 36 cooperative workers. It verifies page-spaced image/BSS access,
repeated allocation/free/realloc cycles, nested managed registrations, cleanup,
restart, operation durations and callback intervals. It does not edit game entities,
save files, account data or other plugins.

Both image and BSS exceed 2 MiB. The bounded workload uses 37 script entries and
less than 1 MiB of requested test heap. Build it from the SDK source:

```powershell
python tools/build_capacity_acceptance.py --output out/capacity-test3 --devkitpro C:/devkitPro
```

The kit provides the manual NRO, three configuration profiles, a preservation
helper, a read-only Python report reviewer and licenses. Keep the existing Test1 runtime and working DLC layout.
The helper checks runtime/probe hashes before changing configuration. It preserves
the original bytes or original absence and refuses unrelated configuration edits.
Close GTA before each SD change and restart it for each profile.

Test3 reduces periodic status writes to once per 30 seconds and writes one
admission snapshot. Failure and final cleanup reports remain immediate. Physical
Test1/Test2 results apply to their original artifacts; Test3 requires its own
acceptance within a future combined workload. The production runtime, ABI and target
offsets are unchanged. Diagnostic overhead prevents an identical benchmark comparison.

## Optional diagnostic run

The accepted Test2 workload does not require an immediate repeat with Test3.
Use the quieter probe when a later combined workload requires capacity measurements.

If completing Test1, use its existing kit to select `Restore`, restart GTA and
confirm normal gameplay before installing Test3. Do not use Load Now on the
capacity probe during this check. Restore reinstates the original limits, so
`module_image_limit_exceeded` (`0x10023`) is expected if the oversized probe is
loaded again. Preserve the loader report; existing accepted mods should still run.

With GTA closed, install Test3's manual NRO and select `Explicit` from this kit.
Restart, Load Now on **capacity-acceptance** and run for ten minutes: begin in
ordinary gameplay, then walk/drive and pause/resume. Note approximate wall times
of these actions. Keep runtime settings and other mods fixed. Use Kill Process,
Load Now, run for another two minutes and Kill Process again. Close GTA and
preserve the settings directory and loader report before another launch.
Select `Restore`, restart and verify normal operation. Do not load the large
probe after restoring smaller image/BSS limits; normal startup does not require it.

This is an initial diagnostic sample. Extended soak, transitions, multiple resident
owners, process RAM and whole-stack use remain separate acceptance steps.

## Physical sequence

1. Copy the kit's `INSTALL/switch` directory to the SD root. Preserve any existing
   `settings/capacity-acceptance/` reports. Run profile commands from the extracted
   kit, replacing `D:\` with the mounted SD root.
2. Select `Defaults` with the helper. Start GTA, verify the existing accepted mod
   still runs, then Load Now on **capacity-acceptance**. The intended result is
   `module_image_limit_exceeded` (`0x10023`), without a crash. The probe never
   starts in this phase. The next profile switch preserves the available Defaults reports.
3. Close GTA, select `Explicit`, restart and load the probe. Run for at least
   30 seconds, including pause/resume and ordinary gameplay. Use Kill Process,
   Load Now, run again and Kill Process. Close GTA and preserve its settings
   directory and loader report. The helper captures them before the next switch.
4. Repeat step 3 using `Automatic`, preserving separate results before restoring
   the original configuration. The same bounded workload is used in both phases.
5. Close GTA and select `Restore`. Restart and verify normal gameplay and existing
   accepted mods without loading **capacity-acceptance**. This phase verifies the
   original configuration; the oversized probe requires Explicit or Automatic
   settings. Keep the original backup and `phase-results/` reports until review.

```powershell
.\Set-Test-Profile.cmd -SdRoot D:\ -Profile Defaults
.\Set-Test-Profile.cmd -SdRoot D:\ -Profile Explicit
.\Set-Test-Profile.cmd -SdRoot D:\ -Profile Automatic
.\Set-Test-Profile.cmd -SdRoot D:\ -Profile Restore
```

Run one command for its corresponding phase. The helper is for a mounted card;
it does not replace runtime modules or restore game archives. If it refuses an
unrelated edit or damaged backup, retain both files and restore manually only
after inspection. Backups are under `settings/capacity-acceptance/`.

## Expected evidence

- Active: `operation_error=0`, `startup_complete=1`, all 36 workers progressing,
  513 verified image and BSS pages, growing cycle/heartbeat/render counters,
  two child registrations and four total live resource registrations.
- Explicit/Automatic loader: explicit limits match 16/48/16/3/3; Automatic has mask `0x1f`.
  Retained script-stack storage exceeds the 1 MiB bootstrap and dynamic metadata
  is nonzero. Loader/configuration/action/resource errors remain zero.
- Stopped: matching status/cleanup run IDs, one stop callback and parent
  destructor, two child destructors before the parent's body, zero cleanup-order errors, no live owner
  allocations/registrations, successful Trim and zero charged heap bytes.
- Restart: a fresh run ID and complete cleanup without new NRO mappings or
  unnecessary additional stack allocation. Run IDs reset when GTA exits.

Reports update this probe's `status.txt`, `cleanup.txt` and eight bounded history
slots, `run-1.txt` through `run-8.txt`. Run 9 reuses slot 1. The `run` field retains
the actual run number; run IDs reset on game exit. Compare `session_start_tick`
before combining reports. Files from older sessions can remain until overwritten.
The helper
captures available loader/status/cleanup/configuration files before a profile
switch or restoration, including the eight known history filenames, with hashes
and unique phase directories. A capture does not itself prove that GTA ran that
configuration. Error stages are 1-4 setup, 10 extents, 11 render registration, 12 allocator
verification, 15 freeing, 20 worker admission, 21-22 reporting and 90 cleanup.

Script timing covers completed runtime-dispatched entries for this owner. It is
not whole-game FPS, process RAM or whole-stack high-water measurement. Code,
admitted stacks and metadata remain resident after Kill; owner heap Trim is a
separate result. This single-owner probe does not establish larger resident
module counts, full menu compatibility or arbitrary workload limits. See the
[capacity acceptance record](../../docs/verification/CAPACITY-ACCEPTANCE1.md).

## Diagnostic measurements

Schema 2 reports record run wall time and nine non-yielding regions: setup, extent
checking, individual worker admission, individual block verification/allocation,
report formatting, report writes, parent cleanup and heap Trim. Each region has
counts, totals, maxima, the wall-time location of its maximum and exclusive buckets
ending at 100 us, 500 us, 1 ms, 2 ms, 10 ms, 50 ms and above 50 ms. Measurements end
before `WAIT`; they do not include cooperative suspension.

The clock uses the same Horizon `GetSystemTick` boundary and 19.2 MHz unit as the
pinned runtime. Durations include service stalls and OS descheduling, not CPU time.
Elapsed run time also includes waits, pauses and system suspension. Callback gap
buckets end at 17, 34, 50, 100, 250 and 1,000 ms, then above 1,000 ms. These measure
runtime callback dispatch cadence, not GPU presentation or whole-game FPS.

Per-run dispatch differences cover completed calls and total time. The owner
lifetime maximum remains separate and accumulates across Kill/Load. Subtracting
maxima does not produce a run maximum. Reports snapshot prior format/write samples;
their own samples appear later, and the final history write has no later snapshot.
Untimed work remains outside the measured regions. Timing fields can differ
between status, cleanup and history even when final workload counters agree.

From the extracted kit, review the two final reports with Python 3.10+:

```powershell
python Review-Report.py D:/switch/gtav-scripthook/settings/capacity-acceptance/run-1.txt
python Review-Report.py D:/switch/gtav-scripthook/settings/capacity-acceptance/run-2.txt
```

The reviewer validates structure and histogram consistency without modifying
files. It cannot establish artifact identity, physical origin or performance
acceptance. Unknown emulator clock behavior requires separate review.
