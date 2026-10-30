# Vehicle Control NX - Test 2

Original: [ikt32/GTAVVehicleControl](https://github.com/ikt32/GTAVVehicleControl/tree/b807af825ce51bafb8794715d436078d21d18de2), MIT license.
Switch adaptation: MEGATARD (@Geekmaxver), using ScriptHookV-NX.

Remember a vehicle; remotely control engine, locks, six doors, four windows, lights and hazards; locate or forget it. Maximum interaction range: 200 metres.

Test 2 automatically remembers the latest vehicle driven, displays engine/lock
and range states, checks door availability and confirms actions. Locating remains
available outside the interaction range. Automatic remembering can be disabled.

## Differences from the PC mod

One remembered vehicle. PC multi-vehicle lists and radio/neon/bomb-bay pages are not implemented. The common Switch shortcut can be configured in controls.ini. The mod restores its own temporary hazards on stop; explicit engine/door/lock actions remain as selected.

This is a native gameplay adaptation with a replaced platform/UI layer, not a
feature-identical PC binary conversion. Local build and mocked-runtime checks
do not establish Switch gameplay, visual quality or performance.

See the [test guide](../README.md) for controls and installation. Source hashes
and the pinned revision are in upstream.json. Retain LICENSE.upstream.txt when
redistributing upstream-derived code or data.
