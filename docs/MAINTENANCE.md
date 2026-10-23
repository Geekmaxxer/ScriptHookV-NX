# Maintain and verify a checkout

Run the public checks from a clean source checkout:

```powershell
python tools/verify_repo.py --output out/verify-test1
python tools/verify_repo.py --output out/verify-full --desktop --devkitpro C:/devkitPro
```

The first command checks the native index, audited target profile and portable
tools. The second also runs desktop regressions, builds WPF on Windows and
compiles the NRO examples. Use `--cxx c++` to add the pure controller UI tests
with a host compiler. An alternative is `run_ui_tests.py --devkitpro ...`,
which executes the compiled ARM64 model using the optional Unicorn dependency.

Each run needs a new output directory. verification.json lists completed and
skipped checks; logs remain beside it. These checks require no game files.
They do not assign console acceptance to new assets or plugins.

The GitHub workflow runs portable checks on Windows/Linux, the UI model on
Linux, and desktop checks on Windows. It uses read-only repository permissions
and pinned action revisions. GitHub execution is distinct from local validation.
Compiler-dependent cases require the selected toolchain; CI does not bundle it.
Without --devkitpro, desktop checks use --no-toolchain and explicitly report the
four skipped compiler groups. Supply a toolchain for the full compiler suite.

## Game updates

Keep the title/build fingerprint, target instruction guards and exact accepted
runtime reference together. Run the profile checks and compare_core after a
rebuild. Preserve previous references and acceptance records. A new executable
needs a new audit; never bypass a failed fingerprint check.

## Console acceptance

Use the same test cases and game build for each candidate:

1. Start normally with existing NSC and NRO mods; check startup policies.
2. Open the UI demo, scroll, change values, visit subpages, Save, Kill and Load.
3. Restart GTA and confirm the saved values; enter/leave pause and loading states.
4. Spawn each add-on, inspect textures/animations/LODs, and test vehicle impacts.
5. Capture loader/bridge/timing reports and record installed artifact hashes.
6. Separately test full-capacity combinations, custom limits and longer sessions.

Record failures as specific operations and artifacts. Per-plugin elapsed timings
include I/O/native work/preemption; they are not game-wide FPS or peak RAM.
Keep game archives, personal settings and research dumps outside the repository.
