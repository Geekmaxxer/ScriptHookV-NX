# Preset catalog, XML load, save and recovery

This manual example combines large XML input with owner-backed growable
serialization, chunked output verification and backup publication/restoration.
A cooperative preset catalog adds extension filtering, sorted selection and
checked relative paths to the combined hardware workload. The exact batch 4
artifact has scoped physical acceptance for normal operation and final run-2
cleanup. The standalone large-XML example and earlier artifact records remain
available with their original scope.

```powershell
python tools/build_mod.py --project examples/xml-workflow/project.json --output out/xml-workflow --devkitpro C:/devkitPro
```

The C/C++ library profiles, memory, owned resources, catalog/file services and file
mutations are required. All state is registered for cleanup. File calls are
cooperative in 16 KiB chunks; parsing and serialization are synchronous.

First, the example prepares 13 XML presets, one text file and one nested
directory under `presets/`. It enumerates four pages, filters the text file,
verifies case-insensitive ordering and metadata, then opens and parses a selected
preset. Existing fixture files must match exactly and are preserved on failure.

On an empty namespace, the example creates `input.xml` with 262,144 bytes, loads
its value, adds a 98,304-character payload and serializes into a growable buffer.
It creates the original small `current.xml`, writes `prepared.xml`, verifies all
bytes against the serialization, then publishes the prepared file with a backup.
After verifying current/backup, it preserves the replacement as `published.xml`
and explicitly restores the original backup to `current.xml`.

No file is deleted. Successful later runs verify the retained original and
replacement without rewriting them. Unexpected intermediate states, content
changes and parse failures are preserved and reported. Interrupted runs require
inspection rather than automatic cleanup of disk files.

`settings/xml-workflow-example/result.txt` identifies the run, failure stage,
input/output lengths, catalog counts and publication/restoration results. `cleanup.txt` records
the run and final owner cleanup. The result stages are 1 input preparation,
2 input loading/parsing, 3 serialization, 4 original creation, 5 prepared output,
6 publication, 7 restoration, 8 completed-state verification and 9 preset browsing.

The [service checklist](../service-acceptance/README.md) retains the accepted
two-run sequence for reproduction. No repeat is required. The
[catalog/workflow record](../../docs/verification/CATALOG-SCAN1.md)
separates local execution from physical acceptance. File verification does not
provide a transaction or protect against concurrent writers.
