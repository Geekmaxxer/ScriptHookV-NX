# GTA startup and DLC path recovery

Deleting a loose DLC archive can change the filesystem Atmosphere presents to
GTA. For the build-9b485eb8 profile, a console startup failure was resolved by
covering missing DLC paths with the packer template's blank 512-byte placeholders.
This guide covers that specific recovery and the limits of the evidence.

## Before removing DLC content

1. Close GTA and keep a PC backup of the working game files and DLC configuration.
2. Confirm which packs the selected game configuration needs. The eight retained
   base packs below require their original archives.
3. When removing a known optional pack's large archive, retain its path with a
   matching template placeholder. Run the check after cleanup and before launch.
4. Update the game's DLC registration separately when the chosen configuration
   requires it. This helper does not edit `update.rpf` or `dlclist.xml`.
5. Test a single change before removing more content. Storage reduction does not
   establish a performance improvement or safe removal of a particular pack.

Double-click **Check-Game-Layout.cmd** and enter the SD root. The default check
makes no changes and requires Windows PowerShell 5.1 or newer, without Python,
devkitPro, keys or game-archive extraction. It inspects 25 known DLC paths.

## Why a fatal can appear before GTA starts

Atmosphere's LayeredFS merges installed RomFS content with loose files on the SD
card. Removing an SD override may expose an underlying package entry rather than
remove the file from the merged view. That merged layout depends on the installed
package as well as the remaining loose files. A failure in this preparation can
stop `ams.mitm` before GTA's scripts or ScriptHookV-NX can issue their own error.

`010041544D530000` identifies **Atmosphere's ams.mitm service**. It is not a DLC
identifier. It also is not enough on its own to identify this fault. In the
recorded case, repeated `0xFFE` abort reports traced into the RomFS metadata
builder, including launches with the SD runtime and menu modules absent. Correct
coverage of the removed DLC paths restored console startup. The exact failed
assertion and smallest necessary placeholder set remain unidentified.

There is no established persistent list of previously loaded DLCs that needs to
be cleared. Atmosphere rebuilds `romfs_metadata.bin`; deleting that file or game
saves is not an established repair for this case.

## Recover missing optional paths

Use this recovery for the port maker's **build-9b485eb8** loose-RomFS setup, title
`0100b00b51230000`. The helper does not detect the installed game build or inspect
the installed NSP. A full-data NSP or a different port/package needs its own audit.

Close GTA. With the SD mounted, open PowerShell in this helper's folder and run:

```powershell
.\Check-Game-Layout.cmd -SdRoot D:\
.\Check-Game-Layout.cmd -SdRoot D:\ -Action Recover
```

`Recover` creates blank 512-byte files **only at missing known optional paths**.
With all 17 paths missing, it adds 8,704 bytes of placeholder data; filesystem
allocation uses additional space. It preserves every existing archive, mod,
permission file, setting and save. It refuses recovery if a retained base archive
is missing or too small, an existing optional path is invalid, or a checked path
contains a link/junction. It records planned additions before writing, verifies
new files and never replaces an existing file. An interrupted recovery can be
run again; retain its records for diagnosis.

Records are saved under `switch/gtav-scripthook/game-layout-records/`. A placeholder
provides path coverage, **not playable DLC content**. Replace it with the matching
original archive when restoring that DLC. Keep the working coverage when saving
space; removing it again can reproduce the startup failure. For another DLC
profile, restore the complete matching files and configuration from the PC backup.

Safely eject, test one launch, and record whether Story Mode loads or the failure
changes. A successful layout check is not a game-integrity or gameplay test.

| Retained base packs: original archives required | Known optional paths covered by recovery |
| --- | --- |
| `mpbiker`, `mpchristmas2`, `mpheist`, `mppatchesng` | `mpassault`, `mpbattle` (`dlc.rpf`, `dlc1.rpf`), `mpchristmas2017`, `mpchristmas2018` |
| `patchday1ng`, `patchday2bng`, `patchday2ng`, `patchday3ng` | `mpexecutive`, `mpgunrunning`, `mpheist3`, `mpheist4` (`dlc.rpf`, `dlc1.rpf`, `dlc2.rpf`) |
| | `mplowrider`, `mplowrider2`, `mpluxe`, `mpluxe2`, `mpsecurity` (`dlc.rpf`, `dlc1.rpf`) |

Paths use `update/switch/dlcpacks/<pack>/dlc.rpf` unless another filename is shown.
The table defines this recovery profile; it is not a universal DLC compatibility list.

## If startup still fails

- Save the new Atmosphere report from `atmosphere/fatal_errors/` and the layout
  check output. In Mod Porter, use **Tools > Diagnostics > Import Switch diagnostic
  report** for an AFE2 `.bin` report or a runtime text report.
- Record changes made since the last successful launch, the port build, installed
  NSP identity, Atmosphere version, ExeFS modules and runtime configuration.
- For an in-game script/plugin error, preserve `loader-report.txt` and disable the
  specific plugin before retesting. Its report may be stale when startup fails.
- Restore a known working DLC profile from backup before changing firmware or
  runtime code. Preserve the rest of the loose RomFS; it contains game data.
- If filesystem errors or unreadable files are reported, back up accessible data
  and investigate storage separately. This helper does not repair filesystems.

## Evidence and implementation references

The console launch recovery is recorded in the project's
[platform/capacity verification](https://github.com/Geekmaxxer/ScriptHookV-NX/blob/main/docs/verification/PLATFORM-CAPACITY1.md).
Atmosphere's [RomFS builder](https://github.com/Atmosphere-NX/Atmosphere/blob/1.11.2/stratosphere/ams_mitm/source/fs_mitm/fsmitm_romfs.cpp)
and [SD file helper](https://github.com/Atmosphere-NX/Atmosphere/blob/1.11.2/stratosphere/ams_mitm/source/amsmitm_fs_utils.cpp)
document the merge and metadata-file rebuild. This recovery changes neither implementation.
