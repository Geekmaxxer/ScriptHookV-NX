"""Check an SDK/toolchain installation without changing it or requiring game files."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from build_plugin import toolchain, tool

def diagnose(root, devkit=None, check_toolchain=True, core=False):
    checks = []
    def check(name, ok, detail):
        checks.append(dict(name=name, passed=bool(ok), detail=detail))
    check('python', sys.version_info >= (3, 10), sys.version.split()[0]+'; Python 3.10+ required')
    for relative in ('include/script.h', 'include/plugin.h', 'sdk/plugin_api.cpp', 'sdk/plugin_link.ld',
                     'sdk/plugin_start.s', 'sdk/commands_scripthook_nx.sch', 'tools/native_bindings.py'):
        check(relative, (root/relative).is_file(), str(root/relative))
    index = root/'sdk/native-index.json'
    try:
        data = json.loads(index.read_text())
        check('native index', len(data['natives']) == 8540 and data['header_sha256'] ==
              hashlib.sha256((root/'include/natives.h').read_bytes()).hexdigest(),
              '8,540 wrappers; index must match include/natives.h')
    except (OSError, ValueError, KeyError) as error:
        check('native index', False, str(error))
    if check_toolchain:
        try:
            compiler, converter = toolchain(root, devkit)
            result = subprocess.run([str(compiler), '--version'], capture_output=True, text=True, timeout=15)
            check('devkitA64', result.returncode == 0, result.stdout.splitlines()[0] if result.stdout else result.stderr)
            check('elf2nro', converter.is_file(), str(converter))
            if core:
                for relative in ('tools/bin/elf2nso', 'devkitA64/bin/aarch64-none-elf-readelf'):
                    path = tool(compiler.parents[2], relative)
                    check(relative, True, str(path))
        except (OSError, subprocess.SubprocessError) as error:
            check('devkitPro', False, str(error))
    if core:
        try:
            from generate_bindings import profile_bindings, DEFAULT_PROFILE
            import tempfile
            with tempfile.TemporaryDirectory(prefix='shvnx-doctor-') as temporary:
                profile_bindings(Path(temporary)/'generated', DEFAULT_PROFILE)
            check('core target profile', (root/'source/core/main.cpp').is_file(), 'Core builds require the full source checkout')
        except (OSError, ValueError, ImportError) as error:
            check('core target profile', False, str(error))
    return dict(passed=sum(c['passed'] for c in checks), failed=sum(not c['passed'] for c in checks), checks=checks,
                scope='SDK files and compiler availability; does not test a Switch or installed mods')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--devkitpro', type=Path); p.add_argument('--no-toolchain', action='store_true')
    p.add_argument('--core', action='store_true', help='Also check full-checkout core tools and its audited profile')
    p.add_argument('--json', action='store_true'); a = p.parse_args()
    report = diagnose(Path(__file__).resolve().parents[1], a.devkitpro, not a.no_toolchain, a.core)
    if a.json: print(json.dumps(report, indent=2))
    else:
        for row in report['checks']: print(('OK  ' if row['passed'] else 'FAIL')+' '+row['name']+': '+row['detail'])
    raise SystemExit(1 if report['failed'] else 0)

if __name__ == '__main__':
    main()
