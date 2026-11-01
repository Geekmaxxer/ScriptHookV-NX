# Growing cleanup registry

This example registers 512 individually owned heap objects and one final audit.
It yields between construction/verification groups, checks each handle and pin,
then allows normal completion to drain every object in reverse registration order.
The audit checks complete owner cleanup and releases empty payload arenas.

Enable the C/C++ profiles and use the growing-resource runtime candidate.
An older 64-registration runtime can refuse construction; the example reports
the failure and reclaims the objects already registered. No entities, saves or
shared configuration are modified.

```powershell
python tools/build_mod.py --project examples/resource-registry/project.json --output out/resource-registry --devkitpro C:/devkitPro
```

## Combined hardware acceptance

The [service checklist](../service-acceptance/README.md) groups repeated completion,
cancellation/restart and broader gameplay observations for one later installation.
The previously accepted Batch 7 probes do not require separate repeated sessions.
After two complete runs, retain
`switch/gtav-scripthook/settings/resource-registry-example/result.txt`:

```text
run=2
failed=0
completed=1
registered=512
verified=512
destroyed=512
peak_resources=513
reverse_order_verified=1
owner_cleanup_complete=1
owned_allocations=0
registered_resources=0
```

Preserve the completed report before optional Kill during construction and Load
checks overwrite its counters. Partial cancellation can leave completed=0 with
fewer registered/verified objects; reverse order and final cleanup must still pass.
A subsequent Load should complete a fresh lifetime. Metadata capacity remains
resident and reusable; it is separate from owned live objects.

The probe verifies a bounded workload. Larger configurations, nested failure
paths, physical frame time, stack use, fragmentation and long sessions retain
their separate acceptance scope. See [resource growth](../../docs/verification/RESOURCE-CAPACITY1.md).
