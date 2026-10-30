# Cooperative preset catalog verification

This SDK milestone adds a complete-result preset catalog to the unchanged Test1
runtime. Pending batch 3 XML/save/recovery checks join the new catalog workload
in service batch 4. Its hardware run remains deferred so further implementation
can precede the next consolidated acceptance session. Earlier artifacts and
local records remain available with their original acceptance limits.

## Implementation and local verification

- [x] Filter file suffixes while retaining optional directories and exact names.
- [x] Sort collected entries cooperatively using ASCII case folding.
- [x] Conceal entries until traversal, sorting and duplicate checks complete.
- [x] Enforce matched-entry limits and skip unaddressable relative paths.
- [x] Preserve underlying I/O errors and refuse missing optional services.
- [x] Exercise short/malformed native pages and owner-memory allocation refusal.
- [x] Release collected storage on cancellation and recreate state after Kill/Load.
- [x] Add a multi-page preset browser to the existing XML load/save/restore probe.
- [ ] Accept mixed directory pages, sorted selection and XML/save/recovery on Switch.

The [local verification record](../../evidence/catalog-scan1-local-20261030/verification.json)
identifies source hashes, artifacts and executed checks. The catalog fixture and
workflow execute real compiled ARM64 runtime/SDK instructions with modeled OS,
RO, filesystem, game boundaries and outer scheduling. This establishes neither
SD latency nor physical gameplay acceptance.

## Deferred combined hardware batch

The [service checklist](../../examples/service-acceptance/README.md) includes
the existing large XML input, growable serialization, verified prepared output,
publication and explicit backup restoration. The same `xml-workflow` probe now
also creates 13 XML presets, one filtered text file and one nested directory;
it traverses four pages, verifies sorted metadata, and parses a selected preset.
Completed subsequent runs verify retained contents. Reports expose catalog
counts and stage 9 failures without changing the previously documented stages.

The kit is a prepared artifact for the next acceptance session, not a request
for a separate immediate hardware run. Previously accepted batch 1 probes and
the long capacity soak do not require repetition. Batch 2 and batch 3 do not
need standalone hardware runs; this workload includes their pending checks.

Directory completeness under concurrent changes, media failures, power-loss
durability, broader transitions, full-capacity RAM and presentation frame time
remain separate acceptance requirements. See the
[catalog contract](../reference/PRESET-CATALOGS.md) for lifetime and consistency limits.
