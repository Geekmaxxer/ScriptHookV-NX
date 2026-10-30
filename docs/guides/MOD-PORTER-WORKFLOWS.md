# Choose a Mod Porter workflow

| Goal | Start here | Result |
| --- | --- | --- |
| Port a PC C++ script mod | Import a mod, inspect Findings, then Project & build | A copied source project and an ARM64 NRO; original source stays intact. |
| Examine an ASI/DLL | Import the binary | Dependency and compatibility findings; the binary is not converted to NRO. |
| Make a PED or vehicle add-on | Game assets > DLC collection | One editable DLC for one or many models. |
| Expand an installed DLC | DLC collection > Find installed DLCs | An edited copy with the same DLC folder ID. |
| Build one human PED quickly | Prepare assets > Build a loose PED add-on | A complete DLC from four matching files, with built-in metadata when needed. |
| Reduce textures or convert resources | Game assets > Prepare assets | Prepared resources; complete DLC creation uses the separate builder. |
| Register a new DLC | DLC collection > First installation: include game registration | DLCs plus a copy of update.rpf only when registration is needed. |
| Replace a stock asset | Game assets > Add to game > Replace an existing asset | A copied containing archive with the selected entry replaced. |
| Diagnose a plugin | Tools > Diagnostics | Recorded loader/runtime failures and timing information. |

**Build NRO**, **Save DLC** and folder-only actions create ordinary output folders.
Their **+ ZIP** counterparts also create a shareable archive. The desktop app
includes .NET. Only C++ compilation requires Python, devkitA64 and switch-tools;
PED and asset preparation use the built-in desktop implementation.

The DLC page has three steps: choose a collection, add/review models, and save a
new revision. Expand optional editors when needed. For updates, keep the original
DLC folder ID and select the current installation when checking registration.
With GTA closed, back up affected files and copy the prepared atmosphere folder
to the SD root. Preparation itself never changes installed game files.

See the [asset guide](GAME-ASSETS.md) for individual controls and limitations,
the [Porter guide](../../mod-porter/README.md) for source-porting, and the
[quality audit](../verification/QUALITY-AUDIT.md) for validation scope.
