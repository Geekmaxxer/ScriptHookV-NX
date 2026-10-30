# Verification records

Verification records identify the exact implementation, artifact and test scope.
Compilation, structural checks, local ARM64 execution and physical Switch gameplay
are separate stages. Historical records remain immutable.

## Current runtime candidates

| Capability | Implementation and local evidence | Physical acceptance |
| --- | --- | --- |
| Large module configuration | [Capacity candidate](LARGE-MODULES1.md) | Pending |
| Owner-accounted allocation | [Memory service](PLUGIN-MEMORY1.md) | Pending |
| C subset and XML DOM | [Library profile](PLUGIN-LIBRARIES1.md) | Pending |
| Decimal conversion and XML floats | [Numeric profile](PLUGIN-NUMERIC1.md) | Pending |
| Nothrow objects and fallible containers | [C++ profile](PLUGIN-CXX1.md) | Pending |
| Recoverable binary/XML records | [Data storage](RECOVERABLE-DATA1.md) | Pending |
| Chunked files and nested directories | [File services](PLUGIN-FILES1.md) | Pending |
| Paged file/directory catalogs | [Directory catalogs](PLUGIN-DIRECTORY1.md) | Pending |
| File mutation and recoverable publication | [Publication workflow](PLUGIN-PUBLICATION1.md) | Pending |
| Registered cleanup and managed ownership | [Owned-resource lifecycle](PLUGIN-RESOURCES1.md) | Pending |

## Accepted baseline

- [Runtime completion record](RUNTIME-COMPLETION.md)
- [Matched console acceptance](../../evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json)
- [SDK extension gameplay acceptance](../../evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json)

## Tooling reviews

- [Quality audit](QUALITY-AUDIT.md)
- [Automation audit](AUTOMATION-AUDIT.md)

Local checks do not transfer acceptance to modified runtimes, new assets or
larger configurations. The [runtime roadmap](../PC-RUNTIME-ROADMAP.md) lists open gates.
