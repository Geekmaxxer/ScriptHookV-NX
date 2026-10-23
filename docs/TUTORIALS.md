# Examples you can build and change

Generate each lesson with `tools/create_mod.py --template NAME --id YOUR-ID
--output projects/YOUR-ID`, then build its `project.json` with `tools/build_mod.py`.
Run `python tools/create_mod.py --list` for all templates. The original tutorial
sources are also directly buildable with `build_plugin.py --source ...`.

Test **one lesson at a time**. Capacity Test 1 supports eight resident NROs total
(four on earlier public runtimes);
the standalone manager uses one slot and completed providers also stay mapped.
Changing startup policy or killing a script does not free its module slot.

| Template | What it teaches | Expected result |
| --- | --- | --- |
| `hello` | Minimal entry, isolated files, returning normally | `<id>/hello.txt` |
| `overlay` | Frame rendering, pause guard, safe long text components | Top-left text below the game's main HUD; hidden while paused |
| `sdk-parity` | Additional worker, four pools, borrowed pointers, input/render callbacks, PNG texture | Requires SDK Parity Test 1 and a generated PNG; see [setup and expected results](EXTENSIONS.md#buildable-combined-example) |
| `saved-counter` | Defaults, schema, CRC, alternate slots and readback | `<id>/counter.txt` increases after Load Now or the next launch |
| `vehicle-logger` | Read-only handles, 250 ms sampling, writes only on state changes | `<id>/vehicle.txt` changes between on_foot and in_vehicle |
| `native-provider` | Explicit initialize, registration, signed INT ABI and completion | `<id>/provider.txt` says ready; custom handler remains available |
| `nsc-settings` | Generic persistence independent of MEGATARD | NSC counter saved under its own script name |
| `nsc-provider-client` | Calling an NRO-provided native from SanScript | Two saved results: 5 and -7 |

All text/result files above live under `switch/gtav-scripthook/settings/`.
An initialization or file error is not a successful check merely because a file
from an earlier run exists. Check the latest loader report and installed hashes.

## From a lesson to a mod

Keep the plugin ID stable so updates can find old settings. Use a different ID
for another mod. The provider lesson's `0x5348564e58414431` is a demonstration
hash: generating a new project ID does **not** allocate another native hash.
Assign your own hash and update both provider/client declarations and the NSC
project's `custom_hashes` when creating a different public API.

The overlay intentionally changes no gameplay state. It restores its text style
each frame and demonstrates text splitting, rather than promising a complete
HUD layout system. The vehicle logger polls slowly and does not retain a stale
vehicle handle across waits. It writes the latest state, not an endless log.

`SavedSettings<N>` stores uint32 words; it is not a serializer for pointers,
strings or arbitrary C++ structs. Use separate schemas and validate setting
ranges after loading. See [helper contracts](SDK-HELPERS.md).

For full examples, study the three showcase mods and standalone manager. They
demonstrate controller chords, bounded drawing, readiness guards, persistence,
startup policy readback. The optional plugin_manager.h API additionally exposes
cooperative process controls for your own UI. These are larger
examples, not dependencies that every plugin must copy.
