"""Publish the Windows desktop tool, matched SDK and source archive."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import zipfile
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from sd_permissions import copy_distribution

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.9.1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def porter_guide(name='README.md'):
    # The bundle guide lives at its root; the original is in desktop/.
    return (ROOT/'desktop'/name).read_text(encoding='utf-8').replace('](../docs/', '](docs/')


def check_documentation(folder):
    count=0
    for path in folder.rglob('*.md'):
        for link in re.findall(r'\[[^\]]*\]\(([^)]+)\)',path.read_text(encoding='utf-8-sig')):
            value=link.strip('<>').split('#',1)[0]
            if not value or '://' in value or value.startswith('mailto:'):continue
            target=(path.parent/value).resolve()
            if not target.is_relative_to(folder.resolve()) or not target.exists():raise ValueError('Broken packaged guide link: '+str(path)+' -> '+value)
            count+=1
    return count


def public_guide(name, destination):
    # The lean desktop bundle does not contain the entire runtime repository.
    # Keep developer-guide links valid without copying private/build directories.
    source=ROOT/'docs'/name
    def link(match):
        value=match.group(2)
        if '://' in value or value.startswith('#'):return match.group(0)
        path,separator,anchor=value.partition('#');target=(source.parent/path).resolve()
        if not target.is_relative_to(ROOT) or not target.is_file():raise ValueError('Invalid guide link: '+value)
        url='https://github.com/Geekmaxxer/ScriptHookV-NX/blob/main/'+target.relative_to(ROOT).as_posix()
        return '['+match.group(1)+']('+url+(separator+anchor if separator else '')+')'
    destination.write_text(re.sub(r'\[([^\]]+)\]\(([^)]+)\)',link,source.read_text(encoding='utf-8')),encoding='utf-8')


def run(*args):
    subprocess.run([str(a) for a in args], check=True, cwd=ROOT)


def archive(folder, output):
    files = sorted(p for p in folder.rglob('*') if p.is_file())
    with zipfile.ZipFile(output, 'x', compression=zipfile.ZIP_DEFLATED) as z:
        for path in files:
            z.write(path, path.relative_to(folder).as_posix())
    with zipfile.ZipFile(output) as z:
        if z.testzip() is not None:
            raise ValueError('Archive CRC verification failed')
        for path in files:
            if hashlib.sha256(z.read(path.relative_to(folder).as_posix())).hexdigest() != sha(path):
                raise ValueError('Archive readback differs: '+str(path))


def package(output):
    if output.exists():
        raise FileExistsError('Select a new release directory')
    output.mkdir(parents=True)
    stage = output/'Windows'
    run('dotnet', 'publish', 'desktop/ModPorter.App/ModPorter.App.csproj', '-c', 'Release',
        '-r', 'win-x64', '--self-contained', 'true', '-p:PublishSingleFile=true',
        '-p:IncludeNativeLibrariesForSelfExtract=true', '-p:EnableCompressionInSingleFile=true',
        '-p:DebugType=None', '-p:DebugSymbols=false', '-o', stage)
    run('dotnet', 'publish', 'desktop/ModPorter.Cli/ModPorter.Cli.csproj', '-c', 'Release',
        '-r', 'win-x64', '--self-contained', 'false', '-p:DebugType=None',
        '-p:DebugSymbols=false', '-o', stage/'developer-cli')
    for name in ('include', 'sdk'):
        shutil.copytree(ROOT/name, stage/'SDK'/name)
    copy_distribution(stage/'SDK')
    (stage/'SDK/tools').mkdir()
    for name in ('build_plugin.py', 'doctor.py', 'inspect_mod.py', 'native_query.py', 'create_texture_demo.py', 'create_mod.py', 'build_mod.py', 'native_bindings.py', 'runtime_limits.py', 'configure_runtime.py', 'check_examples.py', 'report_summary.py', 'run_ui_tests.py', 'sd_permissions.py'):
        shutil.copy2(ROOT/'tools'/name, stage/'SDK/tools'/name)
    for name in ('LICENSE', 'NOTICE', 'THIRD-PARTY-NOTICES.txt'):
        shutil.copy2(ROOT/name, stage/name)
        shutil.copy2(ROOT/name, stage/'SDK'/name)
    shutil.copytree(ROOT/'desktop/examples', stage/'examples')
    shutil.copytree(ROOT/'desktop/third-party', stage/'third-party')
    (stage/'GUIDE.md').write_text(porter_guide(), encoding='utf-8')
    shutil.copy2(ROOT/'desktop/CHANGELOG.md', stage/'WHATS-NEW.md')
    (stage/'COMPATIBILITY.md').write_text(porter_guide('COMPATIBILITY.md'),encoding='utf-8')
    (stage/'docs').mkdir()
    shutil.copy2(ROOT/'docs/GAME-ASSETS.md', stage/'docs/GAME-ASSETS.md')
    shutil.copy2(ROOT/'docs/AUTOMATION-AUDIT.md', stage/'docs/AUTOMATION-AUDIT.md')
    public_guide('QUALITY-AUDIT.md', stage/'docs/QUALITY-AUDIT.md')
    public_guide('MOD-PORTER-WORKFLOWS.md', stage/'docs/MOD-PORTER-WORKFLOWS.md')
    public_guide('EXTENSIONS.md', stage/'docs/EXTENSIONS.md')
    public_guide('RUNTIME-COMPLETION.md', stage/'docs/RUNTIME-COMPLETION.md')
    public_guide('CONTROLLER-UI.md', stage/'docs/CONTROLLER-UI.md')
    for folder, suffixes in (('examples', {'.cpp', '.sc'}), ('tests', {'.cpp'})):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and path.suffix in suffixes:
                destination = stage/'SDK'/path.relative_to(ROOT)
                destination.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(path, destination)
    (stage/'READ-ME-FIRST.md').write_text(
        '# ScriptHookV-NX Mod Porter 0.9.1 Public Preview\n\n'
        'Open ScriptHookV-NX-Mod-Porter.exe on Windows x64. Keep the SDK and examples '
        'folders beside it. Import a mod or try the included example.\n\n'
        'To build NROs, install Python 3.10+, devkitA64 and switch-tools, then check '
        'their paths in Tools > Build tools. The desktop app includes its .NET runtime.\n\n'
        'Use Build NRO for unpacked output, or Build NRO + Install ZIP to package it. Both include shared SD-access metadata. For a customized main.npdm, run Install-SD-Access.cmd from the build output before copying other files. '
        'Game assets offers folder-only and + ZIP actions for assets, PED add-ons and DLC registration. '
        'DLC collection combines many PEDs/vehicles, includes a full PED metadata and asset editor, and omits update.rpf when the selected installation already registers that pack. '
        'Find installed DLCs opens existing local packs from your selected game folder; complete PED asset replacement and filtered movement profiles support single-step Undo. '
        'The collection page separates choose, add/review and save. See docs/MOD-PORTER-WORKFLOWS.md to choose a workflow. '
        'Add to game builds updated copies from your current archives. See docs/GAME-ASSETS.md. Use Check C++ for compiler diagnostics, or expand SD installation to install with a verified backup and restore it later.\n\n'
        'This release ports supported C++ source and analyzes ASI dependencies. '
        'It does not translate compiled ASI binaries. Generated NROs need Switch testing.\n\n'
        'See GUIDE.md for the workflow. Created by MEGATARD (@Geekmaxver). '
        'Source: https://github.com/Geekmaxxer/ScriptHookV-NX\n', encoding='utf-8')
    dotnet = Path(shutil.which('dotnet')).resolve().parent
    notices = stage/'dotnet-notices'
    notices.mkdir()
    for name in ('LICENSE.txt', 'ThirdPartyNotices.txt'):
        if not (dotnet/name).is_file():
            raise FileNotFoundError('Missing .NET redistribution notice: '+name)
        shutil.copy2(dotnet/name, notices/name)
    app_zip = output/f'ScriptHookV-NX-Mod-Porter-{VERSION}-Windows-x64.zip'
    guide_links=check_documentation(stage)
    archive(stage, app_zip)

    source = output/'Source'/'ScriptHookV-NX-Mod-Porter'
    source.mkdir(parents=True)
    for path in (ROOT/'desktop').rglob('*'):
        if not path.is_file() or any(p in ('bin', 'obj', '.vs', '__pycache__') for p in path.relative_to(ROOT/'desktop').parts):
            continue
        dest = source/'desktop'/path.relative_to(ROOT/'desktop')
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dest)
    for name in ('include', 'sdk'):
        shutil.copytree(ROOT/name, source/name)
    copy_distribution(source)
    (source/'tools').mkdir()
    for name in ('build_plugin.py', 'doctor.py', 'inspect_mod.py', 'native_query.py', 'create_texture_demo.py', 'create_mod.py', 'build_mod.py', 'native_bindings.py', 'runtime_limits.py', 'configure_runtime.py', 'check_examples.py', 'report_summary.py', 'run_ui_tests.py', 'sd_permissions.py'):
        shutil.copy2(ROOT/'tools'/name, source/'tools'/name)
    for name in ('LICENSE', 'NOTICE', 'THIRD-PARTY-NOTICES.txt'):
        shutil.copy2(ROOT/name, source/name)
    (source/'.gitignore').write_text('**/bin/\n**/obj/\n**/.vs/\n__pycache__/\n/releases/\n', encoding='utf-8')
    shutil.copy2(ROOT/'desktop/README.md', source/'README.md')
    (source/'docs').mkdir()
    shutil.copy2(ROOT/'docs/GAME-ASSETS.md', source/'docs/GAME-ASSETS.md')
    shutil.copy2(ROOT/'docs/AUTOMATION-AUDIT.md', source/'docs/AUTOMATION-AUDIT.md')
    public_guide('QUALITY-AUDIT.md', source/'docs/QUALITY-AUDIT.md')
    public_guide('MOD-PORTER-WORKFLOWS.md', source/'docs/MOD-PORTER-WORKFLOWS.md')
    public_guide('EXTENSIONS.md', source/'docs/EXTENSIONS.md')
    public_guide('RUNTIME-COMPLETION.md', source/'docs/RUNTIME-COMPLETION.md')
    public_guide('CONTROLLER-UI.md', source/'docs/CONTROLLER-UI.md')
    for folder, suffixes in (('examples', {'.cpp', '.sc'}), ('tests', {'.cpp'})):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file() and path.suffix in suffixes:
                destination = source/path.relative_to(ROOT)
                destination.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(path, destination)
    (source/'README.md').write_text(porter_guide(),encoding='utf-8')
    (source/'COMPATIBILITY.md').write_text(porter_guide('COMPATIBILITY.md'),encoding='utf-8')
    source_zip = output/f'ScriptHookV-NX-Mod-Porter-{VERSION}-Source.zip'
    source_guide_links=check_documentation(source)
    archive(output/'Source', source_zip)
    result = {
        'version': VERSION, 'windows_x64': {'file': app_zip.name, 'bytes': app_zip.stat().st_size, 'sha256': sha(app_zip)},
        'source': {'file': source_zip.name, 'bytes': source_zip.stat().st_size, 'sha256': sha(source_zip)},
        'sdk_header_sha256': sha(ROOT/'include/natives.h'), 'binary_asi_translation': False,
        'game_files_included': False, 'core_runtime_included': False, 'switch_runtime_verified': False,
        'zip_readback_verified': True,
        'windows_documentation_links': guide_links, 'source_documentation_links': source_guide_links,
        'files': {p.relative_to(stage).as_posix(): sha(p) for p in stage.rglob('*') if p.is_file()}
    }
    (output/'package-verification.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k != 'files'}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    arguments = parser.parse_args()
    package(arguments.output.resolve())
