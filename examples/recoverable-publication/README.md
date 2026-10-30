# Recoverable file publication example

This example prepares and verifies 256 KiB of data in 16 KiB chunks, publishes
the candidate and retains the previous file as a backup. It requires the
unreleased file and mutation services. Catalog and memory services are optional.

```powershell
python tools/build_mod.py --project examples/recoverable-publication/project.json --output out/recoverable-publication --devkitpro C:/devkitPro
```

The dedicated data namespace is `settings/publication-example/`. On an empty
first run, the example creates `current.bin` containing `old`, writes and checks
`prepared.bin`, then publishes it through `FileReplacement`. A successful result
leaves the verified 256 KiB current file and the three-byte `backup.bin`.
`result.txt` records completion. Later completed runs only verify those files.

The example yields while writing and validating data. It uses a static buffer;
no filesystem handle survives a call. It never deletes existing files.

An interrupted or unfamiliar state produces `inspection_required=1` and remains
available for explicit recovery. The example does not automatically overwrite,
resume preparation, restore a backup or discard retained data. Use the
[publication contract](../../docs/reference/FILE-PUBLICATION.md) to inspect and
validate the relevant files before choosing recovery. Remove the dedicated
example data explicitly only when a fresh demonstration is intended.

Local verification is separate from Switch storage, interruption and performance
acceptance. This workflow does not establish atomic replacement or power-loss
durability.
