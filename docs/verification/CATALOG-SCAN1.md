# Cooperative preset catalog verification

This SDK milestone adds a complete-result preset catalog to the unchanged Test1
runtime. Service batch 4 has scoped physical acceptance for multi-page preset
selection and the combined XML load/save/recovery workflow. Earlier artifacts
and local records retain their original acceptance limits.

## Implementation and local verification

- [x] Filter file suffixes while retaining optional directories and exact names.
- [x] Sort collected entries cooperatively using ASCII case folding.
- [x] Conceal entries until traversal, sorting and duplicate checks complete.
- [x] Enforce matched-entry limits and skip unaddressable relative paths.
- [x] Preserve underlying I/O errors and refuse missing optional services.
- [x] Exercise short/malformed native pages and owner-memory allocation refusal.
- [x] Release collected storage on cancellation and recreate state after Kill/Load.
- [x] Add a multi-page preset browser to the existing XML load/save/restore probe.
- [x] Accept mixed directory pages, sorted selection and XML/save/recovery on Switch.

The [local verification record](../../evidence/catalog-scan1-local-20261030/verification.json)
identifies source hashes, artifacts and executed checks. The catalog fixture and
workflow execute real compiled ARM64 runtime/SDK instructions with modeled OS,
RO, filesystem, game boundaries and outer scheduling. This establishes neither
SD latency nor physical gameplay acceptance.

## Combined hardware sequence

The [service checklist](../../examples/service-acceptance/README.md) includes
the existing large XML input, growable serialization, verified prepared output,
publication and explicit backup restoration. The same `xml-workflow` probe now
also creates 13 XML presets, one filtered text file and one nested directory;
it traverses four pages, verifies sorted metadata, and parses a selected preset.
Completed subsequent runs verify retained contents. Reports expose catalog
counts and stage 9 failures without changing the previously documented stages.

Previously accepted batch 1 probes and the long capacity workload did not need
repetition. Standalone batch 2 and batch 3 hardware runs were consolidated into
this workload. Their original artifacts were not retroactively tested.

## Physical batch 4 result - 2026-10-30

The [hardware record](../../evidence/service-batch4-console-20261030/acceptance.json)
matches the installed Test1 runtime and the exact batch 4 `xml-workflow` NRO.
The operator completed two natural runs and closed GTA. The retained result is
`run=2`, `failed=0`, `stage=8`, with successful publication, restoration and
catalog verification. Cleanup reports `run=2`, `owner_cleanup_complete=1`.

Independent host checks verified every byte of the 262,144-byte input, the
98,376-byte serialized replacement and the restored 29-byte original. The
replacement contains the expected value and all 98,304 payload characters.
Neither a prepared file nor a backup remains after explicit restoration.
All 13 mixed-case XML presets, the filtered text sample and the empty nested
directory match the fixture. The probe verifies 14 sorted entries across four
pages, one skipped text file and successful parsing of the selected preset.

The loader reports no loader, configuration, plugin, callback or resource errors.
The probe is resident and inactive with no registered resources; Disarm and
Footprints remain running. Explicit 16/48/16/3/3 configuration bytes match the
previous accepted session. This review performed no SD writes.

The first-run result and cleanup files were overwritten by the second run.
First-run completion is an operator observation; retained file contents and
the second run's completed-state verification support publication/restoration.
No independent first-run timing or cleanup snapshot remains. The other three
kit NROs were installed but not loaded during this session; their earlier
acceptance records remain separate.

Directory completeness under concurrent changes, media failures, power-loss
durability, broader transitions, full-capacity RAM and presentation frame time
remain separate acceptance requirements. See the
[catalog contract](../reference/PRESET-CATALOGS.md) for lifetime and consistency limits.
