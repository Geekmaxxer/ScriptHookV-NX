# Formatted text

This standalone project demonstrates `snprintf` and reusable `va_list` formatting
with integer, string and floating arguments. It requires the C and numeric
profiles and the existing owner-memory service. No new runtime table is needed.

```powershell
python tools/build_mod.py --project examples/formatted-text/project.json --output out/formatted-text --devkitpro C:/devkitPro
```

The script completes naturally and writes
`settings/formatted-text-example/result.txt`. A passing report has `failed=0`,
`va_list_verified=1`, `truncation_verified=1`, `rejected_format_verified=1` and
`owned_allocations=0`. Its formatted sample is
`profile=demo count=2699 value=+12.50%`. Loading again increments `run` without
retaining heap state. There is no gameplay effect or visible overlay.

The example is locally verified; its physical acceptance can join a later
combined batch. The accepted batch 4 XML probe does not need repetition.
See the [format contract](../../docs/reference/FORMATTED-TEXT.md) for supported
conversions, failure behavior and precision limits.
