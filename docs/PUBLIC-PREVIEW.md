# v1.3 Public Preview

Native plugins and developer tools for GTA V on Nintendo Switch.

## Downloads

- **Runtime**: copy `atmosphere` to the SD root with GTA closed. Contains the
  console-accepted runtime and installation guide. Keep existing mods and settings.
- **Developer SDK**: headers, shim, project templates, examples and developer tools.
  Use this to build NRO plugins and custom SanScript native bindings.
- **Mod Porter 0.5.0 for Windows x64**: source-porting and game-asset tools,
  with the matched SDK and examples included.

The complete runtime source is available in the repository. No download contains
game archives or a compiler. NRO builds require Python and devkitA64/switch-tools.

## What's new

- Live ped, vehicle, object and pickup enumeration.
- Multiple cooperative scripts per plugin, script globals and entity pointers.
- Custom PNG textures, HUD rendering and controller callbacks.
- Separate settings folders for each mod, with migration of older files on access.
- Per-plugin elapsed-time diagnostics and isolation of invalid callback yields.
- Improved multi-script PC source adaptation, lifecycle helpers and examples.
- Reproducible NRO output across build locations with identical inputs/toolchains.

Defaults remain **8 resident NROs, 16 scripts and an 8 MiB plugin budget**.
Existing ABI 1 plugins remain supported. The exact shipped runtime passed
[matched console gameplay checks](../evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json),
including the worker/render source port and diagnostics alongside existing mods.

Windows ASI/DLL mods require compatible source ports and ARM64 rebuilds. Kill
Process stops execution; NRO mappings remain until GTA exits. Game assets and
new plugins require their own testing. See [installation](INSTALL.md),
[developer quickstart](START-HERE.md) and [compatibility](COMPATIBILITY.md).

Created by MEGATARD (@Geekmaxver).
Original source: https://github.com/Geekmaxxer/ScriptHookV-NX
