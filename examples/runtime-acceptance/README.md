# Combined runtime acceptance probe

This manual probe exercises the existing runtime candidate without adding an ABI
or changing game state. It combines a managed string/vector session, numeric XML,
chunked storage, paged directory listing, file rename/delete, an additional script,
a render callback, a generated PNG and borrowed ped handles. It does not create,
delete or edit game entities, query account names, or touch save files.

Build from the SDK source with the following command. A prepared test kit can be
installed directly without compiling it again.

```powershell
python tools/build_runtime_acceptance.py --output out/combined-test1 --devkitpro C:/devkitPro
```

The kit includes the NRO in Manual mode, the generated texture, shared SD-access
metadata/install tooling and startup guidance. The runtime and menu modules are
not included. Use the currently verified Test1 runtime and preserve its working
DLC layout. A build on an older runtime must fail required-service initialization.

## Test sequence

1. Close GTA and preserve existing `settings/runtime-acceptance/` data if present.
   Install the kit using its READ-ME-FIRST instructions. Keep the current runtime
   configuration for the first comparison.
2. Start Story Mode and load **runtime-acceptance** in the plugin manager. A small
   colored square should appear near the top-right corner. It indicates the
   callback/draw path; counters still require report inspection.
3. Let it run for at least 30 seconds, including a pause/resume and ordinary walking
   or driving. Record visual behavior and any performance regression separately.
4. Use **Kill Process**, then **Load Now**, let it run again and use **Kill Process**.
   The square and worker should stop. Do not disable other mods to disguise a failure.
5. Close GTA and preserve the loader report plus the probe's settings directory.
   Check the latest `status.txt` and `cleanup.txt` together. Their `run` values must
   match; run numbers are process-local and reset when GTA closes.

## Expected evidence

- While running: `startup_complete=1`, `operation_error=0`, `draw_failures=0`,
  262,144 written and verified bytes, successful XML/catalog/mutation fields,
  increasing heartbeat/worker/render counters, and two resource registrations.
- After stopping: `phase=stopped`, matching run numbers, one stop callback and
  session destructor, zero registrations, zero live/requested heap allocations
  relative to the initial baseline, and no allocation/release/report failures.
- On restart: a new run number, new counters and the same successful cleanup.
  Texture IDs/backing remain cached under the existing retained-code contract;
  zero heap allocations is not a claim that all resident module/texture RAM is freed.

The probe reserves a 1,024-element vector, streams a 256 KiB deterministic file in
16 KiB chunks with cooperative waits, and repeatedly roundtrips 128 numeric XML
records. It samples at most 16 ped handles and only tests the returned borrowed
pointer for non-nullness. A world with no enumerable peds can report zero valid
borrows without an invented entity result.

## Failure and data policy

Reports use decimal fields. `operation_error` values identify the failing stage:
1–4 setup, 10 payload, 11 catalog, 12 file mutations, 13 initial XML, 20–24 active
workload, 30 texture drawing, and 90 final heap/resource cleanup. Check `report_failures`, the loader's
resource/callback fields and the latest matching run; a stale cleanup file does
not establish current success. File writes and frame counters are not FPS or
wall-clock measurements. The probe's storage load is part of the test.

Only this plugin's isolated test files are overwritten. Existing payloads with
unexpected size or mutation sentinels with unexpected content stop the probe for
inspection. A stopped partial write is rebuilt on the next explicit Load Now.
The marker rename/delete test operates only on its two known sentinel files.

## Acceptance limits

Local ARM64 execution models filesystem, RO, scheduling and game-engine boundaries.
It does not establish physical frame cost, archive/media integrity, capacity
growth, entity mutation/streaming ownership or full Menyoo compatibility. Complete
the [platform sequence](../../docs/verification/PLATFORM-CAPACITY1.md) on the exact
installed runtime. The probe supplements the original two Test1 demos; it does
not retroactively extend their acceptance.
