# Build and contribute to the core

Run from any ScriptHookV-NX checkout. Python 3.10+, devkitA64 and switch-tools
are required. No extracted game files, parent workspace, libnx, hactool or menu
are needed for a normal build of the included audited target.
This page uses the full source checkout; the smaller NRO-only SDK ZIP starts at
[the plugin quickstart](START-HERE.md).

```powershell
python tools/doctor.py --core --devkitpro C:/devkitPro
python tools/test_portable.py
python tools/build.py --devkitpro C:/devkitPro --output out/core-test1
```

Alternatively set `DEVKITPRO` to the devkitPro root. Tools with and without
`.exe` are discovered; `/opt/devkitpro` is also recognized. Use explicit paths
when needed. Windows builds are tested; POSIX executable-name discovery is
covered locally, but this release does not claim an actual Linux/macOS build test.
Output paths may contain spaces. A previous output directory is never overwritten.
Use `--resident-limit`, `--script-slots` and `--plugin-memory-mib` to select
compiled defaults, or edit `include/runtime_defaults.h`. See
[runtime limits](RUNTIME-LIMITS.md) for startup overrides and bounded ceilings.

## Target configuration

The default profile lives under
`targets/6e3e9b3af5746e74b615bd981080c9271bc226620/`. It contains small audited
binding constants, code guards and provenance, not game executables or assets.
The profile and binding hashes are validated before creating build output.
`--target-profile path/to/profile` selects an explicitly audited profile.
There is no automatic compatibility with another game version: the runtime
checks target fingerprints before installing hooks.

For research rederivation of the currently supported target, supply both
`--target-main path/to/main.uncompressed.nso` and
`--target-sdk path/to/sdk.uncompressed.nso`. Their exact hashes and instruction
contracts must match the supported target. These inputs are local and are not
copied into builds or releases. A new porter build requires a fresh audit and
profile update, not removing these checks. `--menu` is an optional research-fixture
input that additionally requires those two images; normal production profiles
contain no menu identity.

## Verification and provenance

The builder snapshots source, public headers, tools and target metadata. It
links without unresolved imports, verifies all three NSO segment SHA256 hashes,
checks relative relocations/imports and writes `build-manifest.json` plus an
uncompressed analysis copy. The bounded LZ4 reader needs only Python's standard
library. `--hactool /path/to/hactool` requests an additional independent segment
check. The exported source does not bundle hactool or devkitPro.

```powershell
python tools/compare_core.py --reference targets/6e3e9b3af5746e74b615bd981080c9271bc226620/runtime-reference.json --module out/core-test1/subsdk5
```

This checks initialized code/data, BSS and memory layout against the accepted
v1.3 Runtime Completion Test 1 runtime. Only a structured 20-byte GNU build-ID note that
matches the NSO header is normalized. Debug source paths are mapped to stable
prefixes to make builds reproducible across output directories with the same
toolchain. A modified core should fail the reference comparison; that is expected
and requires its own validation. This comparison does not label a new binary
console-tested. Runtime downloads retain the exact accepted binary.
A different compiler version can also change code generation and fail that
comparison. It is a strict reference check, not a diagnosis of every mismatch.

`runtime-reference.json` identifies the console-accepted v1.3 completion runtime.
`runtime-reference-sdk-parity-test1.json` preserves the preceding SDK Parity runtime.
`runtime-reference-lifecycle.json` preserves the preceding lifecycle reference.
`runtime-reference-public-preview1.json` preserves the earlier reference for
historical comparisons. The target profile directory keeps its existing name;
the canonical 20-byte game build ID is the `build_id` value in profile.json.

## Public and research tests

`test_portable.py` exercises LZ4/NSO verification, corrupted profiles, executable
name resolution, project generation, path bounds and binding validation without
private files. `check_examples.py --output out/example-check1 --devkitpro ...`
builds the standalone NRO examples, including the controller UI and combined SDK extension
probe. Source mods build through build_plugin.
See [tools](TOOLS.md), [contributing](CONTRIBUTING.md) and [NSC modding](NSC-MODDING.md).

The original maintainer workspace also retains target-instruction emulator
regressions, game/SDK research inputs, packaging history and full evidence.
Those private-dependent harnesses are not advertised as runnable public tools or
included as broken scripts in the clean export. Public validation summaries
record their exact scope. Never include extracted images, game archives, private
compiler inputs or installed settings in a source/runtime release.

## Licensing

Use the [project attribution license](../LICENSE). Preserve [NOTICE](../NOTICE),
credit the original source visibly on distributions and identify material changes.
Read [licensing](LICENSING.md) before copying SDK implementation/example code.

Use [the maintenance checks](MAINTENANCE.md) for a single verification command,
public CI coverage and the console regression checklist.
