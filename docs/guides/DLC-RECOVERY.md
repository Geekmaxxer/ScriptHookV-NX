# DLC removal and startup recovery

Removing a large DLC archive can expose a different installed RomFS entry to
Atmosphere. For the build-9b485eb8 loose-RomFS profile, covering missing optional
DLC paths with blank template placeholders resolved a console startup abort.
Keep a backup and preserve the matching path coverage when reducing disk usage.

The maintained [player recovery guide](../../packaging/GAME-STARTUP.md) explains:

- Why an ams.mitm fatal can occur before GTA and its plugins start.
- Which eight retained base packs require their original archives.
- How to check the 25 known DLC paths without changing files.
- How explicit recovery fills only absent optional paths and records additions.
- What placeholders cannot restore and what to collect if failure persists.

Every new runtime/plugin install package carries **Check-Game-Layout.cmd**, its
PowerShell implementation and **GAME-STARTUP.md** through the shared packaging
workflow. Existing downloads require a new package; they are not updated in place.

In Mod Porter, use **Tools > Diagnostics > Check game layout**. Review the missing
paths and profile before selecting **Recover missing DLC paths**. The developer
CLI provides `game-layout --sd <SD-root> --sdk <SDK>` and explicit `--recover`.
Import an AFE2 fatal `.bin` report to identify its process and module-relative trace.
The decoder does not assign a cause from a process ID or an offset from another build.

The [console investigation](../verification/PLATFORM-CAPACITY1.md) records the
observed recovery and its acceptance limits. The exact failed Atmosphere assertion
and minimal necessary placeholder set remain unidentified. These tools do not
verify the installed NSP, improve game performance, or establish runtime acceptance.
