# Repository and current-game archive workflow verification

Completion Test 1 console acceptance covers the unchanged runtime, Disarm arm
hits, Footprints lifecycle and NSC plugin process controls. The exact runtime
rebuild matches the accepted SHA-256. See the separate console-acceptance.json
record under completion-test1-20261021; individual configuration combinations,
full-capacity stress and long-duration performance remain separate checks.

Mod Porter 0.4.0 adds selection of current RPFs from a mounted SD/game folder or
local file, add-on registration using the archive's current DLC list, and exact
existing asset replacements in new archive copies. Originals remain read-only.

Local checks: 136 core tests, 21 desktop workflow checks and 24 portable Python
checks passed. Generated fixtures cover unrelated-byte preservation, resource
replacement, XML list merging, stale source rejection, cancellation, encrypted
table rejection, path resolution and invalid input. The packaged Windows app
also passed its UI workflow with the bundled SDK.

A real 932,831,232-byte Self Radio update.rpf was copied and updated. CodeWalker
independently confirmed all 1,368 top-level entry identities and 71 existing DLC
registrations, with one added registration. Only seven bytes in the original
prefix changed, all in the selected table row. The replacement list was appended.
All other original bytes, including 14 encrypted nested archives, were retained.
Those nested archives were not decrypted; matching reader diagnostics and raw
byte comparison establish preservation rather than semantic validation of them.

The canonical game build ID display now uses the 20 bytes from the audited NSO
header. Historical profile-directory names and acceptance artifacts are retained.
No native hashes or runtime code changed in this repository update.

Game archive outputs have local structural/readback evidence, not Switch asset
gameplay acceptance. Release packages exclude game resources, executables,
installed settings, generated game-file outputs and private research inputs.
verification.json records package hashes and validation scope.
