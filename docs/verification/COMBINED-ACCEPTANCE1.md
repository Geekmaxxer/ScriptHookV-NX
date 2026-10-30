# Combined runtime acceptance probe

The `runtime-acceptance` example combines the existing optional services in one
instrumented workload. It prepares the next physical acceptance step without
changing the Test1 runtime, target profile, ABI or original two demo artifacts.
The exact probe now has scoped physical acceptance. Full platform/capacity,
performance and substantial-port acceptance remain open.

## Implemented workload

- [x] Manage a string/vector session and a separately registered final auditor.
- [x] Stream and verify 256 KiB in cooperative 16 KiB chunks.
- [x] Roundtrip 128 numeric XML records and repeat serialization while active.
- [x] Enumerate isolated files in pages and verify sentinel rename/delete.
- [x] Run an additional script and a render callback with a generated PNG.
- [x] Sample borrowed ped handles without dereferencing layouts or editing entities.
- [x] Record operation stages, run IDs, heap/resource counts and cleanup results.

## Local verification

The [retained evidence](../../evidence/combined-acceptance1-local-20261030/verification.json)
records exact artifact and source hashes. Twenty-four checks execute the compiled
NRO, production runtime, SDK shims, numeric/XML libraries and managed ownership
code as ARM64 instructions. Filesystem, RO, OS, engine and outer scheduling
boundaries are modeled. Engine callbacks do not establish physical rendering,
world ownership or entity compatibility.

- [x] Verify the combined successful workload, cancellation, restart and cleanup.
- [x] Cancel during chunked output and rebuild on explicit restart.
- [x] Inject corruption, conflicting files, missing assets and catalog I/O failure.
- [x] Reject missing services and additional-script admission without leaking ownership.
- [x] Re-run all 57 directory-catalog checks after the shared harness adjustment.
- [x] Build and structurally check all 17 standalone examples.

The plugin image is 204,800 bytes with 20,480 bytes of BSS. Its admitted module,
metadata and main-stack accounting is 454,656 bytes; these are admission values,
not measured peak RAM or GPU usage. The intended existing runtime module SHA-256
is `930d3043c61ee457da833d698836886e20a103a5eab68158d7d786439fbf128f`.

## Physical acceptance

Build the manual test kit with `tools/build_runtime_acceptance.py`. It contains
the probe, generated texture, shared SD-access metadata, preservation tooling,
licenses and instructions. It contains no runtime module or game archives.
The [example checklist](../../examples/runtime-acceptance/README.md) defines
report fields, failure stages and the required Load/Kill/Load/Kill sequence.

- [x] Verify installed artifact identities and retain the working configuration.
- [x] Confirm the combined workload, pause/resume and ordinary gameplay on Switch.
- [x] Verify matching run IDs, destructor counts and zero remaining owner allocations/resources.
- [x] Repeat Load/Kill and check that the worker and square cease after stopping.
- [ ] Measure frame cost, actual RAM and long-session behavior separately.
- [ ] Complete the remaining [platform/capacity sequence](PLATFORM-CAPACITY1.md).

### Physical Test1 result - 2026-10-30

The [console acceptance record](../../evidence/combined-acceptance1-console-20261030/console-acceptance.json)
identifies the installed runtime, NRO, PNG, NPDM and report hashes. All match the
retained candidate and kit. The runtime configuration is unchanged from the
preceding hardware smoke. Disarm and Footprints remain running with zero loader
or action errors.

The completed operator checklist covers the initial 30-second minimum,
pause/resume, ordinary gameplay and Load/Kill/Load/Kill. These observations are
attested rather than independently timed or visually instrumented. The retained
status and cleanup files are identical and both identify `phase=stopped`, `run=2`.
The first run's files were overwritten; its separate cleanup counts are not retained.

| Final-run observation | Result |
| --- | --- |
| Chunked payload | 262,144 bytes written and verified; complete pattern independently rechecked |
| Numeric XML | 57 roundtrips; all 128 saved records independently rechecked |
| Catalog and mutations | Payload found; sentinel rename/delete verified |
| Worker and rendering | 1,844 worker ticks; 1,900 callback frames; zero draw failures |
| Borrowed entities | 556 samples with a non-null borrow |
| Final cleanup | One stop callback and session destructor; zero owner allocations and registrations |
| Heap release | Trim succeeded; zero charged bytes, allocation failures or release failures |

The probe's peak requested heap payload is 45,381 bytes. This excludes mapped
code, stacks and texture backing and does not measure process RAM. The loader
retains 36,864 texture bytes while reporting no active probe callbacks. Its
additional-entry count records retained registration, not a running worker.
Metadata growth was not exercised. This result accepts this workload and final
cleanup on the exact runtime; it does not promote the complete candidate matrix.

Texture backing and mapped code remain resident under existing contracts.
Allocation cleanup does not imply their release. This probe does not cover
capacity growth, a second account, entity mutation/streaming, all game transitions
or a substantial port's complete native requirements. It supplements the original
Test1 demos and does not retroactively extend their acceptance.
