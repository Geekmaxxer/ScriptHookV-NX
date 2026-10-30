# Compatibility

ScriptHookV-NX supports native C++ source ports for one audited GTA V Switch
executable. Compatibility is defined by target identity, versioned services and
verified behavior; a PC update number or matching native name is insufficient.

## Supported target

| Property | Value |
| --- | --- |
| Title | `0100b00b51230000` |
| Executable build | `6e3e9b3af5746e74b615bd981080c9271bc22662` |
| Plugin/caller/runtime ABI | 1 |
| Plugin architecture | AArch64 NRO, freestanding C++17 |
| Default capacity | 8 resident modules, 16 cooperative scripts, 8 MiB accounting |
| Script stack | 64 KiB per scheduled entry |

Changed executables require renewed audits. [Target profiles](../../targets/README.md)
retain fingerprints and runtime references. Capacity and service availability
must be checked against the installed artifact, not inferred from a packaging version.

## Accepted baseline

The published runtime provides typed/raw invocation, cooperative execution,
startup policies, isolated settings and bounded cleanup. Optional extensions add
entity pools, borrowed pointers, additional scripts, PNG textures and input/render
callbacks. [Baseline acceptance](../../evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json)
and [extension acceptance](../../evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json)
record their exact gameplay scope.

Resident code remains mapped until GTA exits, including after script completion
or Kill. This protects suspended execution and cached native handlers. Close GTA
before replacing code. The 2 ms cooperative dispatch budget cannot interrupt a
plugin that does not yield.

## Unreleased services

| Service | Current verification | Remaining requirement |
| --- | --- | --- |
| Configurable image/BSS caps | [Large-module local tests](../verification/LARGE-MODULES1.md) | Hardware capacity/performance |
| Owner-accounted heap | [Allocation/lifetime tests](../verification/PLUGIN-MEMORY1.md) | Horizon allocation/fragmentation |
| C/XML profile | [Compiled library tests](../verification/PLUGIN-LIBRARIES1.md) | Hardware workload acceptance |
| Decimal numeric profile | [IEEE/format tests](../verification/PLUGIN-NUMERIC1.md) | Target performance/stack measurements |
| Nothrow C++ and containers | [Object/container tests](../verification/PLUGIN-CXX1.md) | Broader port and hardware acceptance |
| Recoverable data | [Record/recovery tests](../verification/RECOVERABLE-DATA1.md) | Actual media interruption/durability |
| Bounded file services | [Chunk/path/lifecycle tests](../verification/PLUGIN-FILES1.md) | SD behavior and measured frame cost |
| Paged directory catalogs | [Metadata/page/ownership tests](../verification/PLUGIN-DIRECTORY1.md) | Catalog cost and transition acceptance |
| File mutation/publication | [State/recovery/failure tests](../verification/PLUGIN-PUBLICATION1.md) | Actual SD behavior and interruption acceptance |
| Owned-resource lifecycle | [Pinning/cancellation tests](../verification/PLUGIN-RESOURCES1.md) | Combined ownership and transition acceptance |

Candidates preserve ABI 1 layouts but require their optional services. Older
runtimes may not provide those tables. New SDK functionality and examples do not
inherit previous hardware acceptance automatically.

## Source-port requirements

Audit hashes, native signatures, output layouts, global indices and borrowed
pointers against the supported target. Check optional native availability and
post-call status; handler presence does not establish equivalent semantics.
Allocate within measured budgets, yield regularly and define explicit cleanup.
A matching filename or successful compile does not establish a playable port.

Windows ASI/DLL binaries, Win32/DirectX services, arbitrary PC offsets/hooks,
ScriptHookVDotNet, general hosted C++ libraries and direct NSC loading from SD are
outside the current contract. New assets and content require separate preparation
and gameplay verification. The intended PC 2699/1.61 baseline is not a Switch ABI.

## Acceptance requirements

Separate source compilation, target verification, packaging checks, local ARM64
execution and physical gameplay. Record exact installed hashes, configurations,
coexisting mods, transitions and failure conditions. Measured peak RAM, frame time
and long-session stability remain explicit hardware work.

The [project roadmap](../COMPLETION-ROADMAP.md) and
[runtime roadmap](../PC-RUNTIME-ROADMAP.md) track open implementation and acceptance
gates. [Porting](../guides/PORTING.md) describes the supported source workflow.
