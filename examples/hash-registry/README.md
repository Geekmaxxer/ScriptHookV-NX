# Hash-map registry workload

This example builds an owned registry with 2,048 integer keys, 128 named records
and a deliberately colliding map. It yields between bounded groups, verifies
lookup and erasure, then releases the managed registry and its nested allocations.
One final resource callback records cleanup after natural completion or Kill.

```powershell
python tools/build_mod.py --project examples/hash-registry/project.json --output out/hash-registry --devkitpro C:/devkitPro
```

The NRO requires the existing memory and owned-resource services. Its SDK helper
does not add a runtime ABI or require a different runtime module. Install it as a
manual plugin. The prepared workload has [physical Batch 7 acceptance](../../docs/verification/SERVICE-BATCH7.md).

## Expected result

After two complete Load Now runs, retain
`settings/hash-registry-example/result.txt` under `switch/gtav-scripthook/`:

```text
run=2
failed=0
completed=1
records=1365
names=128
collision_records=64
owner_cleanup_complete=1
owned_allocations=0
registered_resources=0
```

An additional Kill during construction should produce `completed=0` with complete
owner cleanup. A subsequent Load starts a fresh registry. These later actions
change the run counter; preserve the two-run result first.

The example changes no game entities or saves and uses no files beyond its
namespaced report. It is an associative-storage workload, not a hosted STL or
Menyoo implementation. See the [hash-map contract](../../docs/reference/HASH-MAP.md)
and [verification record](../../docs/verification/HASH-MAP1.md).
