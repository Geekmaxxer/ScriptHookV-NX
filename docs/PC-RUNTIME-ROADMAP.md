# Runtime development roadmap

The runtime workstream expands support for substantial PC source ports on the
identified GTA V Switch executable. Full Menyoo porting follows the implementation
and acceptance gates below. Updated 2026-11-01.

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

- [x] Grow installed-plugin catalogues independently of resident admission; verify 160 rows locally.
- [x] Preserve owner identity through suffix changes, removals and failed rescans.
- [x] Report queued/completed scan generations and filter missing unpublished rows.
- [ ] Accept the revised catalogue runtime and matched MEGATARD v1.1 menu on hardware.

- [x] Provide configurable image/BSS extent limits; retain 2 MiB defaults.
- [x] Support automatic and larger positive values for all five runtime settings.
- [x] Grow stable loader/scheduler/owner records and admit additional stacks on demand.
- [x] Remove running-state bitmask assumptions for high-index plugin owners.
- [x] Preserve independent shared accounting and explicit capacity refusal.
- [x] Report measured module extents and minimum configuration requirements.
- [x] Exercise large NROs, failure cleanup, coexistence and restart locally.
- [x] Provide multiple cooperative scripts and callback timing diagnostics.
- [x] Prepare a large-image/BSS, 37-entry and nested-ownership probe with preserved configuration phases.
- [x] Accept large extents, 36 additional workers and extra stack admission under explicit settings on Switch.
- [x] Accept the same bounded workload under automatic settings on Switch.
- [x] Verify original configuration restoration and normal startup after the capacity phases.
- [x] Verify restored bytes, effective limits and expected oversized-probe refusal after restart.
- [x] Prepare per-operation timing, callback-interval histograms and bounded run history locally.
- [x] Accept both Test2 hardware lifetimes and identify synchronous report-save stalls.
- [x] Reduce diagnostic save frequency in a separately verified source revision.
- [ ] Measure peak RAM, frame time and long sessions for large configurations on Switch.
- [ ] Accept full-capacity concurrent workloads on hardware.

Evidence: [large modules](verification/LARGE-MODULES1.md) and [limits](reference/RUNTIME-LIMITS.md).
The [Test2 result](verification/CAPACITY-DIAGNOSTICS2.md) accepts the bounded workload
and measurements. The quieter Test3 probe can join a later combined workload.
A 2 ms cooperative check cannot preempt an infinite loop. Script stacks remain
64 KiB; code remains resident until game exit.

## 3. Allocation and library services

- [x] Implement owner allocation, alignment, calloc/realloc/free, stats and Trim.
- [x] Verify ownership, exhaustion, release failures and retained Kill/Load data.
- [x] Provide opt-in C allocation/string functions and a pinned XML memory DOM.
- [x] Provide strict decimal conversion and typed XML floating values.
- [x] Provide nothrow objects and fallible vector/string/sorted-map helpers.
- [x] Execute a combined numeric XML registry with move-only records locally.
- [x] Accept bounded allocator cycles and nested owner reclamation in the physical Explicit probe.
- [x] Provide allocation-free buffer formatting for common integer/string source-port paths.
- [x] Provide numeric-profile decimal float formatting and independent ARM64 comparison coverage.
- [x] Preserve XML numeric-profile requirements and explicit unsupported-format failures.
- [x] Provide opt-in toolchain math with recorded archive/compiler identity.
- [x] Verify float/double math, exceptional results and per-NRO errno isolation locally.
- [x] Combine geometry and pending formatter checks in one complete example.
- [x] Provide signed/unsigned C-style integer parsing with explicit end/error/range behavior.
- [x] Provide numeric-profile decimal prefix conversion with direct float rounding.
- [x] Share C-profile errno with libm and remove the redundant math-only source unit.
- [x] Merge pending math/formatter checks with parsing in one library-services workload.
- [x] Package one manual batch 5 NRO, preserve notices and verify standalone SDK reproduction.
- [x] Accept the consolidated parsing/math/formatter normal-completion workload in physical Batch 7.
- [x] Provide fallible both-ended ring queues and preserve wrapped/self-borrowed elements through growth.
- [x] Provide move-only inline/heap callables, explicit cloning and RTTI-free target identity.
- [x] Verify independent sequence/lifetime models, 6,144 values, failure preservation and cross-source targets.
- [x] Add a complete 768-action cancellation/restart workflow to the existing deferred Batch 8.
- [ ] Accept the captured-action workflow on Switch alongside pending resource/stream checks.
- [ ] Define broader hosted-container compatibility for concrete source ports.
- [x] Provide owner-backed hash maps with move-only records and allocation-free string-view lookup.
- [x] Verify collision chains, 6,144 keys, alias refusal, allocation failures and managed restart locally.
- [x] Consolidate registry, graphics and library probes in deferred service batch 6 on the unchanged texture runtime.
- [x] Accept the combined registry normal-completion workload in physical Batch 7.
- [ ] Define hosted STL and initialization contracts independently.
- [x] Grow stable heap allocation/arena records and the pointer index through budget-accounted metadata.
- [x] Verify 8,000 simultaneous allocations across two owners and 160 arenas under an explicit 16 MiB budget.
- [x] Verify metadata failure/retry, tombstone rebuilds, realloc, owner isolation and rejected-initialization cleanup.
- [x] Provide a managed 4,096-buffer probe and merge all pending hardware checks in deferred service batch 7.
- [x] Accept record/index growth and completed combined probes on Switch under explicit 16 MiB settings.
- [ ] Complete Horizon allocation/release, larger arena profiles and fragmentation acceptance.

