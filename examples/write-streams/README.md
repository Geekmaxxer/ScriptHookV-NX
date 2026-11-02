# Writable C streams

This example verifies binary output, update reads/writes, explicit zero-filled
holes, append-after-seek behavior, checked formatted output and direct XML
`save_file`/`load_file`. It yields between small transfers and reports final owner
cleanup. It uses the C, C++ and stdio profiles; the C++ profile supplies the pinned
XML file writer's virtual deleting-destructor support.

```powershell
python tools/build_mod.py --project examples/write-streams/project.json --output out/write-streams --devkitpro C:/devkitPro
```

The candidate runtime must supply `scriptFileResizeAvailable()`. Older runtimes
refuse initialization before the workflow changes any file. Paths are restricted
to `settings/write-streams-example/`. Each run rewrites its own `output.bin`,
`formatted.txt`, `direct.xml` and `retained.txt` fixtures; do not store personal
data under these names. `result.txt` records the latest lifetime only.

Load twice to natural completion. Every verification and completion flag must be
1, `failed` must be 0, and final owned allocations/registered resources must both
be 0. Kill during a later transfer, then Load again; partial fixture output is
expected after cancellation and the next lifetime recreates it. No game/save data
is touched. This is synchronous output split by explicit waits, not a power-loss
or measured-latency test.

See [stream contracts](../../docs/reference/READ-STREAMS.md),
[local evidence](../../docs/verification/WRITE-STREAMS1.md) and the
[consolidated checklist](../service-acceptance/README.md).
