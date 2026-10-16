"""Build a generated project, preserving source snapshots and matched NSC bindings."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

from build_plugin import build
from native_bindings import generate
from runtime_limits import render, validate

def inputs(project):
    data = json.loads(project.read_text(encoding='utf-8'))
    if not isinstance(data, dict) or data.get('format') != 1 or data.get('kind') not in ('nro', 'nsc'):
        raise ValueError('Expected a format 1 NRO/NSC project.json')
    def paths(key):
        values = data.get(key, [])
        if not isinstance(values, list) or not all(isinstance(v, str) for v in values):
            raise ValueError(key+' must be an array of relative paths')
        result = []
        for value in values:
            path = (project.parent/value).resolve()
            if not path.is_relative_to(project.parent.resolve()):
                raise ValueError('Project paths must stay inside its directory: '+value)
            if not path.exists():
                raise FileNotFoundError(str(path))
            result.append(path)
        return result
    sources, includes = paths('sources'), paths('includes')
    if not sources or any(not p.is_file() for p in sources):
        raise ValueError('Provide at least one source file')
    if 'runtime_limits' in data:
        validate(data['runtime_limits'])
    return data, sources, includes

def build_project(project, output, devkit=None, menu_sdk=None, dev_ng=None, reference=None):
    if output.exists():
        raise FileExistsError('Use a new output directory to preserve prior builds')
    data, sources, includes = inputs(project)
    if data['kind'] == 'nro':
        build(sources, output, devkit, includes)
        shutil.copy2(project, output/'project.json')
        if 'runtime_limits' in data:
            # Optional suggestion beside the NRO, never auto-installed globally.
            (output/'runtime.ini').write_text(render(data['runtime_limits']), encoding='utf-8')
        return output/'plugin.nro'
    if len(sources) != 1 or sources[0].suffix.lower() != '.sc':
        raise ValueError('Each NSC project builds one named .sc entry')
    if includes:
        raise ValueError('NSC projects use flat sibling .sch includes; project includes is currently NRO-only')
    if not menu_sdk or not dev_ng or not reference:
        raise ValueError('NSC builds require --menu-sdk, --dev-ng and --header-reference')
    wrapper = menu_sdk/'tools/Build-Any.ps1'
    if not wrapper.is_file() or not reference.exists():
        raise FileNotFoundError('Provide an MG-SwitchMenu SDK and an exact-build stock NSC reference file/folder')
    powershell = shutil.which('pwsh') or shutil.which('powershell')
    if not powershell:
        raise FileNotFoundError('NSC compilation requires PowerShell and the separate SanScript toolchain')
    # Use the established compiler/container/header adapter; never duplicate or
    # guess header mutations here. NSC includes and the entry are snapshotted too.
    command = [powershell, '-NoProfile', '-File', str(wrapper.resolve()), '-DevNgRoot', str(dev_ng.resolve()),
               '-SwitchHeaderReferenceFolder' if reference.is_dir() else '-SwitchHeaderReference', str(reference.resolve()),
               '-OutputDirectory', str(output.resolve()), '-Source', str(sources[0])]
    source_snapshot = {p.name: p.read_bytes() for p in sources[0].parent.glob('*.sch')}
    source_snapshot[sources[0].name] = sources[0].read_bytes()
    if sum(len(data) for data in source_snapshot.values()) > 16*1024*1024:
        raise ValueError('NSC source snapshot exceeds 16 MiB')
    subprocess.run(command, check=True)
    snapshot = output/'project-source'
    snapshot.mkdir()
    for name, content in source_snapshot.items():
        if (sources[0].parent/name).read_bytes() != content:
            raise ValueError('NSC source changed during compilation: '+name)
        (snapshot/name).write_bytes(content)
    name = sources[0].stem
    script = output/(name+'.nsc')
    proof = generate([script], output/(name+'.snb'), [int(v, 0) for v in data.get('custom_hashes', [])])
    proof['source_sha256'] = {p.relative_to(snapshot).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                              for p in snapshot.rglob('*') if p.is_file()}
    proof['switch_runtime_verified'] = False
    (output/'project-build.json').write_text(json.dumps(proof, indent=2)+'\n')
    shutil.copy2(project, output/'project.json')
    if 'runtime_limits' in data:
        (output/'runtime.ini').write_text(render(data['runtime_limits']), encoding='utf-8')
    return script

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project', type=Path, required=True); p.add_argument('--output', type=Path, required=True)
    p.add_argument('--devkitpro', type=Path); p.add_argument('--menu-sdk', type=Path)
    p.add_argument('--dev-ng', type=Path); p.add_argument('--header-reference', type=Path)
    a = p.parse_args()
    try:
        result = build_project(a.project.resolve(), a.output.resolve(), a.devkitpro, a.menu_sdk, a.dev_ng, a.header_reference)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        p.exit(1, str(error)+'\n')
    print('Built: '+str(result))

if __name__ == '__main__':
    main()
