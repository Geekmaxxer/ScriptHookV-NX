# Verification records

Verification records identify the exact implementation, artifact and test scope.
Compilation, structural checks, local ARM64 execution and physical Switch gameplay
are separate stages. Historical records remain immutable.

## Current runtime candidates

| Capability | Implementation and local evidence | Physical acceptance |
| --- | --- | --- |
| Large module configuration | [Capacity candidate](LARGE-MODULES1.md) | Pending |
| Owner-accounted allocation | [Memory service](PLUGIN-MEMORY1.md) | Probe allocation/free/Trim accepted; stress pending |
| C subset and XML DOM | [Library profile](PLUGIN-LIBRARIES1.md) | Combined XML workload accepted; broader library cases pending |
| Decimal conversion and XML floats | [Numeric profile](PLUGIN-NUMERIC1.md) | 128-record XML roundtrip accepted; remaining cases pending |
| Nothrow objects and fallible containers | [C++ profile](PLUGIN-CXX1.md) | Managed string/vector accepted; broader cases pending |
| Recoverable binary/XML records | [Data storage](RECOVERABLE-DATA1.md) | Pending |
| Chunked files and nested directories | [File services](PLUGIN-FILES1.md) | 256 KiB probe accepted; nested paths/media failures pending |
| Paged file/directory catalogs | [Directory catalogs](PLUGIN-DIRECTORY1.md) | Basic catalog accepted; multiple pages/errors pending |
| File mutation and recoverable publication | [Publication workflow](PLUGIN-PUBLICATION1.md) | Sentinel rename/delete accepted; recoverable publication pending |
| Registered cleanup and managed ownership | [Owned-resource lifecycle](PLUGIN-RESOURCES1.md) | Combined final-run cleanup accepted; nested/failure cases pending |
| Active profile and growing runtime capacity | [Platform/capacity workflow](PLATFORM-CAPACITY1.md) | Startup and one profile verified; capacity acceptance pending |
| Combined data, ownership, worker and render workload | [Instrumented probe](COMBINED-ACCEPTANCE1.md) | Scoped workload and final run-2 cleanup accepted |

The 2026-10-30 [physical Test1 run](PLATFORM-CAPACITY1.md#physical-test1-demo-run)
verifies exact installed artifacts, successful startup, manual demo loading, one
matched selected-account nickname and an initial managed run. The earlier DLC
startup failure is retained separately in that investigation. The later
[combined probe result](COMBINED-ACCEPTANCE1.md#physical-test1-result---2026-10-30)
accepts a specific workload, restart and final-run ownership cleanup. That result
supports only the cases listed above. Additional profiles, capacity growth,
broader transitions and measured performance/soak acceptance remain open.

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
