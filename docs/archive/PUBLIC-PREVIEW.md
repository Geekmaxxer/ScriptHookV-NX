# ScriptHookV-NX v1.4.1 Preview + Mod Porter 0.9.1

Native plugins, developer tools and compatible Legacy asset preparation for
GTA V on Nintendo Switch.

## Downloads

- **Runtime**: installation files for players, including the SD-access main.npdm
  and an installer that preserves customized metadata. Keep existing examples/mods/settings.
- **Developer SDK**: typed headers, shim, reusable controller UI, templates,
  source examples and tools for NRO and NSC mod development.
- **Mod Porter 0.9.1 for Windows x64**: source-porting, resource preparation and
  DLC editing, with the matched SDK included. Extract the complete archive.

## What's new

- Build one DLC containing multiple PEDs and vehicles, or extend an existing pack.
- Generate human PED metadata directly from four assets; external peds.meta is optional.
- Edit model names, types, movement profiles, every metadata field and complete XML.
- Add, replace, export or remove assets; replace a complete PED set and undo edits.
- Find installed DLCs and avoid copying update.rpf when the pack is already registered.
- Separate choose, add/review and save steps, clearer navigation and collapsed optional editors.
- Improved metadata validation/recovery, SDK diagnostics, packaging checks and documentation.
- Folder-only and ZIP output for supported preparation and build workflows.

The runtime is unchanged from the console-accepted binary. Defaults remain
**8 resident NROs, 16 cooperative script slots and an 8 MiB plugin budget**.
Existing ABI 1 plugins remain supported. The required SD-access metadata from the
v1.4.1 packaging fix is retained. Close GTA before updating and preserve settings.

The audit passed 509 runtime checks in emulation, 216 desktop tests, 125 packaged
workflow checks and 2,970 layout checks. New mods and assets still require their
own Switch tests; this release does not claim new console acceptance for assets.

PC ASI/DLL binaries require compatible source ports and ARM64 builds. NRO modules
stay mapped until GTA exits. Build tools are installed separately; the desktop
app includes .NET and prepares assets without the NRO compiler toolchain.

See [installation](../guides/INSTALL.md), [developer quickstart](../guides/START-HERE.md),
[Porter workflows](../guides/MOD-PORTER-WORKFLOWS.md), [game assets](../guides/GAME-ASSETS.md)
and the [quality audit](../verification/QUALITY-AUDIT.md).

Created by MEGATARD (@Geekmaxver).
Original source: https://github.com/Geekmaxxer/ScriptHookV-NX
