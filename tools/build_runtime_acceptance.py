"""Build a manual combined-service probe and a self-contained console test kit."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import shutil
import zipfile
from build_mod import build_project
from create_texture_demo import create
from sd_permissions import stage as stage_sd_access

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(output, devkit=None, runtime=None):
    if output.exists():
        raise FileExistsError('Choose a fresh output directory')
    if runtime is not None and not runtime.is_file():
        raise FileNotFoundError('Intended runtime module not found: '+str(runtime))
    output.mkdir(parents=True)
    artifact = output / 'plugin-build'
    with (output/'build.log').open('w',encoding='utf-8') as log, contextlib.redirect_stdout(log):
        build_project(ROOT/'examples/runtime-acceptance/project.json', artifact, devkit)
    install = output/'INSTALL'
    target = install/'switch/gtav-scripthook/plugins/runtime-acceptance.nro.manual'
    target.parent.mkdir(parents=True)
    shutil.copy2(artifact/'plugin.nro', target)
    create(install/'switch/gtav-scripthook/assets/runtime-acceptance/probe.png')
    stage_sd_access(install)
    for name in ('LICENSE','NOTICE','THIRD-PARTY-NOTICES.txt'):
        shutil.copy2(ROOT/name,install/name)
    checklist=(ROOT/'examples/runtime-acceptance/README.md').read_text(encoding='utf-8')
    checklist=checklist.replace('../../docs/verification/PLATFORM-CAPACITY1.md',
        'https://github.com/Geekmaxxer/ScriptHookV-NX/blob/main/docs/verification/PLATFORM-CAPACITY1.md')
    (install/'HARDWARE-CHECKLIST.md').write_text(checklist,encoding='utf-8')
    (install/'READ-ME-FIRST.md').write_text('''# Combined runtime acceptance probe

This is a manual test plugin for the current optional-service runtime candidate.
It does not include or replace subsdk4/subsdk5. Preserve the working DLC layout.

Close GTA. Back up the probe's existing settings and shared main.npdm, then copy
switch and atmosphere to the SD root. For a customized NPDM, use the included
Install-SD-Access.cmd and preserve the patched file when copying other files.
Do not change runtime.ini for the first run. Load runtime-acceptance in MG Menu.

The probe displays a small colored square, samples borrowed entity handles,
and writes only its isolated settings/runtime-acceptance/ test data. It overwrites
its own payload.bin, snapshot.xml, status.txt and cleanup.txt on repeated runs.
Other mods, game entities and save data are not edited. Read HARDWARE-CHECKLIST.md
before running it; stop and restart through the plugin manager to collect cleanup.

This package passed its recorded local checks only. It requires physical testing.
GAME-STARTUP.md covers the separate known DLC path problem.
''',encoding='utf-8')
    files = {p.relative_to(install).as_posix():sha(p) for p in install.rglob('*') if p.is_file()}
    archive = output/'ScriptHookV-NX-Combined-Acceptance-Test1.zip'
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED) as z:
        for name in sorted(files):
            z.write(install/name,name)
    with zipfile.ZipFile(archive) as z:
        if set(z.namelist()) != set(files) or any(hashlib.sha256(z.read(n)).hexdigest()!=h for n,h in files.items()):
            raise ValueError('Install ZIP inventory or readback mismatch')
    proof = dict(format=1, plugin_sha256=sha(artifact/'plugin.nro'),
        expected_runtime_sha256=sha(runtime) if runtime else None,
        runtime_module_included=False, game_assets_included=False, generated_png_included=True,
        startup_policy='Manual', files=files, zip_sha256=sha(archive), zip_bytes=archive.stat().st_size,
        zip_readback_verified=True, switch_runtime_verified=False, installed=False, published=False)
    (output/'package-verification.json').write_text(json.dumps(proof,indent=2)+'\n',encoding='utf-8')
    return proof


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--devkitpro',type=Path)
    parser.add_argument('--runtime',type=Path,help='Record the intended existing runtime hash; it is not packaged')
    args=parser.parse_args()
    result=build(args.output.resolve(),args.devkitpro,args.runtime)
    print(json.dumps({k:result[k] for k in ('plugin_sha256','zip_bytes','zip_readback_verified')}))
