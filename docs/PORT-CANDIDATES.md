# Next substantial source ports

Assessment uses pinned upstream source, not the age of a release or its ASI file.
Neither project below is a playable Switch port in this checkout.

## GTAVSpeedo

[Upstream source](https://github.com/ikt32/GTAVSpeedo/tree/ea8898bd290897c1a76b1b6b070aa05c95cce363)
contains 27 C++ files and 89 PNGs across its skins/dependencies. It is an
MPL-2.0 project; preserve its notices and applicable source obligations.

Its script uses createTexture/drawTexture, which the NX SDK supports, but the
port needs explicit work:

- Replace Windows paths, logging/timers and hosted C++ dependencies.
- Replace GTAVMenuBase input/settings UI with the controller UI or another NX UI.
- Audit every native against the selected target index.
- Replace the x86 byte-pattern write in setupGlobals; do not transplant it.
- Replace optional Gears.asi/TurboFix module lookups and PC vehicle field access
  with verified Switch data or explicitly unavailable features.
- Select a skin and plan its resource use. The upstream skin loader requests
  more textures than the current 16-texture per-plugin cache. Combining or
  adapting sprites requires a rendering/resource design; the 89-file repository
  count is not the number simultaneously loaded.

Start with a native-backed speed/unit display, a bounded texture set, controller
configuration and saved settings. Then add RPM/gears/turbo/NOS only when their
data contracts and resource costs are established. Keep source changes explicit.

## Menyoo

[MenyooSP source](https://github.com/MAFINS/MenyooSP/tree/b894e14f31490fbb91740dc554111bda5be39608)
contains 106 C++ files and about 82,674 C++ lines at the audited revision.
Its GPL-3.0 license remains separate from this SDK's attribution license; no
Menyoo implementation is copied into the SDK by this assessment.

The entry point reads Windows module information, initializes GTAmemory,
registers two script threads and installs a keyboard handler. The memory layer
contains PC pattern scans, patches and layout assumptions. getGlobalPtr and
entity pointers do not make those offsets portable. Hosted strings/containers,
filesystem/XML/settings, rendering assets and Windows services also need review.

A practical migration sequence is:

1. Establish an NX entry/lifecycle, allocation strategy, resource footprint and
   controller UI, with PC memory operations disabled behind explicit adapters.
2. Bring up native-only player/vehicle/world actions and test cleanup/restart.
3. Add bounded model catalogs, streaming and vehicle/outfit save formats.
4. Port Spooner placement/XML features with entity lifetime and file tests.
5. Audit each remaining direct-memory feature against the exact Switch build.

Measure the compiled image, BSS, textures and script-stack use at each stage.
The standard runtime retains its 8 MiB plugin budget; a full Menyoo image has not
been built or measured for it. This is a separate port project, not an automatic
result of compiling the Windows sources as NRO.
