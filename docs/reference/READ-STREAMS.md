# C file streams

The opt-in `stdio_runtime` profile supplies familiar C file operations
over the versioned file and resource services. It supports configuration and
metadata loaders, including the pinned XML DOM's `load_file`, without retaining
OS filesystem handles across cooperative waits. Output additionally requires
the candidate runtime's optional file-resize capability. Read-only streams remain
usable on earlier resource/file-capable kernels.

## Build and availability

```json
"c_runtime": true,
"stdio_runtime": true
```

Use `shv/stdio.h` for the supported declarations. The compiler's stdio declarations
provide an opaque `FILE*` type; the implementation allocates its own private
stream object. Never access FILE internals or mix pointers with another library's
stdio implementation. Headers such as `shv/xml.h` can coexist with this profile.
The builder records the enabled profile, source identities and stack-frame data.

Memory, file and resource services are required when opening a stream. Missing
optional services return null with ENOSYS. Operations use the plugin's permitted
owner thread. Paths remain relative to `settings/<plugin-id>/`, with the same
component/depth/character bounds as [file services](FILE-SERVICES.md). Absolute
paths, traversal and other plugins' data are refused. `fopen` creates no directory,
file for a read-only open. Output creation/truncation must be requested explicitly.

## Supported operations

| Operation | Contract |
| --- | --- |
| `fopen(path, "r")`, `fopen(path, "rb")` | Open an existing namespaced file as byte data; no CRLF or encoding conversion |
| `fopen` with `w`, `a`, `r+`, `w+`, `a+` | Explicit truncation, append or update; optional `b` before/after `+`; `x` is exclusive creation for `w` modes |
| `fread(out, size, count, stream)` | Return complete elements; a final partial element's bytes are consumed; check EOF/error separately |
| `fwrite(input, size, count, stream)` | Synchronously write complete chunks and return completed elements; a failed chunk may have changed bytes |
| `fputc`, `putc`, `fputs` | Write an unsigned byte or nonterminated string bytes; no newline/encoding conversion |
| `fflush(stream)` / `fflush(nullptr)` | No pending output buffer; validate the stream/owner and return success; no extra durability guarantee |
| `fprintf`, `vfprintf` | Use the [checked formatter](FORMATTED-TEXT.md), then write; decimal floats require the numeric profile |
| `fgetc`, `getc` | Return an unsigned byte promoted to int, or EOF |
| `fgets(out, capacity, stream)` | Read through newline or capacity-1 bytes and terminate; preserve CRLF; capacity 1 returns an empty string |
| `fseek(stream, offset, SEEK_SET/CUR/END)` | Checked signed 64-bit-long positioning; success clears EOF and invalidates the cache, preserving the error flag |
| `ftell` | Return the logical byte position, or -1 on refusal |
| `feof`, `ferror` | Inspect sticky stream indicators |
| `clearerr` | Clear both indicators without moving |
| `rewind` | Seek to zero and clear indicators on success |
| `fclose` | Consume the cleanup registration and free the stream; return EOF if preconditions refuse closure |

The underlying extent/position limit remains 64 MiB. Seeking beyond the current
EOF within that limit is permitted; reads then report EOF. Invalid origins,
negative positions and arithmetic overflow preserve the old position and cache.
Zero-size/count transfers do nothing. Multiplication overflow is refused before
transfer. Output spans must be writable, non-overlapping and large enough; trusted
native plugins remain responsible for valid caller storage.

Each stream has a 512-byte read buffer, one owned allocation and one cleanup
registration. Buffered character/line reads share the buffer. Large `fread` calls
remain synchronous and can issue many bounded filesystem operations; split large
workloads and yield explicitly. Buffering is not a latency or frame-time guarantee.

## EOF, errors and file changes

