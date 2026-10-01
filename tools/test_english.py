#!/usr/bin/env python3
"""test_english.py -- build source/pvz_english.c + pvz_strings.c on the host
(tools/host/: the miniz calls they make, over zlib) and check the English
layer they make from your two APKs against an independent reading of them:

  python3 tools/test_english.py <game.apk> <english.apk>

game.apk:    PvZ TV Touch 1.1.5 (the newest mod, Chinese).
english.apk: the English PvZ TV Touch 4.0.5 or RedStr1x APK.

Checks: every file the layer swaps in is english.apk's exact file and only
the files the China pak lists; menu signs are english.apk's atlas pixels; each
English font names only pictures the layer has; the string tables keep every
key of the game's, with the game's printf conversions, and (from english.apk)
what text is left Chinese; a second run keeps the layer; no english.apk makes
the text only; language = chinese removes it all; compiled fonts are cleared.
Needs python3 with Pillow and numpy.
"""
import io, os, re, struct, subprocess, sys, tempfile, zipfile, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)
SRC = os.path.join(TOP, 'source')
RES = os.path.join(TOP, 'resources', 'english')

HARNESS = r'''
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pvz_english.h"
static void lg(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }
static unsigned char *slurpn(const char *d, const char *n, size_t *len) {
  char p[600]; snprintf(p, sizeof p, "%s/%s", d, n);
  FILE *f = fopen(p, "rb"); if (!f) { *len = 0; return NULL; }
  fseek(f, 0, SEEK_END); long l = ftell(f); fseek(f, 0, SEEK_SET);
  unsigned char *b = malloc(l); if (fread(b, 1, l, f) != (size_t)l) exit(3); fclose(f); *len = (size_t)l; return b; }
static char *slurp(const char *d, const char *n) {
  char p[600]; snprintf(p, sizeof p, "%s/%s", d, n);
  FILE *f = fopen(p, "rb"); if (!f) return NULL;
  fseek(f, 0, SEEK_END); long l = ftell(f); fseek(f, 0, SEEK_SET);
  char *b = malloc(l + 1); if (fread(b, 1, l, f) != (size_t)l) exit(3); b[l] = 0; fclose(f); return b; }
int main(int argc, char **argv) {   /* apply|remove <game.apk> <english.apk> <out dir> <res dir> */
  if (argc != 6) return 2;
  char files[600], list[600], ext[600];
  snprintf(files, sizeof files, "%s/data/files", argv[4]);
  snprintf(list, sizeof list, "%s/.english", argv[4]);
  snprintf(ext, sizeof ext, "%s/external/files", argv[4]);
  size_t il, hl, hsl;
  unsigned char *ic = slurpn(argv[5], "../buttons/icons.png", &il), *hb = slurpn(argv[5], "../buttons/help_buttons.png", &hl),
                *hs = slurpn(argv[5], "../buttons/help_buttons_small.png", &hsl);
  size_t p0l, p1l;
  unsigned char *p0 = slurpn(argv[5], "../controllers/gamepad0.png", &p0l), *p1 = slurpn(argv[5], "../controllers/gamepad1.png", &p1l);
  PvzEnglishCfg c = {files, list, argv[3], {files, ext},
                     {slurp(argv[5], "AddonStrings_en.txt"), slurp(argv[5], "LawnStrings_en.txt"),
                      slurp(argv[5], "LawnStrings_fix.txt"), ic, hb, hs, il, hl, hsl, {p0, p1}, {p0l, p1l}},
                     lg, NULL, NULL, 0};
  if (!strcmp(argv[1], "remove")) { pvz_english_remove(&c); return 0; }
  mz_zip_archive z; memset(&z, 0, sizeof z);
  if (!mz_zip_reader_init_file(&z, argv[2], 0)) return 4;
  int r = pvz_english_apply(&z, &c);
  mz_zip_reader_end(&z);
  return r ? 1 : 0; }
'''

TAKE_EN = {'reanim/finalwave.png', 'reanim/zombieswon.png', 'reanim/mainmenu3/pvz_logo.png',
           'images/pvz_logo.png', 'reanim/mainmenu3/survival button.png',
           'reanim/mainmenu3/survival pressed.png', 'reanim/mainmenu3/survival selected.png',
           'images/survival_button.png', 'reanim/mainmenu3/almanac plant 10.png', 'images/guide.png'}
