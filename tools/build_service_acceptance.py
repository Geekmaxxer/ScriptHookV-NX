"""Build deferred batch 8: growing resource registry acceptance."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import shutil
import zipfile
from build_mod import build_project
from inspect_mod import inspect
from sd_permissions import stage as stage_sd_access

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_RUNTIME = 'fdbbeba40b2f04e123e8a2c5f3cd5ae4bdef1ade765ba803ba5b6955fc4e19fa'
PROJECTS = ('resource-registry',)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(output, devkit=None, runtime=None):
    if output.exists():
        raise FileExistsError('Choose a fresh output directory')
    if runtime is None or sha(runtime) != EXPECTED_RUNTIME:
        raise ValueError('Provide the exact growing-resource candidate with --runtime')
    output.mkdir(parents=True)
    stage = output/'KIT'
    plugins = stage/'INSTALL/switch/gtav-scripthook/plugins'
    plugins.mkdir(parents=True)
    artifacts = {}
    for name in PROJECTS:
        artifact = output/name
        with (output/(name+'.log')).open('w',encoding='utf-8') as log, contextlib.redirect_stdout(log):
            build_project(ROOT/'examples'/name/'project.json',artifact,devkit)
        result = inspect(artifact/'plugin.nro')
        if not result['structural_checks_passed']:
            raise ValueError('Structural preflight failed: '+name)
        destination = plugins/(name+'.nro.manual')
        shutil.copy2(artifact/'plugin.nro',destination)
        metadata = json.loads((artifact/'plugin-build.json').read_text())
        artifacts[name] = {key:metadata[key] for key in ('nro_sha256','image_bytes','bss_bytes','accounted_bytes')}
        if metadata.get('math_runtime_enabled'):
            artifacts[name]['math_library'] = metadata['math_library']
    guide = (ROOT/'examples/service-acceptance/README.md').read_text(encoding='utf-8')
    for name in ('RESOURCE-CAPACITY1.md', 'SERVICE-BATCH7.md'):
        guide = guide.replace('../../docs/verification/'+name,
            'https://github.com/Geekmaxxer/ScriptHookV-NX/blob/main/docs/verification/'+name)
    (stage/'READ-ME-FIRST.md').write_text(guide,encoding='utf-8')
    sd_access = stage_sd_access(stage/'RUNTIME-CANDIDATE', ROOT)
    runtime_destination = stage/'RUNTIME-CANDIDATE/atmosphere/contents/0100b00b51230000/exefs/subsdk5'
    shutil.copy2(runtime, runtime_destination)
    manifest = dict(format=1,kit='Service acceptance batch 8',runtime_sha256=EXPECTED_RUNTIME,
                    artifacts=artifacts,runtime_module_included=True,shared_configuration_included=False,
                    asset_files=0,sd_access=sd_access)
    (stage/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    for name in ('LICENSE','NOTICE','THIRD-PARTY-NOTICES.txt'):
        shutil.copy2(ROOT/name,stage/name)
    # Preserve libm's complete notices without bundling its binary archive.
    notices = stage/'third-party/newlib'
    notices.mkdir(parents=True)
    for name in ('LICENSE.txt','provenance.json'):
        shutil.copy2(ROOT/'sdk/third_party/newlib'/name,notices/name)
    notice_file = stage/'THIRD-PARTY-NOTICES.txt'
    notice_file.write_text(notice_file.read_text(encoding='utf-8').replace(
        'sdk/third_party/newlib/','third-party/newlib/'),encoding='utf-8')
    files = {p.relative_to(stage).as_posix():sha(p) for p in stage.rglob('*') if p.is_file()}
    archive = output/'ScriptHookV-NX-Service-Acceptance-Batch8.zip'
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED) as package:
        for name in sorted(files):package.write(stage/name,name)
    with zipfile.ZipFile(archive) as package:
        if set(package.namelist()) != set(files) or any(hashlib.sha256(package.read(n)).hexdigest()!=h for n,h in files.items()):
            raise ValueError('Service kit inventory/readback mismatch')
    result = dict(**manifest,files=files,zip_sha256=sha(archive),zip_bytes=archive.stat().st_size,
                  zip_readback_verified=True,game_files_required_for_build=False,installed=False,
                  switch_runtime_verified=False,published=False)
    (output/'package-verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--devkitpro',type=Path)
    parser.add_argument('--runtime',required=True,type=Path,help='Exact growing-resource candidate module to verify and include')
    args=parser.parse_args()
    try:
        result=build(args.output.resolve(),args.devkitpro,args.runtime)
    except (OSError,ValueError) as error:
        parser.exit(1,str(error)+'\n')
    print(json.dumps({key:result[key] for key in ('zip_sha256','zip_bytes','zip_readback_verified')}))
