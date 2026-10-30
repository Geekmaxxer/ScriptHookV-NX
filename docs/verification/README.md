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
| Active profile and growing runtime capacity | [Platform/capacity workflow](PLATFORM-CAPACITY1.md) | Pending; next integration gate |

The 2026-10-30 [console observation](PLATFORM-CAPACITY1.md#console-observations---2026-10-30)
records a physical startup failure in the Atmosphere RomFS metadata builder and
scoped Eden demo results. The retained accepted runtime reproduces the same
normalized fatal call chain. Later failures also occur with the SD `subsdk4` and
`subsdk5` files absent. Console launch recovery was subsequently reported after
correcting the missing-DLC placeholder placement. The precise assertion and active
package identity remain unverified. Physical acceptance of Test1 still requires
the runtime and required menu module to be restored and tested.

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
