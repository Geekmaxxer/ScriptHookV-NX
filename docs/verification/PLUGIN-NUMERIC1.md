# Plugin Numeric Test 1

Date: 2026-10-29. Unreleased SDK candidate extending
[Plugin Libraries Test 1](PLUGIN-LIBRARIES1.md). The owner-heap core is reused
unchanged; no new core build, installation or public release is performed.

This profile adds decimal float/double parsing and formatting, including typed
pugixml attribute/text access. It supports coordinates, rotations and other
numeric XML data without a hosted C/C++ runtime. General CRT/STL, constructor
lifecycle and streaming files remain separate roadmap work.

The subsequent [C++ profile](PLUGIN-CXX1.md) adds explicit nothrow objects and
fallible containers. It can be combined with this numeric profile without adding
hosted STL, global initialization or exception support.

## Build and runtime requirements

```powershell
python tools/build_plugin.py --source path/to/main.cpp --c-runtime --numeric-runtime --output out/numeric-mod --devkitpro C:/devkitPro
python tools/build_mod.py --project examples/numeric-xml/project.json --output out/numeric-example --devkitpro C:/devkitPro
```

NRO project options are `"c_runtime": true` and `"numeric_runtime": true`.
Both must be JSON booleans. Numeric support requires the C profile and is invalid
for an NSC project. The numeric option defaults to false, preserving the earlier
text/integer profile and its explicit unsupported-import checks.

The plugin-memory candidate is required. Its independent memory table is bound
before initialization; an absent service refuses initialization before entry.
Base runtime/plugin/extension ABI 1 remains unchanged. The accepted public core
does not supply this candidate heap service. A shared ABI number alone is not
proof that an optional service is present.

`plugin-build.json` records `numeric_runtime_enabled`, dependency/source hashes,
required memory support and compiler stack-frame reports. It records the largest
individual frame, not a complete call-chain bound or hardware high-water value.

## Strict numeric API

Include `shv/numeric.h`. Parsing takes a pointer, explicit byte length excluding
any terminating NUL, and an output reference. Formatting takes a value, output
buffer/capacity and byte-count reference.

```cpp
double coordinate = 0;
auto status = ScriptHook::Numeric::ParseDouble(text, textBytes, coordinate);
if (status == ScriptHook::Numeric::Status::Ok) {
    char output[32]; size_t bytes = 0;
    status = ScriptHook::Numeric::FormatDouble(coordinate, output, sizeof(output), bytes);
    // Consume output only when status is Ok. bytes excludes the trailing NUL.
}
```

`ParseFloat` and `ParseDouble` accept a complete decimal token, one sign,
decimal/exponent notation and surrounding ASCII whitespace. Input is limited to
4,096 bytes. Embedded NUL, suffixes, locale commas, hexadecimal and incomplete
numbers fail. Non-finite values and overflow/underflow are reported distinctly.
Zero, including negative zero, is supported. Every failure preserves the original
output value. Correctly representable subnormal values are accepted.

`FormatFloat` and `FormatDouble` produce roundtripping scientific decimal text
with the shortest significant digits. Output is locale-independent ASCII and
includes a terminating NUL. Finite values, including negative zero, are supported.
Failure leaves output storage unchanged and sets the byte count to zero. Thirty-two
bytes suffice for the supplied binary32/binary64 shortest formatter.

Status values are `Ok`, `Invalid`, `OutOfRange`, `NonFinite`, `BufferTooSmall` and
`InputTooLong`. These functions allocate no heap memory, throw no exceptions and
provide no `errno` or locale service. Callers must still supply valid readable
input spans and writable output storage.

## XML floating data

With both build options enabled, include `shv/xml.h` before upstream headers.
`as_float`, `as_double`, floating `set_value` and floating text `set` are available
on the pinned pugixml memory DOM. Defaults use the library's 9/17 significant-digit
float/double precision. Ordinary default-precision finite values roundtrip.

