# JSON documents and presets

The opt-in JSON profile provides owner-accounted documents, mutable copies,
serialization and cooperative file loading for source-port configuration and
presets. It uses pinned yyjson 0.13.0 with an explicit allocator. No new kernel
table, native binding or configuration default is introduced.

## Build and dependency

```json
"c_runtime": true,
"json_runtime": true
```

Include `shv/json.h` before the upstream header, or `shv/json_file.h` for file
loading. C++ registered examples additionally enable `cxx_runtime`. Numeric,
math and stdio profiles are independent. The JSON library supplies its own
number conversion; enabling JSON does not enable general C decimal conversion.

The unmodified parser/writer and MIT license are pinned to commit
`6447536015f3d600f3d65323b10976103b337ca7`. See
[upstream release](https://github.com/ibireme/yyjson/releases/tag/0.13.0) and
`sdk/third_party/yyjson/provenance.json`. Builders record source hashes and the
selected profile. Binary kits preserve its license/provenance. The profile
disables upstream filesystem APIs, incremental parsing, pointer/patch utilities
and non-standard syntax. Omitted facilities are not provided by this profile.

## Documents and failure preservation

| Operation | Contract |
| --- | --- |
| `Document::Parse(data, bytes, memory_limit)` | Parse a complete owned document; failed parsing preserves the prior document |
| `CreateObject(memory_limit)` | Replace with a new mutable object only after successful allocation |
| `CopyFrom(source, memory_limit)` | Create an independent mutable copy of an immutable or mutable source |
| `ReadRoot()` | Borrow the immutable root, or null when empty/mutable |
| `EditRoot()`, `NativeMutable()` | Borrow the mutable root/document for checked pinned-library operations |
| `SetUInt`, `SetInt`, `SetBool`, `SetNull` | Copy a key and put a typed field into a mutable root object |
| `SetReal` | Put a finite double; NaN and infinities are refused before mutation |
| `SetString(key, bytes, length)` | Copy explicit string bytes, including embedded NUL; serialization validates UTF-8 |
| `MemoryBytes`, `PeakBytes` | Current/peak requested bytes for this document, including its allocator-state allocation |
| `LastError`, `Reset`, move | Inspect the last operation, release owned data, or transfer the stable allocator context |

Object puts replace matching keys and may change member order. Failed value/key
allocation does not publish a replacement, but successfully allocated pool space
can remain retained until Reset. Native mutable operations are checked through
their upstream return values; the wrapper cannot recover from arbitrary invalid
nodes, cycles, ownership mixing or corruption introduced through raw access.

Input must be a complete readable span. The default reader copies into owned
storage; it retains no input pointer. The input extent is limited to 64 MiB and
the default document limit is 1 MiB of requested memory. Memory, arithmetic,
syntax, UTF-8 and depth failures are reported separately where available;
LastError includes the upstream error code and read position.

Parsing uses strict JSON syntax and UTF-8 checks, with at most 64 nested containers.
Comments, trailing commas, single quotes, BOM, invalid Unicode and non-finite
numbers are refused. Signed/unsigned 64-bit integer types are retained exactly
within their ranges; larger numbers may convert to double or fail when out of
range. This is not arbitrary-precision numeric storage. Duplicate object keys
can be retained by the parser; upstream lookup finds the first match. Validate
uniqueness as an application policy before consuming ambiguous configuration.

## Serialization and allocation limits

`Buffer::Serialize(document, maximum_bytes, memory_limit)` produces a complete
NUL-terminated UTF-8 buffer. Bytes excludes the terminator. Failure preserves
the prior buffer. Both immutable and mutable documents are supported. Invalid
Unicode, non-finite raw mutable values and a 65th nested container are refused.

Each document/buffer has its own stable allocator context. Admission counts its
state and library-requested blocks, including overlapping replacements during
reallocation. Allocation refusal preserves the old block. Limits exclude caller
input, other documents/buffers, outer containing objects and heap arena/metadata
overhead; all those remain subject to the shared runtime budget independently.
Copying or replacing a document may temporarily hold both old and new contexts.

The output-size limit is checked after serialization into a memory-bounded
candidate. Select sufficient memory for temporary writer buffers as well as the
final text. Current bytes can be lower than peak bytes. Mutable pools retain
unused/replaced values and strings until their document is released.

Parsing, copying, editing and serialization are synchronous. A nesting policy
and per-function stack report do not establish full call-chain stack usage or
physical frame time. Bound input/work and yield between larger processing stages.

## Cooperative file loading

`Json::FileLoadTask` combines bounded `FileReadTask` transfers with a separate
parse stage. It owns the input buffer and the last successfully parsed Document.

| Operation | Contract |
| --- | --- |
| `Begin(path, maximum_bytes, memory_limit, chunk)` | Start from an idle/terminal state; retain the last complete document |
| `Step()` | Perform one bounded transfer operation or one synchronous parse |
| `State()` | Idle, Reading, Parsing, Complete, Failed or Cancelled |
| `Current()` | Borrow the last successfully parsed document; inspect State before treating it as the new result |
| `LastError`, `TransferError`, `FileStatus` | Inspect JSON and transfer failures separately |
| `Transferred()` | Bytes transferred in the latest operation, retained after completion/cancellation |
| `Cancel`, `Reset` | Cancel the transfer while retaining the last document, or release both |

A successful parse releases the transfer buffer and replaces Current. Failed
parsing preserves Current and releases that candidate input. Failed I/O can retain
its private partial buffer until Cancel/Reset/retry/destruction. Retrying from a
terminal state resets transfer state. The reader checks extent changes; same-size
concurrent edits are not a snapshot guarantee. Data paths retain the existing
namespaced file-service bounds. No OS file handle survives a step.

Use registered ownership for documents/tasks that must survive WAIT and be
reclaimed on whole-plugin cancellation. Borrows become invalid after replacement,
Reset, cancellation cleanup or game exit. Mutations and destructors must run on
the permitted owner thread without yielding or reentrant mutation. Plain stack
locals are not unwound after Kill.

## Verified publication

The [complete preset example](../../examples/json-presets/README.md) combines
mutable edits with prepared writes, complete byte verification,
[backup-preserving publication](FILE-PUBLICATION.md) and explicit restoration.
JSON APIs themselves perform no file writes or implicit recovery. Retain
conflicting prepared/backup files and inspect state after interrupted mutations.
Serialization success does not establish SD durability or schema correctness.

The [verification record](../verification/JSON-SERVICES1.md) identifies local
coverage and the combined pending hardware workload. This profile does not
supply nlohmann/json compatibility, hosted STL, Windows services, exception
unwinding or arbitrary-mod acceptance.
