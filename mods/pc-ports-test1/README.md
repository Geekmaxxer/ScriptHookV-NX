# PC gameplay ports - Test 2

Ten native C++ Switch adaptations of existing open-source PC mods. These are
separate NRO plugins, not asset conversions or ASI binary translations.
This revision refines controls, status displays and gameplay feedback. New
changes require Switch testing; local ARM64 tests use mocked game services.

Requires ScriptHookV-NX v1.4 Preview. Copy the test ZIP's `switch` folder to the
SD root with GTA closed. Every plugin starts **Manual**. Load it through the
plugin manager, close the manager, then use its shortcut below. **Hold L + R,
tap the listed button, then release all three before navigating.** Close one
mod menu before opening another. The old four-button shortcuts are replaced.

**Updating Test 1:** extract the new ZIP and run `install.cmd`,
choosing your SD root. It replaces the installed copy even if you changed its
startup mode to Auto or Disabled. For a manual update, replace the existing
NRO under its existing filename; avoid leaving an older `.nro` beside a new
`.nro.manual` copy. Preserve your settings folder.

These packages also include the shared SD-access `main.npdm`. `install.cmd`
preserves custom NPDM changes and enables SD access before updating plugins.
For a manual install with a standard NPDM, copy both atmosphere and switch.

| Order | Port / upstream | Menu: L + R + | What to test |
| --- | --- | --- | --- |
| 1 | [YASM NX](https://github.com/rikkamus/yasm) | D-Pad Left | Drive; change units, position and opacity; save and restart. |
| 2 | [Lost Wallets NX](https://github.com/root-cause/v-lost-wallets) | D-Pad Up | Check ON/OFF; use the manual wallet attempt near a pavement; collect its cash. |
| 3 | [Vehicle Control NX](https://github.com/ikt32/GTAVVehicleControl) | D-Pad Right | Drive a car to remember it automatically; exit; test engine, locks, doors, windows and hazards. |
| 4 | [SP Vehicle Mines NX](https://github.com/root-cause/v-spmines) | ZL | Check eligibility and mine upgrade status; press the normal vehicle horn to deploy. |
| 5 | [RC Bomb NX](https://github.com/root-cause/v-rcbomb) | ZR | Locate/create the workshop van; approach the van; deploy, detonate or return. |
| 6 | [Pizzaboy NX](https://github.com/root-cause/v-pizzaboy) | X | Locate a shop; start on foot; deliver boxes to the marked customers. |
| 7 | [SP Gun Locker NX](https://github.com/root-cause/v-spgunlocker) | A | Buy a locker; store/withdraw a weapon; restart and check persistence. |
| 8 | [Signal Jammer Hunt NX](https://github.com/root-cause/v-spjammerhunt) | B | Locate and destroy a jammer; check the reward and saved progress. |
| 9 | [Treasure Hunt NX](https://github.com/root-cause/v-sptreasurehunt) | Left-stick click | Begin; follow the note/clue waypoints; investigate nearby objectives. |
| 10 | [Los Santos Slasher NX](https://github.com/root-cause/v-spkiller) | Right-stick click | Investigate the clues and van; follow the visible night-ambush requirements. |

Use D-Pad to navigate, A to select and B to close. Buttons use Nintendo labels.
Each mod has a separate status badge and objective. Hold L + R to show its
shortcut; the menu's **Show status badge** preference is saved between launches.
L + R + D-Pad Down stays available to the plugin manager, and L + R + Y to Footprints.
Custom shortcut settings are described in [CONTROLS.md](CONTROLS.md).
The pizza bike uses the vehicle next/previous-radio controls to throw boxes;
the menu also provides explicit left/right throw actions.

## Test in two groups

The standard runtime holds eight resident NROs. Existing plugins also consume
slots, and **Kill Process does not release a resident slot**. Test a few ports,
close GTA, then test the next group. Use one mission/remote-driving mod at a time.
No runtime configuration, NSC menu, game archive or existing settings is replaced.

Installed DLC supplies the models and props used by the mission mods. Missing
models are reported with a bounded request timeout; no replacement asset is
silently substituted. The ports do not bundle game models, textures or audio.

Settings/progress/status files go in `switch/gtav-scripthook/settings/<plugin-id>/`.
Wallet ambient spawning defaults to the original **3% every 30 seconds**; this
can legitimately take minutes. Change its chance/interval or use its manual
wallet action for an immediate attempt. Wallet settings are saved automatically.
Check `status.txt` there and the normal `loader-report.txt` when reporting a problem.
For cash/inventory missions, keep a separate story save while testing.

## Source and builds

Each port includes `PORT.md`, `upstream.json` and the original MIT license.
The notes distinguish the adapted gameplay from PC-only features that are not
implemented. The original authors' work and licenses remain credited.

From the full source checkout, with Python and devkitA64/switch-tools installed:

```powershell
python mods/pc-ports-test1/build_ports.py --output out/pc-ports-test1 --devkitpro C:/devkitPro
```

The separate port-source ZIP includes the matching SDK files needed by this
command. Choose a new output directory for every build. No PC build script or
downloaded ASI is executed by this workflow.
