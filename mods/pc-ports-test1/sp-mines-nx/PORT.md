# SP Vehicle Mines NX - Test 2

Original: [root-cause/v-spmines](https://github.com/root-cause/v-spmines/tree/d62decaf0bafd540067159078de6d509dffae221), MIT license.
Switch adaptation: MEGATARD (@Geekmaxver), using ScriptHookV-NX.

Original compiled default vehicle/mine sets, installed upgrade selection, five-second deployment cooldown and owned model/weapon requests.

## Differences from the PC mod

The PC XML customization parser and unconditional particle preloads are not implemented. Requires the corresponding installed vehicle, upgrade and mine weapon assets. Existing mines are game projectiles and are not recalled by Kill Process.

This is a native gameplay adaptation with a replaced platform/UI layer, not a
feature-identical PC binary conversion. Local build and mocked-runtime checks
do not establish Switch gameplay, visual quality or performance.

See the [test guide](../README.md) for controls and installation. Source hashes
and the pinned revision are in upstream.json. Retain LICENSE.upstream.txt when
redistributing upstream-derived code or data.
