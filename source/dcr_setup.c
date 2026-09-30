/* dcr_setup.c -- a first launch from nothing but game.apk.
 *
 * The game folder (/switch/pvz_touch_nx) needs only the user's own APK, under
 * any name (pvz_apks.c tells it by its contents; "game.apk" below means that
 * file), and the launcher NRO (launcher/). Everything else is made from the
 * APK here, before anything is loaded:
 *   libnative_code.so libGameMain.so libHomura.so   lib/armeabi-v7a/ out of game.apk
 *   classes.txt        the Java class names its classes*.dex define
 *                      (jni_core.c answers FindClass with exactly those)
 *   data/files/...     the English layer (pvz_english.c), when config.ini
 *                      [game] language is english: the text, and with
 *                      the English APK (the user's English build of the mod) the
 *                      pictures, menu signs and fonts too
 *   data/files/properties/levels.xml   the chosen adventure difficulty, when
 *                      [game] original_adventure_difficulty is false
 * and made again whenever game.apk changes: .setup records the CRC-32 of each
 * source entry. Files that are already right (copied by tools/stage_sd.py,
 * say) are checked once and kept. The game reads its data straight out of the
 * APK, compressed or not (zziplib), so the APK itself is left as it is.
 *
 * UPDATES FROM THE NRO. This program runs as a forwarder title through an
 * ExeFS override, /atmosphere/contents/<title id>/exefs.nsp, which the
 * launcher wrote on its first run (it carries pvz_nx.nsp in its romfs). When
 * the NRO in the game folder carries a NEWER build than the one running
 * (romfs:/pvz_nx.build against DCR_BUILD), the override is rewritten from it
 * (dcr_exefs.h) and the program restarts into the new build: updating is
 * copying the new NRO over the old one. Never a downgrade, never a file this
 * program did not come from (the override must exist and name this title),
 * and never twice for the same build (.update records the attempt). MIT.
 */
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <switch.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz/miniz.h>

#include "config.h"
#include "pvz_apks.h"
#include "dcr_build.h"
#include "dcr_config.h"
#include "pvz_english.h"
#include "dcr_exefs.h"
#include "dcr_formats.h"
#include "error.h"
#include "util.h"

const char *dcr_game_root(void); /* main.c */

static const char *const k_libs[] = {PVZ_LIB_NATIVE, PVZ_LIB_GAME, PVZ_LIB_HOMURA};
#define ABI_DIR "lib/armeabi-v7a/"

static void root_path(char *out, size_t cap, const char *name) {
  snprintf(out, cap, "%s/%s", dcr_game_root(), name);
}

/* Setup work shows on screen as a progress bar with what is being done (util.c
 * log_console_progress); the log it writes goes to debug.log only -- or
 * scrolls on screen instead with [debug] boot_log_on_screen. */
static int g_setup_shown;
static void setup_progress(const char *what, int permille) {
  if (!g_setup_shown) {
    g_setup_shown = 1;
    debugPrintf("[setup] setting up Plants vs. Zombies Touch -- this happens once\n");
  }
  log_console_progress(what, permille);
  log_console_update(); /* the log on screen, if it is */
}

static long file_size(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISREG(st.st_mode) ? (long)st.st_size : -1;
}

/* ---------------------------------------------------------------- .setup */
/* One line per made file: "<name> <crc of its source> <size>". */
#define MAX_STAMP 8
typedef struct {
  char name[32];
  unsigned long crc, size;
} Stamp;
static Stamp g_stamp[MAX_STAMP];
static int g_nstamp, g_stamp_dirty;

static void stamp_load(void) {
  char path[300];
  root_path(path, sizeof path, ".setup");
  FILE *f = fopen(path, "r");
  if (!f)
    return;
  while (g_nstamp < MAX_STAMP && fscanf(f, "%31s %lx %lu", g_stamp[g_nstamp].name,
                                        &g_stamp[g_nstamp].crc, &g_stamp[g_nstamp].size) == 3)
    g_nstamp++;
  fclose(f);
}

