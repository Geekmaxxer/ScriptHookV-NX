"""Read-only structural preflight for a ScriptHookV-NX NRO or .snb binding file."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

def require(ok, message):
    if not ok: raise ValueError(message)

def inspect_nro(data):
    require(0x1000 <= len(data) <= 2*1024*1024 and len(data) % 4096 == 0, 'NRO image must be page aligned and at most 2 MiB')
    require(data[0x10:0x14] == b'NRO0', 'Missing NRO0 header')
    version, size, flags = struct.unpack_from('<III', data, 0x14)
    text, ts, ro, rs, rw, ds, bss = struct.unpack_from('<7I', data, 0x20)
    require(version == flags == text == 0 and size == len(data), 'Unsupported NRO header/version/size')
    require(ts >= 4096 and all(n % 4096 == 0 for n in (ts, rs, ds, bss)) and
            ro == ts and rw == ro+rs and rw+ds == size and bss <= 2*1024*1024, 'Invalid NRO segment layout')
    mod = struct.unpack_from('<I', data, 4)[0]
    require(0x80 <= mod <= size-28 and data[mod:mod+4] == b'MOD0', 'Missing MOD0')
    dynamic = mod + struct.unpack_from('<i', data, mod+4)[0]
    require(0 <= dynamic <= size-16, 'Dynamic table out of range')
    tags = {}; ended = False
    for at in range(dynamic, size-15, 16):
        tag, value = struct.unpack_from('<QQ', data, at)
        if not tag: ended = True; break
        require(tag not in (12, 13, 25, 26, 32) or not value, 'Global constructors/destructors/preinit are unsupported')
        require(tag != 1, 'External shared-library dependencies are unsupported')
        tags[tag] = value
    require(ended, 'Unterminated dynamic table')
    require(all(k in tags for k in (4, 5, 6, 10, 11)) and tags[11] == 24, 'Missing supported symbol/string tables')
    require(tags[4]+8 <= size, 'Symbol hash table out of range')
    count = struct.unpack_from('<I', data, tags[4]+4)[0]
    require(count <= 65536 and tags[6]+count*24 <= size and tags[5]+tags[10] <= size, 'Symbol/string table out of range')
    exported = False
    for i in range(count):
        name, info, visibility, section, value, _ = struct.unpack_from('<IBBHQQ', data, tags[6]+i*24)
        require(name < tags[10], 'Symbol name outside string table')
        end = data.find(b'\0', tags[5]+name, tags[5]+tags[10])
        require(end != -1, 'Unterminated symbol name')
        text_name = data[tags[5]+name:end]
        require(section != 0 or not name, 'Unresolved import: '+text_name.decode('ascii', errors='replace'))
        if text_name == b'shvPluginQuery' and section and info >> 4 in (1, 2) and info & 15 == 2 and visibility & 3 == 0 and value < ts:
            exported = True
    require(exported, 'Missing exported shvPluginQuery function')
    return dict(kind='nro', image_bytes=size, bss_bytes=bss,
                accounted_bytes=2*(size+max(bss, 4096))+4096, query_export_present=True,
                note='Descriptor ABI, IDs, initialization, native behavior and console RO acceptance require loading/tests')

def inspect_bindings(data):
    require(16 <= len(data) <= 65536, 'Binding file must be 16 bytes to 64 KiB')
    magic, version, count, crc = struct.unpack_from('<4I', data)
    require(magic == 0x424e4853 and version == 1 and count <= 64, 'Invalid binding header')
    require(zlib.crc32(data[16:]) == crc, 'Binding CRC mismatch')
    records = []; at = 16; names = set()
    for _ in range(count):
        require(at+48 <= len(data), 'Truncated binding identity')
        name, code, natives, entries = struct.unpack_from('<4I', data, at)
        fingerprint = data[at+16:at+48].hex(); at += 48
        require(name and name not in names and code >= 32 and natives <= 65535 and 0 < entries <= 256,
                'Invalid/duplicate binding identity')
        names.add(name); indices = set(); hashes = []
        for _ in range(entries):
            require(at+16 <= len(data), 'Truncated native binding')
            index, reserved, native = struct.unpack_from('<IIQ', data, at); at += 16
            require(index < natives and index not in indices and reserved == 0 and native, 'Invalid native binding')
            indices.add(index); hashes.append(f'0x{native:016x}')
        records.append(dict(name_hash=f'0x{name:08x}', code_bytes=code, native_count=natives,
                            entry_fingerprint=fingerprint, custom_hashes=hashes))
    require(at == len(data), 'Trailing binding bytes')
    return dict(kind='snb', identities=records,
                note='Regenerate from the exact compiled NSC after every build; structural validity does not prove that match')

def inspect(path):
    require(path.stat().st_size <= 2*1024*1024, 'File exceeds the supported 2 MiB preflight limit')
    data = path.read_bytes()
    result = inspect_bindings(data) if data[:4] == b'SHNB' else inspect_nro(data)
    return dict(path=str(path), sha256=hashlib.sha256(data).hexdigest(), structural_checks_passed=True,
                switch_runtime_verified=False, **result)

def main():
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('file', type=Path)
    a = p.parse_args()
    try: result = inspect(a.file)
    except (OSError, ValueError, struct.error) as error: p.exit(1, 'Preflight failed: '+str(error)+'\n')
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
