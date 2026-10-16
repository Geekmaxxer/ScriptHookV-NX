"""Standalone NSO reader: segment hashes, dynamic symbols and ARM64 PLT metadata.

Adapted from this workspace's NSO-UserMusic analysis utilities; no game data is
embedded. Supports bounded LZ4 blocks without an external hactool dependency.
"""
import hashlib
from pathlib import Path
import struct


def segments(data):
    if len(data) < 0x100 or data[:4] != b"NSO0":
        raise ValueError("Not an NSO0")
    flags = struct.unpack_from('<I', data, 0xc)[0]
    result = []
    for index, (name, header) in enumerate((("text", 0x10), ("rodata", 0x20), ("data", 0x30))):
        file_offset, memory_offset, size = struct.unpack_from('<III', data, header)
        compressed = bool(flags & (1 << index))
        stored_size = struct.unpack_from('<I', data, 0x60 + index * 4)[0] if compressed else size
        if size > 256*1024*1024 or file_offset < 0x100 or file_offset + stored_size > len(data):
            raise ValueError('Invalid '+name+' file extent/size')
        if memory_offset + size > 0x100000000:
            raise ValueError('Invalid '+name+' memory extent')
        result.append(dict(name=name, file_offset=file_offset, memory_offset=memory_offset,
                           size=size, stored_size=stored_size, compressed=compressed,
                           hash_required=bool(flags & (1 << (index+3))),
                           header_sha256=data[0xa0+index*32:0xc0+index*32].hex()))
    return result


def lz4_block(data, expected):
    if not 0 <= expected <= 256*1024*1024:
        raise ValueError('Invalid LZ4 output bound')
    out = bytearray(); at = 0
    def length(value):
        nonlocal at
        if value == 15:
            while True:
                if at >= len(data): raise ValueError('Truncated LZ4 length')
                byte = data[at]; at += 1; value += byte
                if value > expected: raise ValueError('LZ4 length exceeds output bound')
                if byte != 255: break
        return value
    while at < len(data):
        token = data[at]; at += 1
        literals = length(token >> 4)
        if at+literals > len(data) or len(out)+literals > expected:
            raise ValueError('Invalid LZ4 literal extent')
        out.extend(data[at:at+literals]); at += literals
        if at == len(data): break
        if at+2 > len(data): raise ValueError('Truncated LZ4 offset')
        offset = int.from_bytes(data[at:at+2], 'little'); at += 2
        if not offset or offset > len(out): raise ValueError('Invalid LZ4 back-reference')
        count = length(token & 15) + 4
        if len(out)+count > expected: raise ValueError('LZ4 match exceeds output bound')
        # Copy in bounded slices, preserving overlapping back-reference semantics.
        while count:
            take = min(count, offset); out.extend(out[-offset:][:take]); count -= take
    if len(out) != expected: raise ValueError('LZ4 output size mismatch')
    return bytes(out)


def decode_nso(data):
    layout = segments(data)
    blocks = []
    for segment in layout:
        block = data[segment['file_offset']:segment['file_offset']+segment['stored_size']]
        if segment['compressed']: block = lz4_block(block, segment['size'])
        if segment['hash_required'] and hashlib.sha256(block).hexdigest() != segment['header_sha256']:
            raise ValueError('Segment hash mismatch: '+segment['name'])
        blocks.append(block)
    if not any(s['compressed'] for s in layout): return data
    result = bytearray(data[:0x100])
    struct.pack_into('<I', result, 0xc, struct.unpack_from('<I', data, 0xc)[0] & ~7)
    for index, block in enumerate(blocks):
        result.extend(bytes((-len(result)) % 16))
        struct.pack_into('<I', result, 0x10+index*16, len(result))
        struct.pack_into('<I', result, 0x60+index*4, len(block))
        result.extend(block)
    return bytes(result)


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


