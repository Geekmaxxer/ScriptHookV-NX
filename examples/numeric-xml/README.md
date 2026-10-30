# Numeric XML example

Requires the unreleased plugin-memory runtime and the current numeric SDK profile.
Build from the repository or copied source SDK:

```powershell
python tools/build_mod.py --project examples/numeric-xml/project.json --output out/numeric-xml --devkitpro C:/devkitPro
```

The plugin parses a small in-memory pose, validates its decimal coordinate,
updates `x` to 124.5 and adds a float heading of 90.125. It writes `pose.xml` and
`result.txt` under `switch/gtav-scripthook/settings/numeric-xml-example/`, then
returns normally. Cleanup trims the unused DOM pool. No world state is changed.
`result.txt` should contain `saved=yes`; XML includes `x="124.5"`, unchanged
`y="-0.03125"` and `<Heading>90.125</Heading>`.

Source/library execution and modeled settings writes are locally checked. New
Switch gameplay acceptance remains pending. The ordinary flat-path, 64 KiB write
and raw single-file durability contracts still apply. Distribute required MIT
notices with a plugin containing pugixml/fast_float; source notices and Ryu's Boost
license are in `sdk/third_party/` and `THIRD-PARTY-NOTICES.txt`.

See [numeric contracts](../../docs/verification/PLUGIN-NUMERIC1.md) for strict parsing versus
permissive XML access, precision limits and hardware requirements.
