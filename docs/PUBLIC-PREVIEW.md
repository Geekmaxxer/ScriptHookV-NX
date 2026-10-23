# v1.4 Preview + Mod Porter 0.6.0

Native plugins, developer tools and compatible Legacy asset preparation for
GTA V on Nintendo Switch.

## Downloads

- **Runtime**: installation files for players. Close GTA, copy `atmosphere` to
  the SD root and keep existing mods and settings.
- **Developer SDK**: headers, shim, templates, examples and developer tools,
  including the reusable controller UI framework.
- **Mod Porter 0.6.0 for Windows x64**: source-porting and asset tools, with
  the matched SDK and examples included. Extract the complete ZIP before use.

## What's new

- Controller menus with scrolling, subpages, toggles, selectors, descriptions
  and a settings-persistence example. See [controller UI](CONTROLLER-UI.md).
- Complete loose-PED DLC wrappers using explicitly selected current-game metadata.
- Add-on checks for registration, startup activation, model/mod-kit conflicts,
  malformed numeric metadata and unresolved shader dependencies.
- Multi-pack preparation that preserves the current installation's DLC list.
- Expanded supported Legacy embedded-texture traversal while retaining geometry,
  skeleton and physics data.
- Separate folder-only and **+ ZIP** actions for assets, PED builds and DLC
  registration. CLI equivalents accept `--no-zip`.
- Updated examples, maintenance checks, documentation and desktop layout validation.

The runtime is byte-identical to the console-accepted v1.3 binary. Defaults remain
**8 resident NROs, 16 cooperative script slots and an 8 MiB plugin budget**.
Existing ABI 1 plugins remain supported. The new SDK/UI and asset tools have
local build and file validation; new mods and assets need their own Switch tests.

Build tools are installed separately: Python and devkitA64/switch-tools are
required for NRO builds. ASI/DLL mods require compatible C++ source ports and
ARM64 rebuilds. Kill Process stops execution; NRO mappings remain until GTA exits.
Asset preparation does not repair arbitrary rigs or vehicle collision complexity.
No download includes game archives or saved settings.

See [installation](INSTALL.md), [developer quickstart](START-HERE.md),
[game assets](GAME-ASSETS.md) and [compatibility](COMPATIBILITY.md).

Created by MEGATARD (@Geekmaxver).
Original source: https://github.com/Geekmaxxer/ScriptHookV-NX
