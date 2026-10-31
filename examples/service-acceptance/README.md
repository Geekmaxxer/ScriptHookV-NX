# Service acceptance batch 5

This batch merges the pending math and formatter checks with numeric parsing in
one **library-services** NRO. Its hardware run remains deferred while implementation
continues. The sequence below is retained for the next consolidated acceptance
session. No standalone formatter/math run or repetition of accepted batch 4 is
required. Preserve the working Test1 runtime, configuration, game files and saves.

## One combined sequence

1. With GTA closed, copy `INSTALL/switch` to the SD root. The NRO is manual;
   no runtime or capacity-profile replacement is included.
2. Start GTA and select Load Now for **library-services**. Allow natural completion;
   the script yields between 32 geometry samples and has no visible overlay.
3. Load **library-services** once more and allow completion, then close GTA.
4. Retain `loader-report.txt` and `settings/library-services-example/result.txt`.
   The final report should contain:

   - `run=2`, `failed=0`, `owned_allocations=0`.
   - `integer_verified=1`, `decimal_verified=1`, `errno_verified=1`.
   - `geometry_verified=1`, `math_errors_verified=1`.
   - `format_verified=1`, `truncation_verified=1`, `rejection_verified=1`.
   - `formatted=run=2 parsed=2699 distance=13.00`.

The probe checks base detection, consumed prefixes, overflow, direct float
rounding, math errors, formatted output and variadic-list reuse. It allocates
no owner heap buffer and changes no game entity or save data. A result is still
reviewed against exact installed artifact hashes and the loader snapshot.

## Contents and scope

`manifest.json` identifies the required existing runtime, NRO extents and selected
math archive. The kit contains no runtime module, NPDM, game archive, shared
`runtime.ini` or automatic installer. Newlib notices are under `third-party/newlib/`.
The original batch 4 ZIP and its physical acceptance record remain immutable;
earlier accepted probes do not need reinstalling for this batch.

Physical timing, whole-process RAM, stack use, alternate floating-point state,
media failures and broader transitions remain separate acceptance requirements.
Repeated natural completion does not test power-loss durability or arbitrary
thread behavior. Preserve failed results for inspection rather than deleting
them to obtain a passing report.

See the [consolidated verification record](../../docs/verification/NUMERIC-PARSING1.md)
and [accepted batch 4 scope](../../docs/verification/CATALOG-SCAN1.md).