Reaching the end exactly does not set EOF until a read attempts to pass it. An EOF
indicator remains set until successful seek, rewind or clearerr. Error indicators
remain set until clearerr or successful rewind; callers may retry after transient
errors. Successful operations do not reset errno. Common failures map to EINVAL,
EBADF, ENOENT, ENOMEM, ENOSYS, EPERM, EOVERFLOW, EBUSY or EIO in the existing
NRO-local C-profile error state.

## Output, append and partial failures

Writable opens require `scriptFileResizeAvailable()` before any mutation.
`w` discards existing contents, `r+` requires an existing file, and `a` creates
a missing file or appends to an existing one. Exclusive `wx` creation preserves
an existing destination. The adapter reserves and registers stream storage before
truncating or creating files, so allocation/registration refusal preserves existing
bytes. Parent directories must already exist. `a+` starts reading at zero;
every append write refreshes EOF and writes there regardless of prior seeking.

Writes invalidate that stream's read cache. Update streams permit switching
directions at their current logical position; use seeking to select a new position.
Seeking beyond EOF followed by output explicitly writes zero bytes throughout
the hole, independent of raw resize semantics. Text mode remains byte-preserving.
Concurrent writers are not locked; append query/resize/write is not atomic.
Coordinate access to each path and do not assume consistency between separately
buffered streams.

Each write uses at most 64 KiB and requests the existing filesystem flush option.
No output is deferred to `fflush`, close or owner cleanup. A resize/write/readback
error may follow partial or complete physical modification. Complete earlier
chunks remain counted/consumed; a failing chunk is not counted. Inspect the file
before retrying when its integrity matters. Opening with `w` is intentionally
destructive and supplies no retained backup. Use prepared files and
[verified publication](FILE-TRANSFERS.md) for persistent presets.

Formatted output first uses a 512-byte stack buffer; larger results require one
temporary owner allocation of the full formatted length plus terminator. Invalid
formats and temporary-allocation failure write nothing. Write failures can still
partially modify output. Formatting and output are synchronous, including large
hole zeroing, and establish no frame-time bound. Split large work and yield.
Standard output/error streams, printf and formatted scanning remain unavailable.

The pinned XML DOM's direct `save_file` requires both `stdio_runtime` and
`cxx_runtime` (for its virtual deleting destructor). It truncates its destination
and performs synchronous writes; this is separate from the cooperative,
backup-preserving XML workflow.

Each refill verifies the current extent against its latest known extent. Size changes,
failed reads and short fills set an error; a failed fill publishes no bytes from
that fill. Bytes already delivered by earlier fills remain consumed. `fread`
returns the complete elements delivered before an error. `fgets` returns null on
a read error, including after a partial line; output contents are then unspecified.
When no byte is read at EOF, fgets returns null and leaves output unchanged.

This is not a snapshot: same-size replacement or modification can go undetected,
and previously cached bytes can differ from newly read disk contents. Do not edit
files being read. Use [verified transfer/publication](FILE-TRANSFERS.md) for a
workflow requiring independent validation and conservative recovery.

## Lifetime and compatibility

The stream is pinned while registered. `fclose` uses resource release; if a pending
raw native call prevents cleanup, it returns EOF/EBUSY with the stream still live.
Complete the call before retrying. Kill, natural completion and rejected
initialization drain unclosed streams automatically. Every borrow becomes invalid
after close/cancellation; an address reused by a later stream does not revive an
old pointer. Foreign and unrecognized pointers are refused before dereferencing
stream storage, but arbitrary memory corruption is outside the contract.

Only the operations above are implemented. Standard streams, descriptors, process I/O, formatted scanning,
ungetc and other hosted stdio facilities remain unavailable. Strict linking rejects
used unsupported symbols. The profile does not supply general hosted libc/STL,
threads or compiler-managed global initialization.

The [complete example](../../examples/read-streams/README.md) and
[verification record](../verification/READ-STREAMS1.md) identify local execution and
the original read-only workload. The [output example](../../examples/write-streams/README.md)
and [output verification](../verification/WRITE-STREAMS1.md) track the subsequent
candidate separately.