# pvz_english.c's k_keep_game: never taken (the bubble with the Xbox prompt painted in)
KEEP_GAME = {'images/store_speechbubble2.png'}
CJK = re.compile(r'[　-鿿＀-￯]')
CONV = re.compile(r'%(?!%)[-+ #0]*[\d*]*(?:\.[\d*]+)?([hlLqjzt]*[diouxXeEfFgGaAcspn])')
FAILS = []


def check(cond, msg):
    if not cond:
        FAILS.append(msg)
        print('FAIL:', msg)


def ci_index(z, prefix):
    return {n[len(prefix):].lower(): n for n in z.namelist()
            if n.startswith(prefix) and not n.endswith('/')}


def parse(text):
    if text.startswith('﻿'):
        text = text[1:]
    out, cur, buf = {}, None, []
    for line in text.replace('\r\n', '\n').split('\n'):
        m = re.fullmatch(r'\[([A-Za-z0-9_]+)\][ \t]*', line)
        if m:
            if cur is not None and cur not in out:
                out[cur] = '\n'.join(buf).strip('\n')
            cur, buf = m.group(1), []
        elif cur is not None:
            buf.append(line)
    if cur is not None and cur not in out:
        out[cur] = '\n'.join(buf).strip('\n')
    return out


def convs(s):
    return [m.group(1) for m in CONV.finditer(s)]


def conv_ok(en, zh):
    a, b = convs(en), convs(zh)
    return a == b if b else not any(c[-1] in 'snp' for c in a)


def tex_rgba(b):
    w, h, fmt = struct.unpack_from('<III', b, 12)
    import numpy as np
    px = np.frombuffer(zlib.decompress(b[48:]), np.uint8).reshape(h, w, 4)
    return px[:, :, [2, 1, 0, 3]]


