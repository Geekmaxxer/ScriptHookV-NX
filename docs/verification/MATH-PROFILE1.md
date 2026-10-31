# Math profile verification

This milestone adds explicit toolchain-libm linkage and an NRO-local errno hook
for common PC source-port geometry and numerical paths. The existing runtime,
game target profile and accepted batch 4 artifacts remain unchanged. Compiled
math functions execute inside each newly built plugin.

## Implementation and local verification

- [x] Validate explicit NRO-only math options and the required C profile.
- [x] Resolve the selected compiler's archive and record its exact identity.
- [x] Keep strict unresolved-symbol, constructor and NRO structural checks.
- [x] Exercise float/double trigonometry, powers, logs, rounding and distance math.
- [x] Compare deterministic input samples against independent host references.
- [x] Exercise NaN, signed zero, subnormal input, overflow and domain/range errors.
- [x] Verify errno isolation between two NROs and operation after Kill/Load.
- [x] Check decomposition/scaling outputs and qualified C++ math calls.
- [x] Verify ordinary hosted file streams remain unresolved.
- [x] Execute a combined math/formatter example with exact report verification.
- [ ] Accept the combined geometry/formatter workload on physical Switch hardware.

The [local record](../../evidence/math-profile1-local-20261031/verification.json)
identifies archive/compiler hashes, exact NROs, test counts and numerical tolerances.
Production runtime, SDK, Newlib and example ARM64 instructions execute with
modeled OS, RO, game/filesystem boundaries and outer scheduling. Tests use the
model's default floating-point control state. Reference comparisons establish
the exercised samples rather than an exhaustive ULP or correctly-rounded guarantee.

## Compatibility and hardware scope

The [profile contract](../reference/MATH-PROFILE.md) defines header mode, error
ownership, resource limits and unsupported dependencies. The math archive is
selected from the installed compiler rather than vendored into the SDK. Its
hash and compiler version are required evidence when reproducing an artifact.
License text is retained with notice provenance; no new game import or offset
is introduced.

The original [math example](../../examples/math-profile/README.md) remains
available. Its pending geometry/error and formatter checks now join parsing in
[library-services](../../examples/library-services/README.md). The later
[parsing milestone](NUMERIC-PARSING1.md) moves errno ownership into the common C
profile without changing its per-NRO scope. One deferred hardware workload covers
the consolidated additions. Standalone formatter/math and accepted batch 4 probes
do not need separate runs. Stack use, performance, alternate FP state and
coexistence retain their physical requirements.
