# Completion roadmap

Release evidence is scoped to individual binaries and operations. Compilation,
emulation and file readback are separate from Switch gameplay acceptance.

| Area | Current implementation and remaining work |
| --- | --- |
| Real mod ports | Footprints and Disarm gameplay passed their recorded tests. Continue lifecycle, coexistence and soak tests; add mods exercising settings and streaming. |
| Plugin lifecycle | Optional bounded cleanup implemented; local ARM64 checks cover stop, completion, restart and call-state violations. Footprints cleanup and NSC menu Kill/Load passed console acceptance. Mapped NROs remain resident. |
| Portable APIs | [SDK extensions](EXTENSIONS.md) implement entity pools, multiple scripts, globals/entity pointers, PNG textures and input/render callbacks. Combined probe gameplay, changing entity counts, rendering, controller input and restart passed on Switch. USB keyboard is optional. |
| Source analysis | Actual compiler signature/type and conditional-code checks added. Lexical dependency findings remain a separate review. Additional libraries/hooks need explicit adapters. |
| Assets | Loose-PED DLC creation from selected target definitions, registration/model/shader audits, batch merging and Legacy physics-child texture traversal implemented. Arbitrary rig/collection repair, mesh/collision simplification and per-category gameplay acceptance remain. |
| Desktop delivery | Toolchain checks, report import, verified SD NRO installation/restore and current-game archive selection implemented. Continue real-mod recipes and fresh-PC toolchain testing. |
| Runtime completion | [Elapsed-time diagnostics, callback isolation and multi-script source-port lifecycle adaptation](RUNTIME-COMPLETION.md) implemented. Local tests cover eight resident NROs, sixteen scripts and eight render callbacks together; matched console acceptance covers the multi-script worker/render example, diagnostics and coexistence without observed regressions. |
| Performance | Optional diagnostics attribute elapsed work to plugin scripts/callbacks without periodic logging. Instrumented game-wide frame-time and peak-memory measurements remain separate from gameplay observations. |
| Reusable UI | Header-only controller menus, scrolling, settings selectors, measured text and a persistence demo. New UI appearance/controller/coexistence checks need console acceptance. |
| Maintenance | One public verification command, portable model tests and pinned Windows/Linux CI workflow. Hosted CI execution and console acceptance remain separate from local checks. |
| Binary-only ASIs | Separate research: executable translation and PC API/engine compatibility. Source ports remain the supported route. |

An unsupported operation must produce a clear limitation, not a fabricated success.

[Pinned speedometer and Menyoo assessments](PORT-CANDIDATES.md) list concrete
dependencies and a staged migration plan for the next substantial source port.
Enhanced assets, arbitrary PC hooks, managed plugins and binary translation are
not covered by Legacy resource naming or native source wrappers.
