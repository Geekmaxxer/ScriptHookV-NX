# SDK Parity Test 1 verification

Matched [Switch gameplay acceptance](hardware-20261022/console-acceptance.json)
is now recorded separately from the local checks below. It covers the combined
probe's rendering, controller input, additional script, pointer availability,
coexistence, Kill/Load and restart. USB keyboard and live pickup checks remain.

The runtime and SDK implement full ped/vehicle/object/pickup enumeration, owned
additional cooperative entries, validated global/entity pointers, PNG textures,
and controller, USB keyboard and script-frame rendering callbacks.

The original plugin/API descriptors remain ABI 1; optional extensions use their
own ABI 1 table. The runtime SHA-256 is recorded in verification.json.

Local checks passed: 539 core regression assertions, 19 lifecycle assertions,
46 existing four-plugin coexistence assertions, 142 Mod Porter checks, 24
portable-tool tests and eight compiled/structurally checked NRO examples. The
GUID allocation path additionally executed the target allocator, generation
and handle instructions with extension-node allocation mocked. The two runtime
builds used for these checks are byte-identical.

OS services and game input/filesystem/render state are mocked by the ARM64
harnesses. PNG decoding executes production code; GPU allocation/rendering is
mocked. These results do not establish console gameplay, GPU or keyboard
acceptance. The combined SDK probe is provided for that separate test.

Defaults remain 8 resident NROs, 16 cooperative script slots and an 8 MiB plugin
budget. Static mapped span is 1,413,120 bytes, 65,536 bytes above the preceding
settings-folder runtime. Loader/GPU allocations and actual frame cost require
console measurements. Cached textures remain resident until process exit.

The final combined probe additionally passed seven local assertions covering
texture upload, worker/callback registration, controller and keyboard toggles,
stop and restart with cached texture reuse. OS/game/GPU functions remain mocked.
A clean curated source export rebuilt from a relocated path with spaces to the
exact same runtime SHA-256, using an explicitly selected devkitPro toolchain.
