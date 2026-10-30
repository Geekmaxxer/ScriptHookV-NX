# Recoverable binary and XML records

`shv/blob_store.h` provides bounded, checksummed two-slot storage through the
existing isolated settings API. It supports binary payloads and serialized XML
without adding filesystem signatures or changing the runtime ABI.

This is an unreleased SDK service requiring the candidate owner heap. Local
compiled tests cover recovery and failure behavior. Physical Switch storage and
power-loss durability remain unverified.

## Interface

```cpp
#include "shv/blob_store.h"

void LoadConfiguration() {
    ScriptHook::Data::BlobStore records(1, "config-a.bin", "config-b.bin");
    char data[1024]; size_t bytes = 0;
    auto status = records.Load(data, sizeof(data), bytes);
    // Interpret or parse data only when status is BlobStatus::Ok.
}
```

| Operation | Behavior |
| --- | --- |
| `Load(output, capacity, bytes, generation)` | Select and revalidate the newest valid record; copy only after complete checks |
| `Save(payload, bytes)` | Reprobe both records, write the inactive slot and verify header, CRC and payload readback |
| `Initialize(payload, bytes)` | Explicitly create both slots only when neither validates; refuse an existing valid record |
| `Repair(payload, bytes)` | Explicitly write the peer of a known-valid record, permitting recovery when that peer is unreadable |

All operations are synchronous on the permitted owner thread. Payloads may be
empty and are limited to **65,504 bytes**. The 32-byte header keeps every record
within the existing 65,536-byte raw write limit. Filenames must be distinct,
flat ASCII basenames up to 63 bytes. Duplicate names are rejected case-insensitively;
path components, leading/trailing dots and consecutive dots are rejected.

Each operation allocates one 64 KiB scratch buffer from the owner heap and frees
it before returning. Empty arenas may remain until Trim. Caller buffers must be
valid for their declared lengths. No automatic writes occur in destructors and
operations must not yield or run concurrently against the same record pair.

## Failure and recovery policy

`Load` leaves payload output unchanged on failure and sets byte count/generation
outputs to zero. A small output buffer returns `BufferTooSmall`. Schema, framing,
size, checksum and generation selection are checked before payload use.

The legacy file API returns a boolean. It cannot establish whether an unreadable
slot is missing, unavailable or affected by media errors. Consequently:

- Normal Save requires both slots to be readable and at least one valid record.
- Load may recover the remaining valid record when its peer is unreadable/corrupt.
- Initialize is explicit authorization to replace invalid or unreadable records
  when neither validates. It must not be an unconditional error fallback.
- Repair explicitly trusts a known-valid record and replaces its peer. It may
  discard a newer but unreadable peer, so recovery policy belongs to the mod.

Malformed but successfully read records may be replaced by a normal Save when the
other record validates. The latest known-valid slot is preserved during that write.
Failures during creation of the second initial slot can leave one valid record;
Load can recover it, followed by an explicitly authorized Repair.

The returned statuses are `Ok`, `InvalidArgument`, `NoMemory`, `NoValidRecord`,
`ReadFailed`, `WriteFailed`, `VerificationFailed`, `BufferTooSmall`, `Conflict`
and `AlreadyExists`. `NoValidRecord` does not mean both files are absent.

A failed write acknowledgement/readback does not prove the write never completed.
A subsequent Load may select the newly completed record. Every Save reprobes disk
rather than trusting cached generation state. Callers requiring exactly-once
application actions need a separate application-level protocol.

## Record format

| Offset | Field |
| --- | --- |
| 0 | Four-byte `NXB1` magic |
| 4 | Little-endian uint32 format version, currently 1 |
| 8 | Little-endian uint32 application schema |
| 12 | Little-endian uint64 generation |
| 20 | Little-endian uint32 payload length |
| 24 | Reserved uint32, zero |
| 28 | CRC32 of header bytes 0-27 followed by payload |
| 32 | Payload bytes |

Generation comparison uses unsigned serial arithmetic and supports wraparound.
A half-range difference, or unequal checked records sharing a generation, returns
Conflict. CRC32 detects accidental corruption; it is not authentication or
protection against deliberate collision construction. Use a new schema or an
explicit validated migration when application data changes.

## XML workflow and acceptance

The [complete example](../../examples/recoverable-data/README.md) initializes its
own sample namespace explicitly, loads XML, updates a counter, saves and verifies
the recovered payload. It exercises normal return, pool cleanup and restart.
XML parsing, application validation and binary record validity remain separate checks.

Checksummed alternating records improve recoverability from modeled partial and
corrupt writes. They do not establish filesystem atomicity, SD-controller ordering
or durability after physical power loss. Streaming, directories and larger files
remain separate services. See the [verification record](../verification/RECOVERABLE-DATA1.md)
for actual local scope and open hardware acceptance.
