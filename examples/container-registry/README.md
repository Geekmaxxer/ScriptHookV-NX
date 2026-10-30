# Container registry example

Requires the unreleased memory core and current C, numeric and C++ SDK profiles.

```powershell
python tools/build_mod.py --project examples/container-registry/project.json --output out/container-registry --devkitpro C:/devkitPro
```

The plugin parses three XML presets with decimal coordinates. A growable vector
owns move-only records and dynamic label strings; a sorted map indexes record IDs.
Import commits only after the temporary registry is complete. It serializes the
presets and writes `presets.xml` plus `result.txt` under
`switch/gtav-scripthook/settings/container-registry-example/`, then returns.
Cleanup deletes the registry and trims unused pools. No game entities are spawned.

Expected report: `loaded=yes` and `records=3`. Records are Harbor (ID 1), Workshop
(2699) and Hill (42); Workshop has coordinates -12.5, 100 and 0.125. An allocation
failure reports `loaded=no`; it is not a successful export because an old XML
file still exists. The settings API retains its flat paths, 64 KiB write cap and
raw single-file durability. In-memory import does not make SD writes transactional.

The compiled workflow, restart, cleanup and allocation failure are locally
checked with modeled filesystem/RO services. Physical Switch acceptance remains
pending. See [C++ contracts](../../docs/verification/PLUGIN-CXX1.md) and retain project and
third-party notices when redistributing a mod containing these libraries.
