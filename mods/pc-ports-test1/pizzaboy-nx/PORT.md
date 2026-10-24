# Pizzaboy NX - Test 2

Original: [root-cause/v-pizzaboy](https://github.com/root-cause/v-pizzaboy/tree/a4a706e7fb4077386c64d1504648b5307eac79e7), MIT license.
Switch adaptation: MEGATARD (@Geekmaxver), using ScriptHookV-NX.

Four original shops; five-minute Faggio job, up to four marked customers, five throwable boxes, $10-$30 delivery rewards, shop restock and job/death cleanup.

## Differences from the PC mod

Customer generation has a finite retry budget. Scripted customer animations, speech, fades, PC localization and automatic shift continuation are not implemented. Boxes and the delivery bike use installed stock game resources.

This is a native gameplay adaptation with a replaced platform/UI layer, not a
feature-identical PC binary conversion. Local build and mocked-runtime checks
do not establish Switch gameplay, visual quality or performance.

See the [test guide](../README.md) for controls and installation. Source hashes
and the pinned revision are in upstream.json. Retain LICENSE.upstream.txt when
redistributing upstream-derived code or data.
