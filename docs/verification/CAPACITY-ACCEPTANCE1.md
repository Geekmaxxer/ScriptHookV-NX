# Large-module and script-capacity acceptance

The `capacity-acceptance` probe exercises module extents,
additional script stacks, scheduler metadata and nested ownership on the existing
Test1 runtime. The accepted combined workload remains a separate result. No
production runtime, ABI, native signature or target offset changes in this work.
Defaults refusal and the Explicit/Automatic workloads now have scoped physical
acceptance. Original configuration restoration and fresh startup are also verified.
Performance/soak acceptance remains pending.

## Prepared workload

- [x] Build an image and BSS that each exceed the default 2 MiB limit.
- [x] Verify page-spaced access through both extents, including retained BSS on restart.
- [x] Execute one main entry and 36 distinct cooperative workers.
- [x] Register two managed children, their parent and a final cleanup auditor.
- [x] Repeat bounded allocation/free/realloc cycles with complete byte checks.
- [x] Record worker progress, render callbacks and completed-script timing.
- [x] Compare compiled defaults, explicit limits and automatic settings.
- [x] Preserve original configuration and available reports before changing phases.

The NRO image is 2,134,016 bytes and BSS is 2,105,344 bytes. Static module,
mapping and registration accounting is 8,482,816 bytes. The bounded test heap
requests less than 1 MiB; core metadata and retained script stacks are separate.
These values do not measure process/GPU memory or whole-stack use.

## Local verification

The [verification summary](../../evidence/capacity-acceptance1-local-20261030/verification.json)
identifies the compiled NRO, exact runtime and source hashes. Maintainer tests
execute production runtime, SDK and probe ARM64 instructions with modeled OS,
RO, filesystem, game boundaries and outer scheduling.

- [x] Verify default image refusal, independent BSS refusal and shared-budget refusal.
- [x] Execute all 37 entries with stack and scheduler metadata growth.
- [x] Verify nested cancellation, zero remaining owner allocations and successful Trim.
- [x] Restart using retained mappings and already admitted stacks.
- [x] Execute the same bounded workload under automatic settings.
- [x] Cancel during admission and reject partial admission without leaking ownership.
- [x] Reject missing ownership services before initialization succeeds.
- [x] Exercise profile backup/restore, exact original bytes and original absence.
- [x] Refuse changed binaries/profiles, unrelated edits, damaged backups and linked paths.
- [x] Capture phase reports without reading profile-name or account files.

The example checker now respects a project's declared image/BSS/budget settings.
Default inspection limits remain unchanged. This allows the large example to
receive structural checks under its explicit project requirements.

## Physical sequence

Build the kit with `tools/build_capacity_acceptance.py`; follow its
[checklist](../../examples/capacity-acceptance/README.md). It includes the manual
NRO, profiles and a configuration-preservation helper. It does not bundle runtime
modules, replace NPDM metadata or automatically install a shared configuration.
The helper requires runtime SHA-256
`930d3043c61ee457da833d698836886e20a103a5eab68158d7d786439fbf128f`.

- [x] Accept expected `0x10023` refusal under compiled defaults while accepted mods continue.
- [x] Accept the explicit 16/48/16/3/3 profile and independently verify installed identities.
- [x] Verify all workers, large extents, nested cleanup and repeated Kill/Load on Switch.
- [x] Accept the same bounded workload with automatic mask `0x1f`.
- [x] Inspect available phase captures and verify their file hashes.
- [x] Confirm original configuration bytes against the preserved backup.
- [x] Verify normal startup after restoration and expected refusal of the oversized probe.
- [ ] Record longer sessions, transitions and measured timing/performance separately.

## Physical Defaults and Explicit results - 2026-10-30

The [Defaults record](../../evidence/capacity-acceptance1-console-20261030/defaults-acceptance.json)
confirms `0x10023` refusal before probe initialization. The installed NRO exceeds
the effective 2 MiB image limit; no probe status or cleanup files are expected.
The runtime and Defaults profile hashes match the retained kit, and existing
Disarm/Footprints scripts continue running without a global loader error.

The [Explicit record](../../evidence/capacity-acceptance1-console-20261030/explicit-acceptance.json)
matches the installed Test1 runtime, NRO and 16/48/16/3/3 profile. The operator
reports an initial 30-60 seconds, Kill/Load, approximately another minute and a
second Kill, with acceptable subjective performance. Duration and FPS were not
independently measured. Matching status and cleanup identify stopped run 2; the
first run's detailed probe reports were overwritten.

| Retained Explicit observation | Result |
| --- | --- |
| Large extents | 513 verified page probes in both image and BSS |
| Cooperative workers | All 36 progressing; each reached 2,955 ticks |
| Allocator workload | 63 verified allocation/free/realloc cycles |
| Nested cleanup | Two children destroyed before the parent; one parent destructor and stop callback |
| Owner heap/resources | Zero live allocations, requested/charged bytes and registrations; Trim succeeded |
| Retained script stacks | 2,555,904 bytes: 39 stacks, above the 16-stack bootstrap |
| Additional runtime metadata | 24,576 bytes; zero metadata/admission errors |
| Peak probe requested heap | 188,311 bytes; excludes process RAM and static backing |

