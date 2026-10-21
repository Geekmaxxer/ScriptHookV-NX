# Install ScriptHookV-NX

Optional [runtime limits](RUNTIME-LIMITS.md) let you change resident NRO count,
script slots and plugin memory budget. No configuration file is required.

1. Close GTA. Back up any existing `exefs/subsdk5`.
2. Copy `atmosphere` from the runtime ZIP to your SD root. Install plugin files
   separately under `switch/gtav-scripthook/plugins/`.
3. Launch Story Mode. Keep your existing menu and Self Radio files installed.

This build supports title `0100b00b51230000`, game build
`6e3e9b3af5746e74b615bd981080c9271bc22662`.

## Add or disable mods

Put compatible ScriptHookV-NX mods in `switch/gtav-scripthook/plugins/`.

| Filename | Startup mode |
| --- | --- |
| `example.nro` | Auto: starts when Story Mode scripts run |
| `example.nro.manual` | Manual: waits for Load Now |
| `example.nro.disabled` | Disabled: cannot start until its mode changes |

Use MEGATARD's **Menu Settings > ScriptHookV-NX Plugins**, or the optional
standalone Plugin Manager, to select **Load Now**, change **Startup Mode**, or
**Rescan**. Mode changes save immediately. Load Now leaves the startup mode alone.
Running plugins stay loaded until GTA closes; changing their mode takes effect
on the next launch. Rescan discovers files without unloading running mods.

**Kill Process** stops the selected cooperative script without changing its
startup mode. Its code and native handlers remain mapped. **Load Now** can restart
a stopped or finished run entry. The current runtime calls a plugin's optional stop callback for owned-effect/entity
cleanup. Plugins without that callback retain their game changes. Kill does not
run C++ destructors or free a resident NRO slot; close GTA to replace code or release that slot.

Capacity Test 1 allows up to eight resident NROs, with the same combined 8 MiB
plugin accounting budget. Older published runtimes allow four; check the
`resident_limit` field in `loader-report.txt`. Current runtime lifecycle and
NSC menu management passed console checks; full-capacity stress testing is separate.
The optional standalone manager uses one of those slots. Completed plugins also
stay resident because other scripts may still call their custom natives.
Each mod needs a unique plugin ID. Keep one policy file per plugin; do not keep
both `example.nro` and `example.nro.manual`.

Mods may also supply `.snb` files for `switch/gtav-scripthook/bindings/`.
Keep those matched to the mod's compiled NSC. Windows `.asi` and `.dll` mods need
porting; copying them to this folder will not load them.

## If a mod fails

Check `switch/gtav-scripthook/loader-report.txt`. It lists the plugin filename,
loading step and readable error. Disable the failing plugin and restart GTA.
Keep settings under `switch/gtav-scripthook/settings/` when upgrading.

To remove ScriptHookV-NX, restore your backed-up `subsdk5`. Disable scripts that
require its custom natives before launching GTA without it.
