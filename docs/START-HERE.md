# Make your first Switch mod

The NRO SDK needs **Python 3.10+**, **devkitA64** and **switch-tools** from
devkitPro. It includes its own freestanding shim: libnx, a Windows SDK, game
extractions and the core build toolchain are not needed to build NRO plugins.
Run these commands from the extracted SDK folder. Paths containing spaces work.

```powershell
python tools/doctor.py --devkitpro C:/devkitPro
python tools/create_mod.py --template hello --id my-first-mod --output projects/my-first-mod
python tools/build_mod.py --project projects/my-first-mod/project.json --output out/my-first-mod-test1 --devkitpro C:/devkitPro
python tools/inspect_mod.py out/my-first-mod-test1/plugin.nro
```

With GTA closed, copy `plugin.nro` to
`switch/gtav-scripthook/plugins/my-first-mod.nro`. Launch Story Mode with a
compatible ScriptHookV-NX runtime installed. The example writes
`switch/gtav-scripthook/settings/my-first-mod-hello.txt` and finishes. Finishing
is normal: the NRO remains mapped, but its script uses no scheduler stack.
Build into `out/my-first-mod-test2` after editing; close GTA before replacing
the installed file. See [installation](INSTALL.md) for the runtime and startup modes.

Try [the tutorials](TUTORIALS.md) next. Use [native search](TOOLS.md) to find
signatures, the [external API reference](API-REFERENCE.md) for supported calls,
and read [porting](PORTING.md) before adapting a PC plugin.
For SanScript use [the NSC workflow](NSC-MODDING.md); it needs a separate compiler
and the MG-SwitchMenu SDK. Neither is silently downloaded or bundled here.

## Which files matter?

| File/folder | Use |
| --- | --- |
| `include/script.h` | ScriptMain, native namespaces, WAIT and entry macro |
| `include/shv/` | Optional bounded text, timers and journaled settings helpers |
| `include/plugin_manager.h` | Optional standalone manager interface |
| `examples/tutorials/` | Five small NRO and two NSC lessons |
| `examples/public_plugin.cpp` | Advanced provider and seven-check diagnostic sample |
| `mods/showcase-test1/` | Drive HUD, Pit Stop and Stunt Journal sources |
| `mods/plugin-manager/` | Standalone manager source; no NSC menu dependency |
| `sdk/commands_scripthook_nx.sch` | Public SanScript native declarations |
| `docs/` | User, developer and maintainer guides |

The SDK refinement keeps ABI 1 and the accepted runtime unchanged. New tutorial
code is locally built/tested; it still needs its own console gameplay tests.

The [attribution license](../LICENSE) permits use, edits, distribution and sale
with original-source credit. See [licensing](LICENSING.md) when sharing SDK code.
