"""Create a small, buildable NRO or NSC project without overwriting files."""
import argparse
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
TEMPLATES = {
    'hello': ('nro', 'tutorial-hello', 'One-shot namespaced file; finishes normally'),
    'overlay': ('nro', 'tutorial-overlay', 'Frame drawing, pause guard and long UTF-8 HUD text'),
    'saved-counter': ('nro', 'tutorial-counter', 'Versioned alternating settings and readback'),
    'vehicle-logger': ('nro', 'tutorial-vehicle', 'Read-only vehicle detection, timed sampling and change-only I/O'),
    'native-provider': ('nro', 'tutorial-provider', 'Public custom native usable by C++ and NSC clients'),
    'nsc-settings': ('nsc', None, 'Generic NSC persistence with schema validation'),
    'nsc-provider-client': ('nsc', None, 'NSC client for the native-provider tutorial'),
}

def create(template, identity, output):
    if template not in TEMPLATES:
        raise ValueError('Unknown template: ' + template)
    if not re.fullmatch(r'[A-Za-z0-9_-]{1,31}', identity):
        raise ValueError('ID requires 1-31 ASCII letters, digits, underscores or hyphens')
    if output.exists():
        raise FileExistsError('Choose a new output directory; existing projects are never overwritten')
    kind, old_id, description = TEMPLATES[template]
    folder = ROOT/'examples/tutorials'/template
    source = next(folder.glob('*.cpp' if kind == 'nro' else '*.sc'))
    text = source.read_text(encoding='utf-8')
    if old_id:
        text = text.replace('"'+old_id+'"', '"'+identity+'"')
    name = 'main.cpp' if kind == 'nro' else identity.replace('-', '_')+'.sc'
    output.mkdir(parents=True)
    (output/name).write_text(text, encoding='utf-8')
    if kind == 'nsc':
        shutil.copy2(ROOT/'sdk/commands_scripthook_nx.sch', output/'commands_scripthook_nx.sch')
    config = dict(format=1, kind=kind, id=identity, sources=[name], includes=[],
                  custom_hashes=['0x5348564e58414431'] if template == 'nsc-provider-client' else [])
    (output/'project.json').write_text(json.dumps(config, indent=2)+'\n', encoding='utf-8')
    (output/'.gitignore').write_text('/out/\n__pycache__/\n*.pyc\n', encoding='utf-8')
    (output/'README.md').write_text('# '+identity+'\n\n'+description+'.\n\n'
        'Build with `python <SDK>/tools/build_mod.py --project project.json --output out/test1`.\n'
        + ('NSC builds also require `--menu-sdk`, `--dev-ng`, and `--header-reference`; see the SDK NSC guide.\n' if kind == 'nsc' else '')
        + 'Use a new build output directory each time. See the SDK tutorial guide for installation and expected results.\n', encoding='utf-8')
    return config

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--list', action='store_true')
    p.add_argument('--template', choices=TEMPLATES, default='hello')
    p.add_argument('--id'); p.add_argument('--output', type=Path)
    a = p.parse_args()
    if a.list:
        for name, (kind, _, help_text) in TEMPLATES.items():
            print(f'{name:21} {kind:3}  {help_text}')
        return
    if not a.id or a.output is None:
        p.error('--id and --output are required unless using --list')
    try:
        result = create(a.template, a.id, a.output.resolve())
    except (OSError, ValueError) as error:
        p.exit(1, str(error)+'\n')
    print(f'Created {result["kind"]} project: {a.output.resolve()}')

if __name__ == '__main__':
    main()
