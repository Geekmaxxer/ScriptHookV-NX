# File mutation and recoverable publication

The unreleased mutation service adds file rename and deletion inside a plugin's
data namespace. An optional `FileReplacement` helper coordinates a prepared
candidate, a current file and a retained backup. It supports application-validated
data up to the file service's 64 MiB limit, using the existing audited filesystem
bindings. Normal publication/backup retention and explicit XML backup restoration
have scoped physical acceptance. Physical failure recovery and performance remain open.

## Mutation service

Include `plugin_file_mutations.h` or `script.h`. Check
`scriptFileMutationsAvailable()` explicitly. The independent 24-byte
`ShvNxFileMutations` ABI 1 table preserves the file, catalog and base plugin tables.

| Operation | Behavior |
| --- | --- |
| `scriptFileRename(from, to)` | Move a file without replacing an existing destination |
| `scriptFileDelete(path)` | Explicitly delete one file and verify its absence |

Both paths follow the [file service's bounds and grammar](FILE-SERVICES.md) and
remain under `settings/<plugin-id>/`. Names are validated before mutation, calls
require the owning thread, and all temporary handles/mounts close before return.
Rename rejects identical or case-equivalent paths. Parents must already exist.
There is no directory mutation, recursion or automatic deletion of conflicts.

The runtime checks a source file's length before mutation. Rename then verifies
destination length and exact source absence; deletion verifies exact absence.
`VerificationFailed` means the mutation reported success but its postconditions
could not be confirmed. Only the audited filesystem missing-path result proves
absence. Other I/O failures are not equivalent to a missing file.

**Any mutation error can follow a completed or partially applied operation.**
Inspect the current paths before retrying. Postchecks establish observed path and
length conditions, not content identity, transaction isolation or media durability.
Do not infer file integrity from equal lengths.

## Publication helper

`<shv/file_replacement.h>` provides `ScriptHook::Data::FileReplacement`. It copies
three distinct bounded paths into the object; case-equivalent aliases are rejected.
It needs no heap allocation or persistent handle. `Inspect` needs the read-only
file table; publication/recovery additionally require the mutation table.

```cpp
ScriptHook::Data::FileReplacement files(
    "catalog/current.bin", "catalog/prepared.bin", "catalog/backup.bin");
ScriptHook::Data::ReplacementState state;
if (files.Inspect(state) != ShvNxFile_Ok) return;
// Finish writing and validate prepared.bin using the application's format.
// Serialize every writer that can access these three paths.
if (files.Publish() != ShvNxFile_Ok) {
    // Inspect again and validate the relevant files before choosing recovery.
    return;
}
// Validate current.bin before accepting it or retiring any retained backup.
```

The helper does not validate checksums, schemas or payloads. Prepare and validate
the entire candidate before calling `Publish`; validation must complete before
publication starts. After success, validate the new current file before accepting
it or explicitly retiring the backup. The [complete example](../../examples/recoverable-publication/README.md)
demonstrates chunked preparation and verification with cooperative waits.

## State and recovery policy

`Inspect` reports a presence mask and the three observed file lengths. On error,
its output is cleared and must not be interpreted as three absent files. Only a
successful inspection establishes the stated observations. Ensuring the owner
data root can create that directory; inspection does not create the three files.

| Current | Prepared | Backup | `Publish()` behavior |
| --- | --- | --- | --- |
| Absent | Absent | Either | `NotFound`; preserve everything |
| Present | Absent | Either | `NotFound`; preserve everything |
| Absent | Present | Absent | Move prepared to current |
| Present | Present | Absent | Move current to backup, then prepared to current |
| Either | Present | Present | `AlreadyExists`; preserve the conflict |

After a failed first or second rename, stop. A failed acknowledgement or failed
postcheck can leave either operation completed. Reinspect and validate contents;
do not repeat publication blindly.

`RestoreBackup()` is explicit recovery authorization. It moves backup to current
only when current is absent and leaves prepared untouched. An existing current
file always blocks restoration, including one with invalid application content.
To recover that case, the application must first preserve/quarantine or explicitly
remove the invalid current file after validating its recovery source. The helper
does not make that decision.

A successful publication retains its backup. No method silently removes it.
Use `scriptFileDelete` only after application validation and an explicit retention
decision. A backup that already exists blocks a later publication until resolved.

## Concurrency and acceptance limits

The workflow requires one application-controlled writer. Inspections and mutations
are separate operations; another writer can change the files between them.
There is no locking, multi-call snapshot, atomic replacement or persistent journal.
SDK postchecks cannot guarantee recovery from filesystem corruption or power loss.

The original [small recoverable records](RECOVERABLE-DATA.md) retain their separate
checksummed format. Hosted `stdio`, `fstream`, unrestricted filesystem access and
directory removal remain outside this service. See the
[verification scope](../verification/PLUGIN-PUBLICATION1.md) for local failure
models and outstanding physical acceptance.
