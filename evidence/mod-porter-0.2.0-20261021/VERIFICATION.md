# Mod Porter 0.2.0 verification

95 regression checks and 16 packaged desktop workflow checks passed, including both NRO build modes, metadata encodings,
nested archives, hostile paths and texture reconstruction.

Four pinned real PC mod sources were analyzed. Footprints received an explicit
controller/streaming adaptation and built as an AArch64 NRO. RealisticReload,
VaritrafV and GTAVDisarm retain reported source-porting blockers.

A real Legacy Switch texture dictionary was resized while retaining its BC format;
graphics allocation fell from 278,528 to 131,072 bytes. A real nested OPEN DLC pack
was rebuilt and read back. Original inputs were preserved and game data is excluded.

The source download rebuilt with zero warnings/errors. ZIP contents passed readback.
Switch gameplay confirmed Footprints startup, L + R + Y activation and the visible
flaming-footprint effect. Acceptance is scoped to the tested NRO hash in
[the console record](footprints-console-acceptance.json). Other ports and extended
lifecycle/performance checks remain pending. The core runtime is unchanged.

An extracted self-contained Windows package completed the workflow with a relocated
SDK and invalid DOTNET_ROOT override. Independent CodeWalker readback accepted the
rebuilt DLC (57 resource payload hashes unchanged) and resized NTD (two original
texture names, BC format preserved, dimensions and mip counts valid).
