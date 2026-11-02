# Writable C streams verification

Date: 2026-11-02. This candidate extends the optional mutation table with a sized
resize tail and adds output/update/append to the existing stdio profile. It uses
the already audited SetFileSize binding; no native signatures or binary offsets
are added. Base plugin, file, catalog and extension layouts remain unchanged.

## Implemented contract

- [x] Preserve the original 24-byte mutation prefix and accept old-sized tables.
- [x] Validate owner/path/extent and read back successful resize postconditions.
- [x] Reserve stream storage/cleanup before destructive create/truncate operations.
- [x] Provide fwrite, fputc/putc, fputs, update/append and exclusive creation modes.
- [x] Explicitly zero holes without assuming raw extension-byte contents.
- [x] Provide checked fprintf/vfprintf with failure-preserving temporary formatting.
- [x] Support pinned XML save_file with C++ deleting-destructor support.
- [x] Complete 440 ARM64 checks and 44 portable source tests.
- [x] Reproduce the kernel, probe and all 18 kit files byte for byte from an isolated 616-file source export.
- [x] Compile and structurally check all 32 SDK examples/projects.
- [ ] Accept the consolidated normal/cancellation workload on physical Switch.
- [ ] Measure latency, stack, peak memory and long sessions under substantial load.

Output is synchronous, with at most 64 KiB per filesystem write and the existing
flush option. Large calls/hole zeroing can perform many operations without yielding.
No output waits until close or cleanup. A failed chunk may have changed bytes;
earlier complete chunks remain counted. Truncation supplies no backup or
transaction. Append is not atomic across concurrent writers. Direct XML saving
is separate from the cooperative prepared-file/publication workflow.

## Verification scope

The [retained local record](../../evidence/write-streams1-local-20261102/verification.json)
contains source identities and result hashes: 154 writable-stream checks, 111
read-stream regressions, 77 file-service checks, 75 scheduler checks, three combined
profile checks and 20 complete-kit checks. The latter execute the exact output
NRO and all four unchanged Batch 8 probes concurrently. Cancellation at three
transfer phases and restart after a one-shot write failure preserve owner cleanup.
An isolated source build outside the workspace passes 44 portable tests and
reproduces the kernel, output NRO and all 18 kit files byte for byte. All 32 SDK
examples/projects compile and pass structural inspection with the current SDK.

The candidate kernel SHA-256 is
`174ad27581792b81a496f9dd4ca1cd0181841c6a2a3aff670db5178539307ddc`.
The manual output NRO SHA-256 is
`4f59cabe19c05db32174010b2fc8d85a3e83c99500a1dc04609e810a79b507ea`.

Local execution uses compiled production ARM64 loader/storage/SDK/stream code
with modeled OS, RO, filesystem and outer scheduling. Independent bytearray and
XML models check contents. The model deliberately fills raw extensions with
nonzero bytes, injects write/resize failures, exercises old-sized tables and
runs new SDK reads on the physically accepted Batch 8 kernel. These checks
establish no physical SD latency, power-loss durability or full Menyoo support.

The [stream contract](../reference/READ-STREAMS.md),
[complete example](../../examples/write-streams/README.md) and
[one consolidated checklist](../../examples/service-acceptance/README.md) define
the supported workflow. The [Batch 8 result](SERVICE-BATCH8.md) remains unchanged.
