# Standalone plugin manager

Use this optional NRO when your NSC menu has no plugin manager. It uses one resident plugin slot: Capacity Test 1 allows eight total, while earlier public runtimes allow four.

Hold **L + R**, then press **D-pad Down** to open. Use Up/Down to select, Left/Right to choose a startup policy, **A** to save that policy, **X** to load the selected plugin now, **Y** to rescan, and **B** to close. Close other mod menus first.

**Auto** loads at game startup. **Manual** waits for Load Now. **Disabled** does not load. A loaded plugin stays resident until GTA closes; changing its policy never unmaps running code.

The C++ manager helper also exposes KillProcess/IsRunning for custom UIs. This
standalone UI maps X to Load Now; it has no separate Kill button. MEGATARD exposes
Kill Process in its plugin detail page. Cancellation does not free a resident slot.

Build from the project root:

```powershell
python tools/build_plugin.py --source mods/plugin-manager/main.cpp --include mods/showcase-test1/common --output build/plugin-manager-new-test
```

The SDK helper is `include/plugin_manager.h`. The integration check is `tools/test_standalone_manager.py`; it tests the actual ARM64 NRO with mocked game/input/OS services. Console testing is separate.
