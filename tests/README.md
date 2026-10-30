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

`test_service_batch.py` executes the exact three NROs produced by
`build_service_acceptance.py` together under compiled defaults. It checks manual
policy, publication/backup verification, nested catalog data, shared-state
Kill/Load, retained mappings and closed file handles. It requires `--core`,
`--artifact <service-kit-build-directory>` and `--output` with the same private
runtime harness inputs. It does not install files or replace hardware acceptance.
