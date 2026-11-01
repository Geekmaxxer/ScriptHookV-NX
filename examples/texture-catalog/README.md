# Growing texture catalogue

This probe loads 80 PNG files, yielding after each file. It then displays five
pages of 16 rotating tiles over 360 game frames and completes naturally. A second
Load Now checks retained IDs. The runtime cancels queued sprites on completion
or Kill while retaining cached textures for later runs.

The example requires the growing-texture runtime candidate. Older runtimes
refuse files beyond their cache limits; capability bit 7 alone does not identify
the larger catalogue. Build the NRO with:

```powershell
python tools/build_mod.py --project examples/texture-catalog/project.json --output out/texture-catalog --devkitpro C:/devkitPro
python tools/create_texture_demo.py --catalogue --output out/texture-catalog-assets
```

The combined candidate kit supplies `tile00.png` through `tile79.png` under
`switch/gtav-scripthook/assets/texture-catalog-example/`. Each file is a small
generated RGBA palette. Copy the generated tiles to that asset directory and the
NRO to `plugins/texture-catalog.nro.manual`. The project does not include game assets.

## Combined acceptance

- [ ] With the candidate runtime installed, run the manual probe to completion.
- [ ] Confirm five distinct tile palettes, rotation, transparency and no stale sprites.
- [ ] Load again; confirm the same appearance and natural completion.
- [ ] Preserve `settings/texture-catalog-example/result.txt` and `loader-report.txt`.
- [ ] Verify `run=2`, `failed=0`, `loaded=80`, `completed_frames=360`,
      both ID checks equal 1, and `stop_count=2` after final cleanup.
- [ ] Check Kill during loading/display and restart within the same consolidated
      session; the cancelled run may legitimately have fewer than 360 frames.

Run the pending library-services workload in the same acceptance session. The
probe changes no entities or game saves. GPU allocation, visual output, physical
timing and long-session behavior remain hardware acceptance requirements. Local
ARM64 tests exercise decoding and catalogue behavior with modeled GPU operations.

See [texture verification](../../docs/verification/TEXTURE-CATALOG1.md).
