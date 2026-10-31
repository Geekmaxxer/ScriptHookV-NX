# C-style numeric parsing

The C profile supplies `strtol`, `strtoul`, `strtoll`, `strtoull`, `atoi`, `atol`
and `atoll`. The numeric profile additionally supplies decimal-prefix `strtod`,
`strtof` and `atof`. These functions support common source-port configuration
paths without linking hosted libc, allocating memory or changing the game runtime.
Include `shv/crt.h` for freestanding declarations and `<errno.h>` for error access.

```cpp
char* end = nullptr;
errno = 0;
const long value = strtol(text, &end, 0);
if (end == text || *end != '\0' || errno == ERANGE || errno == EINVAL) return;
// Apply the application's range and semantic checks before consuming value.
```

Both profiles require `c_runtime=true`; floating parsing additionally requires
`numeric_runtime=true`. Math support is independent. Compiler-hosted headers may
expose the equivalent standard declarations, but the SDK's global C declarations
remain the freestanding entry points. Hosted containers and throwing string
conversion APIs are separate compatibility work.

## Integer conversions

| Function | Result on this AArch64 LP64 target |
| --- | --- |
| `strtol`, `strtoll` | Signed 64-bit value |
| `strtoul`, `strtoull` | Unsigned 64-bit value |
| `atoi` | Signed `int` for representable input |
| `atol`, `atoll` | Signed `long`/`long long` convenience conversions |

The checked functions skip leading ASCII whitespace, accept one optional sign,
and consume the longest digit sequence for base 2–36. Base zero selects decimal,
leading-zero octal or a hexadecimal `0x` prefix followed by a valid digit. An
explicit base 16 accepts that same prefix. Upper/lower ASCII digits are equivalent.
C23 binary-prefix syntax, locale digits and wide strings are outside this profile.

When `end` is supplied, it receives the first unconsumed byte. No digits returns
zero and leaves it at the original input, including any whitespace/sign. A null
end pointer discards this result. Invalid bases return zero, leave end at input
and set `errno=EINVAL`.

Signed overflow saturates at the corresponding minimum/maximum and sets ERANGE.
Unsigned overflow saturates at the maximum, including an overflowing negative
magnitude. A representable unsigned magnitude with a minus sign is negated in
the unsigned result type. Parsing continues through every valid digit after
overflow so end still identifies the complete consumed token. In-range success
and an ordinary no-conversion result preserve the previous errno value.

The `atoi` family uses decimal parsing without an end pointer. Use checked
functions for imported or potentially out-of-range values; narrowing an
out-of-range `atoi` result is not a portable error-handling policy.

## Decimal floating conversions

`strtod` and `strtof` parse the longest decimal prefix after leading ASCII
whitespace and one optional sign. Fractional and exponent notation are supported.
An incomplete exponent is left unconsumed. Infinity/NaN spellings and supported
NaN payload syntax follow the pinned fast_float parser; payload bits are not a
preserved-data contract. `strtof` rounds directly to binary32 rather than parsing
through a binary64 intermediate. `atof` delegates to the same double parser.

No conversion returns positive zero and leaves end at the original input.
Parser-reported overflow/underflow sets ERANGE and retains the consumed prefix
length. Correctly representable subnormal values are accepted without promising
an errno report for every subnormal result. Successful conversions preserve
prior errno. Infinity and NaN literals are values, not automatic range errors.

This is a decimal subset, not a complete C floating grammar. Hexadecimal floating
syntax is unsupported: `strtod("0x1p2", &end)` consumes the initial decimal zero
and stops at `x`. Do not infer full-token validity from a numeric return value.
Locale separators, long-double parsing and scanf are unavailable. Use the
existing [strict numeric API](../verification/PLUGIN-NUMERIC1.md#strict-numeric-api)
when whole-token validity, a 4,096-byte bound, explicit statuses and finite-only
output are required. The pinned XML bridge retains its original semantics.

## Error state and execution

One resident NRO has one errno value, shared by its cooperative entries and its
optional libm calls. Other NROs have separate state. The common C parser now owns
the hook previously provided only by the math profile. Set errno to zero before
an operation when detecting a new error. State can persist across Kill/Load;
this is not OS-thread-local storage or a general reentrant C runtime.

Callers must supply valid NUL-terminated readable input and a writable end-pointer
slot when present. These functions do not validate arbitrary memory. They scan
input synchronously, do not yield and do not allocate. Very long input still costs
time; decimal parsing scans the remaining string to establish its explicit span.
The strict numeric API is preferable when a caller needs a bounded input contract.

The [combined example](../../examples/library-services/README.md) merges parsing,
math and formatter checks for a later hardware batch. The
[verification record](../verification/NUMERIC-PARSING1.md) identifies actual local
coverage and remaining physical requirements. The upstream
[C conversion reference](https://sourceware.org/newlib/libc.html#strtol) provides
general API context; the explicit subset above defines this SDK's behavior.
