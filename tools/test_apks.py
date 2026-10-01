#!/usr/bin/env python3
"""Host test for the runtime's rt_migrate.c and rt_apkfind.c with this port's
settings (source/port_config.h: PORT_OLD_ROOT_PATHS, RT_MIGRATE_ONCE_MARKER,
PORT_APK_ROLES): the old folder's move and the APKs told apart by their
contents.

    python3 tools/test_apks.py <game.apk> <english.apk> [other game.apk] [not-the-game.apk]

<game.apk>: PvZ TV Touch 1.1.5; <english.apk>: the English 4.0.5 (or RedStr1x)
APK; the optional ones: another 1.1.5 build, and any APK that is not the game.
The APKs are hard-linked (not copied) into a scratch SD card under odd names:

  1. the move: /switch/pvztouch (old launcher, APKs, config.ini, .setup, a
     library, saves) into /switch/pvz_touch_nx (which already has the new
     launcher, the same game APK under its release name, and a data/ folder):
     everything moves but PvZTouch.nro and the game APK the new folder
     already holds; folders both have are merged; .moved stops a second move;
  2. the APKs: the game and the English source found whatever their names
     (game.apk / english.apk win a tie), macOS "._" files and other APKs
     ignored, and no game APK reported as such.
"""
import os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(HERE)
SRC = os.path.join(TOP, 'source')
RT = os.path.join(TOP, 'runtime', 'source')

HARNESS = r'''
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "rt_settings.h"
#include "rt_apkfind.h"
#include "rt_migrate.h"
static const RtApkRole roles[] = {PORT_APK_ROLES};
static void logf(const char *fmt, ...) { va_list a; va_start(a, fmt); vprintf(fmt, a); va_end(a); }
int main(int argc, char **argv) {  /* move|find <root> */
  (void)argc;
  if (!strcmp(argv[1], "move")) {
    RtMigrateResult r;
    memset(&r, 0, sizeof r);
    rt_migrate_port(argv[2], 1, NULL, &r);
    if (r.msg[0]) printf("%s\n", r.msg);
    return 0;
  }
  RtApkEnv env = {NULL, logf, 1};
  RtApkFound f;
  int rc = rt_apk_find(argv[2], roles, (int)(sizeof roles / sizeof roles[0]), &env, &f);
  printf("RC=%d\nGAME=%s\nENGLISH=%s\nSUMMARY=%s\n", rc, f.path[0], f.path[1], f.summary);
  return 0;
}
'''

failures = []


def check(cond, what):
    print(('OK: ' if cond else 'FAIL: ') + what)
    if not cond:
        failures.append(what)


def link(src, dst):
    try:
        os.link(src, dst)
    except OSError:
        os.symlink(os.path.abspath(src), dst)


