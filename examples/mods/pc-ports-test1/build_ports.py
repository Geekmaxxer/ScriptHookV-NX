"""Build all ten native gameplay adaptations using the matching ScriptHookV-NX SDK."""
import argparse
import contextlib
import json
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--devkitpro', type=Path)
    parser.add_argument('--sdk-root', type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parent
    sdk = (args.sdk_root or source.parents[1]).resolve()
    sys.path.insert(0, str(sdk / 'tools'))
    from build_plugin import build
    from inspect_mod import inspect
    from sd_permissions import stage as stage_sd_access
    output = args.output.resolve()
    if output.exists():
        raise FileExistsError('Choose a new output folder.')
    output.mkdir(parents=True)
    stage_sd_access(output, sdk)
    records = []
    for unit in sorted(source.glob('*/main.cpp')):
        name = unit.parent.name
        with (output / (name + '.log')).open('w', encoding='utf-8') as log:
            with contextlib.redirect_stdout(log):
                build([unit], output / name, args.devkitpro, [source / 'common'])
        records.append(dict(id=name, **inspect(output / name / 'plugin.nro')))
        print('Built ' + name, flush=True)
    (output / 'build-results.json').write_text(json.dumps(records, indent=2) + '\n', encoding='utf-8')
    print('Build/file checks only. New mods require Switch gameplay testing.')


if __name__ == '__main__':
    main()
