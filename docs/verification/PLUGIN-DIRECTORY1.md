# Plugin directory catalog candidate

This unreleased candidate adds paged directory catalogs through independent
catalog ABI 1. Local compiled execution and target review do not transfer prior
Switch acceptance to the new runtime. Existing accepted release artifacts remain
unchanged.

## Implementation and target review

- [x] Preserve the existing file table and base plugin/extension layouts.
- [x] Bind optional catalog services before explicit initialization.
- [x] Enumerate root/nested directories inside the owning plugin namespace.
- [x] Return complete names, entry kinds, file lengths and component compatibility.
- [x] Bound pages to 16 records and traversal to 4,096 native entries.
- [x] Close listing and root-validation handles before returning.
- [x] Reject malformed native metadata and incomplete/error pages explicitly.
- [x] Extend the cooperative streaming example with directory metadata checks.
- [x] Review target directory-mode validation with IDA Pro 9.5.
- [ ] Accept storage latency, gameplay transitions and extended use on Switch.

IDA review used an isolated copy of the existing SDK database. Reviewed bytes
were matched against the pinned decoded SDK image; the original database was
unchanged. The target accessor validates the low two directory-mode bits and
forwards mode 3 for files and directories. Existing native entry stride/type/size
evidence and the local libnx header provide the layout/type cross-check.
No target binding, binary offset or game native signature was added.

## Local verification

The compiled fixture exercises two owner namespaces, native-to-public layout
translation, multiple page capacities, EOF boundaries, short batches, full names,
unsupported component names, malformed metadata, the traversal limit, output
validation, I/O failures, foreign threads and Kill/Load. Every completed probe
checks that file handles, mounts and directory cursors were released.

The production runtime and NRO instructions execute locally. Filesystem, OS,
RO and outer game scheduling remain modeled. The retained
[verification summary](../../evidence/plugin-directory1-local-20261029/verification.json)
records artifact identities, completed counts and the bounded target review.

## Remaining acceptance

- [x] Verify nested file access and sample metadata on actual Switch storage.
- [x] Verify mixed/multi-page catalogs on actual Switch storage through batch 4.
- [ ] Exercise directory changes between pages and explicit rescan behavior.
- [ ] Test pause/loading/death/character changes with concurrent plugins.
- [ ] Measure directory traversal cost and game-frame stalls at larger counts.
- [ ] Complete extended-session and media-error acceptance.
- [ ] Define large-file replacement/recovery separately from enumeration.

The [public contract](../reference/DIRECTORY-CATALOG.md) documents current-order
pagination, repeated skip cost, output invalidation and absence of snapshot or
transaction guarantees.

The [physical service batch 1](SESSION-LOCAL1.md#physical-batch-1-result---2026-10-30)
independently verifies the full 256 KiB nested sample and the completed metadata
report. Its single-file catalog did not establish mixed or multi-page acceptance.
The later [batch 4 result](CATALOG-SCAN1.md#physical-batch-4-result---2026-10-30)
adds 14 sorted file/directory entries across four pages, case-insensitive XML
filtering and checked selection. Concurrent directory changes and catalog errors
retain separate acceptance requirements.
