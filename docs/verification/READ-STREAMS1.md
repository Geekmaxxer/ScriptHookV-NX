# Read-only streams and expanded service batch 8

## Physical Batch 8 result - 2026-11-02

The [combined acceptance](SERVICE-BATCH8.md) verifies this exact probe's latest
normal completion and final owner cleanup under explicit 16/48/16/3/3 settings.
Repeated loads and normal gameplay performance are operator observations. Earlier
per-run files and the final test-session loader report were not retained.
Cancellation, broader transitions, profile/media failures and measured performance
remain separately open. Historical local records retain their original scope.

## Original implementation record

Date: 2026-11-01. This SDK milestone adds opt-in read-only C file streams through
the existing memory, file and resource services. The pending growing-resource
kernel remains byte-identical; no target bindings, public ABI or defaults change.
The [stream contract](../reference/READ-STREAMS.md) defines the supported subset,
namespace, error/position behavior, buffering and lifecycle.

## Implementation and local verification

- [x] Supply buffered binary/line reads, checked positioning and EOF/error state.
- [x] Refuse write/update modes, unsafe paths, foreign streams and wrong-thread calls.
- [x] Close registered stream objects on natural completion, Kill and rejected initialization.
- [x] Compare exact binary results and logical positions with independent models.
- [x] Verify partial elements, unsigned bytes, EOF clearing and arithmetic bounds.
- [x] Exercise short/error reads, partial-line errors, changed extents and recovery.
- [x] Verify 100 simultaneous streams and metadata-admission failure cleanup.
- [x] Execute the pinned XML DOM's `load_file` through the stream implementation.
- [x] Validate the explicit project/build option and reject malformed combinations.
- [x] Verify the combined C/numeric/C++/math profile and unsupported write-link refusal.
- [x] Execute the exact combined Batch 8 probes through completion/cancellation/restart.
- [x] Reproduce the kernel, both NROs and complete kit from isolated source.
- [ ] Accept the stream and growing-resource probes on physical Switch in one later session.
- [ ] Measure storage latency, stack/RAM use and broader concurrency/long sessions.

The [local evidence](../../evidence/read-streams1-local-20261101/verification.json)
records sources, exact binaries and verification scopes. Production ARM64 stream,
DOM, allocator, resource and filesystem bridge instructions execute locally;
OS, RO, storage and outer game scheduling remain modeled. Emulator instruction
allowances establish no physical timing bound or media-durability guarantee.

The current stream, combined-probe and profile checks pass 132 local assertions;
the portable suite passes 41 tests. An isolated build reproduces the unchanged
kernel, both NROs and all 17 kit files byte for byte. These results exclude
physical acceptance.

## Expanded pending hardware batch

The subsequent [captured-action batch](TASK-QUEUES1.md) adds a third manual
probe while retaining this kernel and both preceding probe binaries. Use that
single expanded kit/session. Earlier two-probe artifacts and local results
below retain their original scope; no separate stream-only run is required.

## Original consolidated workload

The expanded [Batch 8 checklist](../../examples/service-acceptance/README.md)
contains resource-registry and read-streams. Its kernel SHA-256 remains
`fdbbeba40b2f04e123e8a2c5f3cd5ae4bdef1ade765ba803ba5b6955fc4e19fa`.
The earlier one-probe Batch 8 kit remains archived; this revised kit supersedes
its pending installation request. No separate test session is requested.

The new [stream example](../../examples/read-streams/README.md) creates original
namespaced binary/text/XML fixtures and verifies retained contents on later runs.
It closes one deliberately unclosed stream during final cleanup. The resource
probe binary remains identical to its prior verified artifact. Two natural runs,
cancellation/restart and broader observations stay grouped in one later session.

[Batch 7 acceptance](SERVICE-BATCH7.md) retains its exact prior-kernel scope.
The revised Batch 8 runtime still needs physical acceptance; no stable release
or installed card content is changed. Hosted C++, automatic initializers,
exceptions and full Menyoo gameplay compatibility remain independent work.
