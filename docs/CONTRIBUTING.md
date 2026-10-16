# Contributing

Start with a bounded change and a fresh build directory. Keep the last accepted
package and its hashes. ABI changes require a versioned design; optional SDK
headers/tools should not force a new runtime when the existing ABI is sufficient.
Contributions for inclusion use the [project license](../LICENSE) unless explicitly
agreed otherwise. Keep original-source credit and identify material modifications.

## Plugins and examples

Use a unique stable plugin ID, a bounded resource plan and short initialization.
Check native identities against the supported target; record runtime tests
separately from compilation/emulation. Yield regularly, debounce input, write
settings only when necessary and keep borrowed handles valid across waits.
Document what happens after pause, death, vehicle changes and Kill Process.

Add a small example when exposing a new public feature. Give it an expected
observable result and a complete install/rollback path. Keep build logs, compiler
debug databases, private compiler inputs and game archives out of player ZIPs.
Do not add guessed PC natives or claim compatibility from a matching name alone.

## SDK and tools

Run doctor, build all public NRO examples, and test a freshly extracted SDK from
a path containing spaces. Check local Markdown links and all Python tool --help
commands. Test missing input, a stale index, damaged records, invalid UTF-8 and
malformed NRO/binding files when changing their respective tools/helpers.
NSC changes need compilation, exact-build header adaptation, name/native checks
and regenerated .snb metadata. The [NSC guide](NSC-MODDING.md) uses the established
MG build engine rather than maintaining a second adapter.

## Core changes

The [maintainer guide](DEVELOPING.md) lists private research/toolchain requirements.
The SDK ZIP alone cannot run target-instruction regressions: those need the
maintainer checkout's extracted images and emulator adapters. Production images
must never contain test-fixture entry points. Record a source manifest, module
hash, test scope and separate console acceptance. Retain ABI compatibility or
make the required break explicit.

Useful bug reports include the installed core/plugin hashes, exact game build,
loader report, startup policy, reproduction steps, observed/expected result and
whether the problem occurs with only that plugin enabled. Omit personal save
contents unless essential to a reported persistence failure.
