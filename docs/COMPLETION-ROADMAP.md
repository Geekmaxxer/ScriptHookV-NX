# Completion roadmap

Release evidence is scoped to individual binaries and operations. Compilation,
emulation and file readback are separate from Switch gameplay acceptance.

| Area | Current implementation and remaining work |
| --- | --- |
| Real mod ports | Footprints and Disarm gameplay passed their recorded tests. Continue lifecycle, coexistence and soak tests; add mods exercising settings and streaming. |
| Plugin lifecycle | Optional bounded cleanup implemented; local ARM64 checks cover stop, completion, restart and call-state violations. Footprints cleanup and NSC menu Kill/Load passed console acceptance. Mapped NROs remain resident. |
| Portable APIs | Controller chords, bounded INI reader, validated settings storage and nearby ped queries implemented. General PC world enumeration still needs target research. |
| Source analysis | Actual compiler signature/type and conditional-code checks added. Lexical dependency findings remain a separate review. Additional libraries/hooks need explicit adapters. |
| Assets | Typed embedded texture reduction, XML validation, missing-resource notices and DLC-list merging and file-backed updates from current game archives implemented. Arbitrary rig/collection conversion, physics-child texture graphs and per-category gameplay acceptance remain. |
| Desktop delivery | Toolchain checks, report import, verified SD NRO installation/restore and current-game archive selection implemented. Continue real-mod recipes and fresh-PC toolchain testing. |
| Performance | Hardware frame-time, startup and peak-memory measurements with several mods/assets; host checks cannot replace these. |
| Binary-only ASIs | Separate research: executable translation and PC API/engine compatibility. Source ports remain the supported route. |

An unsupported operation must produce a clear limitation, not a fabricated success.
Enhanced assets, arbitrary PC hooks, managed plugins and binary translation are
not covered by Legacy resource naming or native source wrappers.
