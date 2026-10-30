# Cooperative buffered file transfers

`shv/file_transfer.h` provides explicitly stepped transfers for larger XML,
configuration and catalog data. It uses the existing file and owner-memory
services without changing the runtime ABI or flat settings limits. Calls belong
to the plugin's runtime owner thread.

## Reading a complete bounded buffer

`ScriptHook::Data::FileReadTask` owns a buffer of the declared file length plus
one NUL byte. Construct it inside a registered `Managed` or `SessionLocal` object
when it must survive `WAIT`; ordinary suspended stack destructors do not run on
whole-plugin cancellation.

| Operation | Behavior |
| --- | --- |
| `Begin(path, maximum, chunk=16384)` | Validate services/path/extent and allocate an unpublished buffer |
| `Step()` | Read one chunk, or perform the final length check on a separate call |
| `Data()` / `Text()` | Return a borrow only when Complete; Text has an appended NUL |
| `Size()` / `Transferred()` | Return expected bytes and successfully transferred bytes |
| `Cancel()` | Release storage and enter Cancelled; source files remain unchanged |
| `Reset()` | Release storage and return to Idle; refuse if the owning heap refuses release |

Step results are Idle, Active, Complete, Failed or Cancelled. Begin is accepted
only from Idle; an attempted second Begin preserves the existing task. Reset
before starting another operation. A failed read retains its allocation until
Reset, Cancel or destruction, while Data/Text remain null. This prevents callers
from accidentally consuming partial data. Embedded NUL bytes are retained;
use Size when parsing binary data or length-delimited XML.

The maximum is explicit and cannot exceed the file service's 64 MiB bound.
Chunks are 1-65,536 bytes. An empty file is valid and still has an owned terminator.
Allocation can fail well below the file limit because owner/shared memory budgets
and process resources remain independent. Do not free the returned buffer or keep
a borrow across reset, cancellation or destruction.

## Preparing an output file

`PreparedFileWriteTask` keeps only a path, expected length and progress. It owns
no file handle, input pointer or heap buffer. Begin exclusively creates a new
fixed-length file; an existing destination is preserved.

| Operation | Behavior |
| --- | --- |
| `Begin(path, bytes)` | Create a new prepared output inside the plugin namespace |
| `Append(data, bytes)` | Write one caller-supplied nonempty span of at most 65,536 bytes |
| `Finish()` | Require all bytes supplied, then verify the current file length |
| `Cancel()` / `Reset()` | Stop or clear task state without deleting any output file |

Append does not retain its input pointer after the call. A span cannot exceed
the remaining expected bytes. Finish on incomplete output fails. Empty prepared
files can finish without Append. Completion verifies the extent and successful
write calls; it does not independently prove byte identity. Read back and validate
the prepared contents before using [FileReplacement](FILE-PUBLICATION.md).

Partial files remain after failure, cancellation, Kill or a restart. They are
never resumed, truncated, replaced or deleted automatically. Inspect retained
data and make an explicit recovery decision before reusing its path. Parent
directories must be created explicitly through the file service.

## Errors and execution limits

`Error()` distinguishes invalid arguments, unavailable services, caller-size limits,
allocation refusal, file errors, changed final length and incomplete work.
`FileStatus()` preserves the underlying `ShvNxFileStatus`; it remains Ok for
non-I/O failures such as allocation refusal or premature Finish. Inspect both
fields. Repeating Step/Append/Finish in a terminal state returns that state
without performing another file operation.
The independently provided [file verifier](XML-BUFFERS.md) also uses these states
and appends ContentMismatch to the error enum while preserving earlier values.

Use a single writer for a transfer. The final size check detects extent changes,
but cannot detect same-size edits or replacement between calls. There is no
snapshot isolation, persistent handle or atomic transaction. A write error may
follow partial or complete modification. Flush requests do not establish
power-loss durability.

Each Step, Append and Finish performs at most one file-service call.
Yield between calls and check the returned state. Begin performs size/allocation
or creation work synchronously. Byte bounds do not impose a wall-clock deadline;
storage can stall the game thread. XML parsing/serialization is a separate operation
and does not become incremental merely because file transfer is cooperative.

See the [large XML example](../../examples/large-xml-file/README.md) and
[verification record](../verification/FILE-TRANSFERS1.md).
