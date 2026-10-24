"""Add explicit SD access to both NPDM FS masks, preserving every other byte.

Uses the actual META/ACI0/ACID offsets, packed FAH/FAC layout, and SdCard bit 21.
References and source hashes are recorded in the SD-access package manifest.
Never modifies its input or an existing output; reversal requires exact hashes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import tempfile
import shutil

TITLE_ID = 0x0100B00B51230000
SD_CARD = 1 << 21
ROOT = Path(__file__).resolve().parents[1]
NPDM_PATH = Path('atmosphere/contents/0100b00b51230000/exefs/main.npdm')
BUNDLED_SHA256 = 'db97307619f2dd536f06c38703721e6164470742a81522edfb273778937e650f'


def bundle(root=ROOT):
    """Fail before packaging if the audited shared metadata is missing/changed."""
    folder = Path(root)/'distribution'
    data = (folder/NPDM_PATH).read_bytes()
    info = inspect(data)
    if sha(data) != BUNDLED_SHA256 or not all(m['flags'] & SD_CARD for m in info['masks']):
        raise ValueError('The audited SD-access main.npdm is missing or changed')
    for name in ('Install-SD-Access.ps1', 'Install-SD-Access.cmd'):
        if not (folder/name).is_file(): raise FileNotFoundError('Missing SD installer: '+name)
    return folder


def copy_distribution(destination, root=ROOT):
    """Include metadata and preservation tooling in source/SDK snapshots."""
    source = bundle(root)
    shutil.copytree(source, Path(destination)/'distribution', dirs_exist_ok=True)


def stage(destination, root=ROOT):
    """Every install/test package carries the common process permission file."""
    source = bundle(root); destination = Path(destination)
    for path in source.rglob('*'):
        if not path.is_file(): continue
        target = destination/path.relative_to(source)
        if target.exists() and target.read_bytes() != path.read_bytes():
            raise ValueError('Refusing to overwrite different SD metadata/install tooling: '+str(target))
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    return dict(npdm_path=NPDM_PATH.as_posix(), npdm_sha256=BUNDLED_SHA256,
                sd_access_included=True, custom_npdm_installer_included=True)


def prepare_package(destination, root=ROOT):
    """Common gate for source SDKs and runtime/plugin install archives."""
    destination = Path(destination)
    if (destination/'include').is_dir() and (destination/'sdk').is_dir():
        copy_distribution(destination, root)
        tools = destination/'tools'; tools.mkdir(exist_ok=True)
        shutil.copy2(Path(root)/'tools/sd_permissions.py', tools/'sd_permissions.py')
    else:
        stage(destination, root)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def inspect(data):
    if len(data) < 0x80 or len(data) > 0x100000 or data[:4] != b'META':
        raise ValueError('Not a bounded META NPDM')
    aci, aci_size, acid, acid_size = struct.unpack_from('<IIII', data, 0x70)
    parents = [('ACI0', aci, aci_size, 0x40), ('ACID', acid, acid_size, 0x240)]
    for name, base, size, header in parents:
        if base < 0x80 or size < header or base + size > len(data):
            raise ValueError(name + ' extent is invalid')
    if max(aci, acid) < min(aci + aci_size, acid + acid_size):
        raise ValueError('ACI0/ACID overlap')
    if data[aci:aci+4] != b'ACI0' or data[acid+0x200:acid+0x204] != b'ACID':
        raise ValueError('ACI0/ACID magic mismatch')
    title = struct.unpack_from('<Q', data, aci + 0x10)[0]
    first, last = struct.unpack_from('<QQ', data, acid + 0x210)
    if title != TITLE_ID or not first <= title <= last:
        raise ValueError('NPDM does not describe the target title ID')
    masks = []
    for name, base, size, header, table, minimum in [
        ('FAH', aci, aci_size, 0x40, aci+0x20, 0x1c),
        ('FAC', acid, acid_size, 0x240, acid+0x220, 0x2c),
    ]:
        sections = []
        for i in range(3):
            offset, length = struct.unpack_from('<II', data, table + i*8)
            if i == 0 and length < minimum:
                raise ValueError(name + ' is too short')
            if not length:
                continue
            if offset < header or offset + length > size:
                raise ValueError(name + ' child section out of range')
            sections.append((offset, offset + length))
        for i, (begin, end) in enumerate(sections):
            if any(max(begin, b) < min(end, e) for b, e in sections[i+1:]):
                raise ValueError(name + ' overlaps another child section')
        offset = base + struct.unpack_from('<I', data, table)[0]
        if struct.unpack_from('<I', data, offset)[0] != 1:
            raise ValueError('Only verified FS access-control version 1 is supported')
        masks.append(dict(name=name, offset=offset+4,
                          flags=struct.unpack_from('<Q', data, offset+4)[0]))
    return dict(bytes=len(data), title_id=f'{title:016x}', masks=masks)

def patch(data):
    before = inspect(data)
    candidate = bytearray(data)
    fields = []
    for mask in before['masks']:
        old = mask['flags']; new = old | SD_CARD
        struct.pack_into('<Q', candidate, mask['offset'], new)
        fields.append(dict(name=mask['name'], offset=mask['offset'],
                           before=hex(old), after=hex(new)))
    candidate = bytes(candidate)
    after = inspect(candidate)
    allowed = {i for f in fields for i in range(f['offset'], f['offset']+8)}
    changed = [i for i, (a, b) in enumerate(zip(data, candidate)) if a != b]
    if len(candidate) != len(data) or not set(changed) <= allowed:
        raise ValueError('A non-permission byte changed')
    if any(m['flags'] != (old['flags'] | SD_CARD)
           for m, old in zip(after['masks'], before['masks'])):
        raise ValueError('Permission verification failed')
    manifest = dict(title_id=before['title_id'], bytes=len(data), added_permission='SdCard',
                    added_mask=hex(SD_CARD), source_sha256=sha(data), output_sha256=sha(candidate),
                    fields=fields, changed_offsets=[hex(i) for i in changed],
                    runtime_verified=False)
    if reverse(candidate, manifest) != data:
        raise ValueError('Exact reversal failed')
    return candidate, manifest

def reverse(data, manifest):
    current = inspect(data)
    if manifest['title_id'] != current['title_id'] or sha(data) != manifest['output_sha256']:
        raise ValueError('Reversal input hash/title mismatch')
    fields = manifest['fields']
    if len(fields) != 2:
        raise ValueError('Unexpected reversal field count')
    result = bytearray(data)
    for field, mask in zip(fields, current['masks']):
        before = int(field['before'], 16); after = int(field['after'], 16)
        if field['name'] != mask['name'] or field['offset'] != mask['offset'] or \
           after != mask['flags'] or after != (before | SD_CARD):
            raise ValueError('Reversal is not a matching SD-only patch')
        struct.pack_into('<Q', result, field['offset'], before)
    result = bytes(result)
    if sha(result) != manifest['source_sha256']:
        raise ValueError('Restored source hash mismatch')
    inspect(result)
    return result

def publish(path, data):
    path = path.resolve()
    if path.exists():
        raise FileExistsError('Use a new output file')
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name+'.', suffix='.partial', delete=False) as f:
            temporary = Path(f.name); f.write(data); f.flush(); os.fsync(f.fileno())
        if sha(temporary.read_bytes()) != sha(data):
            raise ValueError('Temporary output verification failed')
        if path.exists():
            raise FileExistsError('Output appeared while preparing the patch')
        os.replace(temporary, path)
    finally:
        if temporary is not None and temporary.exists():
            temporary.unlink()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    commands = p.add_subparsers(dest='command', required=True)
    for name in ['apply', 'reverse']:
        sub = commands.add_parser(name)
        sub.add_argument('--input', required=True, type=Path)
        sub.add_argument('--output', required=True, type=Path)
        sub.add_argument('--manifest', required=True, type=Path)
    check = commands.add_parser('check')
    check.add_argument('--input', required=True, type=Path)
    pack = commands.add_parser('stage')
    pack.add_argument('--output', required=True, type=Path)
    a = p.parse_args()
    if a.command == 'check':
        info = inspect(a.input.read_bytes())
        info['sd_access'] = all(m['flags'] & SD_CARD for m in info['masks'])
        print(json.dumps(info, indent=2))
        if not info['sd_access']: raise SystemExit('Both FS masks require SdCard permission')
        return
    if a.command == 'stage':
        print(json.dumps(stage(a.output), indent=2)); return
    if a.input.resolve() == a.output.resolve():
        raise ValueError('Input and output must be separate files')
    if a.command == 'apply' and a.manifest.exists():
        raise FileExistsError('Use a new manifest path')
    data = a.input.read_bytes()
    if a.command == 'apply':
        result, manifest = patch(data)
        publish(a.output, result)
        publish(a.manifest, (json.dumps(manifest, indent=2)+'\n').encode('utf-8'))
        print(json.dumps(manifest, indent=2))
    else:
        result = reverse(data, json.loads(a.manifest.read_text(encoding='utf-8')))
        publish(a.output, result)
        print(json.dumps(dict(restored_sha256=sha(result), output=str(a.output))))

if __name__ == '__main__':
    main()
