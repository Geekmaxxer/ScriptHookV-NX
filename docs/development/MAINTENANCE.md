# Maintain and verify a checkout

Run the public checks from a clean source checkout:

Repository navigation and directory boundaries are checked automatically. Tests
live in `tests/`, compiled inputs in `tests/fixtures/`, and gameplay examples in
`examples/mods/`. See [repository organization](REPOSITORY.md) for the full layout.

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

The [quality audit](../verification/QUALITY-AUDIT.md) records the current runtime and desktop
review. The [workflow guide](../guides/MOD-PORTER-WORKFLOWS.md) maps common tasks to their
starting pages. Desktop workflow tests additionally exercise the packaged app;
backend compilation alone does not establish usable controls or correct layout.

The GitHub workflow runs portable checks on Windows/Linux, the UI model on
Linux, and desktop checks on Windows. It uses read-only repository permissions
and pinned action revisions. GitHub execution is distinct from local validation.
Compiler-dependent cases require the selected toolchain; CI does not bundle it.
Without --devkitpro, desktop checks use --no-toolchain and explicitly report the
four skipped compiler groups. Supply a toolchain for the full compiler suite.

## Milestone review and synchronization

After each completed milestone:

1. Update implementation contracts, roadmap checklists and exact verification scope.
2. Run the relevant checks and verify a fresh curated source export.
3. Synchronize that export to the designated GitHub working copy, checking for
   intervening edits and preserving unrelated files and existing work.
4. Verify synchronized file hashes and repository checks. Preserve Git HEAD and
   the index; leave changes unstaged and uncommitted for inspection.
5. Provide a short, descriptive commit title with the milestone report.

Build trees, private research inputs and historical workspace archives are not
source-sync payloads. Synchronization does not publish a release or establish
physical acceptance for a modified runtime.

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
