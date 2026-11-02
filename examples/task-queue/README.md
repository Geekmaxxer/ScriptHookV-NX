# Captured-action queue

This workload combines the fallible ring deque and move-only callable wrapper.
It executes 768 ordered actions with owned string captures, alternating inline
and over-aligned heap targets. A 512-entry queue wraps after front consumption,
then relocates its wrapped contents before executing the remainder. Registered
root ownership reclaims every pending action if the plugin is cancelled.

```powershell
python tools/build_mod.py --project examples/task-queue/project.json --output out/task-queue --devkitpro C:/devkitPro
```

The C/C++ profiles and existing memory/resource services are required. This adds
no runtime table, target binding, game entity, save change or shared configuration.
Work is grouped with explicit cooperative yields; callbacks never wait or throw.

## Combined acceptance

The probe joins resource-registry and read-streams in the expanded
[service batch 8](../service-acceptance/README.md). Use one later installation and
game session. After two natural runs, retain
`switch/gtav-scripthook/settings/task-queue-example/result.txt`:

```text
run=2
failed=0
completed=1
queued=768
executed=768
sum=295296
peak_queue=512
wrapped_verified=1
relocation_verified=1
live_captures=0
owner_cleanup_complete=1
owned_allocations=0
registered_resources=0
```

Preserve that report before optional Kill during construction/execution changes
its counters. A partial run may have completed=0 and lower counts; live captures,
owned allocations and registrations must still return to zero. Load Now must
complete a fresh lifetime. These checks require no separate SD transfer from
the resource/stream tests. The earlier kits remain archived.

This example does not create threads or a game scheduler. It runs a source-port
action queue on the existing cooperative entry. Physical timing, stack use,
low-memory behavior, transitions and long sessions retain their own scope. See
the [queue/callable contract](../../docs/reference/TASK-QUEUES.md).
