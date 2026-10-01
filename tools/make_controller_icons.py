#!/usr/bin/env python3
"""make_controller_icons.py -- the VS screen's controller pictures, as Switch
Pro Controllers.

The game picks a side for each player with a picture of a controller,
images/gamepad0.png (player 1) and gamepad1.png (player 2), 170 x 122: the
Xbox 360 edition's controller in a glow of the player's colour -- the colour
of that player's P1 / P2 label, arrows and cursors in the match (p1_text,
board_selector, seed_selector; the _BLUE ones are player 2's). The port
shows a Pro Controller instead, from resources/controllers/controller.png
(1536 x 1024, transparent round the controller), made the way the game's are:

  the controller   fitted to the 360 controller's box in the game's
                   pictures (153 x 105 at 8, 8), ink outline included
  the outline      the game's art is inked: a black line round the
                   controller, INK pixels wide
  the glow         under them, in the game's own colours: 255,255,0 for
                   player 1, 0,255,255 for player 2, as strong as theirs.
                   Measured from them: GLOW_D / GLOW_DA, its alpha by the
                   distance from the outline along their straight edges;
                   GLOW_T / GLOW_A, its alpha by how much of the controller
                   is near (a Gaussian of 5.5 pixels), which is what fills
                   the hollow between the grips fuller, as theirs is. The
                   glow is the greater of the two.

  resources/controllers/gamepad0.png   player 1
  resources/controllers/gamepad1.png   player 2
source/pvz_setup_plan.c writes them over the game's pictures.
Run it again after changing controller.png (needs Pillow and NumPy).
"""
import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
DIR = os.path.join(os.path.dirname(HERE), 'resources', 'controllers')
W, H = 170, 122
BOX = (8, 8, 153, 105)  # the game's controller with its outline: x, y, w, h
COLOURS = (('gamepad0.png', (255, 255, 0)), ('gamepad1.png', (0, 255, 255)))
INK, INK_RGB = 1.3, (8, 10, 8)
SS = 4  # the outline's supersampling
GLOW_D = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
GLOW_DA = (205, 200, 184, 149, 98, 60, 33, 15, 4, 1, 0)
GLOW_SIGMA = 5.5
GLOW_T = (0, .05, .10, .15, .20, .25, .30, .35, .40, .45, .50)
GLOW_A = (0, 2, 17, 45, 79, 117, 156, 191, 203, 205, 205)


def _dt1(f):
    """1-D squared distance transform (Felzenszwalb & Huttenlocher)"""
    n = len(f)
    d, v, z = np.empty(n), np.zeros(n, int), np.empty(n + 1)
    k, z[0], z[1] = 0, -1e20, 1e20
    for q in range(1, n):
        s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k])
        while s <= z[k]:
            k -= 1
            s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k])
        k += 1
        v[k], z[k], z[k + 1] = q, s, 1e20
    k = 0
    for q in range(n):
        while z[k + 1] < q:
            k += 1
        d[q] = (q - v[k]) ** 2 + f[v[k]]
    return d


def edt(inside):
    """each pixel's distance to the nearest inside one"""
    f = np.where(inside, 0.0, 1e20)
    f = np.stack([_dt1(c) for c in f.T]).T
    return np.sqrt(np.stack([_dt1(r) for r in f]))


def gauss(m, s):
    r = int(np.ceil(3.5 * s))
    x = np.arange(-r, r + 1)
    k = np.exp(-x * x / (2.0 * s * s))
    k /= k.sum()
    p = np.pad(m, r)
    p = np.apply_along_axis(lambda v: np.convolve(v, k, 'same'), 1, p)
    p = np.apply_along_axis(lambda v: np.convolve(v, k, 'same'), 0, p)
    return p[r:-r, r:-r]


def fitted(scale):
    """controller.png fitted into BOX less the outline, on a W*scale x
    H*scale canvas, premultiplied (so the resized edges keep their colour)"""
    im = Image.open(os.path.join(DIR, 'controller.png')).convert('RGBA')
    a = np.array(im).astype(np.float64)
    a[..., 3] = np.minimum(a[..., 3] * 255.0 / max(1, a[..., 3].max()), 255)
    ys, xs = np.nonzero(a[..., 3] > 16)  # not its faint fringe
    a = a[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    h0, w0 = a.shape[:2]
    bw, bh = BOX[2] - 2 * INK, BOX[3] - 2 * INK
    k = min(bw / w0, bh / h0)
    w, h = round(w0 * k * scale), round(h0 * k * scale)
    a[..., :3] *= a[..., 3:] / 255.0
    ch = [np.array(Image.fromarray(a[..., i].astype(np.float32)).resize((w, h), Image.LANCZOS))
          for i in range(4)]
    out = np.zeros((H * scale, W * scale, 4))
    x0 = round((BOX[0] + BOX[2] / 2.0) * scale - w / 2.0)
    y0 = round((BOX[1] + BOX[3] / 2.0) * scale - h / 2.0)
    out[y0:y0 + h, x0:x0 + w] = np.clip(np.stack(ch, -1), 0, 255)
    return out


def over(dst_rgb, dst_a, rgb, a):
    """straight-alpha (rgb, a) over (dst_rgb, dst_a); rgb premultiplied"""
    out_a = a + dst_a * (1 - a)
    out_c = rgb + dst_rgb * (dst_a * (1 - a))[..., None]
    return out_c, out_a


def main():
    ctl = fitted(1) / 255.0                       # premultiplied, 0..1
    big = fitted(SS)[..., 3] / 255.0
    ink4 = (edt(big >= 0.5) / SS <= INK).astype(np.float64)
    ink = ink4.reshape(H, SS, W, SS).mean(axis=(1, 3))
    body = ink > 0.05
    glow = np.maximum(np.interp(edt(body), GLOW_D, GLOW_DA),
                      np.interp(gauss(body.astype(np.float64), GLOW_SIGMA), GLOW_T, GLOW_A)) / 255.0
    for name, rgb in COLOURS:
        c, a = np.array(rgb, np.float64)[None, None] / 255.0 * glow[..., None], glow
        c, a = over(c / np.maximum(a, 1e-9)[..., None], a,
                    np.array(INK_RGB, np.float64)[None, None] / 255.0 * ink[..., None], ink)
        c, a = over(c / np.maximum(a, 1e-9)[..., None], a, ctl[..., :3], ctl[..., 3])
        px = np.zeros((H, W, 4))
        nz = a > 0
        px[nz, :3] = c[nz] / a[nz, None]
        px[..., 3] = a
        Image.fromarray(np.clip(np.round(px * 255), 0, 255).astype(np.uint8)).save(
            os.path.join(DIR, name), optimize=True)
    print('wrote %s (%dx%d) in %s' % (' and '.join(n for n, _ in COLOURS), W, H, DIR))


if __name__ == '__main__':
    main()
