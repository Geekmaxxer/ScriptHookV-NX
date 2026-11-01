"""Create an original small RGBA test PNG; never overwrite an existing asset."""
import argparse,struct,zlib
from pathlib import Path

def create(path):
    size=64
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=bytearray()
    for y in range(size):
        rows.append(0)
        for x in range(size):
            colour=((255,0,0),(0,255,0),(0,0,255),(255,255,255))[(y//32)*2+x//32]
            rows.extend((*colour,255 if x%32<16 else 128))
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',size,size,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('xb') as file:file.write(png)
    return path


def create_catalog(directory):
    """Generate the 80 small RGBA tiles used by the growing-cache probe."""
    directory.mkdir(parents=True, exist_ok=False)
    size = 16

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

    files = []
    for index in range(80):
        rows = bytearray()
        for y in range(size):
            rows.append(0)
            for x in range(size):
                rows.extend(((index // 16 * 51) % 256, index * 7 % 256,
                             255 if (x // 4 + y // 4) % 2 else index * 3 % 256,
                             255 if x < 8 else 128))
        data = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>2I5B', size, size, 8, 6, 0, 0, 0)) +
                chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))
        path = directory / f'tile{index:02}.png'
        with path.open('xb') as output:
            output.write(data)
        files.append(path)
    return files

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--catalogue',action='store_true',help='Create a new directory of 80 texture-catalog tiles')
    a=p.parse_args()
    if a.catalogue:
        print('Created',len(create_catalog(a.output.resolve())),'PNG tiles in',a.output.resolve())
    else:
        print(create(a.output.resolve()))
