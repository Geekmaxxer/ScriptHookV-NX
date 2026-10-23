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

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    print(create(a.output.resolve()))
