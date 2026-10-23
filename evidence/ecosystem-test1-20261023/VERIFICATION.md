# Ecosystem Test 1

The core rebuild is byte-identical to the console-accepted v1.3 runtime.
The new UI/asset/tool features have local verification and need their own
console acceptance. No SD files were changed.

- 169 desktop assertions and 25 portable-tool tests passed.
- 38 pure UI model checks execute as real ARM64 code.
- Eight checks execute the actual UI NRO on the accepted runtime, with game,
  controller, HUD, filesystem and OS services mocked. They cover open/navigation,
  explicit settings saves, measured layout caching and Kill/Load restoration.
- 1,946 WPF layout checks across 144 scenarios passed. The real PED UI builder
  also produced Zoro from the installed mpbiker metadata and populated its
  base-definition selector. Neither operation is a console gameplay test.
- The four test packs pass registration checks. Independent CodeWalker reads
  and byte comparisons cover 11 model/variation resources: geometry, skeleton
  and physics bytes are unchanged. These samples needed no embedded texture
  descriptor reductions; the physics-child reduction path has generated-fixture
  coverage and typed traversal on real assets.

Misaki's three absent accessory maps are diagnosed in the original source.
The combined test retains the earlier documented fallback substitutions.
Warnings about shared shader/audio/handling resources require installed-game
context; they do not establish that the dependency is absent from the game.

The asset output merges the current selected DLC list and retains its other
entries. Rendering quality, animations and collision freezes need the console
matrix in [maintenance](../../docs/MAINTENANCE.md). The pipeline does not alter
mesh/collision complexity or claim to resolve those freezes.

The clean source export passed the full suite with devkitA64. Compiler-free
mode passed 151 assertions and explicitly skipped four compiler groups.
The packaged SDK and desktop SDK both generate the byte-identical UI NRO.
The packaged Windows application also completed the PED builder workflow.
