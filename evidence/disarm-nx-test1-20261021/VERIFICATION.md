# Disarm NX Test 1 verification

The bounded Switch behavior port compiled through Mod Porter with the actual
devkitA64 compiler and ABI 1 SDK. The NRO passed structural/export checks and
its installation ZIP passed CRC and payload hash readback. Runtime subsdk5 is unchanged.

All 17 used native declarations have target registrations. The nearby scanner
helper uses one eight-byte capacity slot and eight-byte handle slots, and passes
the complete writable buffer range to the native caller. Bone tags match the
target declarations. PC pool enumeration and unused Windows RNG setup are absent.

20 host simulation checks passed against the actual C++ port and SDK wrappers.
Coverage includes right-hand/forearm and weapon damage, false bone outputs,
damage clearing before drop, missing/dead/unarmed/player handles, character/death/
control/arrest gates, query errors, timer wrap, the eight-ped processing budget
and bounded status writes. Host simulation does not establish Switch gameplay.

The NRO is 20,480 bytes, with 4,096 bytes of BSS and 53,248 bytes accounted by the
loader memory calculation. This is a build calculation, not peak-memory profiling.
The exact Test 1 binary subsequently passed Switch hand/weapon disarming,
including police shotguns; see console-acceptance.json. Coexistence, soak and
frame-time profiling remain separate checks. The Test 2 arm extension has its own
build and local evidence and still requires console testing.
