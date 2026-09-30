#!/usr/bin/env python3
"""Host test for the console-side setup code (source/dcr_exefs.h, dcr_formats.h).

Compiles the two headers natively and checks them against the Python tools
they replace on the console:

  1. exefs_build_override(pvz_nx.nsp, tid) is byte-identical to
     tools/make_exefs_override.py's exefs.nsp for the same title id;
  2. dex_names() over the APK's classes*.dex gives exactly the class names
     tools/stage_sd.py writes to classes.txt;
  3. nro_romfs_file() finds the launcher NRO's pvz_nx.nsp / pvz_nx.build and
     they are the files it was built from.

    python3 tools/test_setup.py <game.apk> [launcher/pvz_touch_nx.nro]
"""
import os, subprocess, sys, tempfile, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import make_exefs_override as mko  # noqa: E402
import stage_sd  # noqa: E402

HARNESS = r'''
#include "dcr_formats.h"
static unsigned char *slurp(const char *p, size_t *n) {
  FILE *f = fopen(p, "rb"); fseek(f, 0, SEEK_END); *n = (size_t)ftell(f); rewind(f);
  unsigned char *b = malloc(*n); if (fread(b, 1, *n, f) != *n) exit(2); fclose(f); return b;
}
int main(int argc, char **argv) {
  if (!strcmp(argv[1], "override")) {        /* override <nsp> <tid hex> <out> */
    size_t n, on; unsigned char *d = slurp(argv[2], &n), *o;
    if (exefs_build_override(d, n, strtoull(argv[3], NULL, 16), &o, &on)) return 1;
    FILE *f = fopen(argv[4], "wb"); fwrite(o, 1, on, f); fclose(f); return 0;
  }
  if (!strcmp(argv[1], "dex")) {             /* dex <file>... : names, one per line */
    Names ns = {0};
    for (int i = 2; i < argc; i++) { size_t n; unsigned char *d = slurp(argv[i], &n); dex_names(d, n, &ns); }
    for (int i = 0; i < ns.n; i++) puts(ns.v[i]);
    return 0;
  }
  if (!strcmp(argv[1], "nro")) {             /* nro <nro> <name> <out> */
    FILE *f = fopen(argv[2], "rb"); long off; size_t size;
    if (nro_romfs_file(f, argv[3], &off, &size)) return 1;
    unsigned char *b = malloc(size); fseek(f, off, SEEK_SET);
    if (fread(b, 1, size, f) != size) return 1;
    FILE *o = fopen(argv[4], "wb"); fwrite(b, 1, size, o); fclose(o);
    printf("%llu\n", (unsigned long long)nro_build(f));
    return 0;
  }
  return 1;
}
'''


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    apk = sys.argv[1]
    nro = sys.argv[2] if len(sys.argv) == 3 else os.path.join(TOP, 'launcher', 'pvz_touch_nx.nro')
    nsp = os.path.join(TOP, 'pvz_nx.nsp')
    with tempfile.TemporaryDirectory() as t:
        src = os.path.join(t, 'h.c')
        open(src, 'w').write(HARNESS)
        exe = os.path.join(t, 'h')
        subprocess.check_call(['cc', '-O1', '-Wall', '-I', os.path.join(TOP, 'source'), '-o', exe, src])

        # 1. the override
        tid = 0x0571D2F4CB1CF000
        out = os.path.join(t, 'exefs.nsp')
        subprocess.check_call([exe, 'override', nsp, '%016X' % tid, out])
        files = mko.read_pfs0(open(nsp, 'rb').read())
        files['main.npdm'] = mko.patch_npdm(files['main.npdm'], tid)[0]
        want = mko.write_pfs0(files)
        assert open(out, 'rb').read() == want, 'exefs.nsp differs from make_exefs_override.py'
        print('OK: override for %016X is byte-identical to make_exefs_override.py (%d bytes)' % (tid, len(want)))

        # 2. the class list
        dexes = []
        with zipfile.ZipFile(apk) as z:
            for n in z.namelist():
                if n.startswith('classes') and n.endswith('.dex') and '/' not in n:
                    p = os.path.join(t, n)
                    open(p, 'wb').write(z.read(n))
                    dexes.append(p)
        if dexes:
            got = sorted(set(subprocess.check_output([exe, 'dex'] + dexes).decode().split('\n')) - {''})
            want = stage_sd.class_names(apk)
            assert got == want, 'class names differ: %d vs %d' % (len(got), len(want))
            print('OK: %d Java class names from %d dex files, as stage_sd.py lists them' % (len(got), len(dexes)))
        else:
            print('(%s has no classes*.dex -- a repacked APK: class list not checked; any APK with dex '
                  'files can stand in)' % os.path.basename(apk))

        # 3. the NRO's payload
        if os.path.exists(nro):
            for name, ref in (('pvz_nx.nsp', nsp), ('pvz_nx.build', os.path.join(TOP, 'pvz_nx.build'))):
                p = os.path.join(t, name)
                b = subprocess.check_output([exe, 'nro', nro, name, p]).decode().strip()
                assert open(p, 'rb').read() == open(ref, 'rb').read(), name + ' in the NRO differs'
            assert b == open(os.path.join(TOP, 'pvz_nx.build')).read().strip(), 'build number'
            print('OK: %s carries pvz_nx.nsp and build %s' % (os.path.basename(nro), b))
        else:
            print('(no launcher NRO at %s: skipped)' % nro)


if __name__ == '__main__':
    main()
