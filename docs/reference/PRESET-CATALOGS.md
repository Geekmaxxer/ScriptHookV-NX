# Cooperative preset catalogs

`shv/catalog_scan.h` provides `ScriptHook::Data::CatalogScanTask` for preset
browsers and similar source ports. It collects addressable entries from the
existing [directory service](DIRECTORY-CATALOG.md), filters file extensions and
sorts names before exposing results. It changes no runtime table or binary.

## Lifetime and execution

Place a task inside a registered `Managed` or `SessionLocal` object when it must
survive `WAIT`. The task owns a fallible vector and a four-record page buffer;
it is unsuitable for an ordinary stack lifetime spanning whole-plugin Kill.
All operations, including Reset, Cancel and destruction, require the owning
runtime thread. This contract does not add foreign-thread mutation protection.

```cpp
// State is explicitly constructed and registered before this function runs.
if (!state.catalog.Begin("presets", 128, ".xml", true)) return;
for (;;) {
    const auto status = state.catalog.Step();
    if (status == ScriptHook::Data::CatalogState::Failed) return;
    if (status == ScriptHook::Data::CatalogState::Complete) break;
    WAIT(0);
}
char selected[80];
if (state.catalog.BuildPath(0, selected, sizeof(selected))) {
    // Read selected through the isolated file service; check its current extent.
}
```

The example assumes registered state remains valid until its owner is cancelled.
No task operation yields internally. Step performs one four-entry page request,
one heap-sort operation, or one adjacent-name validation. Yield between steps.
Underlying storage access and vector growth remain synchronous; these bounds
do not establish a wall-clock deadline.

## Results and controls

| Interface | Contract |
| --- | --- |
| `Begin(path, maximum=128, extension=nullptr, include_directories=true)` | Start only from Idle; the matched-entry maximum is 1–4,096 |
| `State()` | Idle, Listing, Sorting, Checking, Complete, Failed or Cancelled |
| `Step()` | Advance the current phase and return its state |
| `Count()` / `At(index)` | Publish results only in Complete; otherwise zero/null |
| `Collected()` / `Pages()` / `Scanned()` / `Skipped()` | Report progress, including unpublished matched entries |
| `BuildPath(index, output, capacity)` | Build a checked relative selection; clear a nonempty output buffer on failure |
| `Error()` / `FileStatus()` | Retain the helper error and underlying file-service status |
| `Reset()` / `Cancel()` | Release vector storage; return to Idle or enter Cancelled |

Use `path=""` for the plugin's root. File extensions begin with a dot followed
by ASCII letters/digits, with at most 15 bytes including the dot. Null or empty
extensions admit all files. Matching and ordering fold ASCII letter case while
preserving original names. Directories are included independently of the file
filter when requested. They participate in name sorting without a separate
directory-first group. Equal folded names fail with DuplicateName.

Only compatible names of at most 31 bytes whose assembled paths fit the existing
79-byte/four-component rules enter the catalog. Incompatible, filtered and
unaddressable entries contribute to Skipped. File sizes remain metadata: a listed
file may exceed the separate 64 MiB file-I/O bound or change before it is opened.

Begin and failures keep results private. A second Begin preserves the active or
terminal task. Reset before rescanning. Failed tasks retain their collected
storage until Reset, Cancel or destruction. At returns a borrow invalidated by
those operations. No file or directory handle survives a call.

## Consistency and limits

Pages are independent observations. Duplicate detection catches repeated folded
names; it cannot detect entries skipped by concurrent insertion, deletion or
reordering. Completion does not promise a directory snapshot. Rescan explicitly
when a fresh view is required, and independently validate a selected file.

Reopening and skipping offsets makes the underlying traversal quadratic. Choose
a modest maximum and avoid repeatedly indexing large folders. Sorting is
O(n log n); vector growth can copy already collected records in one call. The
helper does not recurse, watch directories, parse presets or modify files.

The [combined example](../../examples/xml-workflow/README.md) and
[verification record](../verification/CATALOG-SCAN1.md) cover paged selection
alongside XML loading, verified saving and restoration. Physical acceptance of
the combined workload remains deferred.
