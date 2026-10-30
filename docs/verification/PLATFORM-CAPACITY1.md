# Platform profile and capacity candidate

This unreleased candidate adds active-profile nickname access and allocation-driven
capacity settings. It extends the owned-resource candidate while preserving the
published runtime and installed game files. Local instruction execution and
physical gameplay acceptance are separate stages.

## Console observations - 2026-10-30

The [observation record](../../evidence/platform-capacity1-console-20261030/verification.json)
captures a failed physical launch and a scoped Eden run of the exact Test1 runtime.
The installed runtime and both Eden demo NROs match the retained test kit.

The physical report records `std::abort` (`0xFFE`) in Atmosphere's `ams.mitm`
process (`010041544D530000`). Analysis of the exact installed KIP identifies the
RomFS metadata builder and a merged assertion-abort site. Several failed checks
share that call site, so the exact assertion remains unresolved. Later DLC-path
isolation and reported launch recovery are recorded below. The initial working
baseline assumption was superseded by the DLC-removal chronology.
The initial physical loader report predated this candidate. The later exact-artifact
hardware run below supersedes that startup uncertainty within its recorded scope.

Eden's report records four loaded modules, zero loader errors, successful completion
of the profile demo, and an active managed session with one registered resource.
A nonempty valid UTF-8 nickname and a managed cleanup-callback marker exist.
The latest managed session remains active. These files establish scoped demo
observations; they do not verify complete allocation reclamation, the selected
profile shown in the emulator UI, transitions or extended-session behavior.
Automatic capacity was not selected and no metadata growth was exercised.

### Runtime-only baseline comparison

The [baseline comparison record](../../evidence/platform-capacity1-console-20261030/baseline-comparison.json)
verifies that the SD card contains the retained accepted runtime (A). Its new
report, `report_0476ea03b.bin`, has the same error, process, module-relative PC
and normalized backtrace as Test1. The installed Atmosphere KIP also matches the
binary used for analysis. A Test1-specific regression has not been established;
a fault shared by runtime versions or another launch component remains possible.
The shared abort site still does not identify one unique failed assertion.

- [x] Verify the installed A runtime against the retained artifact.
- [x] Decode the A fatal report and compare it with Test1 after address normalization.
- [x] Recheck the installed Atmosphere binary against the analyzed KIP.
- [x] Record a failed launch after removal of both SD `subsdk4` and `subsdk5` overrides.
- [x] Restore the override and verify startup before continuing candidate acceptance.

### Runtime-absent inspection and DLC chronology

The [subsequent inspection](../../evidence/platform-capacity1-console-20261030/runtime-absent-inspection.json)
confirms that both SD module files are absent. The current ExeFS overrides contain
only `subsdk9` and `main.npdm`. All five retained fatal reports share the same error,
process, module-relative PC and normalized call chain. The individual later reports
have not been assigned to unique removal steps. NPDM hashes also differ between
recorded snapshots; the latest setup is a distinct configuration.

DLC files were removed before Test1, with no successful launch check between that
removal and Test1. The earlier working launch therefore does not establish a working
post-removal baseline. At that inspection, the SD overlay lacked 17 DLC paths
present in the packer's base template. The subsequent path-correction result is
recorded below.

A read-only `chkdsk D:` scan completed with no filesystem structural problems
reported. The volume's dirty flag was observed. No repair or sector-health scan
was performed; these results do not validate archive contents or physical media.

The named local NSP was read and its program NCA verified against the retained
container. Its RomFS contains 82 files, while the packer template contains 139.
The incomplete retained SD metadata differs from that local package inventory.
Installed on-device package contents remain unverified. A model using the named
NSP does not reproduce the original-storage offset assertion, so neither that
assertion nor a package mismatch is established as the cause.

An experimental diagnostic covers only the 17 absent paths with the template's
blank 512-byte placeholders (8,704 bytes total). Existing game files are preserved.
Apply/restore and refusal guards passed in scratch fixtures; read-only preflight
passed on the physical SD. Its subsequent placement and launch result are recorded below.
The placeholders contain no playable DLC assets. A changed failure stage would
narrow the investigation without establishing a completed repair.

