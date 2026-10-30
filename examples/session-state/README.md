# Shared C++ session state

This two-source example uses `ScriptHook::Memory::SessionLocal<SessionState>` to
share a fallible string, vector and worker counter between a main entry and an
additional cooperative script. The first explicit access constructs the object;
Kill drains its registered cleanup and the next Load constructs a fresh session.

```powershell
python tools/build_mod.py --project examples/session-state/project.json --output out/session-state --devkitpro C:/devkitPro
```

The C and C++ profiles and owned-resource service are required. There are no
compiler-managed global constructors or destructors. Access occurs on the runtime
owner thread, and every borrow is reacquired after `WAIT`. The worker uses the
same type/tag declaration from a second translation unit.

`settings/session-state-example/started.txt` marks initialization. After Kill,
`cleanup.txt` records the process-local run, cumulative destructor count, worker
progress and final owner cleanup. The second Load/Kill should produce `run=2`,
`destructors=2`, `worker_progressed=1` and `owner_cleanup_complete=1`.

The [service batch](../service-acceptance/README.md) groups this check with file
publication and catalog examples. Local ARM64 execution does not establish new
physical acceptance. See the [session-state contract](../../docs/reference/SESSION-STATE.md).
