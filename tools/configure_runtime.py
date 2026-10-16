"""Generate an optional runtime.ini for a user or a ported NRO mod's test setup."""
import argparse
from pathlib import Path
from runtime_limits import render

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--resident-limit', type=int)
    p.add_argument('--script-slots', type=int)
    p.add_argument('--plugin-memory-mib', type=int)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--force', action='store_true', help='Replace an existing output config')
    a = p.parse_args()
    settings = {key: getattr(a, key) for key in ('resident_limit', 'script_slots', 'plugin_memory_mib')
                if getattr(a, key) is not None}
    try:
        text = render(settings)
        with a.output.open('w' if a.force else 'x', encoding='utf-8', newline='\n') as stream:
            stream.write(text)
    except (OSError, ValueError) as error:
        p.exit(1, str(error)+'\n')
    print('Created '+str(a.output)+'. Copy to SD:/switch/gtav-scripthook/runtime.ini, then restart GTA.')

if __name__ == '__main__':
    main()
