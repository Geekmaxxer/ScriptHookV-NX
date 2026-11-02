# JSON configuration and expanded service batch 8

Date: 2026-11-01. This SDK batch adds pinned JSON parsing/writing, owner-accounted
document/buffer limits, mutable copies and cooperative file loading. The pending
kernel, target bindings, ABI layouts and defaults remain unchanged. Earlier
resource, read-stream and task-queue probes retain their binary identities.
The [contract](../reference/JSON-SERVICES.md) defines allocation, strict-syntax,
duplicate-key, type, lifetime and publication limits.

## Implementation and local verification

- [x] Pin unmodified yyjson 0.13.0 sources, license and source hashes.
- [x] Provide explicit build/project options with checked C-profile requirements.
- [x] Disable upstream file/patch/incremental APIs and non-standard syntax; retain UTF-8 checks and 64-level depth policies.
- [x] Account document/buffer state, library allocations and transient replacement overlap.
- [x] Preserve existing documents and output buffers after syntax, budget or allocation refusal.
- [x] Verify independent JSON/byte/type round trips, integer limits and Unicode/escape errors.
- [x] Verify mutable replacement, exact integer/string types, move contexts and invalid output refusal.
- [x] Verify 2,000 independent records and complete owner cleanup/isolation locally.
- [x] Provide cooperative file loading with retained prior results after malformed, short, cancelled or memory-limited loads.
- [x] Build a complete load/edit/serialize/verify/publish/restore probe with an 8,192-byte payload.
- [x] Execute the exact four-probe kit through repeated completion, cancellation/restart and injected failures.
- [x] Verify combined profiles and complete binary-kit dependency notices.
- [x] Reproduce the unchanged kernel, all four NROs and kit from isolated source.
- [ ] Accept all four Batch 8 probes in one later physical Switch session.
- [ ] Measure physical RAM, stack, parsing/serialization cost, storage latency and long sessions.

The [local record](../../evidence/json-services1-local-20261101/verification.json)
retains source/dependency hashes, artifact identities, suite results and exact kit
contents. Production ARM64 library, wrapper, transfer and ownership instructions
execute with independent JSON/byte/type comparisons; OS, RO, filesystem and outer
game scheduling remain modeled. Instruction allowances establish no physical
frame-time, full-stack or media-durability guarantee.

The JSON suite passes 142 checks; the exact combined kit passes 35 workflow
checks and combined-profile/dependency verification adds three checks. The
portable suite passes 44 tests. An isolated source build reproduces the unchanged
kernel, all four NROs and all 21 kit files byte for byte. Physical acceptance
remains pending.

## One combined pending session

The expanded [Batch 8 checklist](../../examples/service-acceptance/README.md)
contains resource-registry, read-streams, task-queue and json-presets. Earlier
one/two/three-probe kits remain archived; this kit supersedes their pending
installation requests. No separate JSON or earlier-probe session is requested.
The kernel SHA-256 remains
`fdbbeba40b2f04e123e8a2c5f3cd5ae4bdef1ade765ba803ba5b6955fc4e19fa`.

The [preset workflow](../../examples/json-presets/README.md) verifies original
input bytes, typed edits, prepared/current/backup bytes and explicit restoration.
It preserves conflicting files and reports cleanup after cancellation/read
failures. Ordinary hardware cancellation is during input loading after the first
complete fixture run; interrupted publication needs explicit state inspection.

Normal runs, cancellation/restart, coexistence, transitions, longer play and
rollback remain grouped in one later checklist. Unexercised profile/configuration,
low-memory, actual media-failure and measured-performance gates remain open.
[Batch 7 acceptance](SERVICE-BATCH7.md) retains its prior artifact/configuration
scope. No stable promotion or full Menyoo compatibility is claimed.
