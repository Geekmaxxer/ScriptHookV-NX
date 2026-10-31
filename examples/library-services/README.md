# Combined library-service acceptance

This project combines the pending formatter and math checks with C-style integer
and decimal parsing. It provides one workload for the next consolidated hardware
batch. The standalone formatter/math examples remain available for isolation;
neither needs a separate hardware run. Accepted batch 4 does not need repetition.

```powershell
python tools/build_mod.py --project examples/library-services/project.json --output out/library-services --devkitpro C:/devkitPro
```

The manifest enables `c_runtime`, `numeric_runtime` and `math_runtime`. The
existing owner-memory runtime is required. No new game binding, runtime table or
global configuration is introduced. This example changes no entities or saves.

## Workload and report

The script checks integer base detection, end pointers, overflow saturation,
decimal-prefix parsing and a direct binary32 rounding boundary. It then verifies
32 yielding geometry samples, libm domain/range errors, reusable variadic
formatting, truncation and rejected `%n` behavior. It completes naturally and
retains no owned heap buffer.

`settings/library-services-example/result.txt` records:

| Field | Expected passing value |
| --- | --- |
| `failed` | `0` |
| `integer_verified`, `decimal_verified`, `errno_verified` | `1` |
| `geometry_verified`, `math_errors_verified` | `1` |
| `format_verified`, `truncation_verified`, `rejection_verified` | `1` |
| `owned_allocations` | `0` |
| `formatted` on run 1 | `run=1 parsed=2699 distance=13.00` |

Loading again increments the run fields without retaining prior temporary state.
There is no visible overlay. The report's allocation count is an owner snapshot,
not process peak RAM or a stack-use measurement.

## Deferred hardware sequence

When the next combined hardware batch resumes, load **library-services** twice,
allowing natural completion each time, then close GTA. Retain the result and
loader report for exact artifact and content verification. The final report
should have `run=2` and the corresponding formatted run number. No long gameplay
soak or repeated batch 4 sequence is required for these normal-operation checks.

This is a prepared source example; its hardware run remains deferred while
implementation continues. See the [parsing contract](../../docs/reference/NUMERIC-PARSING.md),
[math contract](../../docs/reference/MATH-PROFILE.md) and
[verification record](../../docs/verification/NUMERIC-PARSING1.md) for scope.
