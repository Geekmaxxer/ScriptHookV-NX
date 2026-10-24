"""Export a curated GitHub-ready source tree; do not mirror private workspace output."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import zipfile
from sd_permissions import bundle, copy_distribution

ROOT = Path(__file__).resolve().parents[1]
PUBLIC_TOOLS = ('build_plugin.py', 'native_bindings.py', 'create_mod.py', 'build_mod.py', 'doctor.py',
    'native_query.py', 'inspect_mod.py', 'report_summary.py', 'check_examples.py', 'build.py',
    'generate_bindings.py', 'engine_bindings.py', 'nso.py', 'compare_core.py', 'test_portable.py', 'package_runtime.py',
    'export_source.py', 'generate_natives.py', 'make_native_index.py', 'runtime_limits.py', 'configure_runtime.py', 'create_texture_demo.py', 'run_ui_tests.py', 'verify_repo.py', 'sd_permissions.py', 'test_sd_permissions.py')
ACCEPTANCES = (
    'native-caller-research/hardware-20261015/console-acceptance.json',
    'public-loader-test1-20261015/hardware-20261015/console-acceptance.json',
    'refinement-test2-20261015/hardware-20261015/console-acceptance.json',
    'safe-manager-test1-20261015/hardware-20261016/console-acceptance.json',
    'safe-manager-test2-20261016/hardware-restart-20261016/console-acceptance.json',
    'sdk-parity-test1-20261022/hardware-20261022/console-acceptance.json',
    'runtime-completion-test1-20261022/hardware-20261023/console-acceptance.json',
)

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def documentation_links(stage):
    count = 0
    for path in stage.rglob('*.md'):
        for value in re.findall(r'\[[^\]]*\]\(([^)]+)\)', path.read_text(encoding='utf-8-sig')):
            value = value.strip('<>').split('#', 1)[0]
            if not value or '://' in value: continue
            if not (path.parent/value).exists(): raise ValueError('Broken link: '+str(path)+' -> '+value)
            count += 1
    return count

def export(output):
    if output.exists(): raise FileExistsError('Use a fresh export directory')
    bundle(ROOT)
    stage = output/'repo/ScriptHookV-NX'; stage.mkdir(parents=True)
    selected = [ROOT/name for name in ('README.md', 'CHANGELOG.md', 'LICENSE', 'NOTICE', 'THIRD-PARTY-NOTICES.txt', '.gitignore')]
    selected.extend((ROOT/'.github/ISSUE_TEMPLATE').glob('*.md'))
    selected.extend((ROOT/'.github/workflows').glob('*.yml'))
    extensions = {
        'source': {'.cpp', '.hpp', '.h', '.s', '.ld'}, 'include': {'.h', '.hpp'},
        'sdk': {'.cpp', '.s', '.ld', '.sch', '.json', '.example'}, 'targets': {'.hpp', '.json', '.md'},
        'examples': {'.cpp', '.sc'}, 'tests': {'.cpp'},
        'mods': {'.cpp', '.hpp', '.h', '.md', '.py', '.ps1', '.cmd', '.json', '.txt'}, 'docs': {'.md'},
        'desktop': {'.cs', '.csproj', '.props', '.xaml', '.manifest', '.py', '.md', '.cpp', '.hpp', '.txt', '.json', '.ini'},
    }
    for folder, allowed in extensions.items():
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and path.suffix.lower() in allowed and not any(part.startswith('.') or part in ('__pycache__', 'bin', 'obj') for part in path.relative_to(ROOT/folder).parts):
                if path.name not in ('RESEARCH-WORKSPACE.md', 'native_caller_sample.cpp'): selected.append(path)
    selected.extend(ROOT/'tools'/name for name in PUBLIC_TOOLS)
    selected.append(ROOT/'source/third_party/stb_image.provenance.json')
    selected.extend(ROOT/'evidence'/name for name in ACCEPTANCES)
    # Compact prior regression evidence: source hashes/metrics, no dumps or binaries.
    for relative in ('sdk-refinement1-20261016/sdk-refinement-tests.json',
                     'ecosystem-test1-20261023/verification.json',
                     'ecosystem-test1-20261023/VERIFICATION.md',
                     'runtime-completion-test1-20261022/verification.json',
                     'runtime-completion-test1-20261022/VERIFICATION.md',
                     'sdk-parity-test1-20261022/hardware-20261022/coverage-addendum.json',
                     'sdk-parity-test1-20261022/verification.json',
                     'sdk-parity-test1-20261022/VERIFICATION.md',
                     'sdk-refinement1-20261016/package-verification.json',
                     'mod-porter-20261021/verification.json',
                     'mod-porter-20261021/VERIFICATION.md',
                     'mod-porter-0.2.0-20261021/verification.json',
                     'mod-porter-0.2.0-20261021/footprints-console-acceptance.json',
                     'disarm-nx-test1-20261021/verification.json',
                     'disarm-nx-test1-20261021/console-acceptance.json',
                     'completion-test1-20261021/verification.json',
                     'completion-test1-20261021/VERIFICATION.md',
                     'completion-test1-20261021/console-acceptance.json',
                     'repo-update-20261021/verification.json',
                     'repo-update-20261021/VERIFICATION.md',
                     'disarm-nx-test1-20261021/VERIFICATION.md',
                     'mod-porter-0.2.0-20261021/VERIFICATION.md'):
        selected.append(ROOT/'evidence'/relative)
    for path in selected:
        if path.is_symlink(): raise ValueError('Symlink in public source: '+str(path))
        data = path.read_bytes()
        if b'\0' in data: raise ValueError('Non-text file selected: '+str(path))
        data.decode('utf-8-sig')
        # Acceptance snapshots may contain SD paths; local account/workspace paths
        # are not useful in a public checkout and indicate an export-selection error.
        if re.search(rb'(?i)[a-z]:[\\/]+users[\\/]+[a-z0-9_. -]+', data):
            raise ValueError('Machine-specific path in public source: '+str(path))
        destination = stage/path.relative_to(ROOT); destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, destination)
    copy_distribution(stage,ROOT)
    links = documentation_links(stage)
    files = {p.relative_to(stage).as_posix(): sha(p) for p in stage.rglob('*') if p.is_file()}
    archive = output/'ScriptHookV-NX-Source-Public-Preview.zip'
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as z:
        for relative in sorted(files): z.write(stage/relative, 'ScriptHookV-NX/'+relative)
    with zipfile.ZipFile(archive) as z:
        if set(z.namelist()) != {'ScriptHookV-NX/'+name for name in files}: raise ValueError('ZIP inventory mismatch')
        for name, digest in files.items():
            if hashlib.sha256(z.read('ScriptHookV-NX/'+name)).hexdigest() != digest: raise ValueError('ZIP readback mismatch')
    result = dict(files=files, zip_bytes=archive.stat().st_size, zip_sha256=sha(archive),
                  documentation_links=links, game_or_runtime_binaries_included=False, sd_access_metadata_included=True,
                  private_logs_or_settings_included=False, zip_readback_verified=True,
                  public_tool_count=len(PUBLIC_TOOLS), switch_runtime_verified=False)
    (output/'export-verification.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: result[k] for k in ('zip_bytes', 'documentation_links', 'public_tool_count')}, indent=2))
    return result

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    try: export(a.output.resolve())
    except (OSError, ValueError) as error: p.exit(1, str(error)+'\n')
