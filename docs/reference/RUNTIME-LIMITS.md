# Runtime configuration

`switch/gtav-scripthook/runtime.ini` sets startup capacity for the current
unreleased runtime candidate. The defaults remain eight resident NROs, sixteen
cooperative scripts, an 8 MiB plugin accounting budget and 2 MiB image/BSS limits.
Restart GTA after editing; rescanning plugins does not reload this file.

## Settings

| Key | Unit | Default |
| --- | --- | --- |
| `resident_limit` | Resident NROs | 8 |
| `script_slots` | Cooperative entries across all plugins | 16 |
| `plugin_memory_mib` | Shared plugin backing, heap and texture accounting, in MiB | 8 |
| `module_image_mib` | One NRO image, in MiB | 2 |
| `module_bss_mib` | One NRO BSS extent, in MiB | 2 |

Values are nonnegative decimal whole numbers. **Zero selects automatic capacity**
for that key. Positive values set an explicit limit. The parser and build tools
check integer representation and MiB-to-byte conversion; malformed or duplicate
known keys independently retain their compiled defaults. Missing keys use defaults.

```ini
resident_limit=12
script_slots=24
plugin_memory_mib=12
module_image_mib=3
module_bss_mib=4
```

A configuration can select automatic capacity independently:

```ini
resident_limit=0
script_slots=0
plugin_memory_mib=0
module_image_mib=0
module_bss_mib=0
```

Blank lines, UTF-8 BOM, spaces, CRLF and `#`/`;` comments are supported. Unknown
keys are ignored. An unreadable file or one larger than 4096 bytes uses defaults.
The runtime does not create or modify this shared configuration. An optional
commented template is supplied in `sdk/runtime.ini.example`.

## Allocation and lifetime

Installed-plugin catalogue size is independent of resident admission. Catalogue
metadata grows on demand; see [plugin catalogue and rescanning](PLUGIN-CATALOGUE.md).

Loader/catalog records, scheduler entries, additional-entry lists and heap/resource
owner records grow through stable metadata pages. Existing owner tokens, handler
addresses and suspended contexts remain valid when a table grows. New pages are
allocated on demand; a large configured count does not reserve every record or stack
at startup. Allocation failure refuses the new operation and retains existing work.

The bootstrap contains sixteen 64 KiB script stacks. Additional stacks allocate
when an entry is admitted and remain available for reuse until GTA exits. A lower
configured count does not reclaim bootstrap storage. Startup configuration cannot
orphan scripts or reservations registered before the first valid game tick.

Resident and script counts are independent. A plugin can own several entries;
completed/stopped NROs remain resident. Plugin image/BSS backing, registration
pages, heap arenas, additional heap/resource metadata and textures share
`plugin_memory_mib` accounting. Other core metadata pages and script stacks are
reported separately. [Heap metadata](HEAP-CAPACITY.md) also appears in the core
metadata counter; [resource-registry metadata](OWNED-RESOURCES.md) does likewise.
These overlapping views must not be added together. This budget is not
an estimate of total process or GPU memory. Free heap payload does not release
unused pool capacity until `scriptTrimMemory()`.

Public slot/reservation indices use their existing 32-bit encodings. NRO fields
retain their binary-format widths, byte accounting uses 64-bit sizes, and directory
traversal retains its separate 4096-entry validation bound. Callback, texture,
[heap-record](HEAP-CAPACITY.md) and other service-specific contracts remain independently defined.

## Generate and review configuration

```powershell
python tools/configure_runtime.py --resident-limit 12 --script-slots 24 --plugin-memory-mib 12 --output runtime.ini
```

Supply only the settings required for the workload. Existing output files are
preserved unless `--force` is supplied. Review and merge a mod's suggestions into
the shared file. NROs do not change global settings automatically.

Projects accept the same keys inside `runtime_limits`:

```json
"runtime_limits": { "script_slots": 24, "plugin_memory_mib": 12 }
```

`build_mod.py` validates the object and emits a configuration beside its output;
it does not install it. NRO builds record measured image/BSS extents, static backing
charges and minimum configuration requirements. Those measurements exclude future
heap/texture use, process overhead and measured gameplay performance.

Core builders can set compiled defaults with the corresponding command flags:

```powershell
python tools/build.py --devkitpro C:/devkitPro --output out/core-custom --resident-limit 12 --script-slots 24 --plugin-memory-mib 12
```

The builder snapshots `include/runtime_defaults.h` and records defaults/encoding
bounds in `build-manifest.json`. An installed older runtime retains its own
configuration contract; an SDK or configuration file does not upgrade it.

## Diagnostics and acceptance

`loader-report.txt` includes effective `resident_limit`, `script_slot_limit`,
`plugin_memory_budget`, `module_image_limit` and `module_bss_limit`. Byte quantities
are reported in bytes; default/ceiling MiB fields use MiB. Values are hexadecimal.

- `runtime_config_state`: 1 absent, 2 read, 3 read error.
- `runtime_config_applied_mask` / `runtime_config_invalid_mask`: bits 0 through 4
  correspond to residents, scripts, budget, image and BSS.
- `runtime_config_automatic_mask`: the same bits identify zero-valued settings.
- `runtime_config_stack_error`: the most recent stack admission/configuration error.
- `script_stack_allocated_bytes`: retained bootstrap and additional stack storage.
- `runtime_metadata_bytes` / `runtime_metadata_error`: additional core metadata
  page storage and the most recent metadata allocation result.
- `heap_metadata_bytes`, `heap_block_capacity`, `heap_arena_capacity` and
  `heap_lookup_capacity`: retained additional heap metadata and admitted table capacities.
- `resource_metadata_bytes`: retained additional cleanup record/owner pages; each
  plugin section includes its admitted `resource_capacity`.
- `report_clipped`: whether the bounded report omitted part of a larger catalog.

A stack or metadata admission failure does not rewrite the requested configuration.
Use the error and actual loaded/running counts when assessing a workload. A reported
encoding bound is not an accepted hardware configuration.

See [platform/capacity verification](../verification/PLATFORM-CAPACITY1.md) for the
exact local coverage and hardware checklist. Further substantial scheduler/loader
expansion is gated on physical acceptance of the current candidate.
