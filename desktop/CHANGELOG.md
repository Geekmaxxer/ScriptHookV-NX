# Mod Porter 0.6.0 Public Preview - 2026-10-23

- Offer folder-only output alongside ZIP packaging for assets, PED add-ons and
  DLC registration. CLI equivalents accept `--no-zip`.
- Avoid creating a redundant intermediate ZIP when building PED add-ons.
- Build complete loose-PED DLC wrappers using selected current-game metadata.
- Check model resources, startup activation, numeric values and shader dependencies.
- Prepare several add-ons together while preserving the current DLC list.
- Traverse supported Legacy physics-child textures without changing geometry.

# Mod Porter 0.5.0 Public Preview

## Source-port lifecycle and diagnostics

- Preserve registration-only multi-script/callback DllMain initialization in
  explicit NX lifecycle functions, including Load Now restart behavior.
- Check required extension capabilities and registration results before commit.
- Import per-plugin elapsed-time reports with accurate units and attribution.
- Include a buildable multi-script PC source example and reproducible NRO builds.

## SDK extensions and usability

- Recognize owned multiple scripts, four entity enumeration APIs, validated
  globals/entity pointers, PNG textures and input/render callbacks from the
  matching SDK. Report PC layouts, USB keyboard and NX rendering differences
  as review items. Actual DirectX and Windows dependencies remain blocked.

- Place packaged INI defaults in `settings/<plugin-id>/` for the updated runtime.
- Fix resizing throughout the app, including maximize and minimize/restore.
- Keep output readable, scroll expanded forms, and stack Findings panels in narrow windows.
- Keep long paths and status text within the layout; let the mouse wheel leave lists and logs at their scrolling limits.
- Fit the initial window to the desktop work area and enable per-monitor DPI awareness.
- Write lowercase, sorted RPF directory entries so the game can find exported files.
- Downscale single-mip 32-bit textures while preserving their channel order and generating mipmaps.
- Refined desktop UI: compact side navigation, shorter forms and consistent dark controls.
- Separate asset preparation, game updates and reports; keep main actions visible at small window sizes.
- Separate toolchain setup, native lookup and diagnostics.
- Add finding severity filters, empty states, keyboard shortcuts and concise build output with an optional full log.
- Place prepared DLC packs in the Switch port's `romfs/update/switch/dlcpacks` directory.

# Mod Porter 0.4.0 Public Preview

- Select current RPFs from a mounted SD drive, game folder or local copy.
- Read and merge dlclist.xml directly from the selected update.rpf.
- Prepare updated archive/DLC copies in an SD-root output folder.
- Find matching existing entries and replace a prepared asset in a copied RPF.
- Stream large game archives with progress, cancellation and verified readback.
- Preserve unrelated payloads and the selected source files; record output hashes.

The new archive workflow has local validation; generated assets require Switch testing.

## 0.3.0

- Compiler syntax/type checking before an NRO build.
- SD installation with verified backups, startup-mode preservation and restore.
- Import Switch diagnostic reports.
- Texture caps for supported embedded drawable/fragment dictionaries; shared geometry allocation is preserved.
- Merge an add-on into an existing DLC list and create an archive-import ZIP.
- XML validation and missing model/texture dependency notices.
- SDK controller and bounded INI helpers; optional plugin cleanup callback.
- Disarm NX recognizes either upper arm, forearm, right hand and weapon hits.
- Footprints example supports INI controls and cleanup on the matching test runtime.

The new runtime cleanup and prepared assets require Switch testing. Binary-only
ASI conversion and arbitrary rig conversion remain outside this release.

## 0.2.0

- Separate **Build NRO** and **Build NRO + Install ZIP** actions.
- Assets workflow for Legacy cars, weapons, clothes and peds, with nested OPEN
  RPF rebuilding, Switch naming and metadata path updates.
- Standalone texture caps, encoded mip reuse and BC texture resizing.
- Real Footprints mod source example with controller input and a streaming timeout.
- Custom registered entry names and verified legacy native-name adaptations.
- Better diagnostics for PC version guards, memory hooks, world enumeration and
  keyboard/PC cheat input.

Switch gameplay testing is required for generated ports and prepared assets.
Compiled ASI translation, Enhanced asset conversion and embedded model texture
downsizing are not included.

## 0.1.0

Initial source-porting desktop workflow, offline native lookup, read-only ASI
analysis, recorded adaptations, cancellable builds, NRO validation and test ZIPs.
