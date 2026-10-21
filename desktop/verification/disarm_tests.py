"""Compile/run the real Disarm port against a host native simulator.

Uses an installed MSVC, clang++ or g++; no game files or runtime changes.
This validates C++ control flow and buffer layout, not Switch gameplay.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def compiler_environment(explicit):
    env = dict(os.environ)
    if explicit:
        return str(explicit), env
    for name in ('clang++', 'g++'):
        if shutil.which(name):
            return shutil.which(name), env
    vswhere = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    if os.name == 'nt' and vswhere.is_file():
        installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
        setup = Path(installation)/'VC/Auxiliary/Build/vcvars64.bat'
        if setup.is_file():
            # Read the compiler environment only. No filesystem operation is
            # delegated to another shell and no downloaded command is executed.
            result = subprocess.run(f'cmd.exe /d /s /c ""{setup}" >nul && set"', capture_output=True, text=True, check=True)
            for line in result.stdout.splitlines():
                if '=' in line and not line.startswith('='):
                    key, value = line.split('=', 1)
                    env[key] = value
            compiler = shutil.which('cl.exe', path=env.get('Path', env.get('PATH')))
            if compiler:
                return compiler, env
    raise FileNotFoundError('Install a host C++17 compiler or pass --cxx; devkitA64 builds are a separate check')


def test(sdk, output, explicit):
    if output.exists():
        raise FileExistsError('Choose a new output directory')
    output.mkdir(parents=True)
    compiler, env = compiler_environment(explicit)
    source = sdk/'desktop/verification/disarm_tests.cpp'
    exe = output/('disarm-host-tests.exe' if os.name == 'nt' else 'disarm-host-tests')
    if Path(compiler).name.lower() in ('cl', 'cl.exe'):
        command = [compiler, '/nologo', '/std:c++17', '/EHs', '/W4', '/WX', '/I'+str(sdk/'include'), str(source), '/Fe:'+str(exe), '/Fo:'+str(output/'tests.obj')]
    else:
        command = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I'+str(sdk/'include'), str(source), '-o', str(exe)]
    built = subprocess.run(command, env=env, capture_output=True, text=True)
    (output/'compile.log').write_text(built.stdout+built.stderr, encoding='utf-8')
    if built.returncode:
        raise RuntimeError(built.stdout+built.stderr)
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    (output/'test.log').write_text(result.stdout+result.stderr, encoding='utf-8')
    print(result.stdout)
    counts = re.search(r'RESULT (\d+) passed; (\d+) failed', result.stdout)
    if result.returncode or not counts or int(counts[2]):
        raise RuntimeError('Disarm host simulation failed')
    # Requirements must match the selected SDK, not just the simulator.
    index = json.loads((sdk/'sdk/native-index.json').read_text(encoding='utf-8'))
    by_name = {row['name']: row for row in index['natives']}
    header = (sdk/'desktop/examples/disarm-nx/native_requirements.hpp').read_text(encoding='utf-8')
    requirements = re.findall(r'(0x[0-9a-f]+)ULL, // ([A-Z_]+::[A-Z_]+)', header)
    if len(requirements) != 17 or not all(by_name[name]['registered'] and by_name[name]['hash'] == value for value, name in requirements):
        raise AssertionError('Disarm requirements differ from the registered target declarations')
    report = dict(passed=int(counts[1]), failed=0, compiler=Path(compiler).name,
                  audited_target_natives=len(requirements), sdk_header_sha256=index['header_sha256'],
                  host_simulation=True, switch_runtime_verified=False)
    (output/'verification.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sdk', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--cxx', type=Path)
    a = p.parse_args()
    test(a.sdk.resolve(), a.output.resolve(), a.cxx)
