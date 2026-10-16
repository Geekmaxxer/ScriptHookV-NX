# Eight-plugin capacity - Test 1

Maintainer archive: `releases/capacity8-test1-20261016/ScriptHookV-NX-Eight-Plugins-Test1-EASY.zip`.
It is a module-only update; no game archive or replacement menu is included.

The current source raises the resident NRO cap from four to eight and the
cooperative script pool from eight to sixteen. The combined loader accounting
budget stays at 8 MiB. Catalog capacity stays at sixteen; image/BSS limits stay
at 2 MiB each. Caller, plugin, runtime and manager ABIs are unchanged.

Sixteen 64 KiB stacks reserve 1 MiB instead of 512 KiB. This fixed pool is
separate from plugin image/BSS accounting. The two-millisecond cooperative
dispatch check, per-frame loading, slot reservation, staged native publication,
memory checks and retained-code lifecycle remain in place. A large plugin can
hit the memory budget before the eight-image limit. Plugin count is not a CPU
budget: busy plugins still need to yield and avoid excessive per-frame work.

Existing compatible NROs, NSC menus, bindings and settings can be retained.
Neither the menu nor its `.snb` needs rebuilding for this capacity change. The
standalone manager uses one resident image; the NSC menu does not use an NRO
slot. Kill Process frees a cooperative script slot but keeps its image mapped.
Stopped or completed images still count toward the eight resident limit.

Local regression coverage exercises the ninth-image rejection, all sixteen
script stacks, seventeenth-script rejection, round-robin dispatch, two extra
cooperative scripts beside eight running plugins, memory-budget rejection, manager Kill/Restart,
native bindings and persistence. Local OS/RO/filesystem/game services are mocked;
Switch loader acceptance, gameplay and resource use require their own test.
The built-in Storage Self-Test retains its existing requirement that other
cooperative scripts be stopped before it starts.

The test download contains only `subsdk5`, a short guide, LICENSE and NOTICE.
Back up the currently installed `subsdk5`, replace it with GTA closed, then test
your existing plugins and menu. Do not install dummy duplicate NROs to fill slots;
the loader rejects duplicate plugin IDs. Restore the backup to roll back.

`loader-report.txt` identifies this build as eight-plugin capacity test 1 and
reports `resident_limit=0x8`, `script_slot_limit=0x10`,
`script_stack_bytes=0x10000` and `plugin_memory_budget=0x800000`. Nine enabled
small plugins should load eight and report `resident_plugin_limit_reached` for
the remainder. Memory-budget errors can legitimately occur earlier for large
plugins. The eight-plugin runtime is a test, with console acceptance pending.

The final module passed 387 core verification cases plus 105 existing-mod,
menu and portable-tool checks. All three NSO segment hashes and ZIP readback
were verified. Evidence and resource measurements in the maintainer workspace at
`evidence/capacity8-test1-20261016/README.md`
retain the exact tested module hash and input provenance; these are local checks,
not console acceptance.
