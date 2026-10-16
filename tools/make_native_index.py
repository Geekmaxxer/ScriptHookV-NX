"""Maintainer: reduce wrapper coverage to a portable signature/availability index."""
import argparse
import hashlib
import json
from pathlib import Path

def generate(coverage, header, output):
    source = json.loads(coverage.read_text())
    digest = hashlib.sha256(header.read_bytes()).hexdigest()
    if source['header_sha256'] != digest:
        raise ValueError('Coverage report and public native header differ')
    natives = []
    for row in source['signatures']:
        name = row['module']+'::'+row['name']
        parameters = ', '.join(arg['type']+' '+arg['name'] for arg in row['arguments'])
        natives.append(dict(name=name, hash=f'0x{int(row["hash"], 16):016x}',
                            signature=row['return_type']+' '+name+'('+parameters+')',
                            registered=row['registered_in_target'] is True))
    result = dict(format=1, target_build='6e3e9b3af5746e74b615bd981080c9271bc226620',
                  header_sha256=digest, runtime_verified=False, natives=natives)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, separators=(',', ':'))+'\n', encoding='utf-8')
    return result

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--coverage', type=Path, required=True); p.add_argument('--header', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True); a = p.parse_args()
    r = generate(a.coverage, a.header, a.output)
    print(f'{len(r["natives"])} indexed wrappers; {sum(n["registered"] for n in r["natives"])} static registrations')
