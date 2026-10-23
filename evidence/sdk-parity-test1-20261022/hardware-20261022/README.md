# SDK Parity Test 1 console acceptance

Matched runtime, probe and PNG hashes passed verification against the installed
SD files. Four NROs run together, with zero loader, callback or scheduler-context
errors. The probe confirms an active additional script, rendering/input callbacks,
39 ped handles, 36 vehicle handles, 128 object handles (the example cap), a live
player pointer and an allocated script-global bank.

Console gameplay and visual checks passed: smooth texture rotation, correct
colours, Y overlay toggle, Kill/Load, NSC/NRO coexistence, approximately ten
minutes of gameplay without observed FPS issues, and a full game restart.
The post-restart snapshot does not retain counters from the earlier Kill/Load.

Accounted plugin memory is 268 KiB, including a 36 KiB texture charge. These
figures are loader accounting, not physical peak RAM or GPU measurements.
USB keyboard delivery, live pickups, separate alpha/expiry checks and extended
profiling remain unverified. No arbitrary global/entity memory writes were tested.
See console-acceptance.json for exact artifact/report hashes and test boundaries.
