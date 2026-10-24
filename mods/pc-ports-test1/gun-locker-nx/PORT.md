# SP Gun Locker NX - Test 2

Original: [root-cause/v-spgunlocker](https://github.com/root-cause/v-spgunlocker/tree/746fca5e9371afc4f4882dd203342e7a54b6dc5b), MIT license.
Switch adaptation: MEGATARD (@Geekmaxver), using ScriptHookV-NX.

One $2,500 locker per story character; ten weapon slots; ammo, clip, tint and the 315-entry upstream component catalog; saved location/inventory; empty-locker refund.

## Differences from the PC mod

One locker per character. PC arbitrary locker counts, XML/add-on definitions and attached display-weapon props are not implemented. Original XML saves are not imported. Journal writes precede inventory changes; the game inventory and SD files do not form a single atomic power-loss transaction.

This is a native gameplay adaptation with a replaced platform/UI layer, not a
feature-identical PC binary conversion. Local build and mocked-runtime checks
do not establish Switch gameplay, visual quality or performance.

See the [test guide](../README.md) for controls and installation. Source hashes
and the pinned revision are in upstream.json. Retain LICENSE.upstream.txt when
redistributing upstream-derived code or data.
