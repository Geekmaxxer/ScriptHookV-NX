# Runtime development roadmap

The runtime workstream expands support for substantial PC source ports on the
identified GTA V Switch executable. Full Menyoo porting follows the implementation
and acceptance gates below. Updated 2026-10-30.

Checked implementation/local-validation items do not imply hardware acceptance.
The public baseline remains identified in the [project roadmap](COMPLETION-ROADMAP.md).

## 1. Target and ABI foundation

- [x] Enforce the title/build fingerprint and audited handler declarations.
- [x] Preserve caller, base plugin and extension ABI 1 layouts.
- [x] Extend optional services through sized, versioned contracts.
- [x] Verify BOOL outputs, padded NX vectors and per-call failure status.
- [ ] Audit each substantial port's optional natives, indices and memory needs.
- [ ] Re-audit changed executables before enabling their profiles.

Evidence: [native caller](reference/NATIVE-CALLER.md) and [extensions](reference/EXTENSIONS.md).
The PC 2699/1.61 content baseline is not an equivalent Switch ABI.

## 2. Module capacity and scheduling

- [x] Provide configurable image/BSS extent limits; retain 2 MiB defaults.
- [x] Support automatic and larger positive values for all five runtime settings.
- [x] Grow stable loader/scheduler/owner records and admit additional stacks on demand.
- [x] Remove running-state bitmask assumptions for high-index plugin owners.
- [x] Preserve independent shared accounting and explicit capacity refusal.
- [x] Report measured module extents and minimum configuration requirements.
- [x] Exercise large NROs, failure cleanup, coexistence and restart locally.
- [x] Provide multiple cooperative scripts and callback timing diagnostics.
- [ ] Measure peak RAM, frame time and long sessions for large configurations on Switch.
- [ ] Accept full-capacity concurrent workloads on hardware.

Evidence: [large modules](verification/LARGE-MODULES1.md) and [limits](reference/RUNTIME-LIMITS.md).
A 2 ms cooperative check cannot preempt an infinite loop. Script stacks remain
64 KiB; code remains resident until game exit.

## 3. Allocation and library services

- [x] Implement owner allocation, alignment, calloc/realloc/free, stats and Trim.
- [x] Verify ownership, exhaustion, release failures and retained Kill/Load data.
- [x] Provide opt-in C allocation/string functions and a pinned XML memory DOM.
- [x] Provide strict decimal conversion and typed XML floating values.
- [x] Provide nothrow objects and fallible vector/string/sorted-map helpers.
- [x] Execute a combined numeric XML registry with move-only records locally.
- [ ] Define broader hosted-container compatibility for concrete source ports.
- [ ] Complete Horizon allocation/release and fragmentation acceptance.

Evidence: [heap](verification/PLUGIN-MEMORY1.md), [C/XML](verification/PLUGIN-LIBRARIES1.md),
[numeric conversion](verification/PLUGIN-NUMERIC1.md) and [C++ containers](verification/PLUGIN-CXX1.md).
These contracts do not supply a hosted STL, Windows APIs or throwing allocation.

## 4. Persistent data and assets

- [x] Provide isolated flat settings filenames and bounded 64 KiB raw writes.
- [x] Provide checksummed alternating records for fixed integer settings.
- [x] Parse/edit/serialize numeric XML with allocation and overflow checks.
- [x] Provide recoverable versioned binary/XML records with variable payloads.
- [x] Verify corruption, partial writes, failed readback and recovery locally.
- [x] Provide an independent file table with bounded offset reads/writes and fixed-length creation.
- [x] Provide isolated nested paths and checked directory creation.
- [x] Close file/directory resources before returning; retain no handles across waits or Kill/Load.
- [x] Exercise a cooperative multi-chunk example and conservative restart policy locally.
- [x] Provide independent paged file/directory enumeration with bounded traversal and explicit metadata errors.
- [x] Review mixed directory modes against the pinned SDK and verify catalog ownership/lifecycle locally.
- [x] Provide independently versioned, isolated rename without replacement and checked file deletion.
- [x] Define and verify backup-preserving publication and explicit recovery for prepared files.
- [x] Exercise complete large-candidate validation and publication with cooperative waits locally.
- [ ] Verify media errors and recovery on Switch storage.

Evidence: [recoverable data](verification/RECOVERABLE-DATA1.md) and
[bounded file services](verification/PLUGIN-FILES1.md) and
[paged directory catalogs](verification/PLUGIN-DIRECTORY1.md) and
[recoverable publication](verification/PLUGIN-PUBLICATION1.md). Chunked I/O uses an
independent versioned service; memory parsing does not remove filesystem limits.

