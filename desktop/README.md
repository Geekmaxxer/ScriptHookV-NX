# ScriptHookV-NX Mod Porter

A Windows desktop tool for PC mod source ports, ASI dependency reports and Legacy
asset preparation. **0.9.1 Public Preview**.

Choose a starting point in the [workflow guide](../docs/MOD-PORTER-WORKFLOWS.md).
The [quality audit](../docs/QUALITY-AUDIT.md) records tested behavior and limits.

Extract the complete Windows ZIP and open `ScriptHookV-NX-Mod-Porter.exe`. Its .NET
runtime and matched SDK are included. No game files are bundled.

## Build a plugin

1. In **Import**, open a source folder/ZIP or try the **Footprints** example.
2. In **Findings**, filter the report and select the C++ files for one plugin.
3. In **Project & build**, create a copied project outside the input and SDK folders.
4. Check toolchain paths, then choose **Build NRO** or **Build NRO + Install ZIP**.

Compilation requires [Python 3.10+](https://www.python.org/downloads/windows/) and
[devkitA64 + switch-tools](https://devkitpro.org/wiki/Getting_Started).
Edit the generated `src` files to address findings, then reopen `project.json`.
**Check C++** runs the actual compiler for syntax, native signatures and active
conditional code without producing a binary. Source findings remain a separate
review of unsupported APIs and dependencies.
Original bytes, recorded edits and fingerprints remain in the project. Each build
gets a new output folder and compiler log.

Supported adaptations include standard DLL registration, custom registered entry
names, SDK includes, verified old native names and namespace spellings. Optional
`Sleep` → `WAIT` conversion is limited to cooperative script context.

ASI/DLL inspection reads structure and imports without executing the binary. It
does **not** translate Windows binaries to ARM64. Keyboard input, PC memory hooks,
executable-version guards and hosted libraries require source changes. Registered
PC cheat-string natives are flagged for controller adaptation.

## Prepare assets

For multiple models, use **Game assets > DLC collection**. Drop folders, ZIPs or
complete vehicle/PED DLCs into one collection. Open an existing dlc.rpf to expand
it. The built-in PED editor lists all models and exposes names, types, every field,
full metadata XML and asset management, with Undo/Redo. Existing pack data is retained.

**Find installed DLCs** reads the selected game's DLC list and local folders, then
lets you filter and open an existing pack. Registrations absent from the selected
folder may be stored inside the base game. Discovery never edits game files.

Under **Asset files**, replace all four PED resources from a folder, ZIP or file
selection in one operation. The selected PED keeps its name and metadata. Filter
the model list, expand **Change movement for shown PEDs**, and use **Apply movement to shown PEDs**
for a batch profile change. One Undo restores either complete operation.

The collection page separates choosing a pack, adding/reviewing models and saving
a new revision. Optional editors start collapsed. **Revert unapplied edits**
recovers a definition draft; invalid structural metadata changes leave the working
collection intact. Names and import defaults are under **Names and import options**.

**Save DLC** updates only that pack. **Build with game files** checks the selected
installation and prepares update.rpf only when its DLC ID is not registered yet.
Keep the same ID for later additions. Both actions offer folder and ZIP output.

In **Game assets > Prepare assets**, choose a folder, ZIP, OPEN RPF or loose Legacy resource. Select the
asset type and texture cap, inspect, then choose **Prepare assets** for a folder
or **Prepare assets + ZIP** to also create an archive. The defaults
are Auto and 1024 pixels; Off preserves texture sizes.

- Convert supported `y` resource extensions to `n`, platform directory/archive
  names and XML/meta references, including nested OPEN RPFs.
- Cap standalone YTD/NTD texture dimensions. Reuse encoded mip levels when
  possible; otherwise resize BC1/2/3/4/5/7 textures and preserve their BC format.
- Cap texture dictionaries reached through supported Legacy drawable/fragment
  roots. Shared graphics allocation and geometry remain intact.
- Rebuild and validate standalone texture allocations and archive tables.

**Build a loose PED add-on** creates a complete DLC directly from four matching
PED assets. Drop the folder, ZIP or four files; names and human metadata are
generated automatically. Choose Female for female movement; custom metadata
and base definitions are optional advanced controls. **Build with game files**
also registers the DLC using your current SD/game folder and creates a copied
installation package in one operation. **Add several DLC packs**
checks a collection and merges it with the current DLC list in one operation.
Multiple complete PED sets in the single-PED workflow are bundled automatically.
Registration blockers and unresolved shader dependencies appear in the report.
PED add-on builds and DLC registration packages also offer separate folder-only
and **+ ZIP** buttons. Folder-only output includes the prepared files and reports.

Use **Add to game** to select the prepared DLC/replacement
and your current RPF from an SD drive or local game folder. **Build updated game files**
reads and merges the installed DLC list directly and prepares
an SD-root output folder. Existing registrations and other archive entries remain.
**Find matching entry** and **Build archive with replacement** update an existing
resource in a copied RPF. The selected originals are never overwritten.

See [the game-file workflow](../docs/GAME-ASSETS.md) for installation, nested
archives, output space and rollback. **Prepare DLC registration** also remains
available under **Advanced: archive import files** for an already extracted `dlclist.xml`.

Asset reports have their own **Report** view. Build output shows concise messages;
enable **Full log** for compiler details or use **Copy** to copy the complete log.

Pages adapt to the window size: findings stack in narrow windows, forms scroll
when expanded, and output areas keep a readable height. Long status messages
show their full text in a tooltip.

Keyboard: **Ctrl+O** opens a mod, **Ctrl+Shift+O** opens an NX project,
**Ctrl+F** filters findings, and **Ctrl+Enter** analyzes an import or builds an NRO
on the project screen. **Esc** cancels the current operation.

Physics-child texture graphs, Enhanced/Gen9 profiles, encrypted archive tables,
incompatible rigs and clothing collections still require separate handling.

## Install and diagnose

Close GTA, then expand **SD installation** on the build screen and use **Install built NRO to SD**. New installations default to Manual
startup; updates preserve the existing Auto/Manual/Disabled filename. Previous
plugin bytes and a restore record are stored in `switch/gtav-scripthook/install-backups`.
**Restore previous install** refuses to overwrite a plugin changed since that install.
The NRO installer leaves settings intact. ZIPs may contain default INIs: preserve
customized settings before extracting over an existing installation.
INI defaults now use `settings/<plugin-id>/` and require the updated runtime.
When upgrading from flat settings, run existing mods with that runtime first so
their customized files migrate before installing a ZIP's default INIs.

**Tools > Diagnostics** reads loader/runtime/mod reports and lists recorded
failures. A clean report alone does not establish gameplay correctness.

Compatibility depends on the exact Switch target and required features, **not a
pre-2023 release-date rule**. Compilation and file validation do not establish
playable Switch behavior. See [compatibility and test coverage](COMPATIBILITY.md).

## Develop

Install the .NET 10 SDK on Windows. Restore uses BCnEncoder.Net 2.3.0 and
CommunityToolkit.HighPerformance 8.4.0; their notices are in `third-party`.

```powershell
dotnet build desktop/ModPorter.App -c Release
dotnet run --project desktop/ModPorter.Tests -- .
dotnet build desktop/ModPorter.Cli -c Release
python desktop/verification/real_mods.py --output ../real-mod-checks --devkitpro <devkitPro-root>
python desktop/package.py --output releases/mod-porter-0.9.1
```

Run from the SDK/repository root. The CLI shares the desktop analysis/build/asset
implementation; run `shvnx-port --help`. The optional CLI requires .NET 10.
The desktop download includes the SDK, not the core runtime. The optional
`SHV_NX_PLUGIN_WITH_STOP` export requires Lifecycle Test 1 or a later supporting
runtime for cleanup on stop. Older ABI 1 runtimes ignore that optional export;
the original plugin descriptor and API table sizes are unchanged.

Created by MEGATARD (@Geekmaxver; GitHub: Geekmaxxer).
[Original source](https://github.com/Geekmaxxer/ScriptHookV-NX). Repository LICENSE
and NOTICE apply to this tool; third-party code retains its own license.
