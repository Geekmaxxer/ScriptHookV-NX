# Shared session-state verification

The SDK supplies `SessionLocal<T, Tag>` through the existing Test1 memory and
owned-resource services. It provides explicitly initialized C++ state shared
across translation units and cooperative entries, with registered cleanup and
fresh construction after Kill/Load. Production runtime, ABI and target offsets
are unchanged. New session-state behavior has local verification only.

## Implementation and local checks

- [x] Publish an object only after allocation, construction and registration succeed.
- [x] Share one type/tag slot across two translation units within an NRO.
- [x] Preserve separate slots for independent NROs and distinct tags.
- [x] Preserve the object across cooperative waits and ignore later replacement arguments.
- [x] Refuse recursive construction/destruction access to the same slot.
- [x] Preserve ownership after foreign-thread and unfinished-native reset refusal.
- [x] Destroy unpublished objects after registration exhaustion without leaking.
- [x] Refuse missing required services before publishing an object.
- [x] Drain state on rejected initialization, natural completion and repeated Kill.
- [x] Recreate fresh state after Load while retaining module code.
- [x] Execute the complete two-source worker/string/vector example locally.
- [x] Package the new example with publication and catalog checks in one service batch.
- [x] Execute all three packaged NROs together under compiled defaults, including repeat loading and cleanup.
- [ ] Accept the shared-state example on physical Switch hardware.
- [ ] Accept publication and nested catalog examples from the same batch.

The [local record](../../evidence/session-local1-local-20261030/verification.json)
identifies exact artifacts, source hashes and completed checks. Tests execute the
production runtime, SDK and probe ARM64 instructions with modeled OS, RO,
filesystem, game boundaries and outer scheduling. They do not establish new
physical memory, timing or gameplay results.

## Combined acceptance

Build `tools/build_service_acceptance.py` and follow the
[service batch checklist](../../examples/service-acceptance/README.md). A short
shared-state Kill/Load sequence and two naturally completing file examples can
be reviewed together. There is no requirement to repeat the accepted Test2
capacity workload for this SDK addition. The kit includes manual NROs and leaves
runtime installation, NPDM and shared configuration unchanged.

The [session contract](../reference/SESSION-STATE.md) defines explicit initialization,
borrowed pointers, callback restrictions and ordering. Compiler-managed global
initialization, exceptions and static-local destruction remain separate work.
