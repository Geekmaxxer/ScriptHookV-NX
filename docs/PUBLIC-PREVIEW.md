# v1.4.1 Preview + Mod Porter 0.6.0

Native plugins, developer tools and compatible Legacy asset preparation for
GTA V on Nintendo Switch.

## Downloads

- **Runtime**: installation files for players, including the shared SD-access
  `main.npdm`. Close GTA and keep existing mods and settings. For customized
  NPDM files, use the included `Install-SD-Access.cmd` to preserve other changes.
- **Developer SDK**: headers, shim, templates, examples and developer tools,
  including the reusable controller UI framework.
- **Mod Porter 0.6.0 for Windows x64**: source-porting and asset tools, with
  the matched SDK and examples included. Extract the complete ZIP before use.

## What's new

- Required SD-access metadata is included in runtime downloads, developer builds
  and Mod Porter packaging for plugin loading and saved settings.
- The SD-access installer backs up customized NPDM files and preserves other changes.
- Runtime binary, plugin ABI and default limits remain unchanged. Mod Porter
  remains at version 0.6.0.

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
