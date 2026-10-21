"""Fetch pinned PC sources, check blockers, and build Footprints and Disarm NX ports.

No downloaded plugin or third-party build script is executed. Run after building
ModPorter.Cli in Release. Output must be a new folder outside the SDK.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request


def verify(sdk, output, devkit, python):
    if output.exists():
        raise FileExistsError('Choose a new verification output folder')
    output.mkdir(parents=True)
    cli = sdk/'desktop/ModPorter.Cli/bin/Release/net10.0/shvnx-port.dll'
    if not cli.is_file():
        raise FileNotFoundError('Build ModPorter.Cli in Release first')
    fixtures = [
        ('root-cause/v-footprints', '750fc538a37364da655485decdfbdfbee45b83ad',
         ['Footprints/Main.cpp', 'Footprints/Script.cpp', 'Footprints/Header.hpp', 'LICENSE.md'], {'PC_CHEAT_INPUT'}),
        ('CamxxCore/RealisticReload', 'cc902a6c0b6ab937a2320faffd331517358baeaa',
         ['ManualReload/main.cpp', 'ManualReload/stdafx.h', 'ManualReload/dllmain.cpp'], {'API_GETGAMEVERSION', 'API_GETSCRIPTHANDLEBASEADDRESS', 'API_BYTEPATTERN'}),
        ('ItsMeDeli/VaritrafV', '6ac844f16851a4512959c395862b066755e864a6',
         ['src/main.cpp', 'src/hook.cpp', 'src/hook.h', 'src/resolve.cpp', 'src/resolve.h', 'src/log.h', 'src/dedup.h'], {'API_MH_CREATEHOOK'}),
        ('ikt32/GTAVDisarm', '92d6646e0c649fb6c387189758abdcb72ab376cc',
         ['Disarm/main.cpp', 'Disarm/script.cpp', 'Disarm/script.h'], {'API_WORLDGETALLPEDS'}),
    ]
    results = []
    def command(*args, expected=0):
        p = subprocess.run(['dotnet', str(cli), *map(str, args)], capture_output=True, encoding='utf-8')
        if p.returncode != expected:
            raise RuntimeError(p.stdout+p.stderr)
        return p.stdout
    for repo, commit, paths, expected_codes in fixtures:
        folder = output/repo.replace('/', '-')
        hashes = {}
        for relative in paths:
            url = f'https://raw.githubusercontent.com/{repo}/{commit}/{relative}'
            request = urllib.request.Request(url, headers={'User-Agent': 'ScriptHookV-NX-Mod-Porter-tests'})
            with urllib.request.urlopen(request, timeout=30) as response:
                data = response.read(2 * 1024 * 1024 + 1)
            if len(data) > 2 * 1024 * 1024:
                raise ValueError('Unexpectedly large source file')
            target = folder/relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            hashes[relative] = hashlib.sha256(data).hexdigest()
        report = json.loads(command('analyze', folder, '--sdk', sdk, expected=2))
        codes = {f['code'] for f in report['findings']}
        if not expected_codes <= codes:
            raise AssertionError(f'{repo}: missing expected blockers {expected_codes-codes}')
        results.append(dict(repository='https://github.com/'+repo, commit=commit, source_sha256=hashes,
                            expected_blockers=sorted(expected_codes), blockers_detected=True, compiled=False))
        print('PASS real source analysis: '+repo)
    project = output/'Footprints NX'
    command('port', sdk/'desktop/examples/real-mod-footprints', '--sdk', sdk, '--output', project, '--id', 'footprints-nx')
    command('build', project/'project.json', '--sdk', sdk, '--devkitpro', devkit, '--python', python)
    build = next((project/'builds').glob('*/port-build.json'))
    manifest = json.loads(build.read_text(encoding='utf-8'))
    disarm = output/'Disarm NX'
    command('port', sdk/'desktop/examples/disarm-nx', '--sdk', sdk, '--output', disarm, '--id', 'disarm-nx')
    command('build', disarm/'project.json', '--sdk', sdk, '--devkitpro', devkit, '--python', python)
    disarm_build = next((disarm/'builds').glob('*/port-build.json'))
    result = dict(real_pc_mod_analysis=results, footprints=manifest,
                  disarm=json.loads(disarm_build.read_text(encoding='utf-8')),
                  adaptations='Explicit controller input and bounded particle streaming; original feature retained',
                  console_test_required=True, switch_runtime_verified=False)
    (output/'real-mod-verification.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print('PASS real Footprints AArch64 build and structural NRO validation')
    print('PASS bounded Disarm NX AArch64 build and structural NRO validation')
    print(build.parent/'footprints-nx-Switch-test.zip')
    print(disarm_build.parent/'disarm-nx-Switch-test.zip')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sdk', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--devkitpro', type=Path, required=True)
    p.add_argument('--python', default='python')
    a = p.parse_args()
    verify(a.sdk.resolve(), a.output.resolve(), a.devkitpro.resolve(), a.python)