XML getters retain a permissive decimal-prefix interface: whitespace and a sign
are allowed, numeric suffixes are not validated, invalid input produces zero,
and infinity/NaN/range results are possible. Hexadecimal parsing is outside this
decimal profile. The float getter parses as double before the target float cast,
as in the pinned library's fallback path; that cast uses the current FP control.
Use the strict API when importing coordinates that require full-token validity,
finite/range checks or direct binary32 rounding.

Explicit XML precision follows `%g` formatting rules, including rounding and
exponent selection. Extremely large precision is bounded internally after all
binary64 significant digits are covered. The upstream typed setter retains its
128-byte intermediate buffer: large custom precision can truncate text before
storage. Prefer default precision, or call `FormatFloat`/`FormatDouble`, check its
status, then assign the completed string explicitly. Memory serialization still
requires checking `Xml::Save` for output overflow.

The bridge substitutes only the pinned implementation's numeric calls while its
header is included, then removes those macros. C headers are included before the
substitution. Upstream pugixml files remain unmodified. Generic C `strtod`,
printf/scanf, stdio file methods, streams and XPath remain unsupported; unprovided
imports still fail strict linking. The later [buffer formatter](FORMATTED-TEXT1.md)
adds a specified snprintf/vsnprintf subset. The original numeric evidence predates
that addition and retains its original compiled scope.

## Dependencies and execution

- [fast_float](https://github.com/fastfloat/fast_float) 8.3.2, commit
  `487ef00acfc33cf8b3167938457958299d1276c0`: decimal parsing, selected MIT license.
- [Ryu](https://github.com/ulfjack/ryu), commit
  `4c0618b0e44f7ef027ebae05d2cc7812048f7c8f`: shortest and precision formatting,
  selected Boost Software License 1.0.

Both retain unmodified selected upstream files, provenance/hashes and all upstream
license alternatives under `sdk/third_party/`. Required notices accompany the
source SDK. Include applicable notices with redistributed mods.

GCC 16 gates fast_float's `<system_error>` error-code declarations behind hosted
header mode. Only `plugin_numeric.cpp` parses those compiler headers with
`-fno-freestanding -fno-builtin`; plugin source and the remaining shim stay
freestanding. No hosted library is linked. `-nostdlib`, strict unresolved-import
rejection and constructor checks remain enabled. Library assertions are compiled
out in the selected release units. No downloaded build scripts are executed.

Pure numeric conversion needs no heap pages. XML allocation/destruction continues
through the owner heap. Complete operations before yielding, or keep explicit
module-owned state with a cleanup policy: Kill does not unwind suspended C++
objects. Large synchronous conversions/DOM operations still require measured
frame-time limits on Switch.

## Verification and acceptance

2,689 numeric checks execute actual ARM64 NRO/library code, including 308 binary64
and 306 binary32 samples, exact halfway cases, subnormals, 924 independent general-
format comparisons, malformed/range input, output guards, huge precision, two
owners, allocation failure, restart, missing-service refusal and an isolated
copied SDK in a path with spaces. Subnormal parsing is checked under several FP
rounding/flush modes without changing the control register. Python decimal
parsing/formatting, exact decimal midpoints and raw IEEE bits provide independent
oracles; this is not exhaustive coverage of every IEEE bit pattern.

Both cooperative main functions execute hard parsing, large-precision formatting,
XML allocation and cleanup on 64 KiB script stacks. The largest compiler-reported
frame is 3,536 bytes; whole-stack high-water remains unmeasured. The earlier
62-check C/XML profile and 32 portable-tool checks are rerun separately. The
[buildable example](../../examples/numeric-xml/README.md) also executes its complete
DOM/settings/cleanup workflow with modeled filesystem services.

Retained source, licenses, artifacts' hashes and reports:
[`verification.json`](../../evidence/plugin-numeric1-local-20261029/verification.json).
OS allocation, filesystem and RO are modeled. No game binaries, raw target
disassembly, databases or installable modules are included in retained evidence.
Physical acceptance still requires actual Horizon/RO operation, coexistence,
cleanup/restart, low-memory behavior, transitions and measured RAM/frame-time/
long-session results. No Menyoo port or new game-native compatibility is certified.