class Image:
    def __init__(self, path):
        self.path = Path(path)
        self.data = decode_nso(self.path.read_bytes())
        self.layout = segments(self.data)
        if any(s['compressed'] for s in self.layout):
            raise ValueError('Use the verified uncompressed research NSO')
        for s in self.layout:
            block = self.read(s['memory_offset'], s['size'])
            if s['hash_required'] and hashlib.sha256(block).hexdigest() != s['header_sha256']:
                raise ValueError('Segment hash mismatch: ' + s['name'])
        self.mod = struct.unpack('<I', self.read(4, 4))[0]
        if self.read(self.mod, 4) != b'MOD0':
            raise ValueError('MOD0 not found')
        fields = struct.unpack('<6i', self.read(self.mod + 4, 24))
        self.mod_fields = [self.mod + n for n in fields]
        self.dynamic = []
        for address in range(self.mod_fields[0], self.mod_fields[0] + 0x10000, 16):
            tag, value = struct.unpack('<qQ', self.read(address, 16))
            self.dynamic.append((tag, value))
            if tag == 0:
                break
        else:
            raise ValueError('Unterminated dynamic table')
        self.tags = dict(self.dynamic)
        self.symbols = self._symbols()
        self.relocations = self._relocations()

    def read(self, address, size):
        for s in self.layout:
            start = s['memory_offset']
            if start <= address and address + size <= start + s['size']:
                offset = s['file_offset'] + address - start
                return self.data[offset:offset + size]
        raise ValueError(f'Unmapped initialized range: {address:#x}+{size:#x}')

    def _symbols(self):
        ro = self.layout[1]['memory_offset']
        sym_offset, sym_size = struct.unpack_from('<II', self.data, 0x98)
        # devkitPro elf2nso leaves optional dynsym extents zero. The loader uses
        # DT_SYMTAB/DT_HASH; derive only when BOTH optional fields are absent.
        if sym_offset == 0 and sym_size == 0:
            if 4 not in self.tags:
                raise ValueError('No dynamic symbol extent or SysV hash table')
            buckets, count = struct.unpack('<II', self.read(self.tags[4], 8))
            if not buckets or not count or count > 1000000:
                raise ValueError('Invalid SysV dynamic symbol count')
            self.read(self.tags[4], 8 + (buckets + count) * 4)
            sym_offset, sym_size = self.tags[6] - ro, count * 24
        names = self.read(self.tags[5], self.tags[10])
        if ro + sym_offset != self.tags[6] or sym_size % 24:
            raise ValueError('NSO/ELF dynamic symbol layout disagrees')
        result = []
        for address in range(self.tags[6], self.tags[6] + sym_size, 24):
            name, info, other, section, value, size = struct.unpack('<IBBHQQ', self.read(address, 24))
            if name >= len(names) or names.find(b'\0', name) < 0:
                raise ValueError('Invalid symbol name')
            text = names[name:names.index(0, name)].decode('ascii', errors='replace')
            result.append(dict(name=text, value=value, size=size, section=section,
                               type=info & 15, binding=info >> 4))
        return result

    def _relocations(self):
        result = []
        for label, ptr_tag, size_tag in [('rela', 7, 8), ('plt', 23, 2)]:
            ptr, size = self.tags.get(ptr_tag, 0), self.tags.get(size_tag, 0)
            if size % 24:
                raise ValueError('Invalid RELA size')
            for address in range(ptr, ptr + size, 24):
                target, info, addend = struct.unpack('<QQq', self.read(address, 24))
                symbol, kind = info >> 32, info & 0xffffffff
                if symbol >= len(self.symbols):
                    raise ValueError('Invalid relocation symbol')
                result.append(dict(table=label, target=target, type=kind,
                                   symbol=symbol, addend=addend))
        # ELF RELR: direct entries establish the next base; bitmap bit 1 maps
        # base+0, bit 2 maps base+8, and so on. Values are in-place addends.
        ptr, size = self.tags.get(36, 0), self.tags.get(35, 0)
        if size % 8 or (size and self.tags.get(37) != 8):
            raise ValueError('Invalid RELR format')
        base = None
        for address in range(ptr, ptr + size, 8):
            entry = struct.unpack('<Q', self.read(address, 8))[0]
            if entry & 1:
                if base is None:
                    raise ValueError('RELR bitmap without base')
                targets = [base + bit * 8 for bit in range(63) if entry & (1 << (bit + 1))]
                base += 63 * 8
            else:
                if entry % 8:
                    raise ValueError('Unaligned RELR target')
                targets = [entry]
                base = entry + 8
            for target in targets:
                result.append(dict(table='relr', target=target, type=1027, symbol=0,
                                   addend=struct.unpack('<Q', self.read(target, 8))[0]))
        return result


def import_thunks(image):
    """Match complete AArch64 PLT sequences to verified JUMP_SLOT imports."""
    slots = {r['target']: image.symbols[r['symbol']]['name']
             for r in image.relocations if r['type'] == 1026}
    text = image.layout[0]
    block = image.read(text['memory_offset'], text['size'])
    result, pos = [], 0
    while (pos := block.find(b'\x20\x02\x1f\xd6', pos)) >= 0:
        if pos >= 12 and pos % 4 == 0:
            adrp, ldr, add, branch = struct.unpack_from('<4I', block, pos - 12)
            if (adrp & 0x9f00001f == 0x90000010 and
                    ldr & 0xffc003ff == 0xf9400211 and
                    add & 0xffc003ff == 0x91000210):
                address = text['memory_offset'] + pos - 12
                imm = signed(((adrp >> 5) & 0x7ffff) << 2 | ((adrp >> 29) & 3), 21)
                page = (address & ~0xfff) + imm * 4096
                slot = page + ((ldr >> 10) & 0xfff) * 8
                if slot == page + ((add >> 10) & 0xfff) and slot in slots:
                    result.append(dict(start=address, end=address + 16, slot=slot, name=slots[slot]))
        pos += 4
    return result


if __name__ == '__main__':
    import argparse
    import json
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', type=Path)
    args = parser.parse_args()
    try:
        image = Image(args.module)
    except (OSError, ValueError) as error:
        parser.exit(1, str(error)+'\n')
    print(json.dumps(dict(segments=image.layout, symbols=len(image.symbols),
                         relocations=len(image.relocations), hashes_verified=True), indent=2))