static Stamp *stamp_get(const char *name) {
  for (int i = 0; i < g_nstamp; i++)
    if (!strcmp(g_stamp[i].name, name))
      return &g_stamp[i];
  return NULL;
}

static void stamp_set(const char *name, unsigned long crc, unsigned long size) {
  Stamp *s = stamp_get(name);
  if (!s && g_nstamp < MAX_STAMP) {
    s = &g_stamp[g_nstamp++];
    snprintf(s->name, sizeof s->name, "%s", name);
  }
  if (s && (s->crc != crc || s->size != size)) {
    s->crc = crc;
    s->size = size;
    g_stamp_dirty = 1;
  }
}

static void stamp_save(void) {
  if (!g_stamp_dirty)
    return;
  char path[300];
  root_path(path, sizeof path, ".setup");
  FILE *f = fopen(path, "w");
  if (!f)
    return;
  for (int i = 0; i < g_nstamp; i++)
    fprintf(f, "%s %08lx %lu\n", g_stamp[i].name, g_stamp[i].crc, g_stamp[i].size);
  fclose(f);
}

/* ------------------------------------------------------------- libraries */
static unsigned long crc_of_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return 0;
  static unsigned char buf[1 << 16];
  mz_ulong crc = mz_crc32(0, NULL, 0);
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0)
    crc = mz_crc32(crc, buf, n);
  fclose(f);
  return (unsigned long)crc;
}

/* Write to <dst>.part, then put it in place: a half-written library is never
 * mistaken for a whole one. */
static int extract_entry(mz_zip_archive *zip, int idx, const char *dst) {
  char tmp[320];
  snprintf(tmp, sizeof tmp, "%s.part", dst);
  unlink(tmp);
  if (!mz_zip_reader_extract_to_file(zip, (mz_uint)idx, tmp, 0)) {
    unlink(tmp);
    return -1;
  }
  unlink(dst);
  return rename(tmp, dst);
}

/* THE MOD. The wrapper carries its own build of the mod (mod/, pvz_res.S):
 * the source of the 1.1.5-260925 mod (nightly e91bc9a, online protocol
 * 3199; built as is, byte for byte the APK's code and data), with this
 * port's changes for the Switch's controls. It replaces game.apk's
 * libHomura.so only when that is one of the 1.1.5 builds below, whose game
 * libraries are the same (the 260924 APK differs only in its mod and the
 * online protocol it speaks, 3198); another APK keeps its own.
 * config.ini [debug] mod_from_apk = true: always game.apk's. */
static const unsigned long k_homura_crcs[] = {
    0xbcd6795eu, /* 1.1.5-260925 (the source's own build) */
    0xf5d2a357u, /* 1.1.5-260924 */
};
static int write_atomic(const char *dst, const void *buf, size_t len);
extern const uint8_t pvz_res_homura[];
extern const uint32_t pvz_res_homura_size;

static int ensure_port_mod(unsigned long apk_crc) {
  char dst[300];
  root_path(dst, sizeof dst, PVZ_LIB_HOMURA);
  int known = 0;
  for (unsigned i = 0; i < sizeof k_homura_crcs / sizeof k_homura_crcs[0]; i++)
    known |= apk_crc == k_homura_crcs[i];
  if (dcr_config()->mod_from_apk || !known || !pvz_res_homura_size)
    return 0;
  const unsigned long crc = mz_crc32(0, pvz_res_homura, pvz_res_homura_size);
  Stamp *s = stamp_get(PVZ_LIB_HOMURA);
  if (s && s->crc == crc && file_size(dst) == (long)pvz_res_homura_size)
    return 1;
  setup_progress("Installing the mod", 150);
  debugPrintf("[setup] the mod: this port's build of the 1.1.5 mod's source, with the Switch "
              "controls (%u KB)\n", (unsigned)(pvz_res_homura_size >> 10));
  if (!write_atomic(dst, pvz_res_homura, pvz_res_homura_size))
    fatal_error("Could not write %s.\n\nIs the SD card full or read-only?", dst);
  stamp_set(PVZ_LIB_HOMURA, crc, pvz_res_homura_size);
  return 1;
}

