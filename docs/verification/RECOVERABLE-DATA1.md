# Recoverable data verification

Date: 2026-10-29. Unreleased SDK candidate using the unchanged owner-heap runtime.
The public interface is documented in [recoverable records](../reference/RECOVERABLE-DATA.md).

## Implementation coverage

- [x] Implement bounded variable payloads and an explicit little-endian/CRC32 format.
- [x] Select/revalidate records and preserve caller data on failed loads.
- [x] Alternate writes and verify complete payload readback.
- [x] Refuse ordinary saves to unreadable peer slots.
- [x] Provide explicit initialization and repair operations.
- [x] Exercise a complete XML persistence and restart example.
- [ ] Validate actual Switch media failures, filesystem ordering and power loss.

## Local execution

77 blob checks, five XML workflow checks, 540 C++ regressions, 2,689 numeric
regressions, 34 portable checks and 196 Mod Porter backend checks pass (3,541
behavior checks). Four desktop compiler groups were explicitly skipped; separate
public-example compilation is recorded independently.

The compiled blob suite executes the production shim/heap/runtime and real NROs.
Its independent Python serializer/CRC oracle checks framing, all header-byte
corruptions, truncation, size/schema mismatch, generation wrap/conflict, failed/
partial/corrupt writes, failed acknowledgement, missing/unreadable peers, ownership,
memory exhaustion and restart. A maximum payload produces a 65,536-byte record.

The example independently parses the exported XML, repeats Load Now, corrupts the
newest sample record and checks allocation-failure preservation/recovery. OS
allocation, filesystem and RO boundaries remain modeled. These results do not
establish actual storage timing or power-loss durability.

Repository reorganization is verified separately: maintained documentation links,
public source export, compiler examples, portable checks and Mod Porter backend
tests. Historical evidence stays immutable; exported historical Markdown copies
may relocate navigation links to moved guides.

The curated checkout also builds outside the maintainer workspace under an
arbitrary folder name. Two core builds reproduce the checked candidate exactly;
the baseline accepted runtime remains different and unchanged. Public command help,
all 12 example builds, archive readback and repeat-export equality are checked.

## Evidence and remaining acceptance

Retained summaries and artifact/source identities are recorded in
[`verification.json`](../../evidence/repository-storage1-local-20261029/verification.json).
No accepted runtime, installed package or public release is replaced.

- [ ] Test actual Horizon filesystem/RO behavior with the matched runtime.
- [ ] Validate low-memory, fragmentation and cleanup with concurrent mods.
- [ ] Exercise loading, pause, death, character changes and extended sessions.
- [ ] Measure RAM, synchronous operation latency and frame-time effects.
- [ ] Test storage interruption/recovery using disposable test data.
