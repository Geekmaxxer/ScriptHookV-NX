# Geometry math and formatted diagnostics

This standalone project combines the new math profile with the pending buffer
formatter workload. It uses ordinary `<cmath>` functions for a 32-step unit-circle
check and a three-dimensional distance calculation, then verifies domain/range
errors, reusable variadic formatting, truncation and rejected `%n` behavior.
It changes no game entities, save data or shared runtime configuration.

```powershell
python tools/build_mod.py --project examples/math-profile/project.json --output out/math-profile --devkitpro C:/devkitPro
```

The manifest enables `c_runtime`, `numeric_runtime` and `math_runtime`. The existing
owner-memory runtime is required. The selected devkitA64 toolchain supplies libm;
its identity is recorded in `plugin-build.json`. Preserve the bundled third-party
notices when redistributing a compiled mod.

The script completes naturally after yielding between geometry samples. It writes
`settings/math-profile-example/result.txt`. A passing result has `failed=0`,
`geometry_verified=1`, `domain_verified=1`, `range_verified=1`, `format_verified=1`,
`truncation_verified=1`, `rejection_verified=1` and `owned_allocations=0`.
The first formatted sample is `run=1 distance=13.00 heading=0.785398`.
Loading again increments both reported run numbers without retaining heap state.

Physical acceptance remains pending. The later
[library-services example](../library-services/README.md) merges these checks
with numeric parsing and the earlier formatter workload for one deferred batch.
Separate math/formatter runs and repetition of accepted batch 4 are unnecessary. See the
[math contract](../../docs/reference/MATH-PROFILE.md) and
[verification record](../../docs/verification/MATH-PROFILE1.md).