static void ensure_libs(mz_zip_archive *zip, const char *apk) {
  for (unsigned i = 0; i < sizeof k_libs / sizeof k_libs[0]; i++) {
    char arc[64], dst[300];
    snprintf(arc, sizeof arc, ABI_DIR "%s", k_libs[i]);
    root_path(dst, sizeof dst, k_libs[i]);
    int idx = mz_zip_reader_locate_file(zip, arc, NULL, 0);
    mz_zip_archive_file_stat st;
    if (idx < 0 || !mz_zip_reader_file_stat(zip, (mz_uint)idx, &st))
      fatal_error("%s has no %s.\n\n"
                  "This port needs PvZ TV Touch 1.1.5 (com.trans.pvztv, armeabi-v7a):\n"
                  "put the APK of that version in the game folder (any file name).",
                  apk, arc);
    unsigned long crc = (unsigned long)st.m_crc32, size = (unsigned long)st.m_uncomp_size;
    if (!strcmp(k_libs[i], PVZ_LIB_HOMURA) && ensure_port_mod(crc))
      continue;
    long have = file_size(dst);
    Stamp *s = stamp_get(k_libs[i]);
    if (have == (long)size && s && s->crc == crc)
      continue; /* made from this APK before */
    if (have == (long)size && crc_of_file(dst) == crc) {
      stamp_set(k_libs[i], crc, size); /* already the right file */
      continue;
    }
    setup_progress("Unpacking the game", 20 + (int)i * 40);
    debugPrintf("[setup] unpacking %s from the game APK (%lu KB)...\n", k_libs[i], size >> 10);
    if (extract_entry(zip, idx, dst) != 0)
      fatal_error("Could not write %s (from %s).\n\nIs the SD card full or read-only?", dst, apk);
    stamp_set(k_libs[i], crc, size);
  }
}

/* ------------------------------------------------------------ classes.txt */
static int cmp_str(const void *a, const void *b) {
  return strcmp(*(char *const *)a, *(char *const *)b);
}

static void ensure_classes(mz_zip_archive *zip) {
  /* classes.dex, classes2.dex, ... at the top of the APK */
  int idx[32], n = 0;
  mz_ulong crc = mz_crc32(0, NULL, 0);
  for (int k = 1; k <= 32 && n < 32; k++) {
    char nm[32];
    if (k == 1)
      snprintf(nm, sizeof nm, "classes.dex");
    else
      snprintf(nm, sizeof nm, "classes%d.dex", k);
    int i = mz_zip_reader_locate_file(zip, nm, NULL, 0);
    mz_zip_archive_file_stat st;
    if (i < 0 || !mz_zip_reader_file_stat(zip, (mz_uint)i, &st))
      break;
    idx[n++] = i;
    uint32_t c = st.m_crc32;
    crc = mz_crc32(crc, (const unsigned char *)&c, sizeof c);
  }
  char dst[300];
  root_path(dst, sizeof dst, "classes.txt");
  Stamp *s = stamp_get("classes.txt");
  if (!n || (s && s->crc == (unsigned long)crc && s->size == (unsigned long)n && file_size(dst) > 0))
    return; /* nothing to read, or already made from these */

  setup_progress("Reading the game's Java classes", 170);
  debugPrintf("[setup] listing the Java classes of the game APK (%d dex file%s)...\n", n, n > 1 ? "s" : "");
  Names ns = {0};
  for (int k = 0; k < n; k++) {
    size_t len = 0;
    void *d = mz_zip_reader_extract_to_heap(zip, (mz_uint)idx[k], &len, 0);
    if (d) {
      dex_names(d, len, &ns);
      free(d);
    }
  }
  if (ns.n) {
    qsort(ns.v, (size_t)ns.n, sizeof *ns.v, cmp_str);
    char tmp[320];
    snprintf(tmp, sizeof tmp, "%s.part", dst);
    FILE *f = fopen(tmp, "w");
    int written = 0;
    if (f) {
      fputs("# Java classes defined by the game's APK (names only). The wrapper's JNI\n"
            "# FindClass/Class.forName report exactly these, plus the Android framework.\n", f);
      for (int i = 0; i < ns.n; i++)
        if (i == 0 || strcmp(ns.v[i], ns.v[i - 1])) {
          fputs(ns.v[i], f);
          fputc('\n', f);
          written++;
        }
      if (fclose(f) == 0) {
        unlink(dst);
        if (rename(tmp, dst) == 0) {
          stamp_set("classes.txt", (unsigned long)crc, (unsigned long)n);
          debugPrintf("[setup] classes.txt: %d Java class names\n", written);
        }
      }
    }
  }
  for (int i = 0; i < ns.n; i++)
    free(ns.v[i]);
  free(ns.v);
}

