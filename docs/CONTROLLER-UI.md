# Reusable Switch mod menus

`shv/ui.h` is an optional header-only menu framework. It adds no runtime ABI,
worker thread, heap allocation or automatic settings writes.

Generate the complete example:

```powershell
python tools/create_mod.py --template controller-ui --id my-menu --output projects/my-menu
python tools/build_mod.py --project projects/my-menu/project.json --output out/my-menu --devkitpro C:/devkitPro
```

The demo opens with **L + R + D-Pad Left**. Use D-Pad to navigate, A to select,
B to return or close, and Left/Right for numbers and choices. Its settings page
has an explicit Save action. It changes only its own display settings.

## Frame structure

Keep these objects local in ScriptMain, outside the frame loop:

```cpp
ScriptHook::UI::Menu<> menu;
ScriptHook::UI::Controller controller;
ScriptHook::UI::Renderer renderer;
bool enabled = false;
int32_t strength = 50;

// In the frame loop, after pause/loading guards:
if (controller.TogglePressed()) menu.Toggle();
if (menu.IsOpen()) {
    auto keys = controller.Poll(uint32_t(MISC::GET_GAME_TIMER()));
    menu.Begin("My mod", keys);
    menu.Toggle(1, "Enabled", enabled, "Enable this mod's effect.");
    menu.Integer(2, "Strength", strength, 0, 100, 5);
    menu.Action(3, "Save settings", "Write settings only when requested.");
    menu.End();
    if (menu.Activated() == 3) { /* save with SavedSettings */ }
    renderer.Draw(menu);
}
```

Continue to call WAIT(0) in the outer loop. Draw and poll from a cooperative
script, never from a custom native, OS thread or static initializer.

`Changed()` identifies a changed setting; `Activated()` identifies an action.
Build pages based on `Page()`. `Submenu(rowId, label, pageId)` opens a page; B
restores the parent selection. Default capacity is 32 rows, 8 page histories
and 8 visible rows. Row capacity can be 1-64 and page capacity 1-16. Overflow,
duplicate/zero row IDs and invalid ranges make that frame invalid, without
dispatching actions. Use stable unique IDs within each page.

Labels, values and descriptions are copied with UTF-8 boundaries. Setting
pointers and choice strings must remain valid until End(). Disabled and read-only
rows are skipped. Number arithmetic saturates without signed overflow; choices
wrap. Direction repeats use 350 ms initially and 100 ms subsequently, without
catch-up bursts. Accept/Back use button edges.

## Layout and lifetime

Text widths and wrapped descriptions use the target game's text commands.
Measurements are cached until the text/layout changes. Long labels show an
ellipsis; descriptions wrap and show an ellipsis if they exceed the available
screen area. Use concise plain text; GTA rich-text layout tokens are outside
the helper's contract. Position, width and accent can be customized with Style.

The default menu and renderer together occupy less than 32 KiB of the standard
64 KiB script stack; leave room for your other locals and call depth. Declare
them once. Higher capacities need their own stack/resource assessment.

Controls are suppressed only while the open menu is polled. Close on pause or
loading transitions. Multiple independent menus need an agreed opening chord or
an explicit ownership policy; the framework does not arbitrate other mods.
The Controller constructor accepts a different `ScriptHook::Input::Button`
for its shoulder chord. Release the opening chord before navigating.
Kill Process stops per-frame HUD drawing and control suppression through the
normal runtime lifecycle. SavedSettings persists explicit saves across reloads.

UI model and actual ARM64 NRO checks are local tests. Appearance, controller
feel and coexistence need console acceptance for each new menu.
