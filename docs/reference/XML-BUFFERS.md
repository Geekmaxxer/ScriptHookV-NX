# Growable XML output and streamed file verification

These SDK helpers support the save side of larger XML/configuration ports using
the existing memory, file and owned-resource services. Runtime table layouts,
target bindings and flat settings limits remain unchanged.

## Owner-backed XML serialization

Include `shv/xml_buffer.h`, which selects the same pinned memory-DOM profile as
`shv/xml.h`. Do not include upstream pugixml headers first. Build with `c_runtime`;
use `cxx_runtime` when placing the writer inside Managed/SessionLocal state.

`ScriptHook::Xml::OwnedBufferWriter::Serialize(document, maximum)` grows an owner
allocation as needed, up to the caller's output-byte limit plus a terminator.
It returns true only when the complete UTF-8 serialization fits and all allocation
operations succeed. `Data`, `Text` and `Bytes` expose output only after success.
Text is NUL terminated; Bytes excludes the terminator.

Capacity grows geometrically and is retained for reuse. A later smaller maximum
still limits output bytes but does not shrink an existing allocation. Reset frees
the allocation and invalidates every borrow. A refused release preserves state.
When a later serialization fails, prior output is invalidated and partial output
remains private; capacity may remain allocated until Reset or destruction.

`Error()` distinguishes unavailable memory, invalid terminator-size limits,
output-limit exhaustion and allocation refusal. No throwing allocator, hosted
stream, filesystem write or global dynamic constructor is involved. Normal
destruction releases storage; registered ownership is required across suspended
scripts so Kill can also destroy it.

Serialization is a complete synchronous operation. Growable output does not
make DOM parsing, traversal or serialization preemptible. The document, input
copy, nodes and serialized buffer can coexist and count separately against
owner/shared memory limits. Raw XML output can exceed the flat settings API's
64 KiB limit; use the chunked file services for such data.

## Bounded file comparison

Include `shv/file_verify.h` and use `ScriptHook::Data::FileVerifyTask`. It owns
at most one configured chunk of read scratch storage; it does not retain the
expected-data pointer between calls or allocate a whole-file copy.

| Operation | Contract |
| --- | --- |
| `Begin(path, bytes, chunk=16384)` | Check the initial length and allocate scratch if nonempty |
| `Compare(expected, bytes)` | Read and compare one nonempty span at the current offset |
| `Finish()` | Require all bytes compared and independently recheck the final length |
| `Transferred()` | Number of fully matched bytes before any failure |
| `Cancel()` / `Reset()` | Release scratch; leave files unchanged |

Begin requires Idle. Chunks are 1-65,536 bytes; total length retains the 64 MiB
file-service bound. Compare spans must fit the configured chunk and remaining
length. Caller spans need remain valid only for the call. Empty files finish
without Compare or scratch allocation. Terminal operations return their state
without another file read. Reset permits reuse after completion or failure.

States/errors follow the [file-transfer contract](FILE-TRANSFERS.md).
`TransferError::ContentMismatch` is appended without changing earlier enum values;
it is reported with `ShvNxFile_VerificationFailed`. Short reads, missing files,
changed lengths, wrong-owner access and I/O failures remain distinct. Failed
comparisons retain scratch until explicit cleanup, and never modify the file.

Use the owner thread and yield between comparisons. A bounded call can still
stall on storage. Verification proves comparisons made during that sequence,
not snapshot isolation, atomic publication or immunity to later changes. Keep
a single writer and validate relevant application data before publication.

The [combined example](../../examples/xml-workflow/README.md) verifies prepared
XML before [publication/restoration](FILE-PUBLICATION.md), preserving the original
and the verified replacement. Its [verification record](../verification/XML-WORKFLOW1.md)
identifies actual local coverage and pending hardware work.