/* -------------------------------------------- files for the user files dir */
static void mkdirs_for(const char *file) {
  char p[320];
  snprintf(p, sizeof p, "%s", file);
  for (char *q = p + 6; *q; q++) /* past "sdmc:/" */
    if (*q == '/') {
      *q = 0;
      mkdir(p, 0777);
      *q = '/';
    }
}

/* dst replaced whole or not at all (written to dst.part, then renamed). */
static int write_atomic(const char *dst, const void *buf, size_t len) {
  mkdirs_for(dst);
  char tmp[330];
  snprintf(tmp, sizeof tmp, "%s.part", dst);
  FILE *f = fopen(tmp, "wb");
  int ok = f && fwrite(buf, 1, len, f) == len;
  if (f && fclose(f) != 0)
    ok = 0;
  if (ok) {
    unlink(dst);
    ok = rename(tmp, dst) == 0;
  }
  if (!ok)
    unlink(tmp);
  return ok;
}

/* ------------------------------------------------------ English layer */
extern const char pvz_res_addon_en[], pvz_res_lawn_en[], pvz_res_lawn_fix[]; /* pvz_res.S */
extern const unsigned char pvz_res_button_icons[], pvz_res_help_buttons[], pvz_res_help_buttons_small[];
extern const uint32_t pvz_res_button_icons_size, pvz_res_help_buttons_size, pvz_res_help_buttons_small_size;

static void setup_log(const char *fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  debugPrintf("%s", buf);
  log_console_update();
}

/* A font of the layer the engine would not load (pvz_boot.c, at the
 * engine's close): this build's layer is made again without the button
 * pictures (their layer is what the English layer adds to fonts), so the
 * game starts; a newer build tries them again. */
#define PVZ_FONTS_FAILED ".fonts_failed"
void dcr_setup_font_failed(const char *why) {
  char path[320];
  root_path(path, sizeof path, PVZ_FONTS_FAILED);
  FILE *f = fopen(path, "w");
  if (!f)
    return;
  fprintf(f, "%llu %.300s\n", (unsigned long long)DCR_BUILD, why);
  fclose(f);
  debugPrintf("[quit] the next start makes the fonts again without the button pictures\n");
}

static int fonts_failed(void) {
  char path[320];
  root_path(path, sizeof path, PVZ_FONTS_FAILED);
  FILE *f = fopen(path, "r");
  if (!f)
    return 0;
  unsigned long long build = 0;
  const int got = fscanf(f, "%llu", &build) == 1;
  fclose(f);
  if (got && build == DCR_BUILD) {
    debugPrintf("[setup] a font would not load last time: the fonts go without the button pictures\n");
    return 1;
  }
  unlink(path); /* an older build's */
  return 0;
}

static void english_working(void) { setup_progress("English pictures", 200); }
static void english_progress(int permille, const char *what) {
  setup_progress(what, 200 + permille * 750 / 1000);
}

