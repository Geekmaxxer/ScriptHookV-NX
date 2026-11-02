# JSON preset workflow

This example exercises owner-accounted JSON parsing, mutable copies, typed edits
and serialization with the existing cooperative file-transfer and publication
services. It creates one original preset with an 8,192-byte payload, validates
the complete input, publishes an independently verified replacement, verifies
both replacement and backup bytes, then explicitly restores the original.

```powershell
python tools/build_mod.py --project examples/json-presets/project.json --output out/json-presets --devkitpro C:/devkitPro
```

The project enables the C/C++ and JSON profiles. It requires memory, resource,
file and mutation services. It changes only `settings/json-presets-example/`;
game entities, saves, bindings and shared runtime configuration are unchanged.
Parsing/serialization are synchronous; file steps yield between bounded groups.

## Consolidated acceptance

The probe joins resource-registry, read-streams and task-queue in the expanded
[service batch 8](../service-acceptance/README.md). Use one later installation and
game session. After two natural runs, retain
`switch/gtav-scripthook/settings/json-presets-example/result.txt`:

```text
run=2
failed=0
stage=10
completed=1
parsed=1
serialized=1
publication_verified=1
restored=1
owner_cleanup_complete=1
owned_allocations=0
registered_resources=0
```

`current.json` must contain the original version=2699, enabled=true, ratio=1.25,
Unicode label and full payload. No prepared file or backup remains after natural
completion. Preserve the completed reports before optional cancellation changes
the counters. Test Kill during file loading after one full run has created and
restored the fixture, then Load again. Final owner counts must return to zero;
the restarted lifetime must complete.

An interruption during initial creation, prepared writing, publication or
restoration can leave meaningful partial/current/backup files. The probe preserves
those states and refuses conflicting prepared files or backups. Retain its report
and all three paths before applying the explicit
[publication recovery procedure](../../docs/reference/FILE-PUBLICATION.md).
Do not delete a backup to force a retry. Ordinary cancellation checks need no
separate storage-interruption test or SD transfer.

These checks establish a bounded sample workflow. Media durability, actual storage
errors, concurrent writers, substantial stack/RAM/frame time and large-mod
compatibility remain independently scoped. See the
[JSON contract](../../docs/reference/JSON-SERVICES.md) and
[verification record](../../docs/verification/JSON-SERVICES1.md).
