# Plugin Libraries Test 1

Date: 2026-10-29. Unreleased SDK candidate for the [plugin memory runtime](PLUGIN-MEMORY1.md).
The core runtime binary is unchanged from that candidate. No installation,
accepted release replacement or new physical Switch acceptance is performed.

The subsequent [numeric profile](PLUGIN-NUMERIC1.md) enables decimal float/double
conversion and typed XML floating data with an additional explicit option. The
text/integer-only profile and its historical evidence below retain their scope.

This milestone supplies an opt-in C allocation/string subset and a pinned,
unmodified pugixml 1.16 memory-DOM library. Actual compiled library instructions
execute against the production owner heap in local ARM64 tests. It establishes
a concrete library dependency profile; full CRT/STL support remains open.

## Build and required runtime

```powershell
python tools/build_plugin.py --source path/to/main.cpp --c-runtime --output out/xml-mod --devkitpro C:/devkitPro
```

Alternatively add `"c_runtime": true` to a format-1 NRO `project.json`, then use
`build_mod.py`. The option must be a JSON boolean and is invalid for NSC projects.
It is disabled by default. The desktop Porter's general hosted-library blockers
remain applicable; it does not automatically adapt an arbitrary XML/STL mod.

`plugin-build.json` records `c_runtime_enabled`, `requires_plugin_memory`, source
snapshots and dependency hashes. CRT-enabled initialization refuses an absent
memory table before entering the plugin's initializer or main function. The
accepted public runtime does not supply this new service; base/plugin/extension
ABI 1 numbers alone do not establish compatibility with the candidate profile.

The builder links the additional freestanding C unit and uses function/data
sections, garbage collection and disabled speculative devirtualization. The
last setting prevents unused stdio file-writer branches from becoming dependencies
of a custom memory writer. Strict unresolved-import rejection remains enabled.
The linker retains initializer/finalizer arrays so collection cannot silently
discard unsupported constructors. The normal constructor rejection still applies.

## C subset

Include `shv/crt.h` for explicit freestanding declarations. Allocation functions
are `malloc`, `calloc`, `realloc`, `free` and `aligned_alloc`. They call this NRO's
bound memory service, preserving the existing owner, budget, alignment, overflow,
zero-size and failure behavior. `realloc` failure preserves the original block;
`aligned_alloc` also requires size to be a multiple of alignment. Allocation
failure returns null. No `errno` service is provided.

The additional memory/string functions are `memmove`, `memcmp`, `memchr`, `strlen`,
`strnlen`, `strcmp`, `strncmp`, `strcpy`, `strncpy`, `strcat`, `strncat`, `strchr`,
`strrchr` and `strstr`. The shim already supplies `memcpy` and `memset`. Use
`<string.h>` for the compiler's C++ overload declarations where needed. Callers
must supply valid, terminated strings and sufficient destination storage; these
functions preserve ordinary C caller obligations.

This subset does not add global `new/delete`, `std::allocator`, hosted containers,
exceptions, locale, printf/scanf, floating numeric conversion, C file handles,
threads or automatic static initialization. Unprovided operations still fail
compilation or strict linking. The XML adapter's class-local deleting-destructor
slot is not a general C++ allocation service.

## XML memory profile

Include `shv/xml.h` before any upstream pugixml header. It selects the header-only,
no-STL, no-exceptions, no-XPath configuration and a 2 KiB serializer scratch buffer.
The supplied profile covers UTF-8 text nodes/attributes, tree traversal/editing,
integer access and memory serialization through `ScriptHook::Xml::Save`.

```cpp
#include "script.h"
#include "shv/xml.h"

bool Update(const void* input, size_t inputBytes,
            char* output, size_t capacity, size_t& outputBytes) {
    pugi::xml_document document;
    const auto parsed = document.load_buffer(input, inputBytes,
                                             pugi::parse_default, pugi::encoding_utf8);
    if (!parsed) return false;
    auto root = document.child("Settings");
    if (!root) return false;
    auto count = root.attribute("count");
    if (!count) count = root.append_attribute("count");
    if (!count || !count.set_value(3)) return false;
    return ScriptHook::Xml::Save(document, output, capacity, outputBytes);
} // Document destruction frees its live allocations; empty pools may be retained.
```

`Save` requires a non-null buffer with room for a terminating NUL. Its returned
byte count excludes that NUL. A false result means incomplete output: a prefix
may remain, and must not be saved as a complete document. Check parsing, edits
and serialization separately. Allocation failure is reported through pugixml's
status/results; exceptions are disabled.

Typed floating access/formatting (`as_float`, `as_double`, floating `set_value`),
`load_file`/`save_file`, streams and XPath are unsupported by this profile. Float
strings can be preserved as text, including decimal and exponent attributes;
this does not implement numeric conversion. Using the unsupported float or file
methods fails strict linking rather than returning fabricated data. The header's
`strtod` declaration only allows upstream inline definitions to be parsed.

Use bounded memory input and output with the existing isolated settings API, if
appropriate. Its flat paths, 64 KiB write limit and raw single-file durability
remain unchanged. Parsing a larger memory buffer does not add streaming SD I/O,
transactional saves or directory access. Huge DOMs and large synchronous C copies
can exceed a frame budget even when memory fits; process data at controlled times
and measure actual Switch latency before choosing production limits.

Keep documents scoped to completed operations or explicit module-owned state.
Do not leave one on a suspended script stack and assume Kill unwinds its destructor.
All allocations/frees use the permitted owner thread. Independent NROs retain
independent bindings and allocations. Free owned data explicitly and use Trim
after destruction when unused pool capacity should return to the shared budget.

Upstream revision, hashes and MIT license are retained in
[`provenance.json`](../../sdk/third_party/pugixml/provenance.json) and
[`LICENSE.md`](../../sdk/third_party/pugixml/LICENSE.md). Include the MIT notice with
distributed mods containing pugixml, alongside the project's attribution.
See the [upstream build configuration](https://pugixml.org/docs/manual.html#install.building).
The source SDK includes the vendor files and notices; no downloaded build scripts
or binary mods are executed.

## Verification and remaining acceptance

Local checks exercise two CRT NROs and two XML NROs, overlap/string semantics,
cross-owner refusal, alignment, calloc overflow, realloc failure preservation,
malformed input, UTF-8/entities, integer values, editing and semantic output
roundtrips. A 77,062-byte, 1,000-record document parses through the real DOM; its
deliberately undersized output writer reports overflow. First and mid-parse OS
allocation failures report out-of-memory and free partial document state. Kill/
Load, missing memory service and a copied standalone source SDK are checked.
Negative builds verify unsupported float/file calls and constructors are refused.

The owner-heap, existing extension, lifecycle, capacity and portable-tool suites
are rerun separately with fresh SDK builds. The runtime binary is reused with
its identity checked, rather than described as a new core build. Retained source,
dependency/license hashes and test reports are in
[`verification.json`](../../evidence/plugin-libraries1-local-20261029/verification.json).
OS allocation, filesystem and RO boundaries remain modeled. No game binaries,
raw target disassembly, databases or installable NROs are retained in that evidence.

Physical acceptance still requires actual Horizon/RO operation, low-memory and
fragmentation behavior, coexistence, explicit cleanup/restart, loading/death/
character transitions, and a measured long-session RAM/frame-time soak. Numeric
conversion, hosted C++ containers and bounded streaming/transactional files remain
runtime roadmap work. This milestone does not start or certify a Menyoo port.