- [x] Preserve and normalize all available fatal reports.
- [x] Inspect retained DLCs and compare missing paths with the base template.
- [x] Inspect the named NSP without changing the installed package.
- [x] Complete read-only filesystem checking and guarded diagnostic preparation.
- [x] Inspect the diagnostic placement and record its console launch result.
- [x] Read back the successful Test1 runtime, menu, example and configuration identities.
- [ ] Independently verify installed NSP contents if a package audit is required.

### Startup recovery after DLC path correction

The [startup resolution record](../../evidence/platform-capacity1-console-20261030/startup-resolution.json)
captures a successful console launch reported after correcting the placeholder
placement. All 17 matching files had been found under
`romfs/update/update/switch/dlcpacks`, with an extra `update` component. Their
intended destinations under `romfs/update/switch/dlcpacks` were absent during that
failed launch configuration. The console operator corrected the placement.

The result links the reported startup failure to the deleted DLC overlay paths.
The precise Atmosphere assertion and smallest required placeholder set remain
unidentified. Blank placeholders provide path coverage without restoring playable
DLC content. Preserve the working layout during runtime testing; removing the
placeholder coverage without a replacement profile can reintroduce the failure.

That first recovery observation had no post-launch SD snapshot and did not
establish Test1 acceptance. The subsequent physical demo run below verifies the
restored modules and records its narrower completed acceptance checks.

- [x] Record console launch recovery after correction of the DLC path coverage.
- [x] Verify the working configuration with Test1 and the menu module restored.
- [ ] Complete the profile, ownership, capacity and combined-workload hardware checks.

The RomFS folder contains loose game data and must remain present for this test.
Atmosphere resizes and rebuilds `romfs_metadata.bin` during startup; a stale-cache
repair is not established by these reports. Retained metadata can be incomplete
after an interrupted build and does not by itself prove game-file corruption.
Physical acceptance remains open. No runtime or firmware patch is justified by
the merged abort site alone. The investigation has made no SD writes.

### Physical Test1 demo run

The [hardware record](../../evidence/platform-capacity1-console-20261030/hardware-smoke.json)
verifies the installed Test1 runtime (`930d3043...`), both test NROs and shared
NPDM against the retained kit. The menu module is restored. The complete loader
snapshot reports stage complete, zero loader/configuration/action errors, five
resident modules and three running scripts.

The profile demo completed without a loader error and produced nonempty valid
UTF-8 output. Matching the selected Switch account was confirmed for one profile.
The nickname is excluded from public evidence. The managed demo remained active
with one registered resource and zero resource failures; at least 30 seconds of
execution was reported. Duration was not instrumented. The start marker exists.
Kill Process was not used, so no managed cleanup marker is expected yet.

The configuration is 16 resident slots, 32 script slots, a 10 MiB plugin budget
and 2 MiB image/BSS limits. Automatic settings are disabled, dynamic metadata is
zero and retained stack storage is 1 MiB. This run does not exercise metadata
or additional-stack growth. Other resident code does not imply additional running
mods or gameplay acceptance.

The read-only layout check finds all 25 known DLC paths covered. Seventeen
optional paths still contain blank placeholders; full playable DLC restoration
has not been verified. No new fatal report appeared relative to the preceding
retained inventory. The successful runtime state is the stronger startup evidence.

- [x] Verify exact candidate/runtime, demo and configuration identities.
- [x] Accept physical startup and manual loading of both demos.
- [x] Accept nickname matching for one selected Switch account.
- [x] Record the managed workload's initial active run and registration state.
- [ ] Verify Kill Process cleanup, zero remaining registrations and repeatable restart.
- [ ] Verify a second selected profile, capacity growth and the combined hardware sequence.

## Implementation

- [x] Audit SDK profile exports with IDA Pro 9.5 and verify their exact target bytes.
- [x] Bind an independent optional profile table without changing existing ABI layouts.
- [x] Validate owner thread, output storage, open-user ambiguity and UTF-8 names.
- [x] Support automatic and larger positive values for all five runtime settings.
- [x] Retain integer-conversion, binary-format and independent service validation.
- [x] Grow catalog, scheduler, additional-entry and heap/resource owner metadata on demand.
- [x] Preserve published addresses and suspended stacks while tables grow.
- [x] Allocate additional stacks at admission and retain them for reuse.
- [x] Replace running-state bitmasks with per-owner state comparisons.
- [x] Report dynamic metadata, retained stack storage and allocation errors.
- [ ] Accept account selection, allocation behavior and combined workloads on Switch.
- [ ] Accept the profile workflow in the intended Eden build.

