#!/usr/bin/env python3
"""Read-only STFS (Xbox 360 LIVE/PIRS/CON package) reader.
  stfs.py <pkg>               list
  stfs.py <pkg> x <dir>       extract everything"""
import struct, sys, os

class Stfs:
    def __init__(self, path):
        self.f = open(path, 'rb')
        h = self.f.read(0xB000)
        self.hdr_size = struct.unpack_from('>I', h, 0x340)[0]
        vd = h[0x379:0x379 + 0x24]
        self.sex = (~vd[2]) & 1          # 0: one hash block per table (read-only packages)
        self.ft_count = struct.unpack_from('<H', vd, 3)[0]
        self.ft_block = vd[5] | (vd[6] << 8) | (vd[7] << 16)
        self.first_ht = (self.hdr_size + 0xFFF) & 0xFFFFF000
        self.step0 = 0xAB if self.sex == 0 else 0xAC
        self.step1 = 0x718F if self.sex == 0 else 0x723A

    def backing(self, b):
        r = (((b + 0xAA) // 0xAA) << self.sex) + b
        if b < 0xAA:
            return r
        if b < 0x70E4:
            return r + (((b + 0x70E4) // 0x70E4) << self.sex)
        return (1 << self.sex) + r + (((b + 0x70E4) // 0x70E4) << self.sex)

    def addr(self, b):
        return (self.backing(b) << 12) + self.first_ht

    def l0_hash_addr(self, b):
        if b < 0xAA:
            n = 0
        else:
            n = (b // 0xAA) * self.step0
            n += ((b // 0x70E4) + 1) << self.sex
            if b // 0x70E4:
                n += 1 << self.sex
        return (n << 12) + self.first_ht

    def next_block(self, b):
        self.f.seek(self.l0_hash_addr(b) + (b % 0xAA) * 0x18 + 0x15)
        x = self.f.read(3)
        return (x[0] << 16) | (x[1] << 8) | x[2]

    def read_block(self, b):
        self.f.seek(self.addr(b))
        return self.f.read(0x1000)

    def chain(self, start, count, consecutive):
        b = start
        for i in range(count):
            yield b
            b = b + 1 if consecutive else self.next_block(b)

    def entries(self):
        data = b''
        b = self.ft_block
        for i in range(self.ft_count):
            data += self.read_block(b)
            b = self.next_block(b)
        out = []
        for i in range(0, len(data), 0x40):
            e = data[i:i + 0x40]
            if not e[0]:
                continue
            fl = e[0x28]
            name = e[:fl & 0x3F].decode('latin1')
            blocks = e[0x29] | (e[0x2A] << 8) | (e[0x2B] << 16)
            start = e[0x2F] | (e[0x30] << 8) | (e[0x31] << 16)
            parent = struct.unpack_from('>h', e, 0x32)[0]
            size = struct.unpack_from('>I', e, 0x34)[0]
            out.append(dict(name=name, dir=bool(fl & 0x80), consecutive=bool(fl & 0x40), blocks=blocks,
                            start=start, parent=parent, size=size, index=len(out)))
        # paths
        for e in out:
            parts, p = [e['name']], e['parent']
            while p != -1 and p < len(out):
                parts.insert(0, out[p]['name'])
                p = out[p]['parent']
            e['path'] = '/'.join(parts)
        return out

    def read(self, e):
        out = bytearray()
        for b in self.chain(e['start'], e['blocks'], e['consecutive']):
            out += self.read_block(b)
        return bytes(out[:e['size']])

if __name__ == '__main__':
    s = Stfs(sys.argv[1])
    es = s.entries()
    if len(sys.argv) > 3 and sys.argv[2] == 'x':
        for e in es:
            p = os.path.join(sys.argv[3], e['path'])
            if e['dir']:
                os.makedirs(p, exist_ok=True)
            else:
                os.makedirs(os.path.dirname(p), exist_ok=True)
                open(p, 'wb').write(s.read(e))
        print('extracted', sum(1 for e in es if not e['dir']), 'files')
    else:
        for e in es:
            print('%10d %s%s' % (e['size'], e['path'], '/' if e['dir'] else ''))
