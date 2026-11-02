# Automated tests

Public source checks require Python 3.10+ and no game files:

```powershell
python tests/test_portable.py
python tests/test_sd_permissions.py
python tests/test_game_layout.py
python tests/test_capacity_profiles.py
python tests/test_capacity_diagnostics.py
python tools/verify_repo.py --output out/verification
```

The game-layout suite uses disposable SD fixtures and PowerShell 5.1+ (or pwsh);
it skips when PowerShell is unavailable. It never uses a mounted card.
The capacity-profile suite uses the same host requirement and disposable fixtures
to check byte-preserving restoration, phase capture and refusal of unrelated edits.
The diagnostic-report suite checks bounded parsing, histogram consistency, lifetime
versus per-run counters and explicit rejection of incomplete or conflicting reports.

`tests/fixtures/` contains compiled test inputs. Production modules must not
expose fixture entry points. `bootstrap.py` adds shared build tools to direct
test execution; tests are not installed as plugin dependencies.

Target-instruction and runtime suites additionally require the maintainer's
matching game/SDK images and emulator adapters. Those inputs are private and
excluded from public exports. Such suites execute real compiled instructions
while modeling specified OS/game services; they do not establish physical Switch
gameplay, frame time or memory consumption.

The maintainer-only `test_runtime_acceptance.py` builds the combined acceptance
probe and executes it against the matching production runtime. It covers normal
execution, interruption, restart, ownership cleanup and injected failures. Its
private fixture/harness dependencies are excluded from portable source exports;
the [public record](../docs/verification/COMBINED-ACCEPTANCE1.md) retains its scope
and hashes. Run it with `--core <matching-core-directory> --output <fresh-directory>`.

The maintainer-only `test_capacity_acceptance.py` executes the compiled large
probe against the pinned runtime, including 36 workers, stack growth, nested
cleanup and explicit/automatic profiles. It additionally requires `--artifact`
pointing to the probe's compiled NRO/ELF directory. Its private harness inputs are
excluded from portable exports; the [capacity record](../docs/verification/CAPACITY-ACCEPTANCE1.md)
retains the local scope and evidence.

The maintainer-only `test_session_local.py` builds a two-source C++ fixture and
executes shared session state, reentry/refusal, ownership failure, initialization
rejection and Kill/Load against the pinned runtime. It also executes the complete
session-state worker example. Run with `--core <matching-core-directory>` and
`--output <fresh-directory>`. The public [session record](../docs/verification/SESSION-LOCAL1.md)
retains its exact local scope; private game/emulator inputs are excluded from exports.

`test_service_batch.py` executes the exact consolidated batch 5 NRO produced by
`build_service_acceptance.py` under compiled defaults. It checks manual policy,
combined parsing/math/formatter reports, repeated natural completion, retained
mappings, closed file handles and complete binary-kit notices. It requires
`--core`, `--artifact <service-kit-build-directory>` and `--output` with the same
private runtime harness inputs. Historical batch results retain their original
artifacts and suite scope; this runner does not install files or replace hardware acceptance.

The maintainer-only `test_file_transfer.py` executes owned read buffers, exclusive
prepared writes and the complete large XML example against the pinned runtime.
It checks binary/empty input, bounds, allocation refusal, short/error reads, extent
changes, existing/partial output preservation and cancellation/restart cleanup.
Run with `--core <matching-core-directory>` and `--output <fresh-directory>`.
Its private game/emulator inputs are excluded from public exports; the
[file-transfer record](../docs/verification/FILE-TRANSFERS1.md) retains its exact scope.

`test_xml_workflow.py` executes growable XML serialization, bounded comparison,
output/allocation failures and the combined publication/restoration example.
It preserves corrupt or interrupted disk states and verifies registered cleanup.
Use `--core <matching-core-directory>` and `--output <fresh-directory>` with the
same private harness inputs. The current packaged batch test uses the merged
workflow instead of the earlier standalone XML probe; the
[combined record](../docs/verification/XML-WORKFLOW1.md) identifies its scope.

`test_catalog_scan.py` executes preset filtering, cooperative sorting, complete-result
publication, checked selection paths, allocation/I/O failures and cancellation
during each phase. It uses `--core <matching-core-directory>` and
`--output <fresh-directory>` with the same private runtime adapters. The
[catalog record](../docs/verification/CATALOG-SCAN1.md) preserves the local scope.
The combined XML and packaged-batch suites also verify multi-page preset selection.

`test_formatted_text.py` executes integer/string and numeric-profile formatting
with guarded buffers, independent integer/decimal reference results, overflow and
unsupported-format checks, reusable variadic lists and repeated example execution.
Run with `--core <matching-core-directory>` and `--output <fresh-directory>`.
The [formatted-text record](../docs/verification/FORMATTED-TEXT1.md) preserves the
local scope; runtime images/adapters remain private maintainer inputs.

