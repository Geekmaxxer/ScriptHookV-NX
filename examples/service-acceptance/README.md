# Service acceptance batch 8

Batch 8 verifies the growing cleanup registry with one manual resource-registry
probe. The four Batch 7 workloads completed on physical Switch hardware; their
accepted results and exact runtime remain recorded separately. They do not need
separate repeat installations for this batch.

## Build and contents

```powershell
python tools/build.py --output out/batch8-core --devkitpro C:/devkitPro
python tools/build_service_acceptance.py --runtime out/batch8-core/subsdk5 --output out/batch8 --devkitpro C:/devkitPro
```

The builder verifies the exact growing-resource kernel, compiles the manual NRO,
preserves dependency notices and checks ZIP inventory/readback. No game files are
required. Runtime/NPDM preservation tools are under `RUNTIME-CANDIDATE/`; the probe
is under `INSTALL/switch/`. No shared runtime.ini, settings, game archives, bindings
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
5. Preserve the completed report before optional Kill during construction or
   verification changes its counters. Within the same session, Kill while it is
   active and Load again; cleanup must finish and the next lifetime must complete.
6. Include normal gameplay, existing mods, pause/resume and any loading, death or
   character transitions that can be observed in this session. A longer session
   can collect timing/coexistence observations without another SD transfer.
7. Close GTA and preserve the result and loader report. To roll back, restore the
   backed-up exefs files with GTA closed and fully restart. Record rollback scope
   separately from probe completion.

One resident slot is required. There is no SD removal between steps. Earlier
accepted probes may remain installed; replacing them is unnecessary. Do not
remove media or damage files during play. Keep failures with their exact action
sequence and artifact identities.

## Expected reports

Paths are relative to `switch/gtav-scripthook/`.

| Report | Expected after two natural runs |
| --- | --- |
| `settings/resource-registry-example/result.txt` | run=2, failed=0, completed=1, registered=512, verified=512, destroyed=512, peak_resources=513, reverse_order_verified=1, owner_cleanup_complete=1, owned_allocations=0, registered_resources=0 |
| `loader-report.txt` | Matching runtime/configuration, probe resident and inactive, no unexpected load/resource failures, probe resource_capacity at least 513, retained resource_metadata_bytes |

Loader numeric quantities are hexadecimal. Record capacity and shared metadata
remain resident for reuse after cleanup; live owner allocation/registration counts
must return to zero. Partial cancellation may report completed=0 and fewer
verified objects while preserving correct reverse order and final cleanup.

## Acceptance limits

The probe changes no game entities or saves. It verifies a bounded workload and
cannot establish arbitrary mod compatibility. Larger configurations, nested
failures, low memory, peak RAM, frame time, stack use, fragmentation, storage
failures and extended sessions remain independently scoped. Group observations
in this later session; no standalone repeat of accepted XML Batch 4 or the
Batch 7 library, texture, registry and heap workloads is requested.

See [resource growth](../../docs/verification/RESOURCE-CAPACITY1.md) and
[Batch 7 acceptance](../../docs/verification/SERVICE-BATCH7.md).
