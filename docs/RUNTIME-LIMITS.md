# Configure runtime limits

The standard defaults are **8 resident NROs, 16 cooperative script slots and an
8 MiB plugin accounting budget**. Each can be set independently. Changes take
effect after restarting GTA; rescanning plugins does not reload this file.

## Players

Create `switch/gtav-scripthook/runtime.ini` on the SD card. For example:

```ini
resident_limit=12
script_slots=24
plugin_memory_mib=12
```

The standard build accepts residents **1–16**, script slots **1–32** and budget
**1–64 MiB**. Omit any line to use that setting's compiled default. With no file,
all three use their compiled defaults. Delete the file to restore them.
An optional commented template is in `sdk/runtime.ini.example`.

Use decimal whole numbers. Blank lines, spaces, UTF-8 BOM, Windows line endings
and `#`/`;` comments are supported. Unknown keys are ignored. An invalid or
duplicate known key falls back to its compiled default independently of the
other keys. Unreadable files or files larger than 4096 bytes use all defaults.

More capacity does not make a mod faster or fix unsafe code. The plugin budget
caps image/BSS backing, mapped copies and registration pages; it is not a total
game-memory reservation. The 64 KiB stacks are accounted separately. Defaults
reserve 1 MiB of stacks. Requesting more than 16 slots allocates the additional
stacks once, up to another 1 MiB at 32. They remain allocated until game exit.
Allocation failure keeps the default slot count and is recorded in the report.
Lowering the count does not reclaim stacks compiled into the module.

Residents and runnable scripts are independent limits: a plugin can have more
than one cooperative script, and stopped/completed NROs remain resident until
GTA exits. A lower script or memory limit can prevent loading before the
resident count is reached. The catalog still lists at most 16 plugins.

## NRO and NSC mod developers

No C++ changes are needed to prepare a test configuration:

```powershell
python tools/configure_runtime.py --resident-limit 12 --script-slots 24 --plugin-memory-mib 12 --output runtime.ini
```

Supply only the flags you need; omitted settings stay absent from the generated
file. Existing files are preserved unless `--force` is explicitly supplied.
Give users the file as an optional configuration to review and merge with their
own. It controls the shared runtime, rather than one plugin. Plugins do not
silently raise global limits or overwrite another mod's configuration.

Generated `project.json` projects also accept an optional object:

```json
"runtime_limits": { "script_slots": 24, "plugin_memory_mib": 12 }
```

`build_mod.py` validates this object before compilation and emits `runtime.ini`
beside the NRO/NSC output. It does not install it. Existing ABI 1 NROs, menus and
`.snb` files remain compatible without rebuilding.

## Runtime builders

Set the three defaults in `include/runtime_defaults.h`, or override them for one
build without changing the checkout:

```powershell
python tools/build.py --devkitpro C:/devkitPro --output out/core-custom-test1 --resident-limit 12 --script-slots 24 --plugin-memory-mib 12
```

The builder snapshots the resulting header and records the actual defaults and
ceilings in `build-manifest.json`. The SD file overrides those defaults per key.
Compiled ceilings are also in the header: up to 16 resident catalog entries,
64 script entries and 256 MiB accounting are supported by the source guards.
The shipped ceilings are 16/32/64 MiB. Raising a ceiling requires rebuilding and
testing the runtime; an NRO's compiler flags cannot reconfigure `subsdk5`.

The loader reads configuration once on the first valid game-script tick,
before scanning/loading NROs. There is no file polling, extra worker thread or
change to the two-millisecond cooperative dispatch check. Early API-registered
scripts cannot be orphaned by a startup request to shrink their pool.

## Check the result

`switch/gtav-scripthook/loader-report.txt` reports effective `resident_limit`,
`script_slot_limit` and `plugin_memory_budget` (bytes), plus compiled defaults
and ceilings. Values are hexadecimal.

`runtime_config_state`: 1 means no file, 2 means read, 3 means read error.
`runtime_config_applied_mask` and `runtime_config_invalid_mask` use bit 0 for
residents, bit 1 for scripts and bit 2 for memory. `runtime_config_error` records
file errors; `runtime_config_stack_error` records stack allocation or early
registration conflicts. `script_stack_allocated_bytes` includes the base pool
and any added stacks. Configuration is a startup setting, so restart GTA when
testing a different file.

The configurable-limits runtime has local build/emulator coverage; this does
not claim Switch acceptance or stability at every larger configuration.
