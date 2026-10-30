# Lost Wallets NX - Test 2

Original: [root-cause/v-lost-wallets](https://github.com/root-cause/v-lost-wallets/tree/05f2aafe1eb28f1c9c47ba279a574d68e09ddbd5), MIT license.
Switch adaptation: MEGATARD (@Geekmaxver), using ScriptHookV-NX.

Safe-pavement wallet attempts; original default 3% chance every 30 seconds and $80-$400. Manual attempt, eight-pickup cap and distance/collection cleanup.

Test 2 adds a visible enabled/count badge, saved chance/interval controls,
multiple safe-coordinate attempts and collection/spawn notices.

## Differences from the PC mod

Spawning samples a square around the player; per-zone multiplier INI configuration is not implemented. Tracked pickup handles replace untracked ambient objects.

This is a native gameplay adaptation with a replaced platform/UI layer, not a
feature-identical PC binary conversion. Local build and mocked-runtime checks
do not establish Switch gameplay, visual quality or performance.

See the [test guide](../README.md) for controls and installation. Source hashes
and the pinned revision are in upstream.json. Retain LICENSE.upstream.txt when
redistributing upstream-derived code or data.
