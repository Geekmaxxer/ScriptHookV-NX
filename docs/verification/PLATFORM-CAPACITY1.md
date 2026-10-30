# Platform profile and capacity candidate

This unreleased candidate adds active-profile nickname access and allocation-driven
capacity settings. It extends the owned-resource candidate while preserving the
published runtime and installed game files. Local instruction execution and
physical gameplay acceptance are separate stages.

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

- [ ] Record exact title/build, runtime hash, SDK/plugin hashes and configuration.
- [ ] Start with the standard defaults and an existing accepted plugin.
- [ ] Run the user-profile example with two different selected Switch profiles.
- [ ] Confirm profile output follows the account/save actually used by GTA.
- [ ] Run managed resources, cancel during waits, restart and inspect cleanup counts.
- [ ] Combine numeric XML, containers, files, callbacks/textures and existing mods.
- [ ] Exercise pause/loading/death/character transitions and repeated Kill/Load.
- [ ] Increase explicit capacities incrementally and record actual admission/errors.
- [ ] Exercise automatic settings independently and preserve prior configuration.
- [ ] Measure peak RAM, frame cost, whole-stack use and an extended-session soak.
- [ ] Verify candidate installation and rollback using exact artifact identities.

For Eden, record its exact version, configured profile and launch route. Verify
direct and HOME-menu launch separately; emulator acceptance does not replace
Switch acceptance.

## Development gate

Further substantial scheduler/loader expansion, global initialization support and
Menyoo compatibility claims should wait for the hardware sequence above. The
accumulated heap, library, storage, managed-ownership and capacity candidates have
not inherited the published runtime's gameplay acceptance. Additional API work
would otherwise enlarge the untested integration surface.

Fixes supported by local failures, documentation and test preparation can proceed
within their own validation scope. Hardware failures must be recorded against
the exact tested candidate before revising implementation or acceptance status.