## 5. Execution and initialization lifecycle

- [x] Bind optional services before explicit plugin initialization.
- [x] Provide owned additional scripts/callbacks and bounded cleanup.
- [x] Retain mappings to protect published handlers and suspended code.
- [x] Refuse unsupported global constructors/destructors explicitly.
- [x] Define explicit registered ownership for heap objects across waits and whole-plugin cancellation.
- [x] Pin registered contexts and verify release, transfer, nesting and stale handles locally.
- [x] Drain registered cleanup after user stop and rejected initialization before heap discard.
- [x] Execute a complete managed string/vector lifetime across Kill/Load locally.
- [ ] Define global constructor ordering, exception unwinding and static-local policy.
- [x] Accept the combined managed session's cancellation, restart and final owner cleanup on Switch.
- [ ] Accept broader nested registrations and failure cleanup on Switch.
- [ ] Measure callbacks and initialization under substantial workloads.

Normal scope exit runs destructors; Kill does not unwind suspended objects.
Workers, preemption and arbitrary infinite-loop recovery are outside current guarantees.
Evidence: [owned-resource contract](reference/OWNED-RESOURCES.md) and
[lifecycle verification](verification/PLUGIN-RESOURCES1.md).

## 6. Concrete engine compatibility

- [x] Provide pools, generation-checked pointers and bounded globals.
- [x] Provide controller input, optional USB keyboard, HUD callbacks and PNG textures.
- [x] Provide audited single-open-user nickname access with explicit availability and ambiguity states.
- [ ] Pin a substantial port and inventory required native/memory/resource behavior.
- [ ] Add verified target adapters for missing engine and streaming services.
- [ ] Validate asset ownership, cleanup and transitions with concurrent mods.

PC ScriptHookV exposes natives; it does not replace Windows, DirectX or library
runtimes. Independent reverse engineering is not an original-source or ABI contract.

## 7. Combined runtime acceptance

The [platform/capacity acceptance sequence](verification/PLATFORM-CAPACITY1.md)
is the next gate before further substantial runtime expansion.

- [x] Build smaller combined container/numeric/XML/settings probes.
- [x] Verify the retained runtime installation and compare both startup fatal reports.
- [x] Record startup failure with the SD runtime and menu module overrides absent.
- [x] Inspect DLC chronology, local package metadata and filesystem structure.
- [x] Record launch recovery after correction of the missing DLC path coverage.
- [x] Publish player guidance and package a read-only check with explicit optional-path recovery.
- [x] Add desktop/CLI recovery and bounded Atmosphere fatal-report import.
- [x] Read back the successful configuration and verify startup with Test1 restored.
- [x] Verify both demo NRO identities, manual loading and error-free initial execution.
- [x] Confirm nickname matching for one selected Switch account.
- [x] Record at least 30 seconds of managed execution with one live resource registration.
- [x] Verify the combined managed workload's restart and final-run cleanup on Switch.
- [ ] Verify nickname matching for a second selected Switch profile.
- [x] Build and locally verify a combined data/XML, texture, callback and borrowed-entity probe.
- [x] Accept the combined probe's workload, cancellation and restart on Switch.
- [x] Verify exact installed runtime, probe, texture and configuration identities.
- [x] Record pause/resume, Kill/Load and coexistence with Disarm and Footprints.
- [ ] Validate matched runtime installation and rollback.
- [ ] Test loading, death, character transitions and broader mod combinations.
- [ ] Record measured RAM/frame time and a long-session hardware soak.
- [ ] Publish the accepted service matrix and remaining limitations.

Probe and acceptance limits: [combined runtime verification](verification/COMBINED-ACCEPTANCE1.md).

## 8. Menyoo source-port gate

- [ ] Complete required runtime contracts and combined hardware acceptance.
- [ ] Pin a compatible 2699/1.61 revision and dependencies/licenses.
- [ ] Produce the feature inventory, adaptation plan and acceptance matrix.
- [ ] Implement and verify each required engine/content dependency.
- [ ] Record gameplay parity and explicit remaining differences.

The [pinned assessment](development/PORT-CANDIDATES.md) is a dependency inventory,
not proof of compatibility or the latest suitable revision. Revision selection
must verify actual requirements. Unknown future mods, arbitrary Windows binaries
and unlimited workloads cannot be certified by an API checklist.
