# Completion Test 1 verification

Disarm Test 1 passed Switch hand/weapon disarming, including police shotguns.
Test 2 extends damage recognition to both upper arms and forearms; it has
separate local checks and pending console acceptance.

Local results for the final runtime and desktop source:

- host-tests: 28 passed.
- lifecycle-tests: 19 passed.
- manager-tests: 118 passed.
- plugin-tests: 51 passed.
- runtime-tests: 75 passed.
- ui-tests: 19 passed.
- porter_core: 120 passed.
- portable_python: 24 passed.

The ARM64 emulator executes production scheduler, loader and manager code with
mocked OS/game/RO/filesystem state. It covers stop, natural completion, restart,
cleanup yield rejection and unfinished-native recovery. Manager regressions
retain distinct Finished and Stopped states. This does not measure Switch FPS,
SD behavior or peak memory.

CodeWalker independently read 29 real fragment models and read back the modified
one containing two reduced embedded textures, preserving its three-model count.
Typed synthetic fixtures also verify shared geometry bytes and allocations remain
unchanged. A real nested DLC converted and its registration merged into the
installation's actual DLC list. Assets still require gameplay acceptance; these
checks do not validate arbitrary rigs or collection metadata.

The larger air-races archive contained a malformed compressed speech REL entry
and was rejected rather than emitting a partial conversion. Individual fragment
resources from that archive supplied the independent reader test.

Release ZIPs have CRC and per-file hash readback checks. No game resources or
research binaries are included. ABI 1 descriptor/table sizes and default
8/16/8 MiB limits are unchanged. The new runtime, arm extension and Footprints
cleanup/INI behavior require Switch tests. verification.json records hashes.

## Console acceptance

The packaged runtime, Disarm arm extension, Footprints cleanup and NSC plugin
Kill/Load controls subsequently passed console gameplay checks. Mod Porter 0.3.0
also passed its desktop workflow. console-acceptance.json identifies the exact
package artifacts and remaining scope. This does not cover the new 0.4.0 archive updater.
