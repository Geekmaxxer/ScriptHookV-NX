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

| Service | Recorded scope | Remaining requirement |
| --- | --- | --- |
| Configurable image/BSS caps | [Explicit/Automatic bounded workload accepted](../verification/CAPACITY-ACCEPTANCE1.md) | Full-capacity RAM, performance and long sessions |
| Owner-accounted heap | [Allocation/cleanup workloads accepted](../verification/PLUGIN-MEMORY1.md) | Broader Horizon allocation/fragmentation |
| C/XML profile | [Combined XML workload accepted](../verification/CATALOG-SCAN1.md) | Broader library and failure cases |
| Decimal numeric profile | [IEEE/format tests and bounded XML acceptance](../verification/PLUGIN-NUMERIC1.md) | Target performance/stack measurements |
| Buffer formatting | [C/numeric formatter verified locally](../verification/FORMATTED-TEXT1.md) | New formatter workload on hardware |
| Nothrow C++ and containers | [Managed string/vector workloads accepted](../verification/PLUGIN-CXX1.md) | Broader source-port and ownership cases |
| Recoverable records | [Record/recovery tests](../verification/RECOVERABLE-DATA1.md) | Actual media interruption/durability |
| Bounded files and preset catalogs | [Large XML and four-page preset selection accepted](../verification/CATALOG-SCAN1.md) | Media errors, concurrent changes, traversal cost and transitions |
| File mutation/publication | [Normal publication and explicit restoration accepted](../verification/PLUGIN-PUBLICATION1.md) | Interrupted mutations, actual SD failures and durability |
| Owned-resource lifecycle | [Managed cancellation and nested cleanup accepted](../verification/PLUGIN-RESOURCES1.md) | Broader failures and transition acceptance |
| Active profile and growing capacity | [One selected nickname and bounded growth accepted](../verification/PLATFORM-CAPACITY1.md) | Second selected profile, broader capacity and combinations; Eden remains separate |

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
