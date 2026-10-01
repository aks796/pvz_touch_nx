#!/usr/bin/env python3
"""PopCap's Xbox 360 pak (after the XCompress layer: lzx.c seg): the PC
directory, then each file's data preceded by a little-endian u16 N and N
padding bytes (data aligned for the console: textures to 4 KB)."""
import struct, sys, os

def read_pak360(path):
    d = open(path, 'rb').read()
    assert d[:4] == bytes([0xC0, 0x4A, 0xC0, 0xBA])
    p = 8
    ents = []
    while True:
        fl = d[p]; p += 1
        if fl & 0x80:
            break
        n = d[p]; p += 1
        name = d[p:p + n].decode('latin1'); p += n
        size, ft = struct.unpack_from('<IQ', d, p); p += 12
        ents.append((name.replace('\\', '/'), size))
    out = []
    for name, size in ents:
        pad = struct.unpack_from('<H', d, p)[0]
        p += 2 + pad
        out.append((name, d[p:p + size]))
        p += size
    assert p == len(d), (p, len(d))
    return out

if __name__ == '__main__':
    files = read_pak360(sys.argv[1])
    for name, data in files:
        dst = os.path.join(sys.argv[2], name)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, 'wb').write(data)
    print(len(files), 'files')
