# Large XML file example

This example writes a 256 KiB XML sample in 16 KiB steps, loads it into an
owner-backed buffer through cooperative read steps, then parses the completed
buffer with the pinned XML DOM. Registered session cleanup releases both the
input buffer and document after natural completion or Kill.

```powershell
python tools/build_mod.py --project examples/large-xml-file/project.json --output out/large-xml --devkitpro C:/devkitPro
```

The project requires `c_runtime`, `cxx_runtime`, memory, owned resources and file
services. `settings/large-xml-example/sample.xml` begins with
`<root><value>2699</value></root>` followed by spaces to 262,144 bytes. Creation is
exclusive. A later run reads the existing file without rewriting it. Invalid or
interrupted samples are preserved and produce `failed=1` for inspection.

On success, `result.txt` records `verified_bytes=262144`, `xml_value=2699` and
`failed=0`. Final cleanup writes `owner_cleanup_complete=1` to `cleanup.txt`.
No visible overlay, entity mutation or account/save access is performed.

Each transfer step is bounded by bytes, not execution time. File calls remain
synchronous; the example yields between them. DOM parsing is a separate complete
operation after transfer and does not yield internally. The parser can allocate
its own input copy and nodes in addition to the reader's buffer. Choose input
bounds appropriate to the owner heap and the intended workload.

The [transfer contract](../../docs/reference/FILE-TRANSFERS.md) defines progress,
ownership and failure behavior. The [service batch](../service-acceptance/README.md)
groups the hardware check with already accepted services.
