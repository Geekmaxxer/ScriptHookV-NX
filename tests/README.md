# Automated tests

Public source checks require Python 3.10+ and no game files:

```powershell
python tests/test_portable.py
python tests/test_sd_permissions.py
python tools/verify_repo.py --output out/verification
```

`tests/fixtures/` contains compiled test inputs. Production modules must not
expose fixture entry points. `bootstrap.py` adds shared build tools to direct
test execution; tests are not installed as plugin dependencies.

Target-instruction and runtime suites additionally require the maintainer's
matching game/SDK images and emulator adapters. Those inputs are private and
excluded from public exports. Such suites execute real compiled instructions
while modeling specified OS/game services; they do not establish physical Switch
gameplay, frame time or memory consumption.
