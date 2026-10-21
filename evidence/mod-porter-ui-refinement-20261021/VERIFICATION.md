# Mod Porter UI refinement

The 0.4.0 UI test build uses compact side navigation, separate asset preparation,
game update and report views, and consistent dark controls. Primary asset and DLC
actions remain visible at the minimum 920 x 680 window size. Source findings support
text and severity filters. Build output has a concise view and an optional full log.

The packaged Windows executable passed 42 automated workflow and layout checks
after extraction to a directory containing spaces. Checks used the packaged SDK
and examples, including real Footprints source, and covered both NRO build actions,
compiler checks, native lookup, real asset conversion, mock-SD install/restore,
archive entry selection, long paths, empty filters, busy state and cancellation.
Rendered screens were inspected at 1210 x 850 and 920 x 680.

The desktop build completed without warnings or errors. Windows and source ZIP
entries were read back and compared by SHA-256. No game files or core runtime are
included. This is a UI test under version 0.4.0; no new Switch runtime was produced.
Interactive controller behavior, game rendering and console performance were not
part of this desktop UI verification. Real SD installation was not performed.
