/* pvz_apks.c -- the game folder: moved from its old name, and the APKs in it
 * told apart by what is in them.
 *
 * THE OLD FOLDER. Releases before the rename kept everything in
 * /switch/pvztouch. On the first start of this one, everything there -- the
 * APKs, config.ini, data/ (saves, the English layer), external/, the
 * unpacked libraries and their .setup stamps -- moves into the game folder,
 * file system renames only. The old launcher, PvZTouch.nro, stays behind, as
 * does anything the new folder already has (the new folder's copy wins; two
 * folders are merged entry by entry) and an APK the new folder already holds
 * a copy of (the same size). .moved in the game folder records that the move
 * was done, so it is done once.
 *
 * THE APKS. The user drops their APKs into the game folder under any name.
 * Both the game (PvZ TV Touch 1.1.5, the newest mod) and the English source
 * (the older mod's English build, PvZTouch 4.0.5, or RedStr1x's) are
 * com.trans.pvztv with the same engine, so names and packages cannot tell
 * them apart; their contents can: the English builds carry the mod's
 * translation pak, assets/paks/2.ChangeGameChina.zip, and the game does not.
 *   the English source  an APK with that pak
 *   the game            an APK with lib/armeabi-v7a/libGameMain.so and no pak
 * With several of one kind: the one named game.apk / english.apk, else the
 * newest. MIT.
 */
#include <dirent.h>
#include <miniz/miniz.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "pvz_apks.h"
#include "util.h"

#define OLD_ROOT "sdmc:" PVZ_OLD_ROOT_PATH

/* ------------------------------------------------------------ the old folder */
static struct {
  int looked;   /* the old folder was there and the move ran */
  int moved;    /* entries renamed into the game folder */
  int merged;   /* folders both had: their entries merged */
  int kept;     /* entries the game folder already had (left in the old folder) */
  int failed;   /* renames that failed */
  int nros;     /* old launchers left behind */
  char kept_names[160];
  char failed_names[160];
} M;

static int exists(const char *path, int *is_dir) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  if (is_dir)
    *is_dir = S_ISDIR(st.st_mode);
  return 1;
}

static long long size_of(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISREG(st.st_mode) ? (long long)st.st_size : -1;
}

static int ends_with(const char *s, const char *suffix) {
  size_t n = strlen(s), m = strlen(suffix);
  return n >= m && !strcasecmp(s + n - m, suffix);
}

static void note(char *list, size_t cap, const char *name) {
  size_t n = strlen(list);
  if (n + 3 >= cap)
    return;
  snprintf(list + n, cap - n, "%s%s", n ? ", " : "", name);
}

/* an APK of that size in the game folder already (the same file, copied anew) */
static int same_apk_here(const char *root, long long size) {
  if (size <= 0)
    return 0;
  DIR *d = opendir(root);
  if (!d)
    return 0;
  int found = 0;
  struct dirent *e;
  while (!found && (e = readdir(d))) {
    if (e->d_name[0] == '.' || !ends_with(e->d_name, ".apk"))
      continue;
    char p[512];
    snprintf(p, sizeof p, "%s/%s", root, e->d_name);
    found = size_of(p) == size;
  }
  closedir(d);
  return found;
}

static void move_tree(const char *from, const char *to, int top) {
  DIR *d = opendir(from);
  if (!d)
    return;
  /* names first: renaming while reading a directory can skip entries */
  char (*names)[256] = NULL;
  int n = 0, cap = 0;
  struct dirent *e;
  while ((e = readdir(d))) {
    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
      continue;
    if (n == cap) {
      cap = cap ? cap * 2 : 64;
      void *grown = realloc(names, (size_t)cap * sizeof *names);
      if (!grown)
        break;
      names = grown;
    }
    snprintf(names[n++], sizeof names[0], "%s", e->d_name);
  }
  closedir(d);
  for (int i = 0; i < n; i++) {
    const char *name = names[i];
    if (top && !strcasecmp(name, PVZ_OLD_LAUNCHER)) {
      M.nros++; /* the old launcher stays where its forwarder points */
      continue;
    }
    char src[512], dst[512];
    snprintf(src, sizeof src, "%s/%s", from, name);
    snprintf(dst, sizeof dst, "%s/%s", to, name);
    int src_dir = 0, dst_dir = 0;
    exists(src, &src_dir);
    if (top && !src_dir && ends_with(name, ".apk") && same_apk_here(to, size_of(src))) {
      M.kept++;
      note(M.kept_names, sizeof M.kept_names, name);
      continue;
    }
    if (exists(dst, &dst_dir)) {
      if (src_dir && dst_dir) {
        M.merged++;
        move_tree(src, dst, 0);
        rmdir(src); /* gone if it is empty now */
      } else {
        M.kept++;
        note(M.kept_names, sizeof M.kept_names, name);
      }
      continue;
    }
    if (rename(src, dst) == 0) {
      M.moved++;
    } else {
      M.failed++;
      note(M.failed_names, sizeof M.failed_names, name);
    }
  }
  free(names);
}

