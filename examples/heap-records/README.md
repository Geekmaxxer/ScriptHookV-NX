# Growing heap-record workload

This probe holds 4,096 independently allocated buffers and one registered root
object. It checks exact sizes and payloads, reuses a freed prefix, then frees
buffers in cooperative groups. A final resource audit verifies complete cleanup.
Kill during construction uses the same registered root to reclaim its children.

```powershell
python tools/build_mod.py --project examples/heap-records/project.json --output out/heap-records --devkitpro C:/devkitPro
```

The project enables the C/C++ profiles and requires memory/resource services. Its
4,097 simultaneous owner allocations cross both the original record limit and
the initial pointer-index load threshold. It uses the existing runtime
configuration; shared resources can still refuse admission. No entities or game
saves are modified, and no runtime configuration file is supplied.

## Combined acceptance

The exact probe completed in [physical Batch 7](../../docs/verification/SERVICE-BATCH7.md),
together with the texture, library and hash-registry workloads. Its record/index
workload is accepted under the recorded explicit settings. After two natural runs, retain
`settings/heap-records-example/result.txt` under `switch/gtav-scripthook/`:

```text
run=2
failed=0
completed=1
verified=4096
peak_allocations=4097
owner_cleanup_complete=1
owned_allocations=0
registered_resources=0
```

Preserve this result before additional cancellation tests change the run counter.
Kill during construction should leave `completed=0` and complete owner cleanup;
Load Now must then start a fresh lifetime. Loader reports identify grown record,
arena and lookup capacity separately from live owner allocations.

This workload exercises record/index growth under the existing configuration.
Its recorded hardware run used an explicit 16 MiB budget. The separate local
160-arena check also uses 16 MiB; physical acceptance of larger
arena configurations and measured fragmentation/performance remain open.
See [heap-capacity verification](../../docs/verification/HEAP-CAPACITY1.md).
