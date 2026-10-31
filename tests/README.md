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
