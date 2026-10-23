# Prepare assets using your current game files

The Mod Porter desktop app can prepare compatible Legacy assets and update a
copy of an installed OPEN RPF. Game files come from your own SD drive, game
folder or saved copy. Nothing is downloaded from the game or bundled with the tool.

## Add a complete DLC pack

1. In **Game assets > Prepare assets**, inspect the mod and use **Prepare assets**. Keep
   its complete metadata and choose a texture cap suitable for the mod.
2. Under **Add to game**, select the prepared `dlc.rpf` and
   the current `update.rpf`, or its mounted SD/game folder. Use the archive from
   the installation you intend to update, including any existing mods.
3. Select **Add a DLC pack**, enter its **DLC folder ID**, then choose **Build updated
   game files** and a new output location. The tool reads the current DLC list
   directly, preserves its registrations and avoids duplicate entries.
4. With GTA closed, retain the original archive and any existing DLC folder as
   backups, then copy the output's `atmosphere` folder to the SD root. Test the asset.

The output contains an updated `romfs/update/update.rpf` and
`romfs/update/switch/dlcpacks/<id>/dlc.rpf`. It changes no installed file during preparation.
To undo installation, restore the original archive and DLC folder, or remove the
newly added folder. Apply the result to the same installation used to prepare it.

## Replace an existing model or texture

In **Add to game**, select **Replace an existing asset**. Select the prepared resource and its current containing RPF. Choose **Find
matching entry**; select the exact path if several matches exist. Use **Build
archive with replacement**. With GTA closed, put the resulting archive copy at
the original archive's game location. This output is not an SD-root package.

Only the selected existing entry changes; the archive's other entries remain.
The tool does not guess a vehicle, weapon or clothing slot. For a nested RPF,
select its extracted containing archive, then import the updated nested archive
with OpenIV, CodeWalker or the Switch Toolbox. Script injection uses separate tools.

## Loose add-on PEDs

Under **Prepare assets > Build a loose PED add-on**, choose one four-file PED
variant (`ydd`, `yft`, `ymt`, `ytd`). Enter its lowercase spawn model name and
a separate DLC folder ID. Select your current game's `peds.meta` or its containing
OPEN DLC archive, then choose the matching base PED from that metadata.
For example, the supported installation's mpbiker pack contains the definitions
used by the Misaki and Zoro preparation tests.
Use **Build PED add-on**. The tool creates a resource archive, peds.meta,
content.xml, setup2.xml and startup activation, then reads back the resulting
DLC. **Add to game** registers that pack in the current game's DLC list.

Asset preparation, PED add-on builds and **Prepare DLC registration** each have
a folder-only button and a **+ ZIP** button. Both save prepared files and reports;
**+ ZIP** also creates an archive. Their CLI commands accept `--no-zip` for folder-only output.

Choose the base definition deliberately: a male/female/animal or special rig
cannot be inferred safely from a filename. Geometry and variation data are
preserved. Missing shader textures are reported; the tool cannot reconstruct
their original appearance or repair arbitrary rig incompatibilities.

## Validate and combine packs

Prepared DLC audits check registered files, device paths, startup change sets,
model resources/hash collisions and malformed vector numbers. Typed Legacy
model traversal includes physics-child drawables and identifies shader textures
not supplied by the pack. Shared/base-game dependencies appear as warnings,
not proof of a missing game asset. Registration blockers prevent game updates.

For several add-ons, put the prepared archives in `collection/<pack-id>/dlc.rpf`
and use **Add several DLC packs** in **Add to game**. All packs are checked before
output; the selected current DLC list is merged once and update.rpf is copied
once. Duplicate IDs and model collisions within the batch are rejected. This
does not inspect every previously installed pack for conflicts.

Texture reduction covers supported Legacy shader dictionaries reached through
fragment roots and physics-child drawables. It preserves mesh, bounds and shared
graphics allocation. It does not establish the cause of collision hitches or
reduce collision/mesh complexity automatically.

Check declared DLC paths, custom engine-audio dependencies, mod-kit IDs and shader
texture dependencies before installation. A structurally valid PC pack can still
contain missing files or invalid metadata values.

## Requirements and limits

- Select a mounted SD drive or local files. Copy archives from an MTP device to
  a local folder first if they have no ordinary filesystem path.
- Encrypted archive tables require an OPEN copy from OpenIV or CodeWalker.
- Archive copying uses bounded buffers and supports current game archives larger
  than 512 MiB. Prepared individual assets/DLC packs remain limited to 256 MiB.
- Allow enough output space for a full archive copy. Progress and cancellation
  are available; an interrupted operation has no completion manifest and must
  not be installed. Original game files remain intact.
- Resource naming and texture reduction do not repair incompatible rigs,
  clothing collections, unavailable engine features or missing asset metadata.
- Texture caps reduce texture memory, not mesh complexity or vehicle damage work.

Completed outputs include `game-update.json` with source/output hashes and
changed paths. Game archives and generated game-file outputs belong outside the
source repository. No ScriptHookV-NX runtime update is needed to install asset data.

CLI equivalents:

```text
shvnx-port game-add-dlc prepared/dlc.rpf --game <SD-root-or-update.rpf> --id my-addon --output <new-folder>
shvnx-port archive-list <current.rpf>
shvnx-port addon-check prepared/dlc.rpf
shvnx-port assets-convert source/assets --output out/assets --cap 1024 --no-zip
shvnx-port addon-ped source/ped --id nx_my_ped --model myped --template current/peds.meta --base-ped IG_Malc --output out/my-ped --no-zip
shvnx-port dlc-prepare prepared/dlc.rpf --list current/dlclist.xml --id my-addon --output out/import --no-zip
shvnx-port addon-batch prepared/collection --game <current-update.rpf-or-SD-root> --output out/my-addons
shvnx-port game-replace prepared/car.ntd --archive <current.rpf> --entry <exact/path/car.ntd> --output <new-folder>
```
