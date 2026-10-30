# Footprints — real mod source-port example

Original mod: [Footprints by root-cause](https://github.com/root-cause/v-footprints), MIT license.
This example retains its flaming-footprint feature, custom ScriptInit registration and old SDK spellings.
The NX adaptation replaces PC text input with **L + R + Y** on Nintendo controllers and adds a three-second particle-loading timeout. Walk or run after enabling it to see flaming feet/footprints.

Import this folder into Mod Porter, create a project with ID `footprints-nx`, then build.
The optional stop callback clears the owned effect when stopped on Lifecycle Test 1 or a later supporting runtime. On older runtimes, toggle it off before stopping. Character changes also clear the owned effect; coexistence with external particle mods requires testing.

`config.ini` defaults to Y and disabled at startup. The install ZIP places it at `switch/gtav-scripthook/settings/footprints-nx/config.ini`; preserve any existing customized copy. Set `ToggleButton=X` or `EnableOnStart=true` if desired. SD installation of the NRO alone preserves existing settings.
If the particle asset is absent, the effect stays disabled and a message appears.

Switch gameplay testing confirmed plugin startup, shortcut activation and the flaming-footprint effect. The initial on-screen prompt incorrectly called the face button X; the corrected prompt says Y and uses the same control ID.
Lifecycle testing passed effect cleanup and restart through the NSC plugin manager. Individual INI override combinations and extended coexistence remain separate checks.

`PROVENANCE.json` records the pinned upstream revision and hashes. `upstream-reference` contains unchanged originals with `.txt` suffixes so they are not compiled.