static void ensure_english(mz_zip_archive *zip) {
  char files[300], ext[300], list[300], eng[300];
  root_path(files, sizeof files, "data/files");
  root_path(ext, sizeof ext, "external/files");
  root_path(list, sizeof list, ".english");
  snprintf(eng, sizeof eng, "%s", pvz_english_apk()); /* whatever its name (pvz_apks.c) */
  PvzEnglishCfg c = {
      .files_dir = files,
      .layer_list = list,
      .english_apk = eng,
      .app_dirs = {files, ext},
      .res = {pvz_res_addon_en, pvz_res_lawn_en, pvz_res_lawn_fix, pvz_res_button_icons,
              pvz_res_help_buttons, pvz_res_help_buttons_small, pvz_res_button_icons_size,
              pvz_res_help_buttons_size, pvz_res_help_buttons_small_size},
      .log = setup_log,
      .working = english_working,
      .progress = english_progress,
      .no_button_pictures = !dcr_config()->button_pictures || fonts_failed(),
  };
  /* builds before the layer made only LawnStrings.txt (stamp "LawnStrings") */
  Stamp *old = stamp_get("LawnStrings");
  if (old && old->crc) {
    char dst[320];
    root_path(dst, sizeof dst, "data/files/properties/LawnStrings.txt");
    if (!dcr_config()->english)
      unlink(dst);
    stamp_set("LawnStrings", 0, 0);
  }
  if (!dcr_config()->english) {
    pvz_english_remove(&c);
    return;
  }
  pvz_english_apply(zip, &c);
}

/* ------------------------------------------------------ setup.env */
/* The engine's own options file (Sexy::GetEnv reads "setup.env" through its
 * file system: the files dir first). LAWN_GAMEPAD_MODE=1 (GamepadApp
 * +0x895, what HasGamepad() needs) is the Xbox 360 edition's control scheme:
 * buttons reach the board as gamepad buttons (GameButtonDown: L/R cycle the
 * seed packets, A plants, B digs, X butter, START pauses) and hints show
 * button pictures. Without it, the TV edition's remote scheme: A enters a seed
 * picker, and <A> in hints is spelled out. No APK ships the file. */
#define PVZ_SETUP_ENV "data/files/setup.env"
static uint8_t *read_whole(const char *path, size_t *len);
int b_setenv(const char *name, const char *value, int overwrite); /* bionic_core.c */
#define SETUP_ENV_MARK "# written by the Switch wrapper"

/* The engine's Xbox 360 gamepad mode: GamepadApp takes it from
 * Sexy::GetEnvOption("LAWN_GAMEPAD_MODE"). Its options file, setup.env, is read
 * through the pak file system, which does not look in the files folder, but a
 * name the file lacks falls back to getenv() -- so it goes in the environment.
 * (Build 202609250408 wrote a setup.env the engine never opened: removed.) */
static void ensure_gamepad_mode(void) {
  char dst[320];
  root_path(dst, sizeof dst, PVZ_SETUP_ENV);
  size_t len = 0;
  uint8_t *have = read_whole(dst, &len);
  if (have && len >= sizeof SETUP_ENV_MARK - 1 && !memcmp(have, SETUP_ENV_MARK, sizeof SETUP_ENV_MARK - 1))
    unlink(dst);
  free(have);
  if (dcr_config()->xbox_scheme) {
    b_setenv("LAWN_GAMEPAD_MODE", "1", 1);
    debugPrintf("[setup] controls: the Xbox 360 scheme (LAWN_GAMEPAD_MODE=1)\n");
  } else {
    debugPrintf("[setup] controls: the TV remote scheme\n");
  }
}

/* ------------------------------------------------------------ the logo */
/* The Plants vs. Zombies Touch logo (logo.png at the port's top, sized by
 * tools/make_logo.py, built in: pvz_res.S) over the game's, on the main menu
 * and the title screen: the engine reads the files dir before game.apk. What
 * was there (the English layer's picture) is kept as <name>.orig and put
 * back with [game] touch_logo off. After the English layer, which writes the
 * same names when it is made. */
extern const uint8_t pvz_res_logo_menu[], pvz_res_logo_title[];
extern const uint32_t pvz_res_logo_menu_size, pvz_res_logo_title_size;

