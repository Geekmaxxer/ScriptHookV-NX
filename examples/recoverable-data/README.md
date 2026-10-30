# Recoverable XML data

This example stores an XML launch counter in two checked records. It requires
the candidate memory runtime and C/XML SDK profile.

```powershell
python tools/build_mod.py --project examples/recoverable-data/project.json --output out/recoverable-data --devkitpro C:/devkitPro
```

Files live under `switch/gtav-scripthook/settings/recoverable-data-example/`.
The example explicitly initializes its dedicated sample records when none validate,
increments the counter and verifies the saved payload. `current.xml` contains the
verified XML; `result.txt` contains `saved=yes` on success. Load Now increments
the counter again. Existing files survive replacement of the plugin itself.

Production mods must choose an explicit initialization/recovery policy. An
unreadable record is not proof that a file is absent. Ordinary Save refuses an
unreadable peer; explicit Repair authorizes recovery from the remaining valid
record. Corrupting a sample slot must not be attempted on personal production data.

The workflow and recovery behavior are locally checked with modeled OS services.
Actual storage failure/power-loss acceptance remains pending. See
[the storage contract](../../docs/reference/RECOVERABLE-DATA.md) and retain the
project and pugixml notices when distributing the compiled example.
