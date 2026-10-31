# Formatted-text verification

This SDK milestone adds buffer formatting for common native source-port text
paths. It uses the accepted Test1 runtime without changing runtime binaries,
optional-service tables or target bindings. New formatter code executes inside
each rebuilt plugin; earlier hardware acceptance does not transfer to that code.

## Implementation and local verification

- [x] Provide C-profile `snprintf` and `vsnprintf` for integers, strings and characters.
- [x] Support target-width integer modifiers, field flags and dynamic width/precision.
- [x] Preserve counting/truncation semantics, output guards and reusable `va_list` state.
- [x] Refuse unsupported formats and count overflow without publishing partial text.
- [x] Add numeric-profile decimal floating conversions using the pinned Ryu dependency.
- [x] Compare decimal results against an independent formatting reference, including extrema.
- [x] Exercise mixed variadic register/stack arguments and formatting after Kill/Load in ARM64.
- [x] Preserve the strict C-only XML floating-setter refusal.
- [x] Provide a complete naturally completing example with retained result diagnostics.
- [ ] Accept the new formatting workload on physical Switch hardware.

The [local record](../../evidence/formatted-text1-local-20261030/verification.json)
identifies source hashes, exact artifacts, regressions and executed checks.
Production runtime, SDK formatter and example ARM64 instructions execute with
modeled OS, RO, filesystem, game boundaries and outer scheduling. Tests guard
both sides of every output region and verify no owned heap allocations remain.

## Compatibility scope

The [contract](../reference/FORMATTED-TEXT.md) defines the supported subset and
explicit failures. This milestone does not add hosted streams, exceptions,
automatic constructors, long-double formatting or a general standard library.
The XML adapter keeps its numeric bridge separate from the general formatter;
integer-only XML modules still reject floating setters at strict linking.

The [standalone example](../../examples/formatted-text/README.md) remains available.
Its checks now join math and numeric parsing in
[library-services](../../examples/library-services/README.md), with one deferred
hardware workload covering all three additions. No repeat of accepted batch 4
is required. Its archive and acceptance record remain immutable. No game, runtime or SD installation is
performed by this milestone.