static int put_logo(const char *rel, const uint8_t *png, size_t len, int on) {
  char dst[320], orig[330];
  root_path(dst, sizeof dst, rel);
  snprintf(orig, sizeof orig, "%s.orig", dst);
  size_t have_len = 0;
  uint8_t *have = read_whole(dst, &have_len);
  const int present = have != NULL, ours = present && have_len == len && !memcmp(have, png, len);
  free(have);
  if (on == ours)
    return 0;
  if (on) {
    unlink(orig); /* one from before is stale */
    if (present && rename(dst, orig) != 0)
      return -1;
    return write_atomic(dst, png, len) ? 1 : -1;
  }
  unlink(dst);
  rename(orig, dst); /* none if the game's own was the one showing */
  return 1;
}

static void ensure_logo(void) {
  const int on = dcr_config()->touch_logo;
  const int a = put_logo("data/files/reanim/mainmenu3/PvZ_Logo.png", pvz_res_logo_menu,
                         pvz_res_logo_menu_size, on);
  const int b = put_logo("data/files/images/PvZ_Logo.png", pvz_res_logo_title,
                         pvz_res_logo_title_size, on);
  if (a < 0 || b < 0)
    debugPrintf("[setup] logo: could not write it (is the SD card full?)\n");
  else if (a || b)
    debugPrintf("[setup] logo: %s\n", on ? "Plants vs. Zombies Touch" : "the game's own");
}

/* An earlier build's picture of the lawn's top-right buttons (they are the
 * game's own stone buttons now): gone from the card. */
static void ensure_hud(void) {
  char path[320];
  root_path(path, sizeof path, "data/files/images/switch_hud.png");
  unlink(path);
}

/* ------------------------------------------------- adventure difficulty */
/* The mod's "Custom Difficulty": with its original-difficulty setting off,
 * the engine takes the adventure's levels from properties/levels.xml, and the
 * user files dir comes before the APK's own copy (the Transmension one). As
 * the mod's settings screen does, copy the chosen one from assets/levels/. */
#define PVZ_LEVELS_OUT "data/files/properties/levels.xml"

static void ensure_levels(mz_zip_archive *zip) {
  static const char *const k_files[] = {"levels-normal.xml", "levels-middle.xml",
                                        "levels-hard.xml"};
  static const char *const k_names[] = {"tv_touch", "transmension", "toumai"};
  const DcrConfig *c = dcr_config();
  int pick = c->adventure_levels;
  if (c->homura[HS_NORMAL_LEVEL] || pick == DCR_LEVELS_CUSTOM) {
    if (!c->homura[HS_NORMAL_LEVEL])
      debugPrintf("[setup] adventure difficulty: custom (%s, as you left it)\n", PVZ_LEVELS_OUT);
    return; /* original difficulty: the mod ignores levels.xml */
  }
  char dst[320], entry[64];
  root_path(dst, sizeof dst, PVZ_LEVELS_OUT);
  snprintf(entry, sizeof entry, "assets/levels/%s", k_files[pick]);
  int i = mz_zip_reader_locate_file(zip, entry, NULL, 0);
  mz_zip_archive_file_stat st;
  if (i < 0 || !mz_zip_reader_file_stat(zip, (mz_uint)i, &st)) {
    debugPrintf("[setup] adventure difficulty: the game APK has no %s\n", entry);
    return;
  }
  Stamp *s = stamp_get("levels.xml");
  if (s && s->crc == st.m_crc32 && file_size(dst) == (long)s->size)
    return;
  size_t len = 0;
  void *buf = mz_zip_reader_extract_to_heap(zip, (mz_uint)i, &len, 0);
  int ok = buf && write_atomic(dst, buf, len);
  mz_free(buf);
  if (!ok) {
    debugPrintf("[setup] adventure difficulty: could not write %s\n", dst);
    return;
  }
  stamp_set("levels.xml", st.m_crc32, (unsigned long)len);
  debugPrintf("[setup] adventure difficulty: %s (%s)\n", k_names[pick], k_files[pick]);
}

