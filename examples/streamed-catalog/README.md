# Streamed catalog example

This example creates and verifies a 256 KiB binary sample in 32 KiB chunks,
yielding between chunks, then checks the sample's directory metadata through
four-entry catalog pages. It requires the unreleased file and catalog services
and refuses initialization when either service is unavailable.

```powershell
python tools/build_mod.py --project examples/streamed-catalog/project.json --output out/streamed-catalog --devkitpro C:/devkitPro
```

Data belongs to `settings/streamed-catalog-example/`. `catalog/sample.bin` is
created exclusively; an existing file is verified without being rewritten.
`result.txt` reports streaming and catalog verification. The buffers are static and no filesystem handles
remain open across waits or after completion.

An interrupted first run may leave an incomplete sample. A later run reports a
verification failure and preserves that file. Remove the example's sample
explicitly to generate it again. This workflow demonstrates chunking and
ownership, not atomic replacement or storage durability.

See the [file service contract](../../docs/reference/FILE-SERVICES.md) for bounds,
and the [catalog contract](../../docs/reference/DIRECTORY-CATALOG.md) for pagination,
failure semantics and the pending hardware acceptance requirements.