The retained [local verification summary](../../evidence/platform-capacity1-local-20261030/verification.json)
identifies exact binaries, audited SDK offsets, source hashes and completed checks.
Profile tests model the SDK/account boundary. Capacity/lifecycle tests model OS
allocation, RO, filesystem and outer scheduling. They do not establish gameplay,
Horizon resource availability or measured frame/stack/RAM behavior.

## Required hardware sequence

The [combined probe](COMBINED-ACCEPTANCE1.md) packages storage, numeric XML,
managed cleanup, an additional script, a PNG callback and borrowed ped sampling
for this sequence. It uses the existing Test1 runtime and leaves both original
demo artifacts unchanged. Its [physical result](COMBINED-ACCEPTANCE1.md#physical-test1-result---2026-10-30)
accepts the scoped workload and final-run cleanup. That combined result does not
cover capacity growth, stress or the second selected profile.

The next [controlled capacity kit](CAPACITY-ACCEPTANCE1.md) crosses the module
extent and bootstrap stack thresholds on the same runtime. It supplies Defaults,
Explicit and Automatic phases, preserves original configuration and captures
available phase reports. Defaults refusal and the Explicit/Automatic workloads are now
accepted: large extents, 36 workers, extra stack/metadata admission and final-run
nested cleanup. Automatic mask `0x1f` and the same bounded allocation footprint
are verified. Original configuration restoration and fresh startup are also
verified. Test2 additionally accepts both instrumented lifetimes and identifies
synchronous report-save stalls. Broader performance/soak acceptance remains open.

- [x] Record the exact installed candidate, example and configuration identities.
- [ ] Independently audit installed package contents if its game/SDK identity changes.
- [x] Start with compiled defaults and existing accepted plugins; verify expected large-image refusal.
- [ ] Run the user-profile example with two different selected Switch profiles.
- [x] Confirm profile output matches one selected Switch account.
- [ ] Verify matching across the second selected account/save context.
- [x] Run the combined managed workload, cancel, restart and verify final-run cleanup counts.
- [x] Combine numeric XML, containers, files, callbacks/textures and existing mods.
- [x] Record pause/resume and repeated Kill/Load for the combined workload.
- [ ] Exercise loading, death, character transitions and broader nested cleanup.
- [x] Accept the bounded Explicit workload's stack growth and nested child/parent reclamation.
- [ ] Increase explicit capacities incrementally and record actual admission/errors.
- [x] Exercise automatic settings independently and verify the preserved original backup.
- [x] Verify restored original configuration bytes against the retained backup.
- [x] Verify fresh normal startup with the restored configuration and expected large-probe refusal.
- [x] Prepare locally verified operation timing and bounded run history for the capacity workload.
- [x] Accept both Test2 diagnostic lifetimes and identify synchronous report-save stalls.
- [ ] Measure peak RAM, frame cost, whole-stack use and an extended-session soak.
- [ ] Verify candidate installation and rollback using exact artifact identities.

For Eden, record its exact version, configured profile and launch route. Verify
direct and HOME-menu launch separately; emulator acceptance does not replace
Switch acceptance.

## Development gate

Substantial scheduler/loader changes require targeted integration acceptance.
Compiler-managed global initialization and Menyoo compatibility remain separate
contracts. The combined probe accepts a specific heap/library/storage/ownership
workload on the exact Test1 runtime; Explicit/Automatic capacity runs add bounded
stack/metadata growth, and Test2 supplies instrumented gameplay and cleanup.
Resident-module growth, all transitions and the complete service matrix remain open.

SDK adaptations built on these accepted runtime services can proceed with focused
local checks. Group pending hardware cases into combined workloads and retain
separate artifact identities. The next [service batch](../../examples/service-acceptance/README.md)
combines shared C++ session state, publication and nested catalog access. Remaining
measurement items do not establish acceptance for untested functionality.

Fixes supported by local failures, documentation and test preparation can proceed
within their own validation scope. Hardware failures must be recorded against
the exact tested candidate before revising implementation or acceptance status.

The [Test2 hardware result](CAPACITY-DIAGNOSTICS2.md) identifies report-save stalls
on the unchanged runtime. Prior file readback confirms the original configuration was
restored, and a fresh loader report verifies startup with the original limits.
The large probe's `0x10023` refusal under those limits is expected. This closes
the restoration step; the broader hardware acceptance gate remains open.
