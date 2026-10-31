# Example projects

Start with [tutorials](../docs/guides/TUTORIALS.md) for the accepted baseline SDK.
The source examples use unique plugin IDs and document their observable results.

| Path | Purpose |
| --- | --- |
| `tutorials/` | Basic plugins, settings, input/rendering and NSC providers |
| `numeric-xml/` | Decimal validation and typed XML/settings workflow |
| `formatted-text/` | Checked snprintf/vsnprintf, decimal fields and repeated variadic arguments |
| `math-profile/` | Float/double geometry, error handling and combined formatter checks |
| `library-services/` | Consolidated numeric parsing, geometry, error-state and formatter acceptance |
| `container-registry/` | Move-only records, dynamic labels, ID lookup and numeric XML |
| `recoverable-data/` | Checked binary/XML records, fallback and explicit recovery |
| `streamed-catalog/` | Chunked 256 KiB file access, nested directories and yielding |
| `session-state/` | Explicit shared C++ state and registered worker cleanup |
| `large-xml-file/` | Cooperative owned reads, prepared writes and large XML parsing |
| `xml-workflow/` | Sorted preset browsing, large XML loading, verified saving and backup restoration |
| `service-acceptance/` | Deferred combined hardware checklist and manual probe packaging |
| `recoverable-publication/` | Prepared large-file validation, publication and retained backup |
| `managed-resources/` | Registered string/vector ownership across cooperative waits and cancellation |
| `user-profile/` | Current single-open-user nickname through the optional platform profile service |
| `mods/` | Showcase gameplay mods and optional standalone manager |

The numeric, container, storage, managed-resource and profile projects require unreleased runtime/SDK services;
their README files list exact requirements. Each example's local and hardware
acceptance scope is independent. Build into a fresh output directory and close
GTA before replacing installed code.
