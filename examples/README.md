# Example projects

Start with [tutorials](../docs/guides/TUTORIALS.md) for the accepted baseline SDK.
The source examples use unique plugin IDs and document their observable results.

| Path | Purpose |
| --- | --- |
| `tutorials/` | Basic plugins, settings, input/rendering and NSC providers |
| `numeric-xml/` | Decimal validation and typed XML/settings workflow |
| `container-registry/` | Move-only records, dynamic labels, ID lookup and numeric XML |
| `recoverable-data/` | Checked binary/XML records, fallback and explicit recovery |
| `streamed-catalog/` | Chunked 256 KiB file access, nested directories and yielding |
| `recoverable-publication/` | Prepared large-file validation, publication and retained backup |
| `managed-resources/` | Registered string/vector ownership across cooperative waits and cancellation |
| `mods/` | Showcase gameplay mods and optional standalone manager |

The numeric, container, storage and managed-resource projects require unreleased runtime/SDK services;
their README files list exact requirements. Each example's local and hardware
acceptance scope is independent. Build into a fresh output directory and close
GTA before replacing installed code.
