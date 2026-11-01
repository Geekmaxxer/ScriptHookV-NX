# Project roadmap

This roadmap tracks ScriptHookV-NX, its SDK and delivery tools. The
[runtime roadmap](PC-RUNTIME-ROADMAP.md) contains the active substantial-plugin
workstream. Updated 2026-11-01.

## Progress conventions

A checked item means the stated implementation or acceptance step has evidence.
An unchecked item remains open. Local checks establish only their documented
scope; hardware acceptance belongs to the exact tested artifact and configuration.

## Runtime foundation

- [x] Separate installed-plugin catalogue growth from resident admission and verify 160 entries locally.
- [x] Expose completed-scan status and preserve catalogue owners on failed rescans.
- [ ] Accept the revised v1.4.1 catalogue runtime with the matched MEGATARD v1.1 menu on hardware.

- [x] Audit the executable and enforce build fingerprints before hooks.
- [x] Implement typed/raw invocation, native output layouts and failure reporting.
- [x] Implement cooperative scripts, multiple entries and bounded stop callbacks.
- [x] Preserve resident code for cached handlers across Kill/Load.
- [x] Provide startup policies, diagnostics and isolated settings.
- [x] Record [baseline console acceptance](../evidence/runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json).
- [ ] Complete measured RAM, frame-time and extended-soak acceptance at full capacity.

## SDK and substantial source ports

- [x] Provide entity pools, borrowed pointers and bounded global access.
- [x] Provide controller/render callbacks, additional scripts and PNG textures.
- [x] Replace fixed texture cache counts with stable, budget-accounted pages and verify 160 records locally.
- [x] Provide an 80-image visual example and combine it with pending library checks in one candidate kit.
- [ ] Accept the growing-texture runtime and combined graphics/library workload on Switch.
- [x] Record [combined extension gameplay acceptance](../evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json).
- [x] Verify configurable large-module image/BSS caps locally.
- [x] Verify owner-accounted allocation, alignment, realloc and Trim locally.
- [x] Verify C allocation/string and pinned XML DOM libraries locally.
- [x] Verify decimal conversion and typed XML floating data locally.
- [x] Provide checked C-profile buffer formatting and numeric-profile decimal floats locally.
- [x] Provide opt-in float/double math and per-NRO error handling with local reference checks.
- [x] Combine the math and formatter workloads for one later hardware batch.
- [x] Provide integer/base and decimal-prefix parsing with end-pointer and error checks locally.
- [x] Consolidate pending parsing/math/formatter checks in one library-services workload.
- [x] Package and locally verify the consolidated manual service batch 5.
- [ ] Accept the consolidated library-services workload on physical Switch hardware.
- [x] Verify nothrow objects and fallible vector/string/map workflows locally.
- [x] Verify recoverable binary/XML storage and modeled recovery failures locally.
- [ ] Accept actual storage interruption/recovery on Switch.
- [x] Provide bounded chunked I/O and isolated nested data directories locally.
- [x] Verify a cooperative 256 KiB sample and interruption/restart behavior locally.
- [x] Verify paged directory enumeration, complete metadata and bounded failure behavior locally.
- [x] Verify isolated file mutations, backup-preserving publication and explicit recovery locally.
- [x] Verify registered resource pinning, reverse cleanup and managed cancellation locally.
- [x] Verify registered cleanup on rejected initialization and a yielding string/vector example locally.
- [x] Verify growing capacity tables, lazy stack admission and high-index ownership locally.
- [x] Audit and verify the optional active-profile nickname interface locally.
- [x] Build and locally verify a combined storage/XML, managed-lifetime and render/worker acceptance probe.
- [x] Prepare a large-module, 37-entry capacity test with preserved configuration and phase reports.
- [x] Accept default image refusal and the explicit large-module, 37-entry nested-cleanup workload on Switch.
- [x] Accept the bounded capacity workload under automatic settings on Switch.
- [x] Verify original configuration restoration and normal startup.
- [x] Verify restored configuration bytes and prepare a measured Test2 capacity probe locally.
- [x] Accept Test2 timing/workload results and identify synchronous report-save stalls.
- [x] Reduce diagnostic reporting overhead and provide shared C++ session state locally.
- [x] Package shared-state, file publication and catalog checks as one service batch.
- [x] Accept the shared-state, publication and nested catalog workload on Switch.
- [x] Provide cooperative owned read buffers and prepared-file write tasks with large XML parsing locally.
- [x] Add growable XML output and bounded file verification with a combined save/restore example locally.
- [x] Provide cooperative preset filtering/sorting and checked selection paths locally.
- [x] Merge multi-page preset selection with XML/save/recovery in service batch 4.
- [x] Accept the combined preset/XML load/save/restore workload on Switch.
- [x] Independently verify retained input/output, restored original, preset fixtures and final run-2 cleanup.
- [ ] Complete remaining platform/capacity acceptance for substantial runtime changes.
- [x] Accept the combined managed session's cancellation, restart and final cleanup on Switch.
- [ ] Accept broader nested ownership, failure and transition cleanup on Switch.
- [ ] Define broader global initialization, exception and static-local policies.
- [x] Validate the instrumented combined workload on the supported Switch.
- [ ] Complete measured performance, capacity growth and extended combined-workload acceptance.

## Mod compatibility and examples

- [x] Record Footprints and Disarm gameplay acceptance within their documented scope.
- [x] Provide tutorials, standalone process management and controller UI examples.
- [x] Provide combined numeric XML and container-registry projects.
- [ ] Accept experimental ports and controller UI on hardware.
- [ ] Audit a pinned substantial mod's engine/memory/asset dependencies.
- [ ] Select a compatible 2699/1.61 Menyoo revision after runtime acceptance gates.
- [ ] Implement and accept Menyoo against a published feature matrix.

## Desktop and assets

- [x] Implement source analysis, compiler checks and copied project workflows.
- [x] Implement verified NRO installation, backup/restore and report import.
- [x] Implement supported Legacy resources and DLC-list merging.
- [x] Implement loose-PED workflows, collections and metadata editing.
- [ ] Expand real-mod recipes and fresh-machine toolchain verification.
- [ ] Complete per-category asset gameplay checks, including collision and LODs.
- [ ] Investigate unsupported rigs and geometry simplification separately.

## Repository and maintenance

- [x] Provide a repeatable public verification command and pinned CI definitions.
- [x] Separate introductory, user, API, development and verification documentation.
- [x] Separate build tools from regressions/fixtures and group example mods.
- [x] Verify reorganized exports, isolated core builds and complete SDK example projects.
- [x] Add automated navigation/layout checks to prevent repository drift.
- [ ] Record hosted CI execution separately from local checks.

## Scope boundaries

Binary-only ASI translation, Windows/DirectX emulation, managed runtimes and
arbitrary future-game compatibility are separate research areas. Native names or
successful compilation do not establish equivalent game behavior.

See [compatibility](reference/COMPATIBILITY.md), [validation records](verification/README.md)
and [contribution requirements](development/CONTRIBUTING.md) before promoting a candidate.
