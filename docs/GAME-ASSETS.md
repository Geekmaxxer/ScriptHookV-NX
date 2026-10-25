# Prepare assets using your current game files

The Mod Porter desktop app can prepare compatible Legacy assets and update a
copy of an installed OPEN RPF. Game files come from your own SD drive, game
folder or saved copy. Nothing is downloaded from the game or bundled with the tool.

## Keep many models in one DLC

Use **Game assets > DLC collection**. Drop several folders, ZIPs or DLC archives,
or use **Add folders** / **Add files**. A parent folder containing several complete
four-file PED sets is detected automatically. Vehicle packs retain their supplied
handling, tuning, texture dependencies and other metadata. Loose vehicles need
their author's vehicles.meta and related definitions; physical settings are not
invented from mesh files.

The page follows three steps: **Choose a collection**, **Add and review models**,
then **Save a new revision**. Optional definition, resource, metadata and import
controls expand when needed. **Names and import options** contains the DLC folder
ID, movement defaults and texture cap. **First installation: include game
registration** contains the game-file build actions. Smaller windows scroll the
form; controls keep their normal readable size.

To expand a pack already installed, **Open DLC** first and add more models to it.
Keep its **DLC folder ID**. All models stay in one dlc.rpf. **Save DLC** writes a
ready-to-copy DLC folder; **+ ZIP** additionally packages it. Saves use new revision
folders and preserve the original archive.

**Find installed DLCs** reads the current game/SD folder selected in **Add to game**
and lists registered and local packs. Select **Choose game folder** to switch
installations. Filter by ID and **Open selected DLC** to edit a copy directly.
The scan reads the DLC list and directory entries without loading every archive.
A registered pack marked **Not in this folder** may be inside the base game;
that status does not establish a broken installation.

For the first installation, **Build with game files** registers the collection
using the selected current game/SD folder. If that ID is already registered,
the tool exports only dlc.rpf and does not write update.rpf. Otherwise it prepares
an updated archive copy once. This check uses the installation selected for that
operation; keep it current after installing the first revision.

Select a PED in the model list to edit:

- **Model name** renames its assets and matching metadata references together.
- **Human male/female profile** adjusts movement/type while retaining custom fields.
- **All fields** edits every element and attribute; **Definition XML** supports
  nested/custom data. **Apply definition** keeps the edits in the working collection.
- **Edit full peds.meta / metadata** opens the built-in whole-file editor for every
  metadata document in the DLC. Use model actions for coordinated renames/removal.
- **Asset files** replaces, exports, adds or removes resources. Enable **Show every
  resource** for shared textures, props or tuning parts outside the selected model.
- **Undo/Redo** restores working edits. Missing required assets, malformed XML,
  duplicate models and mod-kit conflicts block export.

For a complete PED resource update, use **Replace four PED assets** or **Use
replacement folder** under **Asset files**. A folder, ZIP or the four matching
PC/Switch files are accepted. The source filenames may differ from the selected
PED: replacement resources keep the selected model's name and existing metadata.
All four files are validated before the collection changes, texture caps apply,
and one Undo restores the whole set. A folder containing multiple variants is
ambiguous; choose one variant's folder or its four files. Shared assets and unusual
split-archive layouts use the individual resource controls instead.

To update several PED movement profiles, filter the model list, expand **Change
movement for shown PEDs**, choose the profile, then **Apply movement to shown PEDs**. This action affects only the PEDs
currently shown, preserves custom fields and creates one Undo step. It does not
infer rig compatibility or gender from filenames.

Names, output folders, new PED definitions, path rebasing, resource conversion and
texture preparation are automatic. Existing unknown fields and unrelated payloads
are retained, including each supplied PED metadata header and resident animation
or texture references. Distinct metadata headers remain separately registered
inside the same DLC. Encrypted/Enhanced formats, conflicting semantic definitions and
compiled files embedding old device paths require separate handling. New or renamed
models still need Switch appearance, animation and gameplay tests.

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

The output contains `romfs/update/switch/dlcpacks/<id>/dlc.rpf`. It includes
`romfs/update/update.rpf` only when adding a new registration. It changes no installed file during preparation.
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

Under **Prepare assets > Build a loose PED add-on**, select or drop a folder/ZIP
containing one four-file variant (`ydd`, `yft`, `ymt`, `ytd`). You can also drop
the four matching files together, or browse to one of them to use its folder.
Click **Build PED add-on**. Model name, DLC folder ID, peds.meta, resource archive,
content.xml, setup2.xml and startup activation are generated automatically.
No game metadata, base PED name, Python or NRO build tools are required.
When the input contains several complete PED models, this action automatically
builds one collection. Use **DLC collection** to edit its models or add vehicles.

The default human profile uses male movement; select **Female** for female movement.
A mod-supplied peds.meta is reused when present. **Advanced: names and custom
metadata** optionally accepts your own target definitions; the base name is
selected automatically when there is one definition or a matching model.
Human presets cannot infer gender or repair animal/special rigs.

For ready-to-copy game files, choose **Build with game files** instead. Select
your SD/game folder once (or select the current update.rpf in **Add to game**).
The tool builds the PED and merges its registration into a copy of your current
game archive in one operation. The output's `game-install/atmosphere` folder is
ready to copy to the SD root with GTA closed. The + ZIP action archives those
installation files directly. Existing registrations and original files are retained.
An existing game's update.rpf is needed only for this registration step; it is
never required just to build the DLC. Encrypted archives need an OPEN copy.

Asset preparation, PED add-on builds and **Prepare DLC registration** each have
a folder-only button and a **+ ZIP** button. Both save prepared files and reports;
**+ ZIP** also creates an archive. Their CLI commands accept `--no-zip` for folder-only output.

Blank names are inferred from the assets and a blank output path gets a new
folder under Documents/ScriptHookV-NX Projects. Multiple variants and incomplete
file sets produce specific errors. Geometry and variation data are preserved;
missing shader textures and incompatible rigs still need their own fixes and
Switch appearance/animation tests.

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
  than 512 MiB. Individual resources remain limited to 256 MiB; DLC archives and
  each imported source/collection are limited to 512 MiB. Split larger collections.
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
shvnx-port addon-ped source/ped --output out/my-ped --no-zip
shvnx-port addon-ped source/ped --profile female --game <SD-root-or-update.rpf> --output out/my-ped-install
shvnx-port addon-ped source/ped --template current/peds.meta --base-ped IG_Malc --output out/custom-ped --no-zip
shvnx-port dlc-bundle source/peds source/vehicle.zip --id my-mods --output out/my-collection --no-zip
shvnx-port dlc-bundle source/more-peds --existing installed/my-mods/dlc.rpf --output out/expanded --no-zip
shvnx-port dlc-bundle --existing prepared/my-mods/dlc.rpf --game <SD-root-or-update.rpf> --output out/install
shvnx-port dlc-models prepared/my-mods/dlc.rpf
shvnx-port dlc-library --game <SD-root-or-update.rpf>
shvnx-port dlc-edit prepared/my-mods/dlc.rpf --model oldname --replace-ped-assets <folder-or-zip> --output out/replaced --no-zip
shvnx-port dlc-edit prepared/my-mods/dlc.rpf --model oldname --name newname --profile female --output out/edited --no-zip
shvnx-port dlc-prepare prepared/dlc.rpf --list current/dlclist.xml --id my-addon --output out/import --no-zip
shvnx-port addon-batch prepared/collection --game <current-update.rpf-or-SD-root> --output out/my-addons
shvnx-port game-replace prepared/car.ntd --archive <current.rpf> --entry <exact/path/car.ntd> --output <new-folder>
```
