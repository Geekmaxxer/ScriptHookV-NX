# Read-only C stream workload

This example checks buffered binary reads, line reads, seeking and a complete
`pugi::xml_document::load_file` operation through the namespaced C stream profile.
It creates three original fixtures under its own settings directory, validates
their retained contents and leaves one stream registered across a wait so final
cleanup closes it before the owner audit. Game entities and saves are unchanged.

```powershell
python tools/build_mod.py --project examples/read-streams/project.json --output out/read-streams --devkitpro C:/devkitPro
```

The project enables `c_runtime` and `stdio_runtime`. It requires the existing
memory, file and resource services. The read-only stream layer creates no new
runtime table or game binding. Fixture creation uses the separate explicit file
service; `fopen` itself never writes or truncates files.

## Combined hardware acceptance

The probe joins resource-registry in [service batch 8](../service-acceptance/README.md).
Use one later installation and game session. After two natural runs, retain
`switch/gtav-scripthook/settings/read-streams-example/result.txt`:

```text
run=2
failed=0
completed=1
binary_verified=1
text_verified=1
seek_verified=1
xml_verified=1
owner_cleanup_complete=1
owned_allocations=0
registered_resources=0
```

Preserve the completed result before optional Kill during binary reading and
Load Now checks overwrite it. Perform cancellation after the first complete run
has created all fixtures. A later run verifies existing files; it does not replace
an incomplete or unexpectedly edited fixture. If initial fixture creation was
interrupted, retain its report/files and recreate only this probe's fixture
directory with GTA closed before repeating the batch.

Cancellation may leave completed=0 and some verification flags unset, while final
owner cleanup must still pass. Load should then complete a fresh lifetime. This
probe supplies no visible overlay or timing guarantees. Actual storage failures,
concurrent edits, stack use, latency and long sessions remain separate acceptance
work. See the [stream contract](../../docs/reference/READ-STREAMS.md).