The final loader reports the probe stopped with no active callbacks, errors or
resource failures; Disarm and Footprints remain running. Its 8,589,312 bytes of
shared plugin accounting include all three resident modules. Stack and core
metadata storage are reported separately. The original configuration backup is
intact, but restoration has not yet been exercised.

The timing API reports 155,728 completed dispatches, 2,176,818 microseconds total
and a 78,574-microsecond maximum. These owner counters accumulate across both
lifetimes. They measure elapsed time around coroutine resumes, including any
synchronous service stalls or descheduling, and do not identify the maximum's
run or callsite. The outlier remains an open performance investigation; it is
not a whole-game frame-time/FPS measurement or a completed performance signoff.

All four available phase-capture inventories match their recorded hashes. One
Explicit-labeled capture contains Defaults-effective loader values and is not
accepted as an Explicit execution. Current live loader values and matching run-2
reports establish the accepted Explicit result.

## Physical Automatic result - 2026-10-30

The [Automatic record](../../evidence/capacity-acceptance1-console-20261030/automatic-acceptance.json)
matches the installed Test1 runtime, probe and all-five-key Automatic profile.
The loader confirms applied/automatic masks `0x1f`, no invalid keys and no
configuration, admission, loader or cleanup errors. The operator reports two
approximately one-minute runs separated by Kill/Load and a final Kill. Duration
was not independently measured.

Matching status/cleanup files identify stopped run 2. All 36 workers progressed
to 860 ticks each; the retained run completed 18 allocator cycles and 894 render
callbacks, with 513 verified page probes in each extent. Both children were
destroyed before their parent. The final owner heap and registration counts are
zero, Trim succeeded and no allocation/release/resource failures were reported.
Disarm and Footprints remain running.

Recorded allocation totals match Explicit: 2,555,904 retained stack bytes,
24,576 additional metadata bytes, 8,589,312 shared plugin-accounting bytes and
188,311 peak requested owner-heap bytes. These are separate accounting categories;
process peak RAM and whole-stack use remain unmeasured.

Owner timing totals are 91,981 completed dispatches, 2,018,740 microseconds total
and a 96,369-microsecond maximum across both lifetimes. The different activity
counts and uninstrumented durations do not support a controlled comparison with
Explicit. Both the 78.6 ms Explicit maximum and 96.4 ms Automatic maximum require
further investigation; their source callsites are unknown.

All five available phase captures pass their recorded hash checks. The newest
Explicit capture exactly preserves the previously accepted live Explicit reports.
The original configuration backup is intact. Run Restore and verify normal
startup before closing the restoration item.

Snapshots are captured before profile changes and restoration. A snapshot alone
does not prove that GTA ran its recorded configuration; review loader values and
matching probe run IDs. Earlier per-run reports within a phase are overwritten.
The helper retains original configuration backups and refuses unrelated edits.

This single-owner workload does not exercise resident-module growth, the second
selected account, media failure, all engine transitions or full Menyoo behavior.
Completed-script timings do not measure whole-game FPS. Restoration, performance
and the remaining [platform sequence](PLATFORM-CAPACITY1.md) are open.

## Configuration readback and diagnostic successor - 2026-10-30

The [restoration readback](../../evidence/capacity-acceptance1-console-20261030/restoration-readback.json)
matches all 416 original configuration bytes and the backup journal's SHA-256.
The Automatic phase capture exactly preserves the accepted Automatic reports.
The current loader report still reflects Automatic settings; the operator confirms
that a fresh launch has not occurred. Configuration restoration is verified at the
file level. Normal startup after restoration remains pending.

The current source builds [Test2 diagnostics](CAPACITY-DIAGNOSTICS2.md), a separate
probe revision with per-operation timing and bounded run history. The Test1 hardware
records above identify the original NRO and retain their own acceptance scope.

## Physical startup after restoration - 2026-10-30

The [post-restoration record](../../evidence/capacity-acceptance1-console-20261030/restoration-acceptance.json)
verifies a fresh complete loader report under the original 416-byte configuration.
Its effective settings are 16 residents, 32 script slots, a 10 MiB shared budget
and 2 MiB image/BSS limits. Automatic settings are disabled. Runtime, probe,
Disarm and Footprints identities match their previously reviewed artifacts.
Both existing mod scripts are running with zero plugin errors. Loader,
configuration, action, metadata and cleanup errors are zero.

Attempting Load Now on the original capacity probe produces `0x10023`
(`module_image_limit_exceeded`) at `read_and_validate`. Its image is 2,134,016
bytes, 36,864 bytes above the restored 2,097,152-byte limit. The module is neither
resident nor initialized. This is the intended refusal under the original limits;
the Restore phase verifies normal startup without loading the large probe.

The retained status/cleanup files are byte-identical to the earlier Automatic
results. They are historical reports, not a new run. No fresh probe output is
expected before initialization. Original configuration restoration and runtime
startup are accepted within this scope; individual mod effects, transitions and
performance were not independently retested. Test2 is not installed or accepted.
