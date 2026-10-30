# Project roadmap

This roadmap tracks ScriptHookV-NX, its SDK and delivery tools. The
[runtime roadmap](PC-RUNTIME-ROADMAP.md) contains the active substantial-plugin
workstream. Updated 2026-10-29.

## Progress conventions

A checked item means the stated implementation or acceptance step has evidence.
An unchecked item remains open. Local checks establish only their documented
scope; hardware acceptance belongs to the exact tested artifact and configuration.

## Runtime foundation

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
- [x] Record [combined extension gameplay acceptance](../evidence/sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json).
- [x] Verify configurable large-module image/BSS caps locally.
- [x] Verify owner-accounted allocation, alignment, realloc and Trim locally.
- [x] Verify C allocation/string and pinned XML DOM libraries locally.
- [x] Verify decimal conversion and typed XML floating data locally.
- [x] Verify nothrow objects and fallible vector/string/map workflows locally.
- [x] Verify recoverable binary/XML storage and modeled recovery failures locally.
- [ ] Accept actual storage interruption/recovery on Switch.
- [x] Provide bounded chunked I/O and isolated nested data directories locally.
- [x] Verify a cooperative 256 KiB sample and interruption/restart behavior locally.
- [x] Verify paged directory enumeration, complete metadata and bounded failure behavior locally.
- [x] Verify isolated file mutations, backup-preserving publication and explicit recovery locally.
- [ ] Define broader initialization, cancellation and exception policies.
- [ ] Validate a substantial combined workload on the supported Switch.

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
