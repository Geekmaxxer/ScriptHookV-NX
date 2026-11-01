# Plugin catalogue and rescanning

The installed-plugin catalogue grows through stable metadata pages independently
of `resident_limit`. Discovery does not reserve an executable image or script
stack for every file. Loading remains subject to configured admission, memory
availability and platform loader behavior.

`SHVNX_RELOAD_PLUGINS` queues a scan on the runtime owner thread. The scanner
collects a complete directory snapshot before updating plugin presence, filenames
or policies. A directory failure retains the previous catalogue. Duplicate policy
files remain explicit conflicts. Existing residents retain their owner tokens,
handlers, callback state and mapped code.

The shared declarations are in [commands_scripthook_nx.sch](../../sdk/commands_scripthook_nx.sch).
Two additive natives accompany catalogue revision 1:

| Native | Hash | Result |
| --- | --- | --- |
| `SHVNX_IS_PLUGIN_LISTED(index)` | `0x5348564e58505657` | True for a present file or retained resident/published mapping. |
| `SHVNX_GET_PLUGIN_SCAN(generation, pending, error)` | `0x5348564e58505332` | Writes three INT outputs and returns whether the call was valid. |

Capture the generation before requesting a scan. Completion means `pending` is
zero and the generation differs from the captured value. A nonzero error records
scan failure. Completion does not mean queued Auto plugins have finished loading.
The generation advances for successful and failed scans and skips zero on wrap.

`SHVNX_GET_PLUGIN_COUNT` reports catalogue slots, including missing retained rows.
Filter through `SHVNX_IS_PLUGIN_LISTED`; visible row positions are not persistent
identities. A missing unpublished row can be reused. Managers must re-resolve the
selected canonical filename before Load, Kill or policy changes. A small display
cache should cover the visible window, rather than cap the catalogue.

Deleting a loaded file does not unload code. Its retained row can report a missing
file while published addresses remain valid. Restart GTA to replace that code or
recover its mapped-memory admission. External suffix changes update startup
policy; they do not implicitly stop a running plugin.

The `loader-report.txt` fields `plugin_scan_generation` and `plugin_scan_error`
record the latest completed scan. No fixed total directory count is imposed by
the loader's paged scan; allocation failure and representable indices still
bound operation. Paged plugin-owned directory APIs retain their separate explicit
contracts in [directory catalogs](DIRECTORY-CATALOG.md).
