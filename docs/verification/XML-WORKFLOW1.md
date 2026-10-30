# Combined XML workflow verification

This milestone merges the pending large XML/file-transfer hardware check with
new growable XML serialization, bounded file comparison and verified publication/
backup restoration. It uses the unchanged Test1 runtime and adds no native
signature, engine offset or runtime table. Physical acceptance remains pending.

## Implementation and local checks

- [x] Serialize complete UTF-8 XML into a fallible owner-backed growable buffer.
- [x] Retain capacity for reuse and hide partial/stale output after failure.
- [x] Exercise output limits, allocation refusal and terminator-size overflow.
- [x] Compare files in bounded chunks without retaining caller span pointers.
- [x] Detect byte mismatch, short/error reads, incomplete comparison and changed final size.
- [x] Preserve owned output/scratch after foreign reset refusal.
- [x] Recreate empty writer/verifier state after Kill/Load.
- [x] Load 256 KiB XML, add 96 KiB of text and serialize beyond the flat write limit.
- [x] Verify prepared/published output and exact original backup contents.
- [x] Preserve the replacement separately, restore the original and verify both.
- [x] Clean every buffer/document allocation on natural completion and interruption.
- [x] Refuse conflicting or interrupted disk states on restart without deleting data.
- [x] Execute all four packaged probes together under compiled defaults and repeat loading without remapping.
- [ ] Accept the combined load/save/restore workload on physical Switch hardware.

The [local record](../../evidence/xml-workflow1-local-20261030/verification.json)
identifies source hashes, exact artifacts and executed tests. Production runtime,
SDK, XML and probe ARM64 instructions execute with modeled OS, RO, filesystem,
game boundaries and outer scheduling. Instruction budgets are local execution
bounds, not measured hardware timing.

## Consolidated hardware sequence

Batch 3 hardware acceptance is deferred into the later
[preset catalog milestone](CATALOG-SCAN1.md). Its batch 4 kit includes this
load/save/recovery sequence and new catalog selection checks. The local batch 3
record above retains its exact original artifacts and test scope.

The current [service checklist](../../examples/service-acceptance/README.md) replaces the
standalone batch 2 XML check. Load **xml-workflow** twice, allowing natural
completion, and retain its reports/input/original/published files. Previously
accepted service probes remain optional. No repeat of the long capacity test is
required. Batch 2 artifacts and local evidence are preserved as historical records.

The standalone `large-xml-file` artifact remains locally verified. Acceptance of
the new combined artifact would establish its tested transfer behaviors, not a
retroactive hardware run of the old NRO. New serialization/comparison helpers
also retain separate physical acceptance until the combined run is reviewed.

The [helper contracts](../reference/XML-BUFFERS.md) keep parsing/serialization,
memory limits, synchronous storage delay and ownership explicit. The workflow
does not establish power-loss durability, automatic recovery of arbitrary mod
data, presentation FPS or full-process memory usage.
