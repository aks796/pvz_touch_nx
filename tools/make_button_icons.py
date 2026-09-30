#!/usr/bin/env python3
"""make_button_icons.py -- the Switch's buttons as pictures, drawn for this port.

The game's text names buttons with tags (<A>, <S>, <LT>...), and the console
editions drew them as pictures with glyphs in their fonts; of the English
fonts only one has any, Xbox ones. So the port draws its own: the face
buttons in the Super Famicom / New 3DS colours (A red, B yellow, X blue, Y
green, the letter white); + and - and the D-pad dark grey as the Super
Famicom's Start, Select and cross, the sticks dark as the Switch's (all with a
light rim, to show on dark backgrounds); the shoulders light grey; and
source/pvz_english.c adds them to every English font as a layer of their own
(each character a picture, sized to the font), and to the help bar's sheets.

  resources/buttons/icons.png         every button at 96 pixels high, in a row
  source/pvz_button_icons.h           where each is in that row (generated)
  resources/buttons/help_buttons.png  the help bar's sheet (images/): 13 cels
  resources/buttons/help_buttons_small.png   of 42 (and 21) pixels, in the
                                      game's order A B X Y Start RB LB D-pad
                                      RT LT Back RS LS -> the Switch's A B X Y
                                      + R L D-pad ZR ZL - right/left stick
The letters: DejaVu Sans Bold (tools/fonts, free licence: LICENSE_DEJAVU).
Run it again after changing the drawing (needs Pillow).
"""
import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, 'resources', 'buttons')
FONT = os.path.join(HERE, 'fonts', 'DejaVuSans-Bold.ttf')

H = 96          # master height
SS = 4          # drawn at 4x, then reduced: smooth edges
DARK = (34, 34, 34, 255)
WHITE = (255, 255, 255, 255)
GREY = (58, 58, 62, 255)       # + -, the sticks, the D-pad
RIM = (168, 168, 172, 255)     # their rim
SHOULDER = (214, 214, 218, 255)
# the face buttons: fill, rim
FACE = {'A': ((214, 38, 46, 255), (120, 14, 20, 255)), 'B': ((244, 196, 0, 255), (150, 110, 0, 255)),
        'X': ((44, 82, 196, 255), (18, 36, 110, 255)), 'Y': ((40, 158, 72, 255), (14, 88, 34, 255))}

# (name, kind, label): the order is the characters' (U+E000 on)
BUTTONS = [
    ('A', 'round', 'A'), ('B', 'round', 'B'), ('X', 'round', 'X'), ('Y', 'round', 'Y'),
    ('PLUS', 'round', '+'), ('MINUS', 'round', '-'), ('L', 'shoulder', 'L'), ('R', 'shoulder', 'R'),
    ('ZL', 'trigger', 'ZL'), ('ZR', 'trigger', 'ZR'), ('LSTICK', 'stick', 'L'), ('RSTICK', 'stick', 'R'),
    ('DPAD', 'dpad', ''),
]


def font(px):
    return ImageFont.truetype(FONT, px)


def centred_text(d, box, text, px, fill=DARK, stroke=0, stroke_fill=DARK):
    f = font(px)
    l, t, r, b = d.textbbox((0, 0), text, font=f, stroke_width=stroke)
    x0, y0, x1, y1 = box
    d.text(((x0 + x1 - (r - l)) / 2 - l, (y0 + y1 - (b - t)) / 2 - t), text, font=f, fill=fill,
           stroke_width=stroke, stroke_fill=stroke_fill)


