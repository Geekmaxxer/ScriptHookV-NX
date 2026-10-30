# Managed resource lifetime example

This example owns a C++ session containing a dynamic string and vector. The
session remains live across cooperative waits. Stopping the plugin destroys it
through the resource registry, including when its stack destructor is abandoned.

```powershell
python tools/build_mod.py --project examples/managed-resources/project.json --output out/managed-resources --devkitpro C:/devkitPro
```

The project requires the unreleased owner-memory and resource services plus the
C/C++ profiles. It refuses initialization when the required services are absent.
`settings/managed-resources-example/started.txt` records startup. Kill invokes
the session destructor and writes `cleanup.txt`; the loader reports zero remaining
registered resources. Load starts a fresh session without reviving the old handle.

The example reacquires a borrowed object pointer on every iteration and bounds
its sample vector to 128 entries. It provides no custom stop callback: registered
ownership supplies the cancellation cleanup. String/vector members release their
own allocations normally when the session destructor runs. Freed arena capacity
remains subject to the existing explicit Trim contract.

See [owned resource lifetimes](../../docs/reference/OWNED-RESOURCES.md). This is
explicit managed ownership, not general stack unwinding or global constructor
support. An [initial physical run](../../docs/verification/PLATFORM-CAPACITY1.md#physical-test1-demo-run)
kept the managed session active for a reported minimum of 30 seconds. Kill/restart,
full lifetime behavior and performance acceptance remain pending.
