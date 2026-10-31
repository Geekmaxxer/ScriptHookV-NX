# Paged plugin directory catalogs

The unreleased catalog service enumerates files and subdirectories beneath a
plugin's data root. It uses an independent `ShvNxCatalog` ABI 1 table and leaves
the existing 48-byte file table and base plugin/extension contracts intact.
Check `scriptCatalogAvailable()` during explicit initialization before use.

```cpp
static ShvNxDirectoryEntry entries[4];
uint32_t offset = 0;
do {
    ShvNxDirectoryPage page{};
    if (scriptDirectoryList("catalog", offset, entries, 4, &page) != ShvNxFile_Ok)
        return;
    for (uint32_t i = 0; i < page.count; ++i) {
        // Inspect entries[i].kind, flags, bytes and name here.
    }
    offset = page.next_offset;
    WAIT(0);
} while (offset);
```

Use `path=""` to enumerate the plugin's own data root. Nested paths follow the
[file service path rules](FILE-SERVICES.md); they cannot escape
`settings/<plugin-id>/`. The root is created and checked if necessary. A missing
nested directory returns `NotFound` without being created implicitly.

## Pages and resource ownership

- Each request returns at most 16 entries, in the filesystem's current order.
- `count` is the number of valid output records. `next_offset=0` means EOF;
  otherwise pass it to the next request. Offsets count physical entries.
- Each call opens one listing handle, skips to the requested offset, reads a
  page and one lookahead entry, then closes the handle before returning.
  Root validation also opens and closes a separate directory handle.
- At most 4,096 entries may be traversed. Discovering an entry beyond that limit
  returns `DirectoryLimit`, including when the excess is found by lookahead.
  Earlier pages can succeed before the limit is encountered.
- An offset beyond current EOF returns an empty final page. An offset above
  4,096 is rejected before filesystem access.
- No persistent cursor, plugin heap buffer or operating-system handle survives
  a call, wait or lifecycle transition. Calls require the owning thread.

Pages are independent observations, not a snapshot. Inserting, deleting or
reordering files between calls can repeat or skip entries. Restart a scan when
the application requires a fresh view. Reopening and skipping makes a complete
multi-page traversal quadratic in entry count; yield between pages and avoid
large per-frame scans. Bounds do not establish a maximum SD access latency.

## Entry format

`ShvNxDirectoryEntry` is a public 792-byte record, independently translated from
the target SDK's 784-byte native entry. Do not reinterpret one layout as the
other. The public record contains:

| Field | Meaning |
| --- | --- |
| `kind` | `SHV_NX_CATALOG_DIRECTORY` or `SHV_NX_CATALOG_FILE` |
| `flags` | `SHV_NX_CATALOG_COMPATIBLE_NAME` when the name meets the file service's single-component grammar |
| `bytes` | File length; zero for directory entries |
| `name[769]` | Complete NUL-terminated filesystem name, up to 768 bytes |
| `reserved[7]` | Zero; retained for a stable explicit record layout |

Long, hidden or non-ASCII names are returned without truncation and without the
compatibility flag. The flag applies to one component. The assembled path must
still fit the file service's total length/depth limits, and large files retain
the separate 64 MiB I/O limit. Names are copied as filesystem bytes; the service
does not validate Unicode or normalize case. Other flag bits are currently zero.

Prefer static or owner-allocated page arrays. Sixteen records occupy 12,672 bytes;
smaller capacities reduce the caller's storage requirement. The runtime's native
read buffer is static and does not consume a plugin's cooperative stack.

## Failure behavior

A bound call validates and clears the page result. Invalid result pointers are
never written. Entry/result storage must not overlap. On any error, discard all
entry contents: a partially filled buffer can remain, while the returned page
reports zero count and offset. An unavailable SDK service leaves outputs alone.

Malformed native counts, missing name termination, empty names, unknown entry
types and negative file sizes return `InvalidEntry`. I/O, ownership, path and
extent errors retain their documented file-status values. A failed lookahead is
an error, not evidence that the just-filled page was the final page.

The optional [preset catalog helper](PRESET-CATALOGS.md) supplies cooperative
filtering and sorting in plugin code using the existing memory service.

This low-level service does not sort, recurse, delete, rename or transactionally replace
files. It supplies catalog metadata rather than a hosted `std::filesystem`
implementation. See the [combined example](../../examples/streamed-catalog/README.md)
and [verification record](../verification/PLUGIN-DIRECTORY1.md). Nested metadata
and a four-page mixed preset catalog have scoped physical acceptance. Directory
changes, storage errors, transitions and traversal performance remain open.
