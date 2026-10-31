# Buffer formatting

The C profile provides `snprintf` and `vsnprintf` for labels, diagnostics and
source-port configuration text. Include `shv/crt.h` and enable `c_runtime`.
The numeric profile adds decimal floating conversions. Both functions compile
into the plugin and require no new runtime service, game binding or engine offset.

```cpp
#include "shv/crt.h"

char label[64];
const int required = snprintf(label, sizeof(label), "vehicle=%u speed=%+.2f", 42u, 12.5);
if (required < 0) return;                         // Unsupported format or overflow.
if (size_t(required) >= sizeof(label)) return;   // Output was truncated.
```

This example requires `c_runtime=true` and `numeric_runtime=true`. Integer,
character and string formatting needs only the C profile. Standard `<cstdio>`
declarations and qualified `std::snprintf` calls can refer to the same functions;
other hosted stdio operations remain unavailable.

## Supported conversions

| Conversion | Behavior |
| --- | --- |
| `%d`, `%i` | Signed decimal integers |
| `%u`, `%o`, `%x`, `%X` | Unsigned decimal, octal and hexadecimal integers |
| `%c`, `%s` | A promoted byte character or a NUL-terminated byte string |
| `%p` | Lowercase hexadecimal with a `0x` prefix, including `0x0` for null |
| `%%` | One literal percent character |
| `%f`, `%F`, `%e`, `%E`, `%g`, `%G` | Decimal binary64 formatting through the numeric profile |

Integer length modifiers are `hh`, `h`, `l`, `ll`, `j`, `z` and `t`. The target
uses the AArch64 LP64 data model; supply the matching argument type rather than
assuming Windows `long` widths. Floating arguments are promoted to `double`;
`l` is accepted for these conversions. Wide characters/strings, long double,
hexadecimal floats, positional arguments and `%n` are unsupported.

Flags `-`, `+`, space, `#` and `0`, decimal width/precision and `*` width/precision
are supported where applicable to each conversion. A negative width selects left
alignment; a negative dynamic precision is omitted. Integer precision suppresses
zero field padding. Floating precision defaults to six and is limited to 64
digits; `%g` precision zero selects one significant digit. Alternate form retains
the decimal point and significant trailing zeros. Numeric conversion reuses the
existing pinned Ryu dependency. Infinity/NaN and letter case are explicit; their
field padding uses spaces. The decimal separator is always `.`.

## Results and failure behavior

The return value is the byte count that would have been written, excluding the
terminating NUL. A nonzero capacity reserves one byte for that terminator. A
truncated result remains a complete NUL-terminated prefix; compare the return
value with capacity before using it. `%c` may produce an embedded NUL, so the
reported byte count can differ from `strlen`.

`snprintf(nullptr, 0, ...)` performs a counting pass. `vsnprintf` copies its
incoming `va_list` internally; the caller may reuse that list for a second pass
before calling `va_end`. Each separate call evaluates the current argument data.

Unsupported formats, a null format, floating use without the numeric profile,
floating precision above 64, field/count overflow beyond `INT_MAX` and `INT_MIN`
dynamic width return `-1`. When output is non-null and capacity is nonzero,
failure clears its first byte. A null output with nonzero capacity returns `-1`
without writing. `%n` is rejected before accessing its pointer. No errno state
is supplied. Earlier bytes beyond a cleared terminator need not be erased.

Callers must provide readable format/argument storage, a writable buffer of the
declared capacity and correct variadic types. Overlapping inputs/outputs and
invalid pointers are outside the contract. These functions do not validate game
memory, convert Unicode or make untrusted format strings safe.

## Resource and compatibility limits

Formatting allocates no heap memory and retains no mutable global state. Scratch
storage is local to the call. Truncated padding is counted without iterating over
discarded bytes. Parsing, string scans and decimal conversion remain synchronous;
there is no implicit yield or guaranteed frame-time deadline. Compiler frame
reports are not whole-call-stack or physical timing measurements.

The profile supplies buffer formatting, not `printf`, file streams, `sprintf`,
scanf, locale or a hosted C library. XML floating setters retain their separate
numeric-profile requirement: using the C-only formatter does not enable them.
The [example](../../examples/formatted-text/README.md) and
[verification record](../verification/FORMATTED-TEXT1.md) identify local coverage.
Physical acceptance of this new code remains pending for a later combined batch.
