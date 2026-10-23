# Compatibility and verification

The target is identified by the selected SDK's executable build fingerprint and
native index. A PC title-update number is not an equivalent Switch ABI.
Release date alone is not a compatibility test: an old plugin can still require
PC pointers and hooks, while a later native-only plugin may use available features.
Do not bypass version guards by pretending the Switch executable is a PC build.

SDK Parity Test 1 supplies multiple scripts, four world pools, globals/entity
pointers, PNG textures and input/render callbacks. The analyzer recognizes them;
PC indices, offsets, DirectX use and custom DllMain work still need adaptation.
See [extension contracts](../docs/EXTENSIONS.md). These additions require the
matched runtime. The combined probe has scoped Switch gameplay acceptance;
new ports still need independent testing.

## Real mod source checks

| Mod / pinned upstream revision | Result |
| --- | --- |
| [Footprints](https://github.com/root-cause/v-footprints/tree/750fc538a37364da655485decdfbdfbee45b83ad) | Source port built; Switch gameplay confirmed startup, L + R + Y activation and the flaming-footprint effect. |
| [RealisticReload](https://github.com/CamxxCore/RealisticReload/tree/cc902a6c0b6ab937a2320faffd331517358baeaa) | PC version checks, entity pointers and byte-pattern hooks are blockers. |
| [VaritrafV](https://github.com/ItsMeDeli/VaritrafV/tree/6ac844f16851a4512959c395862b066755e864a6) | PC hooks and dependencies are blockers. |
| [GTAVDisarm](https://github.com/ikt32/GTAVDisarm/tree/92d6646e0c649fb6c387189758abdcb72ab376cc) | Original PC pool helper remains a blocker. The bounded Disarm NX behavior port passed hand/weapon disarming on Switch. Test 2 arm-hit behavior also passed Switch gameplay testing. |

Footprints and Disarm NX have the gameplay acceptance described above. The other
entries are source analysis results. Footprints also needs its
particle asset on the installed game build. Its missing-asset path
reports failure rather than enabling the effect. The optional cleanup example passed Kill/Load lifecycle testing. INI override
combinations remain separate tests; older runtimes require toggling off before stopping.
Disarm NX uses the nearby scanner, with a 32-ped capacity and eight checks per
frame. It is not a global pedestrian-pool adapter. RealisticReload and VaritrafV
are not included as playable ports. Upstream sources are fetched only by the
optional verification script; Disarm NX contains a rewritten Switch implementation.

`verification/real_mods.py` pins revisions, records source hashes, checks expected
blockers and builds the Footprints and Disarm NX examples using the actual SDK/compiler.
No third-party build scripts or downloaded ASI binaries are executed.

`verification/disarm_tests.py` additionally checks the actual C++ Disarm source
against a host native simulator, including full array pointer ranges, failed
queries/outputs, damage ordering, character/death gates and processing budgets.

## Asset coverage

| Input | Supported operation |
| --- | --- |
| Legacy YTD/NTD v13, ordinary 2D textures | Inspect; retain smaller encoded mips; resize BC1/2/3/4/5/7 when no suitable mip exists. |
| Legacy YFT/NFT v162, YDR/NDR and YDD/NDD v165, YBN/NBN v43 | Check resource container; preserve payload; convert filename/platform references. |
| Other mapped Legacy resources; PSO/RBF metadata | Check recognized container and preserve payload; no claim of target-specific semantic validation. |
| OPEN RPF7, including nested archives | Validate table/bounds, convert supported entries, rebuild and read back. |
| XML/meta | Convert explicit platform/resource references while preserving definitions. |
| Embedded Legacy drawable/fragment dictionaries | Reduce typed root dictionaries; preserve shared geometry allocation. Physics-child graphs still need review. |
| Incompatible rigs/collections | Report; no automatic rig or collection conversion. |

Texture caps are maximum dimensions, not a guarantee of a memory or frame-rate
target. Encoded mip reuse avoids recompression; fallback BC encoding is lossy.
Normal-map channels are averaged independently; textures are not material-aware.
Uncompressed texture formats need existing smaller mip levels. Unsupported data
is rejected rather than replaced with a broken asset.

Local checks also cover a real Switch NTD with a missing mip chain and a real
nested DLC pack. The NTD test reduced graphics allocation by 144 KiB at a 256-pixel
cap. Game data used for verification is excluded from downloads.