void pvz_old_folder_move(const char *theRoot) {
  char marker[512];
  snprintf(marker, sizeof marker, "%s/.moved", theRoot);
  int old_dir = 0;
  if (!exists(OLD_ROOT, &old_dir) || !old_dir || exists(marker, NULL))
    return;
  mkdir("sdmc:/switch", 0777);
  mkdir(theRoot, 0777);
  M.looked = 1;
  move_tree(OLD_ROOT, theRoot, 1);
  FILE *f = fopen(marker, "w");
  if (f) {
    fprintf(f, "moved from %s: %d moved, %d kept there, %d failed\n", PVZ_OLD_ROOT_PATH, M.moved, M.kept,
            M.failed);
    fclose(f);
  }
}

void pvz_old_folder_report(void) {
  if (!M.looked)
    return;
  debugPrintf("[setup] the game folder is now %s: moved %d item%s from %s (APKs, settings, saves, the "
              "unpacked game)%s\n",
              PVZ_ROOT_PATH, M.moved, M.moved == 1 ? "" : "s", PVZ_OLD_ROOT_PATH,
              M.merged ? " -- folders both had were merged" : "");
  if (M.nros)
    debugPrintf("[setup]   left there: %s, the old launcher\n", PVZ_OLD_LAUNCHER);
  if (M.kept)
    debugPrintf("[setup]   left there, this folder has them already: %s\n", M.kept_names);
  if (M.failed)
    debugPrintf("[setup]   could NOT be moved (copy them over by hand): %s\n", M.failed_names);
  debugPrintf("[setup]   %s can be deleted once this runs well\n", PVZ_OLD_ROOT_PATH);
}

/* ------------------------------------------------------------------ the APKs */
enum { APK_OTHER, APK_GAME, APK_ENGLISH, APK_UNREADABLE };
static const char *const k_role[] = {"not the game", "the game", "the English source", "not a readable zip"};

static char g_game[512], g_english[512], g_summary[400];
static int g_have_english;

static int classify(const char *path) {
  mz_zip_archive zip;
  memset(&zip, 0, sizeof zip);
  if (!mz_zip_reader_init_file(&zip, path, 0))
    return APK_UNREADABLE;
  const int pak = mz_zip_reader_locate_file(&zip, "assets/paks/2.ChangeGameChina.zip", NULL, 0) >= 0;
  const int engine = mz_zip_reader_locate_file(&zip, "lib/armeabi-v7a/" PVZ_LIB_GAME, NULL, 0) >= 0;
  mz_zip_reader_end(&zip);
  return pak ? APK_ENGLISH : engine ? APK_GAME : APK_OTHER;
}

int pvz_apks_find(const char *theRoot) {
  snprintf(g_game, sizeof g_game, "%s/%s", theRoot, DCR_APK_NAME);
  snprintf(g_english, sizeof g_english, "%s/%s", theRoot, PVZ_ENGLISH_APK);
  g_summary[0] = 0;
  DIR *d = opendir(theRoot);
  if (!d)
    return -1;
  int have_game = 0, have_english = 0;
  int game_named = 0, english_named = 0; /* the chosen one has the preferred name */
  time_t game_time = 0, english_time = 0;
  struct dirent *e;
  while ((e = readdir(d))) {
    /* "._x.apk": the resource forks macOS leaves on FAT cards */
    if (e->d_name[0] == '.' || !ends_with(e->d_name, ".apk"))
      continue;
    char path[512];
    snprintf(path, sizeof path, "%s/%s", theRoot, e->d_name);
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0)
      continue;
    const int role = classify(path);
    debugPrintf("[apk] %s: %s (%lld MB)\n", e->d_name, k_role[role], (long long)st.st_size >> 20);
    size_t n = strlen(g_summary);
    if (n + 8 < sizeof g_summary)
      snprintf(g_summary + n, sizeof g_summary - n, "%s%s (%s)", n ? ", " : "", e->d_name, k_role[role]);
    if (role == APK_GAME) {
      const int named = !strcasecmp(e->d_name, DCR_APK_NAME);
      if (!have_game || (named && !game_named) || (named == game_named && st.st_mtime > game_time)) {
        snprintf(g_game, sizeof g_game, "%s", path);
        game_time = st.st_mtime;
        game_named = named;
      }
      have_game = 1;
    } else if (role == APK_ENGLISH) {
      const int named = !strcasecmp(e->d_name, PVZ_ENGLISH_APK);
      if (!have_english || (named && !english_named) || (named == english_named && st.st_mtime > english_time)) {
        snprintf(g_english, sizeof g_english, "%s", path);
        english_time = st.st_mtime;
        english_named = named;
      }
      have_english = 1;
    }
  }
  closedir(d);
  g_have_english = have_english;
  if (have_game)
    debugPrintf("[apk] the game: %s\n", strrchr(g_game, '/') + 1);
  if (have_english)
    debugPrintf("[apk] the English source: %s\n", strrchr(g_english, '/') + 1);
  return have_game ? 0 : -1;
}

const char *dcr_game_root(void); /* main.c */

const char *pvz_game_apk(void) {
  if (!g_game[0]) /* asked before pvz_apks_find */
    snprintf(g_game, sizeof g_game, "%s/%s", dcr_game_root(), DCR_APK_NAME);
  return g_game;
}
const char *pvz_english_apk(void) {
  if (!g_english[0])
    snprintf(g_english, sizeof g_english, "%s/%s", dcr_game_root(), PVZ_ENGLISH_APK);
  return g_english;
}
int pvz_apks_have_english(void) { return g_have_english; }
const char *pvz_apks_summary(void) { return g_summary[0] ? g_summary : "no APK at all"; }
