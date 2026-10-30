# RC Bomb NX - Test 2

Original: [root-cause/v-rcbomb](https://github.com/root-cause/v-rcbomb/tree/2c06c46e1344e069e26c086c3eed2fe46753677e), MIT license.
Switch adaptation: MEGATARD (@Geekmaxver), using ScriptHookV-NX.

Original Vinewood workshop location; owned van and RC Bandito; remote drive, 200-metre signal range, safe return/cancel, detonation and lifecycle cleanup.

Deployment and locating follow the van if it has moved. A status badge and
notices explain setup, deployment and returning from control.

## Differences from the PC mod

The menu creates the van on request. Cosmetic van attachments, frame selection and first-person/timecycle presentation are not implemented. Detonation within 15 metres of the return point is cancelled to avoid exploding the returned player. The mod does not alter global invincibility, radar or timecycle state.

This is a native gameplay adaptation with a replaced platform/UI layer, not a
feature-identical PC binary conversion. Local build and mocked-runtime checks
do not establish Switch gameplay, visual quality or performance.

See the [test guide](../README.md) for controls and installation. Source hashes
and the pinned revision are in upstream.json. Retain LICENSE.upstream.txt when
redistributing upstream-derived code or data.
