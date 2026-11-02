# Deques, captured actions and expanded Batch 8

Date: 2026-11-01. This SDK batch adds an owner-backed ring deque and a fallible
move-only void-callable wrapper. The pending resource kernel, its resource probe
and the read-stream probe retain their exact binary identities. No public ABI,
defaults, native bindings or target offsets change. The
[contract](../reference/TASK-QUEUES.md) defines ownership, invalidation, aliasing,
type policies, synchronous execution and allocation-failure behavior.

## Implementation and local verification

- [x] Provide both-ended ring operations and checked capacity growth.
- [x] Preserve wrapped order and reacquire self-borrowed elements after relocation.
- [x] Support move-only elements and explicit transactional copying where permitted.
- [x] Verify mixed operations against independent sequence models and 6,144 retained values.
- [x] Preserve source/destination buffers and bindings on refused allocation.
- [x] Provide inline and over-aligned heap callable targets with explicit empty invocation.
- [x] Support move-only captures, optional explicit cloning and safe unrelated-type queries.
- [x] Verify target identity, invocation and cloning across two source files without RTTI.
- [x] Verify target lifetime accounting, owner isolation and complete cleanup.
- [x] Reject throwing calls/moves, unsupported alignment and invalid copy policies at compile time.
- [x] Build a combined 768-action workflow with owned strings and mixed inline/heap targets.
- [x] Execute the exact three-probe kit through repeated completion and cancellation/restart.
- [x] Reproduce the unchanged kernel, all three NROs and complete kit from isolated source.
- [ ] Accept the combined Batch 8 workloads on physical Switch in one later session.
- [ ] Measure substantial queue growth/cleanup, RAM, stack use and frame-time cost.

The [local record](../../evidence/task-queue1-local-20261101/verification.json)
identifies source hashes, compiled fixtures, suite results and exact package files.
Production ARM64 deque/callable/ownership code executes with independent sequence
and lifetime checks. OS, RO, filesystem and outer game scheduling remain modeled.
Local instruction allowances establish no physical scheduling or stack bound.

The queue/callable suite passes 433 checks, the exact three-probe workflow passes
25 checks and five compiler-policy refusals pass. The portable suite passes
41 tests. An isolated source build reproduces the unchanged kernel, all three
NROs and all 18 kit files byte for byte. Physical acceptance remains pending.

## Subsequent combined batch

The [JSON milestone](JSON-SERVICES1.md) adds a fourth manual probe while retaining
this kernel and all three preceding NROs. Use that single expanded kit/session.
The three-probe package and results below keep their original artifact scope;
no separate action-only or earlier-kit run is required.

## Original expanded pending batch

[Batch 8](../../examples/service-acceptance/README.md) contains three manual probes:
resource-registry, read-streams and task-queue. Earlier one/two-probe kits remain
archived; this kit supersedes their pending installation requests. No separate
hardware run is requested for any of those additions. The kernel SHA-256 remains
`fdbbeba40b2f04e123e8a2c5f3cd5ae4bdef1ade765ba803ba5b6955fc4e19fa`.

The new [task-queue example](../../examples/task-queue/README.md) executes 768
ordered actions, peaks at 512 queued actions, wraps/relocates its contents and
checks zero final captured objects, allocations and resource registrations.
Construction/execution cancellation discards pending actions through registered
cleanup; Load creates a fresh lifetime. Existing settings, bindings, game content,
saves and shared configuration are preserved.

Two complete runs per probe, cancellation/restart, coexistence, broader gameplay
observations and rollback stay in one later checklist. Physical media failures,
configuration/profile coverage, low memory, measured performance and long sessions
remain open when not exercised. [Batch 7 acceptance](SERVICE-BATCH7.md) retains its
earlier kernel/configuration scope. The published baseline is unchanged; hosted
STL and full Menyoo gameplay compatibility are not established by this batch.
