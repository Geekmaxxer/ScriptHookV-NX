"""Compile and structurally check all standalone NRO tutorials in a fresh directory."""
import argparse
import contextlib
import hashlib
import json
from pathlib import Path
import subprocess
from build_plugin import build
from inspect_mod import inspect

def check(root, output, devkit=None):
    if output.exists(): raise FileExistsError('Choose a fresh output directory')
    output.mkdir(parents=True)
    rows = []
    examples = [('scriptmain', root/'examples/scriptmain_plugin.cpp'), ('public-sample', root/'examples/public_plugin.cpp')]
    examples += [(p.parent.name, p) for p in sorted((root/'examples/tutorials').glob('*/main.cpp'))]
    for name, source in examples:
        with (output/(name+'.log')).open('w', encoding='utf-8') as log, contextlib.redirect_stdout(log):
            build([source], output/name, devkit)
        proof = inspect(output/name/'plugin.nro')
        rows.append(dict(name=name, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), **proof))
        print(name+': built and structurally checked')
    report = dict(passed=len(rows), examples=rows, switch_runtime_verified=False)
    (output/'example-checks.json').write_text(json.dumps(report, indent=2)+'\n')
    return report

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--output', required=True, type=Path)
    p.add_argument('--devkitpro', type=Path); a = p.parse_args()
    try: check(Path(__file__).resolve().parents[1], a.output.resolve(), a.devkitpro)
    except (OSError, ValueError, subprocess.CalledProcessError) as error: p.exit(1, str(error)+'\n')
