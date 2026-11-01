# Plugin catalogue revision: local verification

Recorded 2026-11-01. [Verification record](../../evidence/plugin-catalogue1-local-20261101/verification.json).

The revised production ARM64 module passes 51 loader checks, 118 manager checks,
75 scheduler checks and 18 catalogue/namespace checks. The catalogue suite exposes
160 entries with the default eight-resident configuration, exercises a failed
second directory page, and verifies recovery, stable owners, suffix changes,
removed residents and independent six-/260-word NSC records.

The failed-load regression identified a null unregistration path that could
release a reservation and decrement unrelated active-script accounting. The
revised scheduler ignores null functions; the memory-budget rejection case
retains the existing resident's active count.

The paired MEGATARD v1.1 source passes 62 NSC tool tests. Its 526 native entries
match the compiler intermediate; 507 match stock Switch script coverage and 19
are registered runtime natives. C# and Python generation reproduce the same
binding metadata from the final NSC.

Fourteen additional checks execute selected live SanScript catalogue routines
through a limited integer/string interpreter with modeled persistence natives.
They cover 96 saved names, cache behavior, failed preparation/root publication,
swap deletion, asynchronous selection and interrupted legacy migration. These
checks establish algorithm behavior, not target VM or physical SD acceptance.

Filesystem/RO services, allocation and outer game state are modeled for the ARM64
suites. Physical startup, live host-side filesystem updates, vehicle effects,
frame time and the complete v1.1 pair remain pending in one combined hardware
session. Earlier acceptance identifies its original hashes and is not transferred
to this revised module.
