# Numeric parsing and consolidated library verification

This SDK milestone adds C-style integer and decimal-prefix parsing. The common
C profile now owns per-NRO errno state, replacing the separate math-only hook.
Runtime binaries, target bindings and accepted batch 4 artifacts remain unchanged.
Rebuilt plugins contain new code and require their own hardware acceptance.

## Implementation and local verification

- [x] Provide target-width signed/unsigned base-2–36 parsing and convenience functions.
- [x] Verify base detection, signs, end pointers, saturation and complete overflow consumption.
- [x] Preserve existing errno on successful/no-conversion paths and refuse invalid bases explicitly.
- [x] Verify integer extrema and deterministic arbitrary-base reference samples.
- [x] Provide numeric-profile decimal strtod/strtof/atof through pinned fast_float.
- [x] Verify decimal prefix lengths, literal nonfinite values and reported range errors.
- [x] Exercise direct binary32 rounding beyond an intermediate-double boundary.
- [x] Verify independent NRO error state and conversion after Kill/Load.
- [x] Combine parsing, math and formatter checks in one complete acceptance example.
- [x] Build and execute the exact manual batch 5 NRO with verified binary-kit notices.
- [x] Reproduce every batch 5 kit file from the lean SDK.
- [ ] Accept the consolidated library-services workload on physical Switch hardware.

The [local record](../../evidence/numeric-parsing1-local-20261031/verification.json)
identifies source hashes, exact artifacts, reference comparisons and regressions.
Production runtime, SDK parsers, math and example ARM64 instructions execute with
modeled OS, RO, game/filesystem boundaries and outer scheduling. These checks
do not establish physical timing, peak memory, stack use or alternate FP state.

## Consolidated pending hardware check

The pending math and formatter hardware checks are merged with parsing in
[library-services](../../examples/library-services/README.md), packaged in
[service batch 5](../../examples/service-acceptance/README.md). Its single
two-run workload covers integer/decimal conversion, parser and libm errors,
geometry, reusable variadic formatting, truncation and rejected-format behavior.
The run remains deferred while implementation continues; no standalone math or
formatter test is requested. Accepted batch 4 does not require repetition.

The [parsing contract](../reference/NUMERIC-PARSING.md) retains explicit ASCII,
decimal-subset, ownership and synchronous-execution limits. Standard streams,
long-double parsing, throwing C++ conversions and general hosted-library support
remain outside this milestone. Historical local evidence and artifact archives
are preserved with their original scope.
