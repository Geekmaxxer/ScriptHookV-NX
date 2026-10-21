# Mod Porter layout audit — 2026-10-21

The 0.4.0 Preview layout update fixes the compressed build output and capped setup
form. Expanded project and asset forms scroll as complete pages. Findings stack
in narrow windows, large windows keep a bounded content width, and long paths
and status messages remain within their columns. Nested logs and lists hand the
mouse wheel to the page when they reach their scrolling limit.

## Validation

- Release build: no warnings or errors.
- Packaged Windows executable: 144 layout scenarios, 1,870 assertions passed,
  zero failures. See `layout-test.json`.
- Packaged executable with its bundled SDK: all 42 workflow checks passed.
  See `ui-test.json`.
- ZIP contents passed CRC and SHA-256 readback. See `package-verification.json`.
- The 10 updated source/documentation files match the GitHub checkout by SHA-256.
  See `source-sync.json`. Existing checkout changes were preserved.

The layout suite exercises all five navigation pages and their tools/asset
subviews, including DLC and replacement modes. Native windows are checked at
800×500, 920×680 and 1200×850, then maximized, minimized and restored. Expanded
sections and long path/log fixtures cover horizontal overflow, action reachability,
output height, nested scrolling and the busy footer.

Simulated display layouts cover 900×672, 1280×720, 1920×1080 and 2560×1440 at
100%, plus 125%, 150% and 200% combinations. These use explicit device-independent
canvas sizes and scaled raster captures; they do not establish physical
cross-monitor DPI transition behavior. The manifest requests PerMonitorV2 using
the [documented Windows DPI settings](https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process).

Workflow checks include import, filtering and source selection, project creation,
compiler checking, both NRO build actions, a real Footprints source build,
toolchain/native lookup, asset inspection/conversion, archive entry discovery,
updated archive creation and cancellation. Install/restore checks use a temporary
mock SD directory. No installed game archives or SD contents were modified.
Desktop validation does not establish Switch runtime compatibility of new assets.

## Reproduce

Run on Windows from the repository root. Choose new output directories each time.

```powershell
dotnet build desktop/ModPorter.App -c Release
& desktop/ModPorter.App/bin/Release/net10.0-windows/ScriptHookV-NX-Mod-Porter.exe `
  --layout-test <new-layout-output-directory>
& desktop/ModPorter.App/bin/Release/net10.0-windows/ScriptHookV-NX-Mod-Porter.exe `
  --ui-test <new-workflow-output-directory> --sdk <SDK-root> `
  --python <python.exe> --devkitpro <devkitPro-root> --asset-input <Legacy-asset-folder>
```

The layout suite uses in-memory fixtures and does not require a compiler or game
files. The workflow suite requires the configured build toolchain; an asset input
enables its conversion checks. Both modes return a nonzero exit code and write
`ui-test-failure.json` if a check fails. Test mode does not save application
preferences.