void dcr_setup_from_apk(const char *apk) {
  mz_zip_archive zip;
  memset(&zip, 0, sizeof zip);
  if (!mz_zip_reader_init_file(&zip, apk, 0))
    return; /* main.c reports a missing or unreadable APK */
  stamp_load();
  ensure_libs(&zip, apk);
  ensure_classes(&zip);
  ensure_english(&zip);
  ensure_logo();
  ensure_hud();
  ensure_levels(&zip);
  ensure_gamepad_mode();
  mz_zip_reader_end(&zip);
  stamp_save();
  if (g_setup_shown)
    setup_progress("Starting the game", 1000);
}

/* ---------------------------------------------------- updates from the NRO */
/* The newest launcher NRO in the game folder: its path and build. */
static uint64_t find_nro(char *path, size_t cap) {
  uint64_t best = 0;
  DIR *d = opendir(dcr_game_root());
  if (!d)
    return 0;
  struct dirent *e;
  while ((e = readdir(d))) {
    size_t n = strlen(e->d_name);
    if (n < 5 || strcasecmp(e->d_name + n - 4, ".nro"))
      continue;
    char p[320];
    root_path(p, sizeof p, e->d_name);
    FILE *f = fopen(p, "rb");
    if (!f)
      continue;
    uint64_t b = nro_build(f);
    fclose(f);
    if (b > best) {
      best = b;
      snprintf(path, cap, "%s", p);
    }
  }
  closedir(d);
  return best;
}

/* The English files the launcher NRO carries (romfs:/english.apk, the English
 * APK cut down to what pvz_english.c reads: tools/make_english_pack.py),
 * copied into the game folder as "PvZ Touch English.apk" -- as if the user
 * had put an English APK there; from then on it is one. main.c asks for it
 * when [game] language is english and the folder has no English APK. 1:
 * copied; 0: the NRO carries none; -1: the copy failed. */
int dcr_setup_english_from_nro(void) {
  char nro[320], dst[320], tmp[340];
  if (!find_nro(nro, sizeof nro)) {
    debugPrintf("[setup] the English files: no launcher NRO in the game folder to take them from\n");
    return 0;
  }
  FILE *f = fopen(nro, "rb");
  long off = 0;
  size_t size = 0;
  if (!f || nro_romfs_file(f, PVZ_NRO_ENGLISH_ROMFS, &off, &size) != 0 || fseek(f, off, SEEK_SET) != 0) {
    if (f)
      fclose(f);
    debugPrintf("[setup] the English files: %s carries none (built without them)\n", nro);
    return 0;
  }
  root_path(dst, sizeof dst, PVZ_NRO_ENGLISH_NAME);
  snprintf(tmp, sizeof tmp, "%s.part", dst);
  FILE *o = fopen(tmp, "wb");
  const size_t chunk = 1u << 20;
  uint8_t *buf = malloc(chunk);
  int ok = o && buf;
  for (size_t done = 0; ok && done < size;) {
    const size_t n = size - done < chunk ? size - done : chunk;
    ok = fread(buf, 1, n, f) == n && fwrite(buf, 1, n, o) == n;
    done += n;
    setup_progress("Unpacking the English files", (int)(done * 150 / size));
  }
  free(buf);
  fclose(f);
  if (o && fclose(o) != 0)
    ok = 0;
  if (ok) {
    unlink(dst);
    ok = rename(tmp, dst) == 0;
  }
  if (!ok) {
    unlink(tmp);
    debugPrintf("[setup] the English files: could not write %s\n", dst);
    return -1;
  }
  debugPrintf("[setup] the English files: copied out of %s as %s (%lu KB)\n", strrchr(nro, '/') + 1,
              PVZ_NRO_ENGLISH_NAME, (unsigned long)(size >> 10));
  return 1;
}

static uint8_t *read_whole(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *b = n > 0 ? malloc((size_t)n) : NULL;
  if (b && fread(b, 1, (size_t)n, f) != (size_t)n) {
    free(b);
    b = NULL;
  }
  fclose(f);
  *len = b ? (size_t)n : 0;
  return b;
}

