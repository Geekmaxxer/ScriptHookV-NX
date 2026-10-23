# Plugin settings folders - Test 1

Each plugin now stores files in `switch/gtav-scripthook/settings/<plugin-id>/`.
Existing ABI 1 NROs use this layout without rebuilding. Legacy prefixed files move
on access using the verified filesystem rename export. Existing destination files
win; failed migration preserves the original and prevents replacement writes.
Generic NSC journals use `settings/nsc-<namespace-hash>/a.bin` and `b.bin` with
migration of both previous journal slots. MEGASAVE/MEGALOAD compatibility is unchanged.

The Porter places packaged INI defaults in the same mod directory. Upgrade the
runtime and run existing mods before extracting defaults over customized INIs.

| Verification | Result |
| --- | --- |
| Production ARM64 core checks | 467 passed, including 26 settings/migration checks |
| Plugin cleanup and restart checks | 19 passed |
| Existing concurrent showcase NROs | 46 passed across 217 emulated frames |
| Portable tools | 24 passed |
| Porter core and real compiler integration | 138 passed |
| Packaged Windows Porter UI workflow | 40 passed |
| NSO segments and ZIP readback | Passed |
| SD settings regrouping | 13 files in 8 directories, unchanged bytes verified |
| Switch gameplay | Pending |

ARM64 checks execute compiled runtime/plugin instructions with game, OS and
filesystem behavior mocked. They do not establish console runtime correctness.
The lifecycle report and final verification refer to the same runtime SHA-256.

Runtime SHA-256: `fee6cb157d5c3f72e275cc004923ecd09e17961b082f200d2d1f7c8b9b394f1a`. Runtime file: 62,451 bytes.
Static mapped span and BSS are unchanged from the previous runtime. No new worker
or periodic scan is introduced. Runtime reports identify `settings_layout=0x2`.

Console checks: load existing mods, confirm saved INI/options are restored, change
an option, restart GTA, and confirm the updated setting remains. Verify Kill
Process/Load Now and ordinary NSC menu persistence. Public versions are unchanged;
these artifacts are a test build. Previous releases remain intact.
