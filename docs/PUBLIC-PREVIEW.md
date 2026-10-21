# Public Preview downloads

Created by MEGATARD (@Geekmaxver).
Original source: https://github.com/Geekmaxxer/ScriptHookV-NX

The current source supports game build `6e3e9b3af5746e74b615bd981080c9271bc22662`, title
`0100b00b51230000`. Caller, runtime and plugin ABI remain 1.

## Downloads

- **Runtime**: the exact console-accepted subsdk5, a short installation guide,
  LICENSE and NOTICE. No game archive, menu, NRO mod, binding or saved settings.
- **SDK**: NRO headers/shim, project templates, optional helpers, development
  tools, source mods and developer guides. No compiler/game files.
- **Source**: a clean repository tree with the core, SDK, audited target profile,
  public tests/tools, compact acceptance records and documentation. Copy the
  extracted ScriptHookV-NX folder into your GitHub checkout before committing.
- **Mod Porter Windows**: desktop source-porting, asset preparation and current-game
  archive workflows. Its .NET runtime and plugin SDK are included.

Runtime installation is [here](INSTALL.md). Plugin authors start
[here](START-HERE.md); core contributors use [the portable build guide](DEVELOPING.md).
Use [the attribution license](../LICENSE): reposts, downloads and sales pages
must visibly credit and link the original source.

## Current acceptance

The current runtime includes optional plugin cleanup and defaults to 8 resident
NROs, 16 cooperative script slots and an 8 MiB plugin accounting budget. Console
acceptance covers Disarm arm-hit gameplay, Footprints cleanup/restart and NSC
menu process controls. Mod Porter 0.3.0 passed its desktop workflow.
[The acceptance record](../evidence/completion-test1-20261021/console-acceptance.json)
identifies the artifacts and remaining scope.

Mod Porter 0.4.0 adds current-game archive editing, with separate local checks.
Prepared assets still need Switch gameplay testing. Use [Mod Porter](../desktop/README.md)
and the [game-file guide](GAME-ASSETS.md) for source and asset workflows.
Full-capacity stress, custom limit overrides and long-duration RAM/FPS profiling
remain separate checks. Historical downloads retain their original evidence.

## Known limits

Windows DLL/ASI binaries need compatible source ports and ARM64 rebuilds.
NROs remain resident; Kill Process stops a script but does not unmap code or
free that resident slot. Restart GTA to replace code/release slots. The scheduler
cannot preempt a plugin that fails to yield. Direct NSC loading from SD is absent.
The SDK does not make unsupported game versions or platform APIs compatible.

The exact runtime download has console evidence. The new portable source build,
SDK tutorials and menu description renderer have their own distinct validation
scope; do not call every new example console-verified. Full gameplay soak tests,
console FPS/peak RAM measurements and independent third-party ports remain work.
Local tests and compiled-runtime equivalence are useful evidence, not substitutes
for console gameplay tests. [Compatibility](COMPATIBILITY.md) lists all limits.
