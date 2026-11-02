# Service acceptance batch 8

Batch 8 combines cleanup-registry growth, read-only streams, captured actions and
JSON presets in four manual probes. The four Batch 7 workloads completed on physical Switch hardware; their
accepted results and exact runtime remain recorded separately. They do not need
separate repeat installations for this batch.

## Build and contents

```powershell
python tools/build.py --output out/batch8-core --devkitpro C:/devkitPro
python tools/build_service_acceptance.py --runtime out/batch8-core/subsdk5 --output out/batch8 --devkitpro C:/devkitPro
```

The builder verifies the exact growing-resource kernel, compiles all four manual NROs,
preserves dependency notices and checks ZIP inventory/readback. No game files are
required. Runtime/NPDM preservation tools are under `RUNTIME-CANDIDATE/`; the probes
are under `INSTALL/switch/`. No shared runtime.ini, settings, game archives, bindings
or automatic installer are included. Current configuration is preserved.

The new kernel changes internal resource records and heap pin tracking; it has
local verification and requires its own physical acceptance. Batch 7 acceptance
does not automatically transfer to this modified artifact. The published runtime
remains unchanged.

## One combined session

1. Close GTA fully. Back up the installed title `exefs/subsdk5` and `main.npdm`
   outside the installation tree. Retain the accepted runtime for rollback.
2. Use RUNTIME-CANDIDATE for this exact candidate. Follow its SD-ACCESS.md procedure
   for a customized NPDM so existing capabilities are preserved.
3. Copy INSTALL/switch to the SD root. Preserve configuration, other plugins,
   bindings, game files and saves. Fully restart GTA once.
4. Load **resource-registry** twice to natural completion. It has no visible
   overlay. Each run registers and checks 512 individually pinned objects, then
   destroys them in reverse registration order and records the final audit.
5. Load **read-streams** twice to natural completion. It creates and verifies its
   own binary/text/XML fixtures, checks buffered reads and seeking, and closes
   retained streams during final cleanup. It has no visible overlay.
6. Load **task-queue** twice to natural completion. It executes 768 ordered actions
   with owned string captures and mixed inline/heap targets, wraps and relocates
   its queue, then verifies final capture/owner cleanup. It has no visible overlay.
7. Load **json-presets** twice to natural completion. It loads and edits its own
   JSON fixture, serializes a bounded candidate, verifies publication and backup
   bytes, and restores the original file. It has no visible overlay.
8. Preserve all four completed reports before optional Kill changes their counters.
   Within the same session, test resource-registry during construction/verification
   and read-streams during binary reading, then Load again. For task-queue, test
   Kill during construction and execution followed by Load. For json-presets,
   cancel during file loading after one full run restored its fixture. Cleanup
   must finish and the next lifetime must complete. Cancel read-streams only
   after its first full run has created the fixtures.
9. Include normal gameplay, existing mods, pause/resume and any loading, death or
   character transitions that can be observed in this session. A longer session
   can collect timing/coexistence observations without another SD transfer.
10. Close GTA and preserve all results and the loader report. To roll back, restore the
   backed-up exefs files with GTA closed and fully restart. Record rollback scope
   separately from probe completion.

Four available resident slots are required. There is no SD removal between steps. Earlier
accepted probes may remain installed; replacing them is unnecessary. Do not
remove media or damage files during play. Keep failures with their exact action
sequence and artifact identities.

## Expected reports

Paths are relative to `switch/gtav-scripthook/`.

| Report | Expected after two natural runs |
| --- | --- |
| `settings/resource-registry-example/result.txt` | run=2, failed=0, completed=1, registered=512, verified=512, destroyed=512, peak_resources=513, reverse_order_verified=1, owner_cleanup_complete=1, owned_allocations=0, registered_resources=0 |
| `settings/read-streams-example/result.txt` | run=2, failed=0, completed=1, binary_verified=1, text_verified=1, seek_verified=1, xml_verified=1, owner_cleanup_complete=1, owned_allocations=0, registered_resources=0 |
| `settings/task-queue-example/result.txt` | run=2, failed=0, completed=1, queued=768, executed=768, sum=295296, peak_queue=512, wrapped_verified=1, relocation_verified=1, live_captures=0, owner_cleanup_complete=1, owned_allocations=0, registered_resources=0 |
| `settings/json-presets-example/result.txt` | run=2, failed=0, stage=10, completed=1, parsed=1, serialized=1, publication_verified=1, restored=1, owner_cleanup_complete=1, owned_allocations=0, registered_resources=0 |
| `loader-report.txt` | Matching runtime/configuration, probe resident and inactive, no unexpected load/resource failures, probe resource_capacity at least 513, retained resource_metadata_bytes |

Loader numeric quantities are hexadecimal. Record capacity and shared metadata
remain resident for reuse after cleanup; live owner allocation/registration counts
must return to zero. Partial cancellation may report completed=0 and fewer
verified objects while preserving correct reverse order and final cleanup.

## Acceptance limits

The probes change no game entities or saves. They verify bounded workloads and
cannot establish arbitrary mod compatibility. Larger configurations, nested
failures, low memory, peak RAM, frame time, stack use, fragmentation, storage
failures and extended sessions remain independently scoped. Group observations
in this later session; no standalone repeat of accepted XML Batch 4 or the
Batch 7 library, texture, registry and heap workloads is requested.

The stream probe preserves existing fixtures. If fixture creation was interrupted,
retain the reports and files before recreating only its `fixtures` directory with
GTA closed. No forced storage interruption is requested. Read-only C streams add
no kernel change beyond the already pending resource candidate.

The JSON probe preserves conflicting prepared/backup files. An interruption during
writing/publication/restoration requires inspection of its report, current.json,
prepared.json and backup.json before an explicit recovery decision. Preserve all
three paths; do not discard a backup to force a retry. Ordinary cancellation for
this batch is performed during file loading, after a complete baseline run.

## Remaining hardware coverage

Use the same later session to record coexistence, active cancellation/restart,
pause/loading/death/character transitions, longer normal play and rollback where
practical. Retain exact configuration and artifact identities. Second-profile
nickname checks, default/larger configurations, low-memory cases, actual media
errors and measured peak RAM/frame time/stack/fragmentation remain open when they
are not exercised. Do not simulate media errors by removing the card during play.
Earlier accepted results keep their original artifact/configuration scope;
new observations on this candidate do not retroactively test an older kernel.

See [JSON presets](../../docs/verification/JSON-SERVICES1.md),
[task queues](../../docs/verification/TASK-QUEUES1.md),
[read-only streams](../../docs/verification/READ-STREAMS1.md),
[resource growth](../../docs/verification/RESOURCE-CAPACITY1.md) and
[Batch 7 acceptance](../../docs/verification/SERVICE-BATCH7.md).
