# Service acceptance batch 9

This deferred candidate groups writable C streams and direct XML saving in one
manual probe. It also provides one session for broader observations still open
after [Batch 8 acceptance](../../docs/verification/SERVICE-BATCH8.md). Accepted
Batch 4, 7 and 8 normal workloads require no separate repeat installations.

## Build and contents

```powershell
python tools/build.py --output out/batch9-core --devkitpro C:/devkitPro
python tools/build_service_acceptance.py --runtime out/batch9-core/subsdk5 --output out/batch9 --devkitpro C:/devkitPro
```

The builder verifies the exact writable-stream kernel, compiles the manual NRO,
preserves notices and checks ZIP inventory/readback. No game files are required.
`RUNTIME-CANDIDATE/` contains matched runtime/NPDM and preservation tools;
`INSTALL/switch/` contains the manual probe. No shared runtime.ini, settings, game
archives, bindings or automatic installer are included. Preserve current settings.

The kernel adds checked resizing through an optional sized mutation-table tail
using existing audited filesystem bindings. Earlier hardware results remain tied
to their exact artifacts. This new kernel requires its own acceptance. Published
stable releases remain unchanged.

## One combined session

1. Close GTA fully. Back up installed title `exefs/subsdk5` and `main.npdm` outside
   the installation tree. Retain the accepted Batch 8 runtime for rollback.
2. Install this matched RUNTIME-CANDIDATE. Follow SD-ACCESS.md for a customized
   NPDM so existing capabilities are preserved. Copy INSTALL/switch to the SD
   root. Preserve configuration, other plugins, bindings, game files and saves.
3. Fully restart GTA and load **write-streams** twice to natural completion. It
   has no visible overlay. Each lifetime yields between transfers, verifies
   binary/update/append/hole bytes, formatted text and direct XML save/load,
   then audits final stream ownership. Allow roughly 30 seconds per lifetime.
4. After two full runs, Kill it during another transfer and Load again. Allow
   the restarted run to finish. Partial fixture output after Kill is expected;
   the restarted lifetime recreates only this example's named fixtures.
5. Keep other normal mods available. Include pause/resume and any practical
   loading, death or character transitions in this same session. Existing Batch 8
   probes can support concurrent-mod observations; no separate SD cycle or repeat
   normal-workload installation is required.
6. Preserve the latest probe report and **loader-report.txt before another GTA
   startup overwrites the session state**. Record observed actions, errors and
   performance separately. Close GTA before accessing the SD card.
7. To roll back, restore the backed-up exefs files with GTA closed and fully
   restart. Record rollback separately from probe completion when practical.

One available resident slot is required. No SD removal is needed between probe
steps. The example overwrites only `output.bin`, `formatted.txt`, `direct.xml` and
`retained.txt` under `settings/write-streams-example/`. Do not store personal files
under those names. It changes no game data or saves.

## Expected reports

Under `switch/gtav-scripthook/settings/write-streams-example/result.txt`:

- [ ] `failed=0` and `completed=1` after the latest natural completion.
- [ ] All six binary/update/append/hole/formatted/XML verification flags are 1.
- [ ] `owner_cleanup_complete=1`, `owned_allocations=0`, `registered_resources=0`.
- [ ] `run` reflects repeated loads; cancelled runs may increment it too.
- [ ] Independently verify retained fixture bytes when collecting results.
- [ ] Record matching runtime/NRO hashes, configuration and test-session loader.

Each result file retains only the latest lifetime. Repeated loads and transition
observations must be recorded separately. Normal completion supplies no measured
frame-time/peak-memory or power-loss/media-error result. Deliberate SD removal,
interrupted publication and corrupted game data are outside this test.

See [verification and limits](../../docs/verification/WRITE-STREAMS1.md).
