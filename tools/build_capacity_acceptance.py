"""Build the manual Test3 capacity diagnostics kit for the pinned Test1 runtime."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import shutil
import zipfile
from build_mod import build_project
from runtime_limits import render

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_RUNTIME = '930d3043c61ee457da833d698836886e20a103a5eab68158d7d786439fbf128f'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(output, devkit=None, runtime=None):
    if output.exists():
        raise FileExistsError('Choose a fresh output directory')
    if runtime is not None and sha(runtime) != EXPECTED_RUNTIME:
        raise ValueError('The selected runtime is not the pinned Test1 candidate')
    output.mkdir(parents=True)
    artifact = output/'plugin-build'
    with (output/'build.log').open('w',encoding='utf-8') as log, contextlib.redirect_stdout(log):
        build_project(ROOT/'examples/capacity-acceptance/project.json',artifact,devkit)
    metadata = json.loads((artifact/'plugin-build.json').read_text())
    if metadata['image_bytes'] <= 2*1024*1024 or metadata['bss_bytes'] <= 2*1024*1024:
        raise ValueError('The probe no longer crosses both default module limits')
    stage = output/'KIT'
    plugin = stage/'INSTALL/switch/gtav-scripthook/plugins/capacity-acceptance.nro.manual'
    plugin.parent.mkdir(parents=True)
    shutil.copy2(artifact/'plugin.nro',plugin)
    profiles = stage/'profiles'; profiles.mkdir()
    explicit = json.loads((ROOT/'examples/capacity-acceptance/project.json').read_text())['runtime_limits']
    settings = {'Defaults':{}, 'Explicit':explicit, 'Automatic':dict.fromkeys(explicit,0)}
    for name,values in settings.items():
        (profiles/(name.lower()+'.ini')).write_text(render(values),encoding='utf-8',newline='\n')
    manifest = {'format':1,'probe_revision':3,'report_schema':2,'runtime_sha256':EXPECTED_RUNTIME,'plugin_sha256':sha(plugin),
        'profiles':{name:sha(profiles/(name.lower()+'.ini')) for name in settings}}
    (stage/'profile-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    shutil.copy2(ROOT/'examples/capacity-acceptance/Select-Profile.ps1',stage/'Select-Profile.ps1')
    shutil.copy2(ROOT/'tools/capacity_diagnostics.py',stage/'Review-Report.py')
    (stage/'Set-Test-Profile.cmd').write_bytes(
        b'@echo off\r\npowershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Select-Profile.ps1" %*\r\nexit /b %errorlevel%\r\n')
    guide = (ROOT/'examples/capacity-acceptance/README.md').read_text(encoding='utf-8')
    guide = guide.replace('../../docs/verification/CAPACITY-ACCEPTANCE1.md',
        'https://github.com/Geekmaxxer/ScriptHookV-NX/blob/main/docs/verification/CAPACITY-ACCEPTANCE1.md')
    (stage/'READ-ME-FIRST.md').write_text(guide,encoding='utf-8')
    for name in ('LICENSE','NOTICE','THIRD-PARTY-NOTICES.txt'):
        shutil.copy2(ROOT/name,stage/name)
    files = {p.relative_to(stage).as_posix():sha(p) for p in stage.rglob('*') if p.is_file()}
    archive = output/'ScriptHookV-NX-Capacity-Diagnostics-Test3.zip'
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED) as z:
        for name in sorted(files):
            z.write(stage/name,name)
    with zipfile.ZipFile(archive) as z:
        if set(z.namelist()) != set(files) or any(hashlib.sha256(z.read(n)).hexdigest()!=h for n,h in files.items()):
            raise ValueError('Capacity kit ZIP inventory/readback mismatch')
    result = {'format':1,'probe_revision':3,'report_schema':2,'plugin_sha256':sha(plugin),'expected_runtime_sha256':EXPECTED_RUNTIME,
        'module':{k:metadata[k] for k in ('image_bytes','bss_bytes','accounted_bytes','minimum_runtime_limits')},
        'profiles':settings,'files':files,'zip_sha256':sha(archive),'zip_bytes':archive.stat().st_size,
        'zip_readback_verified':True,'runtime_module_included':False,'runtime_ini_automatically_installed':False,
        'game_files_required_for_build':False,'switch_runtime_verified':False,'installed':False,'published':False}
    (output/'package-verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path);parser.add_argument('--devkitpro',type=Path)
    parser.add_argument('--runtime',type=Path,help='Optionally verify an existing Test1 module; it is not bundled')
    args=parser.parse_args()
    result=build(args.output.resolve(),args.devkitpro,args.runtime)
    print(json.dumps({k:result[k] for k in ('plugin_sha256','zip_bytes','zip_readback_verified')}))