Evidence: [C-style parsing](verification/NUMERIC-PARSING1.md),
[math profile](verification/MATH-PROFILE1.md),
[formatted text](verification/FORMATTED-TEXT1.md),
[heap](verification/PLUGIN-MEMORY1.md), [growing heap metadata](verification/HEAP-CAPACITY1.md), [C/XML](verification/PLUGIN-LIBRARIES1.md),
[numeric conversion](verification/PLUGIN-NUMERIC1.md), [C++ containers](verification/PLUGIN-CXX1.md)
and [queues/captured actions](verification/TASK-QUEUES1.md).
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
- [x] Accept ordinary 256 KiB publication, retained backup and nested catalog data on Switch.
- [x] Provide caller-bounded read buffers and exclusive prepared-file write tasks with explicit progress.
- [x] Verify complete large XML loading, parsing, failure preservation and cancellation locally.
- [x] Add bounded growable XML serialization and chunked byte verification.
- [x] Combine large XML loading, verified save, publication and backup restoration in one workload locally.
- [x] Provide complete-result catalog tasks with suffix filters, cooperative sorting and checked paths.
- [x] Verify catalog allocation/I/O failures, cancellation and restart during every processing phase locally.
- [x] Consolidate XML/save/recovery and multi-page preset selection in service batch 4.
- [x] Accept the combined preset/XML/file-transfer workload on Switch in service batch 4.
- [x] Verify exact input/output bytes, restored original, 13 presets and final natural-completion cleanup.
- [x] Provide opt-in read-only fopen/fread/fgets/seek/close over existing file/resource services.
- [x] Verify byte/position models, short/error reads, changed extents and cancellation locally.
- [x] Verify 100 simultaneous streams, metadata failure cleanup and the pinned DOM's load_file locally.
- [x] Add the complete stream probe to the pending resource-registry Batch 8 on its unchanged kernel.
- [x] Pin a JSON parser/writer with strict syntax, Unicode checks and independent C-profile allocation.
- [x] Provide failure-preserving document copies, typed edits and serialized buffers under requested-memory limits.
- [x] Verify numeric/type/byte models, nesting and allocation limits, context moves and 2,000 records locally.
- [x] Provide cooperative JSON file loading with retained prior results after transfer/parse failure or cancellation.
- [x] Merge a complete verified JSON publication/restoration probe into the pending four-probe Batch 8.
- [ ] Accept the combined JSON/stream/resource/action workload on Switch.
- [ ] Verify media errors and recovery on Switch storage.

