#!/usr/bin/env python3
"""ptx2png.py <in.ptx> <out.png>   (or: ptx2png.py --all <dir> <outdir>)

PopCap's Xbox 360 textures: DXT5 blocks in linear rows (pitch bytes apart),
16-bit big-endian words, then a 16-byte trailer:
big-endian width, height, pitch, and the D3DFORMAT (0x1A200054, DXT4_5)."""
import os, struct, sys

import numpy as np
from PIL import Image


def decode(data):
    """Linear rows of DXT5 blocks, `pitch` bytes apart, in big-endian 16-bit words."""
    w, h, pitch = struct.unpack_from('>III', data, len(data) - 16)
    fmt = struct.unpack_from('<I', data, len(data) - 4)[0]
    if fmt & 0x3F != 0x14:
        raise ValueError('format 0x%08x is not DXT4_5' % fmt)
    bw, bh = (w + 3) // 4, (h + 3) // 4
    body = np.frombuffer(data, np.uint8)[:-16]
    need = (bh - 1) * pitch + bw * 16
    if len(body) < need:
        body = np.concatenate([body, np.zeros(need - len(body), np.uint8)])
    rows = np.stack([body[by * pitch:by * pitch + bw * 16] for by in range(bh)])  # bh, bw*16
    blk = rows.reshape(bh, bw, 16).copy()
    blk = blk.reshape(bh, bw, 8, 2)[..., ::-1].reshape(bh, bw, 16)  # 8in16
    a0 = blk[..., 0].astype(np.int32)
    a1 = blk[..., 1].astype(np.int32)
    abits = np.zeros((bh, bw), np.uint64)
    for k in range(6):
        abits |= blk[..., 2 + k].astype(np.uint64) << np.uint64(8 * k)
    pal_a = np.zeros((bh, bw, 8), np.int32)
    pal_a[..., 0], pal_a[..., 1] = a0, a1
    gt = a0 > a1
    for i in range(6):
        pal_a[..., 2 + i] = np.where(gt, ((6 - i) * a0 + (i + 1) * a1) // 7, 0)
    for i in range(4):
        pal_a[..., 2 + i] = np.where(gt, pal_a[..., 2 + i], ((4 - i) * a0 + (i + 1) * a1) // 5)
    pal_a[..., 6] = np.where(gt, pal_a[..., 6], 0)
    pal_a[..., 7] = np.where(gt, pal_a[..., 7], 255)
    c0 = blk[..., 8].astype(np.int32) | (blk[..., 9].astype(np.int32) << 8)
    c1 = blk[..., 10].astype(np.int32) | (blk[..., 11].astype(np.int32) << 8)
    cbits = (blk[..., 12].astype(np.uint32) | (blk[..., 13].astype(np.uint32) << 8) |
             (blk[..., 14].astype(np.uint32) << 16) | (blk[..., 15].astype(np.uint32) << 24))

    def expand(c):
        r, g, b = (c >> 11) & 31, (c >> 5) & 63, c & 31
        return np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)], -1)
    p0, p1 = expand(c0), expand(c1)
    pal_c = np.stack([p0, p1, (2 * p0 + p1) // 3, (p0 + 2 * p1) // 3], 2)  # bh, bw, 4, 3
    img = np.zeros((bh, 4, bw, 4, 4), np.uint8)
    for i in range(16):
        ci = ((cbits >> np.uint32(2 * i)) & np.uint32(3)).astype(np.int64)
        ai = ((abits >> np.uint64(3 * i)) & np.uint64(7)).astype(np.int64)
        rgb = np.take_along_axis(pal_c, ci[..., None, None].repeat(3, -1), 2)[..., 0, :]
        alpha = np.take_along_axis(pal_a, ai[..., None], 2)[..., 0]
        img[:, i // 4, :, i % 4, :3] = rgb
        img[:, i // 4, :, i % 4, 3] = alpha
    img = img.reshape(bh * 4, bw * 4, 4)
    return Image.fromarray(np.ascontiguousarray(img[:h, :w]))


if __name__ == '__main__':
    if sys.argv[1] == '--all':
        src, dst = sys.argv[2], sys.argv[3]
        n = 0
        for dp, dn, fn in os.walk(src):
            for f in fn:
                if f.lower().endswith('.ptx'):
                    p = os.path.join(dp, f)
                    o = os.path.join(dst, os.path.relpath(p, src))[:-4] + '.png'
                    os.makedirs(os.path.dirname(o), exist_ok=True)
                    decode(open(p, 'rb').read()).save(o)
                    n += 1
        print(n, 'textures')
    else:
        decode(open(sys.argv[1], 'rb').read()).save(sys.argv[2])
