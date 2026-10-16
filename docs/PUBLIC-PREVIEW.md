# Public Preview

Created by MEGATARD (@Geekmaxver).
Original source: https://github.com/Geekmaxxer/ScriptHookV-NX

This release makes the accepted runtime and developer tools available for early
use. It supports game build `6e3e9b3af5746e74b615bd981080c9271bc226620`, title
`0100b00b51230000`. Caller, runtime and plugin ABI remain 1.

## Downloads

- **Runtime**: the exact console-accepted subsdk5, a short installation guide,
  LICENSE and NOTICE. No game archive, menu, NRO mod, binding or saved settings.
- **SDK**: NRO headers/shim, seven project templates, optional helpers, nine
  development tools, source mods and developer guides. No compiler/game files.
- **Source**: a clean repository tree with the core, SDK, audited target profile,
  public tests/tools, compact acceptance records and documentation. Copy the
  extracted ScriptHookV-NX folder into your GitHub checkout before committing.

Runtime installation is [here](INSTALL.md). Plugin authors start
[here](START-HERE.md); core contributors use [the portable build guide](DEVELOPING.md).
Use [the attribution license](../LICENSE): reposts, downloads and sales pages
must visibly credit and link the original source.

## Known limits

These are the limits of the previously published preview. The newer source's
[Capacity Test 1](CAPACITY-TEST1.md) raises residency to eight and script slots to
sixteen with the same 8 MiB plugin budget; that binary needs separate Switch
acceptance and does not replace the historical accepted download automatically.

Windows DLL/ASI binaries need compatible source ports and ARM64 rebuilds. Four
NROs can remain resident; Kill Process stops a script but does not unmap code or
free that resident slot. Restart GTA to replace code/release slots. The scheduler
cannot preempt a plugin that fails to yield. Direct NSC loading from SD is absent.
The SDK does not make unsupported game versions or platform APIs compatible.

The exact runtime download has console evidence. The new portable source build,
SDK tutorials and menu description renderer have their own distinct validation
scope; do not call every new example console-verified. Full gameplay soak tests,
console FPS/peak RAM measurements and independent third-party ports remain work.
Local tests and compiled-runtime equivalence are useful evidence, not substitutes
for console gameplay tests. [Compatibility](COMPATIBILITY.md) lists all limits.
