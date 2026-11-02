# Bounded plugin file services

The unreleased file-service candidate adds chunked access and nested data paths
through the optional `ShvNxFiles` ABI 1 table. It uses the existing audited target
filesystem calls. Base plugin, native caller and extension ABI 1 layouts remain
unchanged. The published runtime does not provide this new table.

Include `plugin_files.h` or `script.h`. Check `scriptFilesAvailable()` during
explicit initialization. Service discovery is independent of the memory service;
these operations do not allocate plugin heap buffers.

## Operations

| Function | Contract |
| --- | --- |
| `scriptFileSize(path, &bytes)` | Return the current bounded file length |
| `scriptFileCreate(path, bytes)` | Exclusively create a fixed-length file; preserve an existing destination |
| `scriptDirectoryCreate(path)` | Create a directory or verify that an existing path opens as a directory |
| `scriptFileReadAt(path, offset, output, capacity, &bytes)` | Read at most one chunk, shortened at EOF |
| `scriptFileWriteAt(path, offset, data, bytes)` | Write and request flush within an existing file's length |

An empty file is valid. A read at EOF succeeds with zero bytes. An offset beyond
EOF is rejected. Writes cannot extend or truncate an existing file. A zero-byte
operation permits a null data pointer after the path and extent are validated.

Each bound call validates its output pointers and resets size/read-count outputs
to zero before path or I/O processing. Invalid output pointers are never written.
When the service is unavailable, the SDK returns `Unavailable` without touching
outputs. On a failed or incomplete read, the buffer may contain partial data;
only a successful call supplies a completed byte count. Callers that require
unchanged output must read into their own scratch buffer before publishing it.

## Bounds and ownership

- Each file is limited to 64 MiB; each read/write call is limited to 64 KiB.
- Each relative path has at most 79 characters and four components, each at most
  31 characters. ASCII letters, digits, underscores, hyphens and internal dots
  are allowed. Absolute paths, traversal, repeated/leading/trailing dots, empty
  components, backslashes, spaces and drive syntax are rejected.
- Paths resolve under `settings/<plugin-id>/`. Create parents explicitly, in
  order. No migration from legacy flat filenames occurs through this service.
- Calls belong to the plugin's existing owner thread, including initialization
  and bounded cleanup. Threads cannot borrow another plugin's namespace.
- Every operation mounts, performs bounded I/O, closes its file/directory handles
  and restores the thread's filesystem context before returning. No handles,
  directory cursors or heap buffers survive waits, return, Kill or Load.

Yield between chunks. Byte limits do not establish a maximum wall-clock delay:
media access, flushing and file creation can stall the calling game thread.
The limits apply per operation/file, not to aggregate disk use or disk quotas.

## Results and recovery

`ShvNxFileStatus` distinguishes success, unavailable service, invalid arguments,
wrong owner/thread, missing paths, existing destinations, extent violations,
other I/O failures and incomplete reads. Only the audited `0x202` and `0x402`
filesystem results are classified as missing/existing; other failures retain
the general I/O-error classification.

There is no multi-call snapshot, lock, atomic replacement or transaction. Another
writer can change a file between calls. A write error can occur after partial or
complete modification; successful flush is not power-loss durability evidence.
Exclusive creation protects an existing file but an interrupted new file can
remain incomplete. Recovery policy belongs to the plugin.

The original flat `scriptReadFile`/`scriptSaveFile` contracts and the
[recoverable record helper](RECOVERABLE-DATA.md) retain their existing limits and
behavior. An independent [catalog table](DIRECTORY-CATALOG.md) supplies directory
enumeration. A separate [mutation/publication service](FILE-PUBLICATION.md)
supplies checked rename/delete and explicit backup recovery. Atomic large-file
transactions and a hosted `fstream` implementation remain separate work. The
optional [C stream adapter](READ-STREAMS.md) adds checked reads/output/update/append;
output uses the separate mutation table's explicit resize capability.
The SDK's [cooperative file-transfer helpers](FILE-TRANSFERS.md) provide bounded
read buffers and explicit prepared output without changing these file-service limits.

See the [streamed example](../../examples/streamed-catalog/README.md) and
[verification scope](../verification/PLUGIN-FILES1.md). Physical Switch storage,
pause/transition behavior, measured frame cost and interruption recovery remain
required before promotion to an accepted release.
