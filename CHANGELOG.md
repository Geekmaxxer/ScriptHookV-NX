# Changelog

## Unreleased platform profile and capacity - 2026-10-30

- Add a guarded, optional profile service for the single open Switch user's UTF-8 nickname, with explicit account-state and platform errors.
- Support automatic capacity and larger explicit settings across runtime.ini, project manifests and build/configuration tools.
- Grow stable catalog, scheduler, additional-entry and heap/resource owner metadata on demand; retain admitted stacks for reuse and report allocation failures.
- Verify larger cooperative workloads, high-index owner cancellation and the complete profile example locally. Further major runtime expansion is gated on physical acceptance of the combined candidate.

## Unreleased owned-resource lifecycle - 2026-10-30

- Add an independent optional resource table with owner-thread validation, exact heap contexts, pinned allocations and at-most-once cleanup.
- Drain registrations in reverse order after explicit stop, natural completion and rejected initialization; preserve existing ABI layouts and retained-code behavior.
- Add move-only managed heap ownership, callback-state diagnostics and a complete string/vector example across cooperative waits. Physical Switch acceptance remains pending.

## Unreleased recoverable file publication - 2026-10-29

- Add an independent file-mutation table with owner/path/extent checks, rename without replacement and explicit deletion with postcondition verification.
- Add a heap-free publication helper that preserves backups, refuses conflicts and supports explicit restoration after inspection.
- Exercise all file-presence states, failed acknowledgements/postchecks and a cooperative 256 KiB preparation/publication example. Physical storage and performance acceptance remain pending.

## Unreleased directory catalog services - 2026-10-29

- Add an independent optional catalog table for bounded file/directory pages, complete names, file sizes and component compatibility metadata.
- Keep directory handles scoped to one call; preserve the file-table layout, validate owner access and report traversal limits or malformed metadata explicitly.
- Review mixed directory modes with IDA Pro 9.5 and extend the cooperative streaming example with catalog verification. Physical Switch acceptance remains pending.

## Unreleased bounded file services - 2026-10-29

- Add an optional independently versioned file table with owner-thread checks, bounded relative paths, exclusive creation and offset reads/writes.
- Support files up to 64 MiB with transfers up to 64 KiB, checked nested directories and no handles retained across calls or lifecycle changes.
- Add a cooperative 256 KiB example and compiled path, error, ownership and interruption checks. Raw settings and published-runtime contracts remain unchanged; Switch storage/performance acceptance is pending.

## Unreleased repository and data-storage refinement - 2026-10-29

- Rename root implementation/application/install directories to runtime, mod-porter and packaging; update source exports, SDK discovery, builds and documentation.
- Remove disposable IDE/compiler/Python caches and archive superseded release-specific maintenance scripts outside the maintained source.
- Replace the introductory information dump with project purpose, entry points, capabilities and repository ownership.
- Group documentation by audience/purpose, separate regressions and fixtures from build tools, and consolidate gameplay examples under examples/mods.
- Expand project/runtime roadmaps into implementation and acceptance checklists; add navigation/layout verification and consistent editor settings.
- Preserve SDK dependency sources/licenses and complete example manifests in curated exports; validate moved workflows and desktop source paths.
- Add bounded checksummed binary/XML records with verified alternating writes, conservative unreadable-slot handling and explicit initialization/repair. Hardware storage acceptance remains pending.

## Unreleased C++ library candidate - 2026-10-29

- Add explicit owner-backed nothrow scalar/array/aligned allocation and sized/unsized deletion through the existing memory service.
- Supply fallible growable vectors, byte strings and sorted maps with explicit copy/move behavior, alias-safe growth and allocation-failure preservation.
- Add typed construction/deletion helpers, trait checks and owner/lifetime tests; keep throwing new, hosted STL and global initialization unsupported.
- Execute mixed container models, a 4,096-entry map, cooperative workloads and a complete numeric XML registry example. Core and accepted release remain unchanged; physical acceptance is pending.

