#!/usr/bin/env python3
"""make_english_pack.py -- the English APK cut down to what the port reads.

    python3 tools/make_english_pack.py <game.apk> <english.apk> [-o "PvZ Touch English.apk"]

The English layer (source/pvz_english.c) reads only some of the English APK
(PvZTouch 4.0.5 or RedStr1x): its translation pak
(assets/paks/2.ChangeGameChina.zip), the files that pak says the translators
changed, the menu atlas the signs are cut from, the fonts and the string
tables. This runs the layer on the host against the full APK, with the zip
reader recording every entry it extracts (tools/host/miniz_host.c,
MINIZ_TRACE), and writes those entries, unchanged, to a new zip. The console
finds it as the English source whatever it is called, as long as the name
ends in .apk: it carries the translation pak (PORT_APK_ROLES, source/port_config.h).

Then it proves the pack is enough: the layer is made again from the pack and
must be byte-identical to the one made from the full APK (the layer list's
first line, a key over the APKs' contents, differs, as it should).

The pack holds the English build's pictures and text: it is as much the
game's as the APK it came from. Keep it to yourself.
"""
import argparse, filecmp, os, subprocess, sys, tempfile, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import test_english  # noqa: E402  (its harness and paths)


def build_harness(t):
    exe = os.path.join(t, 'h')
    open(os.path.join(t, 'h.c'), 'w').write(test_english.HARNESS)
    subprocess.check_call(['cc', '-O2', '-w', '-I', os.path.join(HERE, 'host'), '-I', test_english.SRC,
                           os.path.join(t, 'h.c'), os.path.join(test_english.SRC, 'pvz_english.c'),
                           os.path.join(test_english.SRC, 'pvz_strings.c'),
                           os.path.join(HERE, 'host', 'miniz_host.c'), '-lz', '-o', exe])
    return exe


def make_layer(exe, game, eng, out, trace=None):
    env = dict(os.environ)
    if trace:
        env['MINIZ_TRACE'] = trace
    os.makedirs(out)
    r = subprocess.run([exe, 'apply', game, eng, out, test_english.RES], capture_output=True, text=True, env=env)
    if r.returncode != 0:
        sys.exit('the English layer failed (%d):\n%s%s' % (r.returncode, r.stdout, r.stderr))
    return r.stdout


def tree(root):
    out = set()
    for d, _, fs in os.walk(root):
        for f in fs:
            out.add(os.path.relpath(os.path.join(d, f), root))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('game', help='PvZ TV Touch 1.1.5 APK')
    ap.add_argument('english', help='the English APK (PvZTouch 4.0.5 or RedStr1x)')
    ap.add_argument('-o', '--out', default='PvZ Touch English.apk')
    a = ap.parse_args()
    if not a.out.lower().endswith('.apk'):
        sys.exit('the name must end in .apk: the port looks for its APKs by that')
    eng = os.path.abspath(a.english)
    with tempfile.TemporaryDirectory() as t:
        exe = build_harness(t)
        trace = os.path.join(t, 'trace.txt')
        make_layer(exe, os.path.abspath(a.game), eng, os.path.join(t, 'full'), trace)
        read = []
        for line in open(trace, encoding='utf-8', errors='surrogateescape'):
            path, _, name = line.rstrip('\n').partition('\t')
            if path == eng and name not in read:
                read.append(name)
        src = zipfile.ZipFile(eng)
        with zipfile.ZipFile(a.out, 'w') as dst:
            for name in sorted(read):
                info = src.getinfo(name)
                dst.writestr(info, src.read(info), compress_type=info.compress_type)
        make_layer(exe, os.path.abspath(a.game), os.path.abspath(a.out), os.path.join(t, 'pack'))

        # the same layer from the pack as from the whole APK
        full, pack = os.path.join(t, 'full'), os.path.join(t, 'pack')
        tf, tp = tree(full), tree(pack)
        bad = sorted(tf ^ tp)
        for rel in sorted(tf & tp):
            if rel == '.english':
                if open(os.path.join(full, rel)).read().split('\n')[1:] != open(os.path.join(pack, rel)).read().split('\n')[1:]:
                    bad.append(rel + ' (its file list)')
            elif not filecmp.cmp(os.path.join(full, rel), os.path.join(pack, rel), shallow=False):
                bad.append(rel)
        if bad:
            os.unlink(a.out)
            sys.exit('the pack makes a different layer: %s' % ', '.join(bad[:10]))
        size = sum(src.getinfo(n).file_size for n in read)
        print('%s: %d of the English APK\'s %d entries (%.1f MB unpacked), %.1f MB; the layer it makes is '
              'byte-identical (%d files)' % (a.out, len(read), len(src.infolist()), size / 1e6,
                                            os.path.getsize(a.out) / 1e6, len(tf)))


if __name__ == '__main__':
    main()
