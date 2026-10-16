# Showcase mods - Test 1

Three small ABI 1 NRO mods for testing together with the accepted public sample.
Console testing passed for all three overlays running simultaneously and for
Manager Test 2 after a full game restart. Sources and local integration tests
are included. Full gameplay testing and console FPS/peak-memory measurements remain pending.

| Mod | Shortcut: hold L + R, then press | Behavior |
| --- | --- | --- |
| Drive HUD | D-pad Right | Vehicle speed/compass; cycles km/h, mph, hidden and saves the mode |
| Stunt Journal | D-pad Up | Shows saved personal bests; automatically records vehicle jumps |
| Pit Stop | D-pad Left | Repairs and washes a stationary car/bike while you are driving it |

Close other mod menus before using shortcuts. The two shoulders distinguish
these shortcuts from the menu opening combination. All three hide their overlays
during pause, cutscenes, fades and death. No entities are spawned. Only an explicit
eligible Pit Stop request changes a vehicle.

## Build one mod with the public SDK

From the ScriptHookV-NX project directory, with devkitA64 installed:

```powershell
python tools/build_plugin.py --source mods/showcase-test1/drive-hud/main.cpp --include mods/showcase-test1/common --output build/my-drive-hud
```

Replace the source path for either other mod. Each output is a standalone plugin;
common code is compiled into that NRO. The generated native requirements header
checks availability before starting. Settings use independent plugin namespaces
and small alternating CRC-checked records. File I/O occurs at startup, explicit
changes/actions, new personal bests or diagnostics, rather than every frame.

## Maintainer audit and joint tests (full research checkout)

```powershell
python tools/build_showcase.py --output build/my-showcase
python tools/test_showcase.py --core build/refinement-test2-final2-20261015 --mods build/my-showcase --sample build/public-sample-final2-20261015
```

The audit checks every used native against the pinned target registration catalog
and validates controller constants against the SDK declarations. It requires the
workspace's extracted target and dev_ng headers; ordinary individual SDK builds
use the already generated requirements header instead.

Integration tests execute all four real NROs through the production loader,
caller, cooperative stacks and file API, with scripted game/native and OS/RO mocks.
They check controls, mutation guards, jumps, restart persistence, damaged-record
fallback and idle disk behavior. This does not measure console FPS or peak memory.
