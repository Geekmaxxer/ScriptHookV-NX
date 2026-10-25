# Mod Porter automation audit - 2026-10-25

| Workflow | Result |
| --- | --- |
| Many PEDs / vehicles | One editable DLC collection; batch folders, ZIPs and existing packs are detected. |
| Existing collection additions | The original identity and unrelated files are retained; models are added transactionally. |
| Repeated game registration | The current list is checked; registered packs produce no update.rpf copy. |
| PED definitions | Built-in human profiles, inferred names and optional supplied metadata remove the external-template requirement. |
| PED metadata edits | Every field/attribute and full XML are editable; model renames coordinate resource names and references. |
| Asset changes | Supported PC resource preparation is automatic on add/replace; resources can be exported or removed. |
| Output revisions | New folders are generated, with separate folder-only / ZIP actions. Originals are preserved. |
| Manual cleanup of downloads | Supporting JSON, previews and documentation no longer block preparation; executables remain rejected. |
| Stale installation selection | New multi-file preparation clears the previously selected asset instead of retaining another mod. |
| Large collections | Nested resource indexing is cached; source/collection limits and undo payload accounting bound editor memory. |
| Collisions and incomplete edits | Model/hash/path/mod-kit conflicts, conflicting handling/streamed resources and missing assets prevent export; failed imports roll back. |
| Repeated file choices | Game paths are remembered; complete vehicle metadata is taken from the mod. |
| Locating installed collections | Discover/filter registered and local DLCs from the selected game/SD folder without loading all model archives. |
| Complete PED asset updates | One folder/ZIP or four-file selection replaces a validated set while retaining identity, metadata and archive subdirectories. One Undo restores the set. |
| Repeated movement edits | Apply a human profile to PEDs shown by the current filter in one transaction; custom fields are retained. |
| Empty edited collections | Removing every model retains the unsaved-changes prompt and Undo history. |
| Workflow discovery | Three separate collection steps, collapsed optional editors and a direct route from resources to complete DLC creation. |
| Metadata recovery | Structural edits are validated before commit; invalid XML can be reverted, and pending DLC ID changes are tracked. |
| Registered batch refresh | Batch game output also omits update.rpf when all selected IDs already exist. |
| Incomplete SDK diagnosis | The tool doctor reports the native-index failure and continues through the remaining tool checks. |

Decisions that remain explicit:

- Which installed DLC to extend, and which installation to use for first-time registration.
- Human movement profile when no suitable definition accompanies the assets. Filenames do not establish gender or rig compatibility.
- Resolution of conflicting model/handling/mod-kit definitions. Guessing different physics settings or silently replacing another model is unsafe.
- Decryption of encrypted archives, Enhanced/Gen9 conversion and repairs to unsupported rigs, binary path references or missing textures.

Desktop validation establishes file structure, resource preparation, preservation
and editor behavior. New collections and edited models require separate Switch
tests. No runtime hooks or game executables are changed by these desktop features.
