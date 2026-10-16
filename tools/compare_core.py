"""Compare initialized runtime segments, allowing only the structured GNU build ID."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from nso import Image

def summarize(module):
    image = Image(module)
    result = dict(format=1, module_sha256=hashlib.sha256(Path(module).read_bytes()).hexdigest(),
                  bss_bytes=struct.unpack_from('<I', image.data, 0x3c)[0], segments={})
    for segment in image.layout:
        data = image.read(segment['memory_offset'], segment['size'])
        ignored = 0
        if segment['name'] == 'rodata':
            note = struct.pack('<III', 4, 20, 3)+b'GNU\0'
            if data[-36:-20] == note and data[-20:] == image.data[0x40:0x54]:
                data = data[:-20]+bytes(20); ignored = 20
        result['segments'][segment['name']] = dict(memory_offset=segment['memory_offset'], bytes=segment['size'],
            normalized_sha256=hashlib.sha256(data).hexdigest(), ignored_gnu_build_id_bytes=ignored)
    return result

def compare(reference, module):
    actual = summarize(module)
    if reference.get('format') != 1:
        raise ValueError('Unsupported reference format')
    if actual['segments'] != reference['segments'] or actual['bss_bytes'] != reference['bss_bytes']:
        raise ValueError('Runtime code/data/layout differs from the reference beyond its GNU build ID')
    return dict(equivalent_initialized_runtime=True, expected_module_sha256=reference['module_sha256'],
                actual_module_sha256=actual['module_sha256'], exact_file_match=actual['module_sha256']==reference['module_sha256'],
                ignored='Only a valid 20-byte GNU build-ID note matching the NSO header', switch_runtime_verified=False)

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference', required=True, type=Path); p.add_argument('--module', required=True, type=Path)
    a = p.parse_args()
    try: print(json.dumps(compare(json.loads(a.reference.read_text()), a.module), indent=2))
    except (OSError, ValueError) as error: p.exit(1, str(error)+'\n')
