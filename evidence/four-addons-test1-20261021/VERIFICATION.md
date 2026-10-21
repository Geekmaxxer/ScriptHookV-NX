# Four add-ons — local verification

Prepared Chrysler 300 Redeye, Rolls-Royce Cullinan, Zoro and Misaki 3.0 from local
PC downloads against the mounted Switch installation. A second Zoro variant is
available as an optional replacement. No console execution occurred during preparation.

## Registration and preservation

- Output location verified against installed DLC packs: `romfs/update/switch/dlcpacks`.
- Preserved all 71 existing DLC-list entries and appended four unique registrations.
- Independent CodeWalker inspection retained all 1,368 top-level update archive entries.
- Only seven original bytes changed, all in the selected DLC-list table row; the new
  list was appended. Every unrelated original byte was retained.
- Fourteen model/skeleton/variation resources across both Zoro choices are byte-identical
  to the original PC resources. Twenty-one PED component drawable hashes resolve.
- Seventeen standard animation bone tags per PED match an installed stock Switch PED.
- All 304 textures across both Zoro choices parse independently and fit the 1024-pixel
  cap. Retained mip reductions and 262 unchanged texture payloads were compared byte
  for byte against the originals; 23 textures required resampling/re-encoding.
- All declared DLC files, startup change sets and shader texture references resolve.
  Shared Cullinan textures resolve against installed `vehshare.ntd`.
- ZIP entries were streamed back and SHA-256 compared with staging files.

## Package repairs

- Chrysler: replaced an absent custom engine-audio dependency with the stock Buffalo
  sound; corrected a malformed numeric offset; used bounded mod-kit ID 1000; retained
  the supplied chassis upgrade; removed absent dummy booster dependencies and borrowed
  police/nitrous flags. Geometry, handling and authored LOD distances were preserved.
- Both cars: removed unused copies of stock vehicle layouts, empty tuning archives and
  references to nonexistent unlock change sets. Removed Cullinan's missing carcols
  reference; its default mod kit requires no custom carcols definition.
- PEDs: created separate registration wrappers using compatible definitions from the
  installed game, preserving the supplied rigs and component variations.
- Misaki: the download omits accessory-0 diffuse, normal and specular maps. Fallback
  aliases use its accessory-1 maps. The missing original appearance cannot be recovered.

## Tooling

The desktop DLC output-path correction passed all 136 Core checks and a clean WPF
build. The optional streaming probe builds as a 16 KiB ARM64 NRO. Its eleven native
calls are present in the target native registration index. It requests models one
at a time with a 20-second timeout and releases each model; it creates no entities.

## Console acceptance pending

Use MG Menu model search to spawn each model separately, then together. Check
materials, animations, outfit variants, driving, doors, lights, glass, damage,
customization and restart behavior. The optional probe reports engine recognition
and streaming; it does not validate rendering or gameplay. High-detail source
geometry is retained, so frame time and memory pressure require console checks.

Private game resources, converted mods, source snapshots, tooling logs and build
artifacts remain outside the repository export. Compact verification records are
included here without game assets. The mounted SD was read only.
