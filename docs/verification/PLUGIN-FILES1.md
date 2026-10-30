# Plugin file services candidate

This candidate adds optional file ABI 1 for isolated, bounded chunked access and
explicit nested directories. It is unreleased. The accepted runtime and existing
distribution packages are unchanged; physical Switch acceptance is pending.

## Implementation

- [x] Bind an independent sized/versioned table before plugin initialization.
- [x] Preserve base plugin/API and extension ABI 1 layouts.
- [x] Validate owner-thread access, relative paths, output pointers and extents.
- [x] Provide exclusive fixed-length creation, size queries and offset I/O.
- [x] Limit files to 64 MiB and each transfer to 64 KiB.
- [x] Close all handles and restore filesystem context before returning.
- [x] Verify existing directory paths through the audited directory-open call.
- [x] Provide a cooperative 256 KiB sample with explicit interruption behavior.
- [ ] Provide bounded directory enumeration and large-file replacement policies.
- [ ] Complete storage/performance/transition acceptance on physical Switch.

The implementation reuses the existing audited OpenFile, GetFileSize, ReadFile,
WriteFile, CreateFile, CreateDirectory and directory open/close bindings. It does
not add target offsets, native declarations or persistent handle resources.

## Local verification

`tests/test_plugin_files.py` compiles an NRO and executes it with the production
runtime's ARM64 instructions. Independent filesystem models verify offsets,
lengths, preserved bytes and directory types. Two modules exercise namespace
isolation. RO, OS, filesystem and outer game scheduling remain modeled.

Coverage includes EOF/empty files, maximum transfer lengths, path/component
boundaries, exclusive creation, invalid pointers, wrong threads, oversized file
reports, partial reads and open/read/write/size/directory failures. Each probe
checks that no handle, mount or directory cursor escaped. Example cases cover
completion across waits, verification-only restart, preserved corrupt input and
mid-stream Kill followed by restart.

The [retained verification summary](../../evidence/plugin-files1-local-20261029/verification.json)
records exact artifact identities and completed check counts. Build success and
modeled local execution do not establish actual SD timing, flush durability,
power-loss behavior or arbitrary gameplay compatibility.

## Hardware acceptance

- [ ] Confirm the exact module/SDK/plugin hashes and installation rollback.
- [ ] Read/write multi-chunk files in two plugin namespaces on Switch storage.
- [ ] Exercise EOF, existing-file protection and interruption/restart policy.
- [ ] Check pause/loading/death/character transitions and concurrent plugins.
- [ ] Measure frame cost, creation/flush stalls and extended-session behavior.
- [ ] Document media errors and recovery without claiming atomic multi-call I/O.

See the [public contract](../reference/FILE-SERVICES.md) for exact limits and
the [runtime roadmap](../PC-RUNTIME-ROADMAP.md) for remaining completion gates.
