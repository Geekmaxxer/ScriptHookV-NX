# Repository organization

The repository separates runtime implementation, public SDK contracts, complete
examples, automation and validation. Established `include/`, `sdk/` and build
command paths remain stable for downstream mod projects.

The root uses responsibility-based names: `runtime/` contains the injected game
runtime, `mod-porter/` contains the Windows application, and `packaging/` contains
shared installation metadata. These replace `source/`, `desktop/` and
`distribution/`, respectively. Use the current paths in scripts and integrations;
older verification records retain the layout of the artifacts they describe.

## Source ownership

| Path | Contents and ownership |
| --- | --- |
| `runtime/core/` | Target discovery, configuration, memory and storage services |
| `runtime/script/` | Native invocation and cooperative script scheduling |
| `runtime/plugins/` | NRO loading, owned resources, callbacks, heap and lifecycle |
| `runtime/bridge/` | SanScript-facing native and persistence integration |
| `include/` | Public ABI and plugin-facing headers |
| `sdk/` | Freestanding plugin shim, linker/startup files and licensed dependencies |
| `examples/` | Standalone projects and tutorials; gameplay examples under `examples/mods/` |
| `tests/` | Regression runners; compiled inputs under `tests/fixtures/` |
| `tools/` | Developer-facing build/configuration/export commands and maintainer utilities |
| `mod-porter/` | Mod Porter application, its platform libraries and dedicated tests |
| `targets/` | Audited target profiles; no extracted game executables |
| `packaging/` | Shared SD-access metadata and installer |

## Documentation responsibilities

- `docs/guides/`: complete user/developer workflows and expected results.
- `docs/reference/`: API contracts, ownership, limits and compatibility.
- `docs/development/`: builds, contributions, organization and maintenance.
- `docs/verification/`: dated implementation/validation summaries.
- `docs/archive/`: historical preview notes and maintainer research context.
- `docs/COMPLETION-ROADMAP.md` and `docs/PC-RUNTIME-ROADMAP.md`: checked implementation and acceptance milestones.

An introduction explains the project and entry points. Reference pages describe
contracts. Verification pages preserve artifact identities, test scope and open
acceptance work. Avoid accumulating release chronology in introductions.

## Generated and private material

`build/`, `out/`, `projects/` and `releases/` are generated local outputs. The
maintainer workspace retains immutable historical evidence under `evidence/`;
public exports select compact summaries and acceptance records explicitly.
Exported checkouts keep those selected reports visible to Git; the workspace's
blanket archive exclusion is removed only from the exported `.gitignore`.
Do not rewrite historical snapshots to match newer source or move private game
inputs into the repository. No blanket copy of the workspace is a source release.

The source exporter includes licensed dependency headers/C sources and notices,
example project manifests, public tests and documentation links. It excludes raw
game images, databases, private logs, settings, build trees and installable modules.
The shared NPDM is an explicitly documented packaging dependency.

Superseded release-specific packagers and evidence-capture scripts are retained
in the external maintainer recovery archive. They are not current build commands
and are excluded from this source tree. Use `tools/export_source.py` for source
exports, `tools/package_runtime.py` for runtime bundles, and
`mod-porter/package.py` for Mod Porter bundles.

IDE state, Python bytecode and .NET `bin/` and `obj/` directories are disposable
local output. They are ignored by version control and excluded from exports.
Removing them does not remove source, third-party notices or acceptance records.

## Conventions

- Give each source file a single responsibility and document ownership at APIs.
- Use explicit errors for unsupported operations; preserve caller data on failure.
- Keep public APIs and optional service versions distinct from internal helpers.
- Use descriptive documentation titles, short introductions and navigable sections.
- Use checklists for roadmaps; separate implementation, local tests and hardware acceptance.
- Retain dependency licenses/provenance and stable downstream SDK paths.

Run `python tools/check_repository.py` after layout or documentation changes.
Run `python tools/verify_repo.py --output out/verification` for public source checks.
New runtime behavior also needs its relevant compiled and hardware acceptance work.
