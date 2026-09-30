#!/usr/bin/env python3
"""make_logo.py -- the Plants vs. Zombies Touch logo in the game's sizes.

From logo.png (the port's logo, transparent) to resources/logo/, which
source/pvz_res.S builds in and source/pvz_logo.c writes over the game's own
at start-up:
  menu.png   the main menu's (reanim/mainmenu3/PvZ_Logo.png): 700 wide as the
             English one, which the menu animation places from the top left,
             so the same 700 keeps it centred; 190 high (the English is 116),
             which ends at the top of the house's roof
  title.png  the title / loading screen's (images/PvZ_Logo.png), which the
             game centres by its width: 692 wide as the English, 220 high
Run it again whenever logo.png changes (needs Pillow).
"""
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "resources", "logo")


def fit(src, canvas_w, canvas_h):
    """src scaled to canvas_h high (narrower if it must be), centred on a
    transparent canvas; premultiplied so the edges keep no dark fringe."""
    s = min(canvas_w / src.width, canvas_h / src.height)
    w, h = round(src.width * s), round(src.height * s)
    small = src.convert("RGBa").resize((w, h), Image.LANCZOS).convert("RGBA")
    out = Image.new("RGBA", (canvas_w, canvas_h), (0, 0, 0, 0))
    out.paste(small, ((canvas_w - w) // 2, (canvas_h - h) // 2))
    return out


def main():
    src = Image.open(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "logo.png"))
    src = src.convert("RGBA")
    src = src.crop(src.getbbox())  # no empty margin
    os.makedirs(OUT, exist_ok=True)
    for name, w, h in (("menu.png", 700, 190), ("title.png", 692, 220)):
        path = os.path.join(OUT, name)
        fit(src, w, h).save(path, optimize=True)
        print(f"{path}: {w}x{h}, {os.path.getsize(path)} bytes")


if __name__ == "__main__":
    main()
