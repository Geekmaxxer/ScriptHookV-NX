# Runtime Completion Test 1

Local verification passed for the exact runtime hash in verification.json:

- 569 core assertions, including timing/output guards, invalid-yield callback
  isolation, eight NROs with sixteen cooperative scripts and eight callbacks,
  optional diagnostics report/chord behavior, and reproducible NRO output.
- 19 lifecycle and 46 existing four-plugin coexistence assertions.
- Four assertions executing the actual lifecycle-adapted PC source example
  through Kill/Load, including restoration of its render callback.
- 152 Mod Porter checks, 25 portable-tool tests and eight NRO example builds.

Game, GPU, OS and filesystem behavior is mocked where the ARM64 harnesses need
it. Source-port examples are compiled with the actual AArch64 toolchain. These
results do not establish console acceptance for this new runtime revision.

The accepted SDK Parity Test 1 baseline and its changing pool-count confirmation
remain recorded separately. Controller input is the release target; USB keyboard
coverage is optional. The shipped probe no longer registers keyboard callbacks.

Original plugin/API/extension ABI 1 and 8/16/8 defaults are preserved. Static
mapped span increases by one 4 KiB page over the Parity runtime; BSS grows by
1,536 bytes. Counters measure elapsed dispatch time, not isolated CPU time or FPS.
The optional diagnostics mod writes only at startup, on request and on stop.

Matched console acceptance was recorded on 23 October 2026 in
[the hardware record](hardware-20261023/console-acceptance.json). The exact runtime
hash matches the installed module. The multi-script worker/render example and
optional diagnostics ran alongside the existing mods without observed regressions.
The original local verification record remains unchanged. SD files were read only.
