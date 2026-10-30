# DLC startup diagnostics and recovery

The [verification record](../../evidence/dlc-startup-recovery1-20261030/verification.json)
identifies the implementation and locally verified packages. The earlier
[console investigation](PLATFORM-CAPACITY1.md) records launch recovery after DLC
path coverage was corrected. The new helper was tested against disposable SD
fixtures; it has not been applied to a physical Switch installation.

## Player workflow

- [x] Explain Atmosphere's merged RomFS, the observed failure and recovery limits.
- [x] Document storage cleanup without assuming a performance improvement.
- [x] Provide a read-only check for eight retained base archives and 17 optional paths.
- [x] Recover only absent optional paths with verified blank 512-byte placeholders.
- [x] Preserve existing archives, settings, saves, modules and DLC registration.
- [x] Record additions before writing and make interrupted recovery safe to repeat.
- [x] Refuse missing base archives, invalid existing paths and links/junctions.
- [x] Include independent player tooling in shared runtime/plugin packaging.
- [x] Add Mod Porter diagnostic controls, a CLI workflow and bounded AFE2 import.
- [ ] Accept the new packaged helper on the intended physical installation.

## Completed local validation

Ten helper tests passed using scratch SD fixtures, including preservation,
idempotence, single-path recovery, invalid roots, missing/truncated archives,
linked paths and independent-package execution. The desktop suite passed 220
checks with its compiler groups enabled, including NRO build/install ZIP coverage.
The packaged WPF application passed 2,978 layout checks across 158 scenarios,
including narrow windows and simulated 100–200% DPI. ZIP contents and hashes were
read back after packaging. Repository documentation and portable checks passed.

The [player guide](../guides/DLC-RECOVERY.md) is linked from installation, asset
cleanup, Mod Porter and the repository introduction. Existing published downloads
are unchanged; new packages include the helper through the shared staging path.

## Acceptance boundaries

The helper targets the known build-9b485eb8 loose-RomFS profile. It does not detect
the installed NSP/build, restore playable DLC assets, adjust DLC registration,
verify archives/media or establish a performance improvement. The precise failed
Atmosphere assertion and minimum required placeholder set remain unidentified.
Fatal import identifies the process and records offsets; it does not use another
build's symbols to assign a cause. No runtime implementation was changed, and
physical profile/capacity acceptance remains a separate gate.