`test_math_profile.py` executes the selected toolchain libm in two NROs with
independent errno state. Deterministic float/double reference samples cover the
documented function families, exceptional paths, decomposition outputs and
restart. It also verifies the combined geometry/formatter example and strict
refusal of hosted stream imports. Use `--core <matching-core-directory>` and
`--output <fresh-directory>`. The [math record](../docs/verification/MATH-PROFILE1.md)
retains the exact archive identity and local scope.

`test_numeric_parsing.py` executes integer/decimal prefix conversion in separate
C and numeric NROs. Independent reference models cover bases, extrema, saturation,
end offsets and errno, including a direct binary32 rounding boundary. The suite
also verifies repeated execution of the consolidated library-services example.
Use `--core <matching-core-directory>` and `--output <fresh-directory>`. The
[parsing record](../docs/verification/NUMERIC-PARSING1.md) retains its exact scope.

`test_texture_catalog.py` executes growing metadata, 160 PNGs, high-ID drawing,
cross-plugin ownership, budget exhaustion, allocation/upload failures and
Kill/Load. Use `--build <matching-core-directory> --output <fresh-directory>`.
`test_texture_catalog_example.py` executes the complete example with
`--build <matching-core-directory> --plugin <built-example-directory>`. Both
retain the existing private target adapters and model GPU operations. The
[texture record](../docs/verification/TEXTURE-CATALOG1.md) separates local checks
from physical rendering and memory/performance acceptance.

`test_hash_map.py` compares production ARM64 containers with independent Python
dictionary and lifetime models. Use `--core <matching-core> --plugin <built-hash-fixture>
--output <fresh-directory>`. It covers wrapped collisions, move-only string/record
ownership, explicit-byte queries, aliased arguments and allocation failures.
`test_service_batch6.py` executes the exact packaged registry/library probes,
checks cancellation/restart and verifies retained graphics identities. Its flags
are `--core`, `--artifact`, `--previous-graphics` and `--output`. Physical acceptance
remains combined and deferred; see [hash-map verification](../docs/verification/HASH-MAP1.md).

`test_heap_capacity.py` exercises stable record/arena growth, pointer-index
rebuilds, metadata failure/retry, two-owner isolation, realloc and rejected
initialization beyond the initial record extent. Use `--core`, `--plugin
<built-heap-capacity-fixture>` and `--output` with the private target adapters.
`test_service_batch7.py` extends the combined probe execution to the managed
4,096-buffer workload on the new kernel; its flags match the batch 6 runner.
The [heap-capacity record](../docs/verification/HEAP-CAPACITY1.md) retains the
8,000-record/160-arena local scope and separate physical acceptance gates.

`test_resource_capacity.py` exercises growing registration pages, budget/mapping
refusal, direct heap pins, 1,024 contexts, peer isolation, record reuse, bounded
nested cleanup and rejected initialization. Use `--core`, `--plugin` and `--output`
with the private adapters. `test_service_batch8.py` runs the exact packaged
512-object workload twice, cancels construction/verification and verifies restart.
Use `--core`, `--artifact` and `--output`; see [resource verification](../docs/verification/RESOURCE-CAPACITY1.md).

`test_stdio.py` compares buffered stream results with independent byte/position
models and injects short/error reads, changed extents, optional-service absence
and allocation/registration failures. Use `--core`, `--plugin` and `--output`.
The expanded `test_service_batch8.py` also executes both exact packaged probes,
verifies all stream fixture bytes and checks cancellation/restart cleanup.
See [stream verification](../docs/verification/READ-STREAMS1.md).

`test_task_queue.py` compares ring operations with independent sequence models
and checks inline/heap target ownership, alias refusal, failed assignment/copy,
move-only captures and target identity across two source files. Use `--core`,
`--plugin` and `--output` with the private ARM64 adapters. The compiled fixture
uses both task-queue sources and its shared header include directory. The expanded
`test_service_batch8.py` executes all three packaged probes, including repeated
action completion and cancellation during construction/execution.
See [queue verification](../docs/verification/TASK-QUEUES1.md).

`test_json.py` checks the pinned parser/writer against independent JSON/type/byte
models and exercises budget/platform refusals, context moves, mutable copying,
Unicode/depth limits and cooperative file-load failure preservation. Use `--core`,
`--plugin` and `--output` with the private ARM64 adapters. The expanded
`test_service_batch8.py` executes all four exact probes, including byte-verified
JSON publication/restoration and cancellation/retry/error-preservation cases.
The portable suite verifies dependency hashes and project/build option rules.
See [JSON verification](../docs/verification/JSON-SERVICES1.md).