def regions(xml):
    tex, out = None, {}
    for m in re.finditer(r'<atlas [^>]*base="([^"]*)" path="([^"]*)"|<image name="([^"]+)" x="(\d+)" y="(\d+)" w="(\d+)" h="(\d+)"', xml):
        if m.group(1) is not None:
            tex = m.group(1) + m.group(2) + '.tex'
        else:
            out.setdefault(m.group(3), (tex,) + tuple(int(v) for v in m.group(4, 5, 6, 7)))
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    game, eng = sys.argv[1], sys.argv[2]
    import numpy as np
    from PIL import Image
    g, e = zipfile.ZipFile(game), zipfile.ZipFile(eng)
    gi, ei = ci_index(g, 'assets/files/'), ci_index(e, 'assets/files/')
    pak = next(n for n in e.namelist() if n.startswith('assets/paks/') and 'changegamechina' in n.lower())
    c = zipfile.ZipFile(io.BytesIO(e.read(pak)))
    ci = ci_index(c, '')
    crc = lambda z, n: z.getinfo(n).CRC

    with tempfile.TemporaryDirectory() as t:
        exe = os.path.join(t, 'h')
        open(os.path.join(t, 'h.c'), 'w').write(HARNESS)
        subprocess.check_call(['cc', '-O1', '-g', '-Wall', '-Wextra', '-fsanitize=address,undefined',
                               '-I', os.path.join(HERE, 'host'), '-I', SRC, os.path.join(t, 'h.c'),
                               os.path.join(SRC, 'pvz_english.c'), os.path.join(SRC, 'pvz_strings.c'),
                               os.path.join(HERE, 'host', 'miniz_host.c'), '-lz', '-o', exe])
        out = os.path.join(t, 'sd')
        files = os.path.join(out, 'data', 'files')
        for d in ('data/files/cached/data', 'external/files/cached/data'):
            os.makedirs(os.path.join(out, d))
            open(os.path.join(out, d, 'DwarvenTodcraft18.txt.cfu2'), 'wb').write(b'old')

        def run(*a):
            r = subprocess.run([exe] + list(a) + [out, RES], capture_output=True, text=True)
            sys.stdout.write(r.stdout)
            if r.stderr:
                print(r.stderr)
            return r.returncode, r.stdout

        rc, log = run('apply', game, eng)
        check(rc == 0, 'apply returned %d' % rc)
        made = open(os.path.join(out, '.english')).read().split('\n')[1:]
        made = [m for m in made if m]
        low = {m.lower() for m in made}
        check(len(low) == len(made), 'the layer list names a file twice')
        for m in made:
            check(os.path.isfile(os.path.join(files, m)), 'listed but missing: ' + m)
        check(not any(f.endswith('.cfu2') for _, _, fs in os.walk(out) for f in fs),
              'compiled fonts were not cleared')

        # --- pictures: exactly the pak's files, as english.apk has them
        want = {}
        for rel, cn in ci.items():
            if '/' not in rel or rel.startswith(('properties/', 'addonfiles/properties/', 'data/')):
                continue
            if rel not in gi or rel not in ei:
                continue
            en = ei[rel]
            if crc(e, en) == crc(c, cn):
                continue
            if rel in KEEP_GAME:
                check(rel not in low, 'taken, though the game\'s is kept: ' + rel)
                continue
            if crc(g, gi[rel]) == crc(c, cn) or rel in TAKE_EN:
                want[rel] = en
        for rel, en in want.items():
            p = os.path.join(files, rel)
            check(rel in low, 'not swapped: ' + rel)
            if rel in low:
                real = next(m for m in made if m.lower() == rel)
                check(open(os.path.join(files, real), 'rb').read() == e.read(en), 'not english.apk\'s bytes: ' + rel)
        print('OK: %d pictures swapped as expected' % len(want))

        # --- menu signs
        crops = 0
        for rel, cn in ci.items():
            if not rel.endswith('.xml') or rel in gi or rel not in ei:
                continue
            er = regions(e.read(ei[rel]).decode('utf-8', 'replace'))
            cr = regions(c.read(cn).decode('utf-8', 'replace'))
            texs = {}
            for name, (tex, x, y, w, h) in er.items():
                outrel = (tex[:tex.rfind('/') + 1] + name).lower()
                if outrel not in gi or outrel in want:
                    continue
                if tex not in texs:
                    texs[tex] = tex_rgba(e.read(ei[tex.lower()]))
                en = texs[tex][y:y + h, x:x + w]
                if name in cr:
                    ctex, cx, cy, cw, ch = cr[name]
                    key = 'zh:' + ctex
                    if key not in texs:  # not in the pak: the English build's, unchanged
                        texs[key] = tex_rgba(c.read(ci[ctex.lower()]) if ctex.lower() in ci else e.read(ei[ctex.lower()]))
                    if np.array_equal(en, texs[key][cy:cy + ch, cx:cx + cw]):
                        check(outrel not in low, 'sign without words written: ' + outrel)
                        continue
                if Image.open(io.BytesIO(g.read(gi[outrel]))).size != (w, h):
                    continue
                check(outrel in low, 'sign not written: ' + outrel)
                if outrel in low:
                    real = next(m for m in made if m.lower() == outrel)
                    got = np.array(Image.open(os.path.join(files, real)).convert('RGBA'))
                    check(np.array_equal(got, en), 'sign pixels differ: ' + outrel)
                    crops += 1
        print('OK: %d menu signs match english.apk\'s atlas' % crops)

        # --- fonts
        fonts = 0
        for rel in ci:
            if not (rel.startswith('data/') and rel.endswith('.txt')) or rel not in gi or rel not in ei:
                continue
            p = [m for m in made if m.lower() == rel]
            check(p, 'font not made: ' + rel)
            if not p:
                continue
            raw = open(os.path.join(files, p[0]), 'rb').read()
            check(raw.startswith(b'\xef\xbb\xbf'), 'font %s: not UTF-8 with a byte-order mark' % rel)
            text = raw.decode('utf-8-sig')
            names = re.findall(r"^\s*LayerSetImage\s+\S+\s+'([^']+)'", text, re.M)
            check(names and all(n.endswith('_en') for n in names), 'font names not renamed: %s %s' % (rel, names))
            for n in names:
                have = [m for m in made if re.fullmatch(r'data/_?%s_?\.(png|gif|jpg|jpeg|tga|bmp)' % re.escape(n), m, re.I)]
                check(have, 'font %s: no picture for %s' % (rel, n))
            ob = e.read(ei[rel])  # read as the engine does: UTF-8 after a byte-order mark, else a byte a character
            orig = ob.decode('utf-8-sig') if ob.startswith(b'\xef\xbb\xbf') else ob.decode('latin-1')
            cut = text.find('Define SwitchButtonChars')  # the port's button layer, appended
            check(cut > 0, 'font %s: no button layer' % rel)
            head = text[:cut].rstrip('\r\n') if cut > 0 else text
            check(re.sub(r"(LayerSetImage\s+\S+\s+')([^']+)'", r"\1\2_en'", orig).rstrip('\r\n') == head,
                  'font descriptor changed beyond the names and the button layer: ' + rel)
            if cut > 0:
                tail = text[cut:]
                chars = re.findall(r"'([^']+)'", tail.split(');')[0])
                check(len(chars) == 13 and all(c == chr(0xE000 + i) for i, c in enumerate(chars)),
                      'font %s: button characters %r' % (rel, chars))
                for layer in re.findall(r'^CreateLayer\s+(\S+?);', text[:cut], re.M):
                    check(re.search(r'^LayerSetCharWidths\s+%s SwitchButtonChars SwitchButtonWidths;' % re.escape(layer), tail, re.M),
                          'font %s: layer %s lacks the buttons\' widths' % (rel, layer))
                pic = re.search(r"LayerSetImage\s+SwitchButtons '([^']+)'", tail)
                ppath = os.path.join(files, 'data', pic.group(1) + '.png') if pic else ''
                check(pic and os.path.exists(ppath), 'font %s: no button picture' % rel)
                if pic and os.path.exists(ppath):
                    im = Image.open(ppath)
                    rects = [tuple(map(int, r)) for r in re.findall(r'\((\d+), (\d+), (\d+), (\d+)\)', tail.split('SwitchButtonRects')[1].split(');')[0])]
                    check(len(rects) == 13 and all(x + w <= im.width and y + h <= im.height for x, y, w, h in rects),
                          'font %s: button rects outside the picture' % rel)
            fonts += 1
        print('OK: %d English fonts, each naming only its own _en pictures, each with the button layer' % fonts)

        # --- the help bar's sheets: the game's own (the Xbox 360 edition's), relabelled for the Switch in
        # cels 4 5 6 8 9 10 (+ R L ZR ZL -), the other cels the game's pixel for pixel
        import numpy as np
        for rel, cel in (('images/help_buttons.png', 42), ('images/help_buttons_small.png', 21)):
            mine = os.path.join(files, rel)
            check(os.path.exists(mine), 'no ' + rel)
            if not os.path.exists(mine):
                continue
            a = np.array(Image.open(mine).convert('RGBA')).astype(int)
            o = np.array(Image.open(io.BytesIO(g.read('assets/files/' + rel))).convert('RGBA')).astype(int)
            check(a.shape == o.shape, rel + ': not the game\'s size')
            if a.shape != o.shape:
                continue
            changed = [i for i in range(13) if (a[:, i * cel:(i + 1) * cel] != o[:, i * cel:(i + 1) * cel]).any()]
            check(changed == [4, 5, 6, 8, 9, 10], '%s: cels changed %s' % (rel, changed))
        print('OK: help bar sheets: the game\'s own, with + R L ZR ZL - (the other cels untouched)')

        # --- the versus screens' controllers: the side picker's the port's Pro Controllers; the held ones
        # the game's own recoloured: its size and transparency, only the controller changed (not the
        # zombie's teeth on its edge), its white plastic gone dark
        for i in range(2):
            rel = 'images/gamepad%d.png' % i
            mine = os.path.join(files, rel)
            check(os.path.exists(mine) and open(mine, 'rb').read() ==
                  open(os.path.join(RES, '..', 'controllers', 'gamepad%d.png' % i), 'rb').read(), rel + ': not the port\'s')
        for rel, keep in (('images/plant_side_selected.png', None), ('images/zombie_side_selected.png', None),
                          ('images/help_menu_image_vs_controllers.png', (80, 78, 100, 87))):
            mine = os.path.join(files, rel)
            check(os.path.exists(mine), 'no ' + rel)
            if not os.path.exists(mine):
                continue
            a = np.array(Image.open(mine).convert('RGBA')).astype(int)
            o = np.array(Image.open(io.BytesIO(g.read('assets/files/' + rel))).convert('RGBA')).astype(int)
            check(a.shape == o.shape and (a[..., 3] == o[..., 3]).all(), rel + ': not the game\'s size and transparency')
            if a.shape != o.shape:
                continue
            ch = (a[..., :3] != o[..., :3]).any(2)
            white = (o[..., 3] > 0) & (o[..., :3].min(2) > 200) & (o[..., :3].max(2) - o[..., :3].min(2) < 12) & ch
            check(ch.sum() > 1500, '%s: only %d pixels changed' % (rel, ch.sum()))
            check(white.sum() > 100 and a[..., :3][white].mean() < 110, '%s: the white plastic is not dark' % rel)
            if keep:
                x0, y0, x1, y1 = keep
                check(not ch[y0:y1, x0:x1].any(), rel + ': the teeth changed')
        print('OK: versus controllers: the side picker\'s the Pro Controller, the held ones recoloured')

        # --- text
        gz = parse(g.read('assets/files/properties/LawnStrings.txt').decode('utf-8'))
        ls = parse(open(os.path.join(files, 'properties/LawnStrings.txt'), encoding='utf-8').read())
        check(set(gz) <= set(ls), 'LawnStrings lost keys: %s' % sorted(set(gz) - set(ls))[:10])
        bad = [k for k in gz if not conv_ok(ls[k], gz[k])]
        check(not bad, 'printf conversions differ: %s' % bad[:10])
        glyph = [k for k in ls if re.search(r'[\u00c0-\u00ff]', ls[k])]
        check(not glyph, 'button glyph letters left (they show as letters in most fonts): %s' % glyph)
        zh_left = [k for k in gz if CJK.search(ls[k])]
        print('LawnStrings: %d keys, %d still with Chinese: %s' % (len(gz), len(zh_left), zh_left))
        for k, v in (('HELP_TEXT_PAGE_1_OF_X', 'Page 1'), ('ITS_RAINING_SEEDS', "It's Raining Seeds"),
                     ('HELP_TEXT_2_A', '\ue000 Plant'), ('P2_JOIN', 'Player 2\nPress \ue004 to join!'),
                     ('OK_BUTTON', '\ue000'), ('BACK_BUTTON', '\ue001'), ('MENU_BUTTON', 'Menu'),
                     ('CROW_SELECTION_10', 'Attack other people in this fun conflict.')):
            check(ls.get(k) == v, '%s = %r' % (k, ls.get(k)))
        tagged = [k for k in ls if not k.startswith('<') and re.search(r'<(A|B|X|Y|S|L|R|LT|RT|D|l|r|b)>', ls[k])]
        check(not tagged, 'button tags left in the text (they show as written): %s' % tagged[:10])
        ga = parse(g.read('assets/files/addonFiles/properties/AddonStrings.txt').decode('utf-8'))
        ad = parse(open(os.path.join(files, 'addonFiles/properties/AddonStrings.txt'), encoding='utf-8').read())
        check(set(ga) <= set(ad), 'AddonStrings lost keys')
        check(not [k for k in ga if not conv_ok(ad[k], ga[k])], 'AddonStrings printf conversions differ')
        a_left = [k for k in ga if CJK.search(ad[k])]
        check(not a_left, 'AddonStrings still Chinese: %s' % a_left)
        oem = parse(open(os.path.join(files, 'properties/LawnOEMStrings.txt'), encoding='utf-8').read())
        check(oem and all(k.startswith('CROW_SELECTION') and not CJK.search(v) for k, v in oem.items()),
              'LawnOEMStrings is not the crow lines in English')
        print('OK: string tables (AddonStrings %d keys, all English; OEM %d crow lines)' % (len(ga), len(oem)))
        # the port's own keys (not in the game's table), buttons as pictures
        for k, v in (('SWITCH_HELP_2_1', '\ue000  Plant, collect, select'), ('CREDITS_EXIT', 'Exit'),
                     ('SWITCH_HELP_2_7', '\ue004  Pause     \ue005  Item bar'), ('SERVER_ROOM_VERSION_ERROR_LOWER', 'Older build'), ('SWITCH_CLOSE', '\ue001  Close')):
            check(ad.get(k) == v, 'AddonStrings %s = %r' % (k, ad.get(k)))
        # the help screen's Controls page: the Switch's seven lines, the rest the game's
        hm = open(os.path.join(files, 'menu/HelpMenu.menu.txt'), encoding='utf-8').read()
        gm = g.read('assets/files/menu/HelpMenu.menu.txt').decode('utf-8')
        page2 = hm[hm.index('#Page 2'):hm.index('#Page 3')]
        check(all("SetText '[SWITCH_HELP_2_%d]'" % i in page2 for i in range(1, 8)) and 'HELP_TEXT_2_A' not in page2,
              'the Controls page is not the Switch\'s')
        check(all(re.search(r"^SetText '\[(\w+)\]';", l) is None or re.match(r"^SetText '\[(\w+)\]';", l).group(1) in ad or
                  re.match(r"^SetText '\[(\w+)\]';", l).group(1) in ls for l in page2.splitlines()),
              'the Controls page names a key no table has')
        check(hm[:hm.index('#Page 2')] == gm[:gm.index('#Page 2')] and hm[hm.index('#Page 3'):] == gm[gm.index('#Page 3'):],
              'the help screen changed beyond its Controls page')
        print('OK: help screen Controls page (%d lines of the Switch\'s controls)' % page2.count('SWITCH_HELP_2_'))

        # --- again: kept
        rc, log = run('apply', game, eng)
        check(rc == 0 and 'up to date' in log, 'second run did not keep the layer')
        # --- english.apk deleted after the layer was made: the layer stays
        rc, log = run('apply', game, os.path.join(t, 'nope.apk'))
        made1 = [m for m in open(os.path.join(out, '.english')).read().split('\n')[1:] if m]
        check(rc == 0 and 'no longer needed' in log and sorted(made1) == sorted(made),
              'the layer did not survive english.apk being deleted')
        # --- an older build's layer, english.apk gone: kept, not downgraded
        lst = os.path.join(out, '.english')
        head, rest = open(lst).read().split('\n', 1)
        open(lst, 'w').write('pvz-english 0000000000000000 ' + head.split(' ', 2)[2] + '\n' + rest)
        rc, log = run('apply', game, os.path.join(t, 'nope.apk'))
        made1b = [m for m in open(lst).read().split('\n')[1:] if m]
        check(rc == 0 and 'kept as it is' in log and sorted(made1b) == sorted(made),
              'an older layer was replaced without english.apk')
        # --- no english.apk and no layer from one: text only
        run('remove', game, eng)
        rc, log = run('apply', game, os.path.join(t, 'nope.apk'))
        made2 = [m for m in open(os.path.join(out, '.english')).read().split('\n')[1:] if m]
        # (the help bar's sheets and the versus controllers too, whatever the text does)
        check(rc == 0 and sorted(made2) == sorted(['properties/LawnStrings.txt', 'properties/LawnOEMStrings.txt',
                                                   'addonFiles/properties/AddonStrings.txt', 'menu/HelpMenu.menu.txt',
                                                   'images/help_buttons.png', 'images/help_buttons_small.png',
                                                   'images/gamepad0.png', 'images/gamepad1.png',
                                                   'images/plant_side_selected.png', 'images/zombie_side_selected.png',
                                                   'images/help_menu_image_vs_controllers.png']),
              'text-only layer: %s' % made2)
        left = [os.path.relpath(os.path.join(dp, f), files) for dp, _, fs in os.walk(files) for f in fs]
        check(sorted(left) == sorted(made2), 'text-only left other files: %s' % sorted(set(left) - set(made2))[:5])
        ls2 = parse(open(os.path.join(files, 'properties/LawnStrings.txt'), encoding='utf-8').read())
        zh2 = [k for k in gz if CJK.search(ls2[k])]
        print('text only: LawnStrings still with Chinese: %d %s' % (len(zh2), zh2))
        # --- chinese: gone
        run('remove', game, eng)
        left = [f for _, _, fs in os.walk(files) for f in fs]
        check(not left and not os.path.exists(os.path.join(out, '.english')), 'remove left %s' % left[:5])

    if FAILS:
        sys.exit('%d FAILED' % len(FAILS))
    print('ALL OK')


if __name__ == '__main__':
    main()
