# Opt-in math profile

The `math_runtime` build option links the selected devkitA64 toolchain's Newlib
libm for source ports that need geometry, interpolation and ordinary numerical
operations. It uses the C profile's per-plugin error hook without changing the
game runtime, target bindings or optional-service ABIs. The library is linked into the NRO;
the game does not supply these functions.

## Build and use

```json
{
  "format": 1,
  "kind": "nro",
  "sources": ["main.cpp"],
  "c_runtime": true,
  "math_runtime": true
}
```

Alternatively pass `--c-runtime --math-runtime` to `tools/build_plugin.py`.
Both options are booleans, default to false and apply only to NRO projects.
Math support requires the existing C profile and its bound owner-memory service.
Enable `numeric_runtime` separately when decimal parsing or floating text output
is also needed. The [combined example](../../examples/math-profile/README.md)
demonstrates all three profiles together.

```cpp
#include <cmath>
#include <errno.h>

const double distance = std::hypot(std::hypot(x, y), z);
errno = 0;
const double result = std::sqrt(value);
if (errno || !std::isfinite(result)) {
    // Apply the port's explicit policy for an invalid or out-of-range value.
}
```

## Verified function families

Local tests execute float and double forms of:

| Family | Functions |
| --- | --- |
| Trigonometry | sin, cos, tan, asin, acos, atan, atan2 |
| Powers and scale | sqrt, cbrt, pow, hypot, exp, exp2, expm1 |
| Logarithms | log, log2, log10, log1p |
| Rounding | floor, ceil, trunc, round, nearbyint, rint |
| Remainders and comparisons | fmod, remainder, fabs, fmin, fmax, fdim, copysign, nextafter |
| Hyperbolic functions | sinh, cosh, tanh, asinh, acosh, atanh |
| Error functions | erf, erfc |

Double frexp/ldexp/modf pointer outputs, qualified C++ calls and fused multiply-add
are also exercised. This is a tested subset of the selected archive, not a
certification of every symbol it contains. Long-double routines, special C++
functions and floating-environment control are outside this verification scope.
Trigonometric arguments use radians. Results retain normal floating-point
rounding, signed-zero, infinity and NaN behavior; inspect each call as appropriate.

## Errors and ownership

`errno` comes from the common [C parser](NUMERIC-PARSING.md)'s private integer
in each resident NRO. Cooperative entries
inside that NRO share it; unrelated plugins do not. Set it to zero before a call
when detecting a newly reported error. Success does not necessarily clear an
earlier error. Domain/range error behavior belongs to the selected library;
the verified toolchain reports EDOM/ERANGE on the exercised exceptional paths.

The value is module state and can persist across Kill/Load. It is not OS-thread
local, and this option supplies neither a general reentrant C runtime nor arbitrary
thread support. Math functions do not yield. The verified subset makes no owner
heap allocation and retains no filesystem handles. Tests use the model's default
floating-point control state; alternate rounding, flush-to-zero modes and physical
frame-time behavior retain separate requirements.

## Toolchain and reproducibility

The builder asks the selected compiler for `libm.a`, verifies it exists before
creating build output, and checks that its hash remains stable during the build.
`plugin-build.json` records the archive name, SHA-256, byte count and compiler
version. Archive contents can differ between devkitA64 releases; retain these
identities with the exact artifact's acceptance record.

GCC 16 gates `<cmath>` and its `<math.h>` forwarding header behind hosted header
mode. This explicit profile enables those declarations while keeping builtins,
exceptions and RTTI disabled. Linking still uses `-nostdlib`, `-nostartfiles`,
strict undefined-symbol rejection and constructor/finalizer checks. The profile
does not link libc or libstdc++, provide streams, or enable automatic constructors.
Some compiler-inline functions may still become target instructions.

Compiler stack reports cover newly compiled units, not every prebuilt archive
routine or the complete call chain. The supported SDK link checks do not establish
Switch stack high-water use or mathematical correctness for every possible input.

The source SDK contains the [Newlib license notice](../../sdk/third_party/newlib/LICENSE.txt)
and notice provenance, with no copied libm archive or implementation. Preserve
applicable third-party notices in binary distributions. The upstream
[math documentation](https://sourceware.org/newlib/libm.html) describes library
families and error handling; the [verification record](../verification/MATH-PROFILE1.md)
defines the narrower tested scope of this SDK profile. Physical acceptance is
pending in the consolidated [library-services workload](../../examples/library-services/README.md),
which also covers numeric parsing and the earlier formatter checks.