Evidence: [recoverable data](verification/RECOVERABLE-DATA1.md) and
[bounded file services](verification/PLUGIN-FILES1.md) and
[paged directory catalogs](verification/PLUGIN-DIRECTORY1.md) and
[recoverable publication](verification/PLUGIN-PUBLICATION1.md) and
[cooperative preset catalogs](verification/CATALOG-SCAN1.md) and
[JSON configuration/presets](verification/JSON-SERVICES1.md). Chunked I/O uses an
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
- [x] Provide explicitly initialized shared C++ session state across source files and entries.
- [x] Verify per-type/tag ownership, failure cleanup, reentry and recreation after Kill/Load locally.
- [x] Accept shared session state with publication/catalog checks in one hardware batch.
- [ ] Define global constructor ordering, exception unwinding and static-local policy.
- [x] Accept the combined managed session's cancellation, restart and final owner cleanup on Switch.
- [x] Accept two managed children draining before their parent in the Explicit hardware probe.
- [ ] Accept broader nested failures and transition cleanup on Switch.
- [x] Grow stable cleanup records through shared-budget admission and preserve direct heap pins.
- [x] Verify 1,024 contexts, independent peers, metadata/budget failures and nested cleanup deferral locally.
- [x] Prepare one 512-object completion/cancellation probe in deferred service batch 8.
- [ ] Accept growing resource registrations and cleanup on Switch.
- [ ] Measure callbacks and initialization under substantial workloads.

Normal scope exit runs destructors; Kill does not unwind suspended objects.
Workers, preemption and arbitrary infinite-loop recovery are outside current guarantees.
Evidence: [owned-resource contract](reference/OWNED-RESOURCES.md) and
[lifecycle verification](verification/PLUGIN-RESOURCES1.md).

## 6. Concrete engine compatibility

- [x] Provide pools, generation-checked pointers and bounded globals.
- [x] Provide controller input, optional USB keyboard, HUD callbacks and PNG textures.
- [x] Grow stable texture cache records with shared-budget metadata accounting; verify 160 PNGs locally.
- [x] Verify high-ID ownership, admission/upload failures, retained Kill/Load IDs and an 80-image example locally.
- [x] Prepare a combined graphics/library candidate with manual probes and preserved rollback requirements.
- [x] Accept the bounded 80-image visual probe and combined library workload in physical Batch 7.
- [ ] Accept broader cache/render transitions and measured physical performance.
- [x] Provide audited single-open-user nickname access with explicit availability and ambiguity states.
- [ ] Pin a substantial port and inventory required native/memory/resource behavior.
- [ ] Add verified target adapters for missing engine and streaming services.
- [ ] Validate asset ownership, cleanup and transitions with concurrent mods.

PC ScriptHookV exposes natives; it does not replace Windows, DirectX or library
runtimes. Independent reverse engineering is not an original-source or ABI contract.
The [texture candidate](verification/TEXTURE-CATALOG1.md) preserves existing API
layouts and engine bindings. GPU footprint, physical rendering and performance
remain acceptance gates before further graphics expansion or stable promotion.

## 7. Combined runtime acceptance

The [platform/capacity sequence](verification/PLATFORM-CAPACITY1.md) retains the
remaining hardware requirements. SDK adaptations using accepted runtime services
can proceed with targeted local checks and combined hardware batches.

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
The accepted [service batch 4](verification/CATALOG-SCAN1.md) consolidates large
XML transfers, preset catalogs, verified saving and backup restoration. Its
normal workload needs no repetition. Pending parsing/math/formatter checks are
merged in the library-services example for a later combined hardware batch;
broader transition and stress gates remain open.

[Batch 7 acceptance](verification/SERVICE-BATCH7.md) closes the bounded
normal-completion and visual checks under its exact configuration. Cancellation,
transitions, rollback and performance remain separately open.
The next [resource-growth candidate](verification/RESOURCE-CAPACITY1.md) groups
its resource/stream/action/JSON probes and broader observations in one deferred
[service batch 8](../examples/service-acceptance/README.md) session. Accepted
Batch 4 and Batch 7 probes need no separate repeat installations.

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
