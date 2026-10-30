# Large Module Test 1

Date: 2026-10-29. Unreleased runtime/SDK candidate. No SD installation, public
release replacement or new Switch acceptance is performed.

The loader previously rejected every NRO image or BSS above 2 MiB regardless of
the configured shared budget. Both caps are now independent startup/build settings:
`module_image_mib` and `module_bss_mib`. Each accepts whole MiB values 1-32 in the
standard build. Defaults remain 2 MiB; the shared plugin budget remains 8 MiB.
Existing ABI 1 plugin descriptors, extension tables and caller layouts are unchanged.

## Capacity and accounting

Each setting caps one module, not all modules combined. Raising it does not raise
the shared budget or reserve RAM. The loader still accounts for backing pages,
mapped image/BSS copies and the NRR page:

```text
accounted bytes = 2 * (image bytes + max(BSS bytes, 4096)) + 4096
```

Other resident modules and charged texture resources share the configured budget.
Script stacks are separate. These charges are policy accounting, not measured
process/GPU peak usage. The 32 MiB per-module ceilings cannot both be filled under
the standard 64 MiB shared ceiling. A cap is a limit, not a supported-memory promise.

The loader checks the image cap before allocation and checks BSS after the verified
layout is read, before BSS allocation/RO registration. Malformed headers still fail.
Errors `0x10023` / `module_image_limit_exceeded` and `0x10024` /
`module_bss_limit_exceeded` distinguish these limits from the shared-budget error
`0x10016`. Config parsing retains independent duplicate/invalid-key fallback and
one-time startup application. Configuration changes require GTA restart.

Loader reports expose effective image/BSS byte limits, compiled defaults and
ceilings. Applied/invalid mask bits 3 and 4 correspond to image/BSS settings;
existing bits 0-2 retain their meaning. Report summaries explain capacity failures.

## Author workflow

`build_plugin.py` reports image bytes, BSS bytes, conservative accounted bytes,
minimum runtime settings and suggestions exceeding standard defaults in
`plugin-build.json`. A generated NRO `project.json` may specify either new key
inside `runtime_limits`. `build_mod.py` combines measured minimum suggestions with
explicit settings and emits an optional `runtime.ini` beside the NRO. It rejects
explicit settings smaller than the compiled module's requirements. It never
installs or silently edits the shared SD configuration.

These are minimum settings for this module's static charge. Other plugins,
textures and workloads need additional capacity. A failed build/validation output
is not a completed package. Preserve the build report when diagnosing rejection.

For example, a measured test module can be prepared with:

```powershell
python tools/configure_runtime.py --module-image-mib 3 --module-bss-mib 4 --plugin-memory-mib 16 --output runtime.ini
python tools/inspect_mod.py plugin.nro --module-image-mib 3 --module-bss-mib 4 --plugin-memory-mib 16
```

Standalone structural preflight keeps default caps unless explicitly overridden;
it reports the same accounting/minimums and checks an explicitly selected budget.
Core builders can set defaults with the matching `build.py` flags. Old published
runtimes do not implement the new keys and retain the 2 MiB caps. Use a matched
candidate runtime; an NRO's compiler settings cannot change the installed core.

## Local evidence

The real compiled fixture contains 2,109,440 image bytes and 3,149,824 BSS bytes.
Its static charge is 10,522,624 bytes; measured minimums are image 3 MiB, BSS 4 MiB
and shared accounting 11 MiB. The probe reads both ends of a 2 MiB read-only payload,
checks zeroed BSS, writes its high BSS address, preserves it across WAIT, and tests
Kill/Load cleanup/restart without new mappings or repeated initialization.

The production ARM64 loader, scheduler, shim, original native handlers and SDK
adapters execute locally. OS/filesystem/RO mapping are modeled. Tests also cover
default refusal, independent caps, duplicate/invalid config fallback, budget
enforcement, preflight agreement, incomplete/malformed headers and load failures.
The emulated module allocator now spaces large mappings independently of the old
2 MiB assumption; this is not a claimed Switch module-base layout.

Retained report and source inventory:
[`large-module-runtime1-local-20261029/verification.json`](../../evidence/large-module-runtime1-local-20261029/verification.json).
There are **437 passing local checks**: 35 new large-module cases, 376 existing
ARM64 startup/loader/scheduler/manager/native/capacity regressions and 26 portable
tool checks. No game inputs or installable binaries are
included in retained evidence. The previously accepted release is preserved.

## Required physical acceptance

Use the exact candidate hashes with a reviewed shared config and rollback copy.
Confirm default behavior with existing NSC/NRO mods; then run the measured large
fixture with larger caps. Test startup, pause/loading/character transitions,
Kill/Load, restart, concurrent mods, allocation/low-memory refusal and a long soak.
Measure process memory and frame-time cost. Local tests do not establish Horizon
RO acceptance or gameplay stability at the new ceilings.

This removes an artificial capacity blocker. General heap/STL, constructors,
streaming/nested data I/O, PC hooks/Windows services and full Menyoo remain separate
work in the [PC runtime roadmap](../PC-RUNTIME-ROADMAP.md).