## Unreleased numeric library candidate - 2026-10-29

- Add explicit C/numeric project and CLI profiles with bounded, status-bearing decimal float/double parsing and roundtripping formatting.
- Enable typed pugixml floating attributes/text through scoped bridges to pinned unmodified fast_float and Ryu; preserve default-profile and strict unsupported-import behavior.
- Test IEEE boundaries, halfway rounding, subnormals, independent precision-format oracles, storage guards, owner/lifecycle failure and actual cooperative stacks; report compiler frames separately from whole-stack measurements.
- Add a complete numeric XML/settings example and retain dependency licenses/provenance. Core/accepted release remain unchanged; hardware acceptance remains pending.

## Unreleased plugin library candidate - 2026-10-29

- Add an opt-in owner-heap C allocation/string subset, with required-memory binding and project/CLI flags.
- Pin unmodified pugixml 1.16 with source hashes and MIT notice; provide a text/integer UTF-8 memory DOM and bounded serialization adapter.
- Preserve strict failures for unsupported float conversion, stdio and hosted C++ dependencies; retain constructor arrays so link-time garbage collection cannot hide them.
- Execute real CRT/XML ARM64 workloads, malformed/large data, allocation failure, cross-owner and restart cases; verify an isolated copied SDK. Core binary and accepted release remain unchanged; Switch acceptance is pending.

## Unreleased plugin memory candidate - 2026-10-29

- Add an optional independently versioned owner-thread memory table without changing base/extension ABI 1 layouts.
- Supply pooled aligned allocation, calloc, realloc, exact-pointer free/size, statistics and Trim under the shared plugin budget.
- Preserve live data across Kill/Load; reclaim unpublished failed initialization and retain failed release phases for correct retry/accounting.
- Add mixed-owner allocation, capacity, lifecycle, reentry and OS failure checks. General CRT/STL and Switch acceptance remain open.

## Unreleased runtime capacity candidate - 2026-10-29

- Begin the expanded PC source-port runtime roadmap; full Menyoo porting remains deferred.
- Add independent startup/build image and BSS caps from 1-32 MiB, preserving 2 MiB defaults and the shared accounting limit.
- Report measured module extents/accounting and configuration suggestions; reject contradictory project limits and support explicit large-module preflight.
- Add a real large NRO lifecycle probe and capacity-failure diagnostics. Local checks do not transfer previous Switch acceptance to the new runtime.

## v1.4.1 Preview + Mod Porter 0.9.1 - 2026-10-25

- Audited the runtime and desktop tool; the rebuilt runtime remains identical to the accepted reference.
- Separate DLC workflow steps and collapse optional controls; add clearer navigation from resources to DLC creation.
- Reject structural metadata errors without damaging editor state; add definition recovery and unsaved-ID protection.
- Correct nested asset selection, incomplete-SDK diagnostics and already-registered batch output.
- Expand compiler, asset, workflow, runtime and resize/DPI checks; document scope in docs/verification/QUALITY-AUDIT.md.

## Mod Porter 0.9.0 Preview - 2026-10-25

- Installed-DLC discovery and filtering from the current game/SD folder.
- Complete PED asset-set replacement with preserved metadata and transaction-level Undo.
- Filter-scoped PED movement profile changes and clearer unavailable-pack handling.

## Mod Porter 0.8.0 Preview - 2026-10-25

- Editable DLC collections combine multiple PEDs and vehicles while retaining source data.
- Full PED field/XML editing, resource management, coordinated renames and Undo/Redo.
- Existing registered collections export only their DLC; first installs update registration once.
- Automatic multi-PED bundling, supporting-file handling and safer prepared-asset selection.

## Mod Porter 0.7.0 Preview - 2026-10-25

- Four-file human PED DLC creation without external game metadata or a manual base name.
- Automatic names/output folders, male/female movement profiles and optional custom metadata.
- One-step PED creation and registration in copied game files, with folder or ZIP output.
- Four-file drag and drop, remembered game paths and clearer asset validation errors.

