# Prepare assets using your current game files

The Mod Porter desktop app can prepare compatible Legacy assets and update a
copy of an installed OPEN RPF. Game files come from your own SD drive, game
folder or saved copy. Nothing is downloaded from the game or bundled with the tool.

## Add a complete DLC pack

1. In **Game assets > Prepare assets**, inspect the mod and use **Prepare assets + ZIP**. Keep
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

A four-file PED download (`ydd`, `yft`, `ymt`, `ytd`) still needs a complete DLC
wrapper. Its `peds.meta` must register the model with a suitable target-game PED
definition, and `content.xml` must mount both that metadata and the PED resource
archive through `setup2.xml`'s startup change set. Preserve the model's component
variation data, skeleton and texture bindings. The converter prepares resources;
it does not generate these missing definitions automatically.

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
shvnx-port game-replace prepared/car.ntd --archive <current.rpf> --entry <exact/path/car.ntd> --output <new-folder>
```