def draw(kind, label):
    s = SS
    w = {'round': H, 'stick': H, 'dpad': H, 'shoulder': int(H * 1.25), 'trigger': int(H * 1.45)}[kind]
    im = Image.new('RGBA', (w * s, H * s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    line = int(H * 0.045 * s)
    if kind == 'round' and label in FACE:
        fill, rim = FACE[label]
        m = line // 2
        d.ellipse((m, m, w * s - m, H * s - m), fill=fill, outline=rim, width=line * 2)
        centred_text(d, (0, 0, w * s, H * s), label, int(H * 0.60 * s), fill=WHITE,
                     stroke=int(H * 0.035 * s), stroke_fill=rim)
    elif kind in ('round', 'stick'):
        m = line // 2
        d.ellipse((m, m, w * s - m, H * s - m), fill=GREY, outline=RIM, width=line)
        if kind == 'stick':
            r = H * s * 0.30
            c = H * s / 2
            d.ellipse((c - r, c - r, c + r, c + r), outline=RIM, width=line)
            centred_text(d, (0, 0, w * s, H * s), label, int(H * 0.34 * s), fill=WHITE)
        else:  # + -
            c, arm, th = H * s / 2, H * s * 0.27, H * s * 0.11
            d.rectangle((c - arm, c - th / 2, c + arm, c + th / 2), fill=WHITE)
            if label == '+':
                d.rectangle((c - th / 2, c - arm, c + th / 2, c + arm), fill=WHITE)
    elif kind in ('shoulder', 'trigger'):
        m = line // 2
        top, bot = int(H * s * 0.14), int(H * s * 0.86)
        d.rounded_rectangle((m, top, w * s - m, bot), radius=int(H * s * 0.30), fill=SHOULDER, outline=DARK, width=line)
        centred_text(d, (0, top, w * s, bot), label, int(H * (0.50 if kind == 'shoulder' else 0.44) * s))
    elif kind == 'dpad':
        c, arm, th = H * s / 2, H * s * 0.47, H * s * 0.34
        pts = [(c - th / 2, c - arm), (c + th / 2, c - arm), (c + th / 2, c - th / 2), (c + arm, c - th / 2),
               (c + arm, c + th / 2), (c + th / 2, c + th / 2), (c + th / 2, c + arm), (c - th / 2, c + arm),
               (c - th / 2, c + th / 2), (c - arm, c + th / 2), (c - arm, c - th / 2), (c - th / 2, c - th / 2)]
        d.polygon(pts, fill=GREY, outline=RIM, width=line)
        r = H * s * 0.08
        d.ellipse((c - r, c - r, c + r, c + r), fill=RIM)
    return im.resize((w, H), Image.LANCZOS)


def fit(im, box):
    k = min(box / im.width, box / im.height)
    return im.resize((max(1, round(im.width * k)), max(1, round(im.height * k))), Image.LANCZOS)


def sheet(icons, cel):
    order = ['A', 'B', 'X', 'Y', 'PLUS', 'R', 'L', 'DPAD', 'ZR', 'ZL', 'MINUS', 'RSTICK', 'LSTICK']
    out = Image.new('RGBA', (cel * len(order), cel), (0, 0, 0, 0))
    for i, name in enumerate(order):
        ic = fit(icons[name], cel - max(1, cel // 21))
        out.alpha_composite(ic, (i * cel + (cel - ic.width) // 2, (cel - ic.height) // 2))
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    icons = {name: draw(kind, label) for name, kind, label in BUTTONS}
    row = Image.new('RGBA', (sum(icons[n].width for n, _, _ in BUTTONS), H), (0, 0, 0, 0))
    x, table = 0, []
    for n, _, _ in BUTTONS:
        row.alpha_composite(icons[n], (x, 0))
        table.append((n, x, icons[n].width))
        x += icons[n].width
    row.save(os.path.join(OUT, 'icons.png'), optimize=True)
    sheet(icons, 42).save(os.path.join(OUT, 'help_buttons.png'), optimize=True)
    sheet(icons, 21).save(os.path.join(OUT, 'help_buttons_small.png'), optimize=True)
    with open(os.path.join(ROOT, 'source', 'pvz_button_icons.h'), 'w') as f:
        f.write('/* pvz_button_icons.h -- generated by tools/make_button_icons.py: where each\n'
                ' * button is in resources/buttons/icons.png. The character of button i is\n'
                ' * U+E000 + i. */\n')
        f.write('#define PVZ_ICON_H %d\n#define PVZ_ICON_COUNT %d\n' % (H, len(table)))
        f.write('enum {\n' + ''.join('  PVZ_ICON_%s,\n' % n for n, _, _ in table) + '};\n')
        f.write('static const struct {\n  int x, w;\n} k_pvz_icons[PVZ_ICON_COUNT] = {\n')
        f.write(''.join('    {%d, %d}, /* %s */\n' % (x0, w, n) for n, x0, w in table) + '};\n')
    print('icons.png %dx%d, help_buttons 546x42, help_buttons_small 273x21' % (row.width, row.height))


if __name__ == '__main__':
    main()
