# Runtime and Mod Porter quality audit - 2026-10-25

Mod Porter 0.9.1 addresses the confirmed findings below. The runtime rebuilt from
the current source is byte-for-byte identical to the accepted runtime reference;
this desktop update does not require replacing subsdk5.

## Findings resolved

| Finding | Result |
| --- | --- |
| Structural XML edits could make the model list unusable | Setup, content and registered model metadata are indexed before an edit commits. Invalid edits preserve the prior working state. |
| Duplicate definition fields could throw from a text-change event | The editor reports invalid fields; Revert unapplied edits restores the working definition. |
| Excessively nested definitions could exhaust field traversal | Definition editing has bounded depth and field counts. |
| Editing only the DLC folder ID bypassed the unsaved-change prompt | Pending ID changes appear as unsaved before Save. |
| Removing every model disabled access to complete metadata | The independent advanced metadata action and Undo remain available. |
| Nested resource selection could choose an outer RPF | The asset picker uses the immediate containing archive. |
| Batch refreshes always copied update.rpf | Registered batches export only their DLCs. Mixed batches register new IDs once. |
| Batch and single-pack archive limits differed | Both use the same 512 MiB DLC ceiling, with existing per-resource limits. |
| An incomplete SDK aborted the tool doctor | Native-index failures are reported alongside remaining tool checks. |
| Collection actions and advanced controls competed for attention | Choose, add/review and save have separate sections; optional controls start collapsed. Batch profiles have their own selector. |

## Validation performed

- Public repository checks: native index, target instruction profile, portable
  tools, SD-access metadata, desktop tests, WPF compilation and NRO examples.
- Production ARM64 runtime: native calling, scheduler/context restoration, plugin
  discovery and manager actions, limits/configuration, additional scripts, input
  and render callbacks, textures, stop/restart cleanup, settings migration, timing
  and eight-plugin/sixteen-script capacity. Thirteen suites pass 509 checks. Game,
  filesystem, RO service and OS interactions are emulated where stated.
- Desktop workflows: source analysis, copied projects, compiler checks, standalone
  NRO and install ZIP builds, Footprints source-port compilation, installation and
  restore against a temporary SD fixture, diagnostics and native lookup.
- Assets: real PED preparation, single-PED folder/ZIP and game-file builds, mixed
  two-PED/two-vehicle collections, editing and undo, complete replacements,
  metadata recovery, installed-pack discovery and registration-aware output.
- Layout: normal, maximized, minimized/restored and small windows; long paths;
  expanded forms; nested scrolling; effective layouts at 100/125/150/200 percent
  DPI. Native multi-monitor DPI changes are separate from simulated DPI layouts.
- Package readback and clean-source export: archive contents, matched SDK and
  shared NPDM hashes, documentation links and exclusion of game/asset files.

Exact results and artifact hashes are retained in the compact
[verification record](../evidence/stability-audit-20261025/verification.json).
Temporary fixtures and private game resources are not part of public exports.

## Practical limits

Local regressions are not a new Switch gameplay test. Prepared or renamed models
still require appearance, animation, LOD and collision testing on the target game.
Compatibility with arbitrary mods or malformed input cannot be guaranteed.

NROs share the game process. Cooperative scripts must yield, callbacks must return,
and raw pointers remain advanced interfaces. A plugin that hangs or corrupts memory
cannot be isolated like a separate application. Resident code stays mapped until
game exit; startup limit changes apply on the next launch.

Assets retain source rigs, collision complexity and unsupported dependencies.
Human profiles do not infer gender or repair rigs. Windows ASI binaries still need
compatible source ports and ARM64 builds. These boundaries remain explicit in the
tool and [asset guide](GAME-ASSETS.md).