void dcr_setup_update_from_nro(void) {
  u64 tid = 0;
  if (R_FAILED(svcGetInfo(&tid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0)) || !exefs_is_forwarder_tid(tid))
    return;
  char ovr[128], marker[300], nro[320];
  snprintf(ovr, sizeof ovr, "sdmc:/atmosphere/contents/%016llX/exefs.nsp", (unsigned long long)tid);
  root_path(marker, sizeof marker, ".update");
  if (file_size(ovr) <= 0)
    return; /* not running through an override: nothing of ours to update */

  uint64_t attempted = 0;
  FILE *mf = fopen(marker, "r");
  if (mf) {
    if (fscanf(mf, "%llu", (unsigned long long *)&attempted) != 1)
      attempted = 0;
    fclose(mf);
    if (attempted <= DCR_BUILD)
      unlink(marker); /* that update took */
  }
  uint64_t build = find_nro(nro, sizeof nro);
  debugPrintf("[setup] build %llu%s\n", (unsigned long long)DCR_BUILD,
              build > DCR_BUILD ? "; the launcher NRO carries a newer one" : "");
  if (build <= DCR_BUILD)
    return;
  if (attempted == build) {
    debugPrintf("[setup] %s: build %llu was installed but this is still build %llu -- not retrying "
                "(delete %s to try again)\n", nro, (unsigned long long)build,
                (unsigned long long)DCR_BUILD, marker);
    return;
  }

  /* the override must be this program's own: 32-bit, this title */
  size_t cur_len = 0, npdm_len, nsp_len = 0;
  uint8_t *cur = read_whole(ovr, &cur_len);
  const uint8_t *npdm;
  uint64_t pid = 0;
  int is64 = 1;
  int ours = cur && exefs_find(cur, cur_len, "main.npdm", &npdm, &npdm_len) == 0 &&
             npdm_info(npdm, npdm_len, &pid, &is64) == 0 && pid == tid && !is64;
  free(cur);
  if (!ours)
    return;

  FILE *f = fopen(nro, "rb");
  long off;
  uint8_t *nsp = NULL, *out = NULL;
  size_t out_len = 0;
  if (f && nro_romfs_file(f, "pvz_nx.nsp", &off, &nsp_len) == 0 && (nsp = malloc(nsp_len)) &&
      fseek(f, off, SEEK_SET) == 0 && fread(nsp, 1, nsp_len, f) == nsp_len)
    exefs_build_override(nsp, nsp_len, tid, &out, &out_len);
  if (f)
    fclose(f);
  free(nsp);
  if (!out) {
    debugPrintf("[setup] %s: its copy of the wrapper is unreadable -- not updating\n", nro);
    return;
  }
  setup_progress("Updating to the new build, then restarting", 1000);
  debugPrintf("[setup] updating to build %llu from %s, then restarting...\n", (unsigned long long)build, nro);
  char tmp[160];
  snprintf(tmp, sizeof tmp, "%s.part", ovr);
  FILE *o = fopen(tmp, "wb");
  int ok = o && fwrite(out, 1, out_len, o) == out_len;
  if (o && fclose(o) != 0)
    ok = 0;
  free(out);
  if (ok) {
    unlink(ovr);
    ok = rename(tmp, ovr) == 0;
  }
  if (!ok) {
    unlink(tmp);
    debugPrintf("[setup] could not write %s -- still running build %llu\n", ovr, (unsigned long long)DCR_BUILD);
    return;
  }
  mf = fopen(marker, "w");
  if (mf) {
    fprintf(mf, "%llu\n", (unsigned long long)build);
    fclose(mf);
  }
  log_flush_ring();
  Result rc = appletRestartProgram(NULL, 0);
  fatal_error("Updated to build %llu from %s.\n\n"
              "Restarting did not work (0x%x): close the game and launch it again.",
              (unsigned long long)build, nro, (unsigned)rc);
}
