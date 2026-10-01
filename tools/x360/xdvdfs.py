#!/usr/bin/env python3
"""Read-only XDVDFS (Xbox 360 game partition) lister and extractor.
  xdvdfs.py                 list
  xdvdfs.py x <path> <dir>  extract one file
  xdvdfs.py xall <dir> [prefix]  extract everything (under prefix)"""
import os, struct, sys

ISO = os.environ.get('X360_ISO', 'pvz360.iso')  # the disc image (Redump), X360_ISO=...
MAGIC = b'MICROSOFT*XBOX*MEDIA'
SECTOR = 2048

def find_base(f):
    for base in (0, 0xFD90000, 0x2080000, 0x18300000):
        f.seek(base + 32 * SECTOR)
        if f.read(20) == MAGIC:
            return base
    raise SystemExit('no XDVDFS volume')

def walk(f, base, sector, size, prefix, out):
    f.seek(base + sector * SECTOR)
    data = f.read(size)
    seen = set()
    stack = [0]
    while stack:
        off = stack.pop()
        if off in seen or off + 14 > len(data):
            continue
        seen.add(off)
        l, r, start, fsize, attr, nlen = struct.unpack_from('<HHIIBB', data, off)
        if l == 0xFFFF and r == 0xFFFF:
            continue
        name = data[off + 14: off + 14 + nlen].decode('latin1')
        path = prefix + name
        if attr & 0x10:
            out.append((path + '/', start, fsize, True))
            if fsize:
                walk(f, base, start, fsize, path + '/', out)
        else:
            out.append((path, start, fsize, False))
        if l:
            stack.append(l * 4)
        if r:
            stack.append(r * 4)

def listing():
    f = open(ISO, 'rb')
    base = find_base(f)
    f.seek(base + 32 * SECTOR + 20)
    root, rsize = struct.unpack('<II', f.read(8))
    out = []
    walk(f, base, root, rsize, '', out)
    return f, base, out

def extract(f, base, path, start, size, dest):
    os.makedirs(os.path.dirname(dest) or '.', exist_ok=True)
    f.seek(base + start * SECTOR)
    with open(dest, 'wb') as o:
        left = size
        while left:
            b = f.read(min(left, 1 << 20)); o.write(b); left -= len(b)

if __name__ == '__main__':
    f, base, out = listing()
    if len(sys.argv) > 2 and sys.argv[1] == 'x':
        for path, start, size, isdir in out:
            if not isdir and path == sys.argv[2]:
                extract(f, base, path, start, size, os.path.join(sys.argv[3], os.path.basename(path)))
                print('extracted', path, size)
    elif len(sys.argv) > 2 and sys.argv[1] == 'xall':
        pre = sys.argv[3] if len(sys.argv) > 3 else ''
        n = 0
        for path, start, size, isdir in out:
            if not isdir and path.startswith(pre):
                extract(f, base, path, start, size, os.path.join(sys.argv[2], path)); n += 1
        print('extracted', n, 'files')
    else:
        print('base', hex(base))
        for path, start, size, isdir in sorted(out):
            print('%12d  %s' % (size, path))
