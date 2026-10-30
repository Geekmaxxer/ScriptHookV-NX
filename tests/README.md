# Automated tests

Public source checks require Python 3.10+ and no game files:

```powershell
python tests/test_portable.py
python tests/test_sd_permissions.py
python tests/test_game_layout.py
python tools/verify_repo.py --output out/verification
```

The game-layout suite uses disposable SD fixtures and PowerShell 5.1+ (or pwsh);
it skips when PowerShell is unavailable. It never uses a mounted card.

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
