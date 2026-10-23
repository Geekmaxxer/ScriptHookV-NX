# Disarm NX

Switch behavior port of [Disarm by ikt32](https://github.com/ikt32/GTAVDisarm).
Active automatically when loaded. Shoot an armed NPC's weapon, right hand or
either upper arm or forearm to make them drop the weapon. No button shortcut is needed.

Select the current source checkout as Mod Porter's SDK (`include/shv/world.h` is
required). Import this folder, create a project with ID `disarm-nx` and build.
Install the generated ZIP with ScriptHookV-NX. Stop the plugin in the manager to
disable future disarms; weapons already dropped remain in the game world.

The port checks up to 32 peds supplied by the game's nearby scanner and processes
at most eight per frame. It does not enumerate the entire PC pedestrian pool.
The player, invalid/dead NPCs and unarmed NPCs are skipped.

`switch/gtav-scripthook/settings/disarm-nx/status.txt` records startup and bounded
activity counters. Hand/weapon disarming passed Switch gameplay testing. The Test 2 extension to both arms also passed Switch gameplay testing. Police may draw a remaining sidearm after dropping their primary weapon.
