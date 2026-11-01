# Verification records

Verification records identify the exact implementation, artifact and test scope.
Compilation, structural checks, local ARM64 execution and physical Switch gameplay
are separate stages. Historical records remain immutable.

## Current runtime candidates

| Capability | Implementation and local evidence | Physical acceptance |
| --- | --- | --- |
| Growing plugin catalogue and completed rescans | [Catalogue revision](PLUGIN-CATALOGUE1.md) | Local verification; matched MEGATARD v1.1 pair pending |
| Large module configuration | [Capacity candidate](LARGE-MODULES1.md) | Explicit large image/BSS accepted; broader configurations pending |
| Owner-accounted allocation | [Memory service](PLUGIN-MEMORY1.md) | Probe allocation/free/Trim accepted; stress pending |
| C subset and XML DOM | [Library profile](PLUGIN-LIBRARIES1.md) | Combined XML workload accepted; broader library cases pending |
| Decimal conversion and XML floats | [Numeric profile](PLUGIN-NUMERIC1.md) | 128-record XML roundtrip accepted; remaining cases pending |
| Nothrow objects and fallible containers | [C++ profile](PLUGIN-CXX1.md) | Managed string/vector accepted; broader cases pending |
| Recoverable binary/XML records | [Data storage](RECOVERABLE-DATA1.md) | Pending |
| Chunked files and nested directories | [File services](PLUGIN-FILES1.md) | 256 KiB nested sample accepted; media failures pending |
| Paged file/directory catalogs | [Directory catalogs](PLUGIN-DIRECTORY1.md) | Nested metadata and four-page mixed preset traversal accepted; errors/changes pending |
| File mutation and recoverable publication | [Publication workflow](PLUGIN-PUBLICATION1.md) | Normal publication, verified replacement and explicit restoration accepted; errors pending |
| Registered cleanup and managed ownership | [Owned-resource lifecycle](PLUGIN-RESOURCES1.md) | Combined and two-child Explicit cleanup accepted; failure/transition cases pending |
| Active profile and growing runtime capacity | [Platform/capacity workflow](PLATFORM-CAPACITY1.md) | One profile and bounded Explicit/Automatic growth accepted; broader capacity pending |
| Combined data, ownership, worker and render workload | [Instrumented probe](COMBINED-ACCEPTANCE1.md) | Scoped workload and final run-2 cleanup accepted |
| Large extents, extra script stacks and automatic settings | [Controlled capacity sequence](CAPACITY-ACCEPTANCE1.md) | Defaults refusal, Explicit/Automatic growth/cleanup and post-restoration startup accepted |
| Operation timing and bounded run history | [Capacity diagnostics Test2](CAPACITY-DIAGNOSTICS2.md) | Both bounded lifetimes/cleanup accepted; synchronous report-save stalls identified |
| Explicit shared C++ session state | [SessionLocal lifecycle](SESSION-LOCAL1.md) | Worker progress, second-lifetime cleanup and coexistence accepted |
| Cooperative buffered reads and prepared output | [File-transfer helpers](FILE-TRANSFERS1.md) | Combined 256 KiB XML transfer accepted; failures/cancellation remain local |
| Growable XML serialization and streamed byte verification | [Combined XML workflow](XML-WORKFLOW1.md) | 98,376-byte output, publication/restoration and final run-2 cleanup accepted |
| Cooperative preset filtering, sorting and checked selection | [Preset catalog workflow](CATALOG-SCAN1.md) | 14 sorted entries, four pages, filter and selected XML accepted |
| Buffer formatting and numeric decimal fields | [Formatted text](FORMATTED-TEXT1.md) | Local ARM64 verification; new workload pending a later combined batch |
| Float/double math and per-plugin errors | [Math profile](MATH-PROFILE1.md) | Local ARM64/reference checks; consolidated library workload deferred |
| C integer and decimal-prefix parsing | [Numeric parsing](NUMERIC-PARSING1.md) | Local end/range/reference checks; consolidated library workload deferred |

The 2026-10-30 [physical Test1 run](PLATFORM-CAPACITY1.md#physical-test1-demo-run)
verifies exact installed artifacts, successful startup, manual demo loading, one
matched selected-account nickname and an initial managed run. The earlier DLC
startup failure is retained separately in that investigation. The later
[combined probe result](COMBINED-ACCEPTANCE1.md#physical-test1-result---2026-10-30)
accepts a specific workload, restart and final-run ownership cleanup. That result
supports only the cases listed above. The later Explicit capacity run adds large
extents, stack/metadata growth and nested owner cleanup. Additional profiles,
resident-module growth, broader transitions and measured
performance/soak acceptance remain open. Automatic also accepts the bounded workload.
Test2 measures report-save stalls up to 122.480 ms while other measured regions
remain below 0.11 ms. These account for reproduced dispatch-stall magnitudes;
individual pairing with the earlier 78.6/96.4 ms maxima remains unavailable.
Original configuration bytes match the backup, and fresh startup under the original
limits is verified. Loading the oversized probe then produces the expected `0x10023` refusal.
The separate Test3 source reduces reporting frequency. Service batch 1 accepts
shared state, normal publication and nested catalog access. The later
[batch 4 result](CATALOG-SCAN1.md#physical-batch-4-result---2026-10-30) accepts the
consolidated XML/save/recovery and multi-page preset workflow. Exact retained
contents and second-run cleanup were verified; full performance signoff remains open.

## Accepted baseline

- [Runtime completion record](RUNTIME-COMPLETION.md)
- [Matched console acceptance](../../evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json)
- [SDK extension gameplay acceptance](../../evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json)

## Tooling reviews

- [DLC startup diagnostics and recovery](DLC-RECOVERY1.md)
- [Quality audit](QUALITY-AUDIT.md)
- [Automation audit](AUTOMATION-AUDIT.md)

Local checks do not transfer acceptance to modified runtimes, new assets or
larger configurations. The [runtime roadmap](../PC-RUNTIME-ROADMAP.md) lists open gates.