## v1.4.1 Public Preview - 2026-10-23

- Include the required shared SD-access `main.npdm` in runtime downloads,
  developer builds and Mod Porter 0.6.0 packaging.
- Provide an installer that backs up customized NPDM files and preserves their
  other permissions and settings.
- Runtime binary, plugin ABI and default limits remain unchanged.

## v1.4 Public Preview - 2026-10-23

- Complete loose-PED DLC wrappers from explicitly selected target metadata.
- Add-on registration/model/shader dependency audits and multi-pack preparation.
- Texture reduction through supported Legacy fragment physics-child drawables.
- Header-only controller UI with measured layout, navigation and persistence demo.
- Public verification command, UI model tests and pinned Windows/Linux CI workflow.
- Mod Porter 0.6.0: folder-only asset/PED/DLC output alongside ZIP packaging,
  selected-game metadata and multi-pack registration checks.
- Ships the unchanged console-accepted v1.3 runtime; new SDK/UI and asset
  preparation features have local build/file validation and require per-mod Switch tests.


## v1.3 Public Preview - 2026-10-23

- Per-plugin diagnostics: per-plugin elapsed-time diagnostics, callback
  isolation after invalid yields, and a controller-only diagnostic/probe workflow.
  Defaults remain 8 resident NROs, 16 script slots and an 8 MiB plugin budget.
- Explicit lifecycle initialization macros and Mod Porter adaptation of
  registration-only multi-script/callback DllMain bodies, with capability and
  registration checks. Custom PC initialization remains explicit.
- NRO builds normalize source/debug paths and are reproducible across output
  locations with the same inputs and toolchain.

- SDK Parity Test 1: optional extension ABI for full ped/vehicle/object/pickup
  enumeration, owned additional scripts, validated script globals/entity pointers,
  custom PNG textures, controller/USB keyboard and HUD rendering callbacks.
  Original API/descriptor ABI 1 remains compatible. Matched Switch checks passed
  smooth PNG rotation/colour, controller input, additional script execution,
  pointer availability, NSC/NRO coexistence, Kill/Load and game restart.
  Controller input is the Switch target; USB keyboard support is optional.
- Combined `sdk-parity` tutorial, generated test texture, capability checks,
  per-plugin cleanup and resource accounting. Mod Porter recognizes the new
  interfaces while keeping PC address/layout and DirectX adaptations explicit.

- Per-plugin settings folders at `settings/<plugin-id>/`, with atomic migration
  of the previous prefixed filenames on access. Existing ABI 1 NROs work without
  rebuilding. Generic NSC journals also have separate namespace folders.
- Optional ABI 1 plugin stop callbacks for owned-effect/entity cleanup. Console
  checks passed with Footprints and MEGATARD's NSC Kill Process/Load Now controls.
- Disarm NX recognizes both upper arms and forearms, the right hand and weapon
  hits. The expanded gameplay behavior passed Switch testing.
- Controller chord, bounded INI and validated nearby-ped helpers for source ports.
- Mod Porter 0.5.0 includes the 0.4.0 archive workflows: prepare updated game-archive copies from selected SD/local
  files; merge DLC registration directly from the installed archive; replace a
  selected existing asset while retaining unrelated entries.
- Compiler-only checks, verified SD NRO installation/restore, diagnostic report
  import, embedded texture reduction and real source-port examples.

Runtime Completion Test 1 passed matched console gameplay acceptance with the
worker/render source port, diagnostics and existing mods running together.
No regressions were observed.

The 0.3.0 desktop workflow passed acceptance. The new 0.4.0 archive updater has
separate local validation and still requires gameplay acceptance for its outputs.
Caller/runtime/plugin ABI versions remain 1. Default limits remain 8 resident
NROs, 16 cooperative scripts and an 8 MiB plugin accounting budget.

Historical release downloads and acceptance records retain their original scope.
