# Changelog

## Unreleased

- Complete loose-PED DLC wrappers from explicitly selected target metadata.
- Add-on registration/model/shader dependency audits and multi-pack preparation.
- Texture reduction through supported Legacy fragment physics-child drawables.
- Header-only controller UI with measured layout, navigation and persistence demo.
- Public verification command, UI model tests and pinned Windows/Linux CI workflow.


## v1.3 Public Preview - 2026-10-23

- Per-plugin diagnostics: per-plugin elapsed-time diagnostics, callback
  isolation after invalid yields, and a controller-only diagnostic/probe workflow.
  Defaults remain 8 resident NROs, 16 script slots and an 8 MiB plugin budget.
- Explicit lifecycle initialization macros and Mod Porter adaptation of
  registration-only multi-script/callback DllMain bodies, with capability and
  registration checks. Custom PC initialization remains explicit.
- NRO builds normalize source/debug paths and are reproducible across output
  locations with the same inputs and toolchain.

- SDK Parity Test 1: optional extension ABI for full ped/vehicle/object/pickup
  enumeration, owned additional scripts, validated script globals/entity pointers,
  custom PNG textures, controller/USB keyboard and HUD rendering callbacks.
  Original API/descriptor ABI 1 remains compatible. Matched Switch checks passed
  smooth PNG rotation/colour, controller input, additional script execution,
  pointer availability, NSC/NRO coexistence, Kill/Load and game restart.
  Controller input is the Switch target; USB keyboard support is optional.
- Combined `sdk-parity` tutorial, generated test texture, capability checks,
  per-plugin cleanup and resource accounting. Mod Porter recognizes the new
  interfaces while keeping PC address/layout and DirectX adaptations explicit.

- Per-plugin settings folders at `settings/<plugin-id>/`, with atomic migration
  of the previous prefixed filenames on access. Existing ABI 1 NROs work without
  rebuilding. Generic NSC journals also have separate namespace folders.
- Optional ABI 1 plugin stop callbacks for owned-effect/entity cleanup. Console
  checks passed with Footprints and MEGATARD's NSC Kill Process/Load Now controls.
- Disarm NX recognizes both upper arms and forearms, the right hand and weapon
  hits. The expanded gameplay behavior passed Switch testing.
- Controller chord, bounded INI and validated nearby-ped helpers for source ports.
- Mod Porter 0.5.0 includes the 0.4.0 archive workflows: prepare updated game-archive copies from selected SD/local
  files; merge DLC registration directly from the installed archive; replace a
  selected existing asset while retaining unrelated entries.
- Compiler-only checks, verified SD NRO installation/restore, diagnostic report
  import, embedded texture reduction and real source-port examples.

Runtime Completion Test 1 passed matched console gameplay acceptance with the
worker/render source port, diagnostics and existing mods running together.
No regressions were observed.

The 0.3.0 desktop workflow passed acceptance. The new 0.4.0 archive updater has
separate local validation and still requires gameplay acceptance for its outputs.
Caller/runtime/plugin ABI versions remain 1. Default limits remain 8 resident
NROs, 16 cooperative scripts and an 8 MiB plugin accounting budget.

Historical release downloads and acceptance records retain their original scope.