def put(path, data=b'x'):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'wb').write(data)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    game, english = sys.argv[1], sys.argv[2]
    other_game = sys.argv[3] if len(sys.argv) > 3 else None
    not_game = sys.argv[4] if len(sys.argv) > 4 else None
    with tempfile.TemporaryDirectory() as t:
        exe = os.path.join(t, 'h')
        open(os.path.join(t, 'h.c'), 'w').write(HARNESS)
        subprocess.check_call(['cc', '-O1', '-g', '-Wall', '-Wextra', '-Wno-unused-parameter',
                               '-fsanitize=address,undefined', '-DPORT_PAYLOAD_NAME="pvz_nx"',
                               '-I', SRC, '-I', RT, os.path.join(t, 'h.c'),
                               os.path.join(RT, 'rt_apkfind.c'), os.path.join(RT, 'rt_migrate.c'), '-o', exe])
        card = os.path.join(t, 'card')  # the harness runs here: "sdmc:/..." is card/sdmc:/...
        old = os.path.join(card, 'sdmc:', 'switch', 'pvztouch')
        new = os.path.join(card, 'sdmc:', 'switch', 'pvz_touch_nx')
        root = 'sdmc:/switch/pvz_touch_nx'
        run = lambda *a: subprocess.run([exe, *a], cwd=card, capture_output=True, text=True, check=True).stdout

        # --- 1. the move
        put(os.path.join(old, 'PvZTouch.nro'), b'old launcher')
        link(game, os.path.join(old, 'game.apk'))
        link(english, os.path.join(old, 'english.apk'))
        put(os.path.join(old, 'config.ini'), b'[game]\nlanguage = english\n')
        put(os.path.join(old, '.setup'), b'libGameMain.so 00000000 1\n')
        put(os.path.join(old, 'libGameMain.so'), b'lib')
        put(os.path.join(old, 'data', 'files', 'userdata', 'user1.dat'), b'save')
        put(os.path.join(old, 'data', 'files', 'modmenu.txt'), b'menu')
        put(os.path.join(old, 'data', 'files', 'cached', 'x.cfu2'), b'old cache')
        put(os.path.join(new, 'pvz_touch_nx.nro'), b'new launcher')
        link(game, os.path.join(new, os.path.basename(game)))
        put(os.path.join(new, 'data', 'files', 'cached', 'x.cfu2'), b'new cache')
        log = run('move', root)
        sys.stdout.write(log)
        n = lambda *p: os.path.join(new, *p)
        o = lambda *p: os.path.join(old, *p)
        for rel in ('config.ini', '.setup', 'libGameMain.so', 'english.apk',
                    'data/files/userdata/user1.dat', 'data/files/modmenu.txt'):
            check(os.path.exists(n(rel)) and not os.path.exists(o(rel)), 'moved: ' + rel)
        check(os.path.exists(o('PvZTouch.nro')) and not os.path.exists(n('PvZTouch.nro')), 'the old launcher stays')
        check(os.path.exists(o('game.apk')) and not os.path.exists(n('game.apk')),
              'the game APK the new folder already has stays in the old folder')
        check(open(n('data/files/cached/x.cfu2'), 'rb').read() == b'new cache', 'merged: the new folder\'s file wins')
        check(os.path.exists(n('.moved')), '.moved written')
        check('moved' in log.lower() and 'pvztouch' in log, 'the move is logged')
        put(o('late.txt'))
        log2 = run('move', root)
        check(log2 == '' and os.path.exists(o('late.txt')), 'a second start moves nothing')

        # --- 2. the APKs
        os.rename(n(os.path.basename(game)), n('Plants vs Zombies 1.1.5 (my copy).apk'))
        put(n('._Plants vs Zombies 1.1.5 (my copy).apk'), b'\0\5\26\7AppleDouble')
        out = run('find', root)
        sys.stdout.write(out)
        check('GAME=' + root + '/Plants vs Zombies 1.1.5 (my copy).apk\n' in out, 'the game found under any name')
        check('ENGLISH=' + root + '/english.apk\n' in out, 'the English source found')
        check('GAME=' + root + '/english.apk' not in out, 'the English APK is not taken for the game')
        check('._Plants' not in out, 'macOS resource forks ignored')
        os.rename(n('english.apk'), n('PvZTouch 4.0.5 [28-08-24].apk'))
        out = run('find', root)
        check('ENGLISH=' + root + '/PvZTouch 4.0.5 [28-08-24].apk\n' in out, 'the English source under any name')
        if other_game:
            link(other_game, n('game.apk'))
            out = run('find', root)
            check('GAME=' + root + '/game.apk\n' in out, 'two games: the one named game.apk')
            os.unlink(n('game.apk'))
        if not_game:
            link(not_game, n('something else.apk'))
            out = run('find', root)
            check('something else.apk (not the game)' in out and '/Plants vs Zombies 1.1.5 (my copy).apk' in out,
                  'an APK that is not the game is passed over')
            os.unlink(n('something else.apk'))
        os.unlink(n('Plants vs Zombies 1.1.5 (my copy).apk'))
        out = run('find', root)
        sys.stdout.write(out)
        check('RC=-1' in out and '(the English source)' in out, 'no game APK: reported, with what is there')
    if failures:
        sys.exit('%d FAILED' % len(failures))
    print('ALL OK')


if __name__ == '__main__':
    main()
