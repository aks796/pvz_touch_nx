/* pvz_setup_plan.c -- Plants vs. Zombies Touch's part of the first launch:
 * its setup plan for the runtime's dcr_setup.c, and its own steps.
 *
 * The game folder (/switch/pvz_touch_nx) needs only the player's own APK,
 * under any name (the runtime tells it by its contents: PORT_APK_ROLES), and
 * the launcher NRO. The runtime makes the libraries and classes.txt from the
 * APK; this file adds the rest, made again whenever the APK changes (.setup
 * stamps):
 *   libHomura.so       this port's build of the mod, instead of the APK's,
 *                      when the APK is a known 1.1.5 build (below)
 *   data/files/...     the English layer (pvz_english.c), when config.ini
 *                      [game] language is english: the text, and with the
 *                      English source (role 1) the pictures, menu signs and
 *                      fonts too
 *   the logo           the port's, over the game's (resources/logo/)
 *   data/files/properties/levels.xml   the chosen adventure difficulty, when
 *                      [game] original_adventure_difficulty is false
 *   LAWN_GAMEPAD_MODE  the engine's Xbox 360 controls, in the environment
 * The game reads its data straight out of the APK, compressed or not
 * (zziplib), so the APK itself is left as it is.
 *
 * The bar, in permille of the whole first launch:
 *     0- 150  the English files copied out of the NRO (pvz_main.c, before)
 *         20  the mod installed (before the libraries: the runtime asks
 *             about each library first)
 *    20- 140  libnative_code / libGameMain / libHomura unpacked (by bytes)
 *   170- 200  the Java class list
 *   200- 950  the English layer
 *        1000 the game starts
 *
 * .setup keys, which must not change (or every player unpacks again once):
 * libnative_code.so, libGameMain.so, libHomura.so, classes.txt, levels.xml,
 * LawnStrings (an older build's, cleared). MIT.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "dcr_build.h"
#include "dcr_config.h"
#include "dcr_setup.h"
#include "error.h"
#include "pvz_apks.h"
#include "pvz_english.h"
#include "util.h"

static const char *const k_libs[] = {PVZ_LIB_NATIVE, PVZ_LIB_GAME, PVZ_LIB_HOMURA};

/* ------------------------------------------------------------- the mod */
/* The wrapper carries its own build of the mod (mod/, pvz_res.S): the source
 * of the 1.1.5-260925 mod (nightly e91bc9a, online protocol 3199; built as
 * is, byte for byte the APK's code and data), with this port's changes for
 * the Switch's controls. It replaces the APK's libHomura.so only when that
 * is one of the 1.1.5 builds below, whose game libraries are the same (the
 * 260924 APK differs only in its mod and the online protocol it speaks,
 * 3198); another APK keeps its own. config.ini [debug] mod_from_apk = true:
 * always the APK's. */
static const unsigned long k_homura_crcs[] = {
    0xbcd6795eu, /* 1.1.5-260925 (the source's own build) */
    0xf5d2a357u, /* 1.1.5-260924 */
};
extern const uint8_t pvz_res_homura[];
extern const uint32_t pvz_res_homura_size;

/* The runtime asks before it unpacks each library: 1 when this one is
 * handled here (installed or already in place). */
int port_setup_lib_override(const char *lib, unsigned long apk_crc, RtSetupCtx *ctx) {
  (void)ctx;
  if (strcmp(lib, PVZ_LIB_HOMURA))
    return 0;
  int known = 0;
  for (unsigned i = 0; i < sizeof k_homura_crcs / sizeof k_homura_crcs[0]; i++)
    known |= apk_crc == k_homura_crcs[i];
  if (dcr_config()->mod_from_apk || !known || !pvz_res_homura_size)
    return 0;
  char dst[300];
  rt_root_path(dst, sizeof dst, PVZ_LIB_HOMURA);
  const unsigned long crc = mz_crc32(0, pvz_res_homura, pvz_res_homura_size);
  unsigned long have_crc = 0;
  if (rt_setup_stamp_get(PVZ_LIB_HOMURA, &have_crc, NULL) && have_crc == crc &&
      rt_file_size(dst) == (long)pvz_res_homura_size)
    return 1;
  dcr_setup_progress("Installing the mod", 20);
  debugPrintf("[setup] the mod: this port's build of the 1.1.5 mod's source, with the Switch "
              "controls (%u KB)\n", (unsigned)(pvz_res_homura_size >> 10));
  if (!rt_write_atomic(dst, pvz_res_homura, pvz_res_homura_size))
    fatal_error("Could not write %s.\n\nIs the SD card full or read-only?", dst);
  rt_setup_stamp_set(PVZ_LIB_HOMURA, crc, pvz_res_homura_size);
  return 1;
}

/* ------------------------------------------------------ English layer */
extern const char pvz_res_addon_en[], pvz_res_lawn_en[], pvz_res_lawn_fix[]; /* pvz_res.S */
extern const unsigned char pvz_res_button_icons[], pvz_res_help_buttons[], pvz_res_help_buttons_small[];
extern const uint32_t pvz_res_button_icons_size, pvz_res_help_buttons_size, pvz_res_help_buttons_small_size;
extern const unsigned char pvz_res_gamepad0[], pvz_res_gamepad1[];
extern const uint32_t pvz_res_gamepad0_size, pvz_res_gamepad1_size;

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
void pvz_setup_font_failed(const char *why) {
  char path[320];
  rt_root_path(path, sizeof path, PVZ_FONTS_FAILED);
  FILE *f = fopen(path, "w");
  if (!f)
    return;
  fprintf(f, "%llu %.300s\n", (unsigned long long)DCR_BUILD, why);
  fclose(f);
  debugPrintf("[quit] the next start makes the fonts again without the button pictures\n");
}

static int fonts_failed(void) {
  char path[320];
  rt_root_path(path, sizeof path, PVZ_FONTS_FAILED);
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

/* the English step's part of the bar (its row in k_steps) */
static int g_eng_p0, g_eng_p1;
static void english_working(void) { dcr_setup_progress("English pictures", g_eng_p0); }
static void english_progress(int permille, const char *what) {
  dcr_setup_progress(what, g_eng_p0 + permille * (g_eng_p1 - g_eng_p0) / 1000);
}

static void step_english(RtSetupCtx *ctx) {
  char files[300], ext[300], list[300], eng[300];
  rt_root_path(files, sizeof files, "data/files");
  rt_root_path(ext, sizeof ext, "external/files");
  rt_root_path(list, sizeof list, ".english");
  snprintf(eng, sizeof eng, "%s", pvz_english_apk()); /* whatever its name (role 1) */
  g_eng_p0 = ctx->p0;
  g_eng_p1 = ctx->p1;
  PvzEnglishCfg c = {
      .files_dir = files,
      .layer_list = list,
      .english_apk = eng,
      .app_dirs = {files, ext},
      .res = {.addon_en = pvz_res_addon_en,
              .lawn_en = pvz_res_lawn_en,
              .lawn_fix = pvz_res_lawn_fix,
              .icons_png = pvz_res_button_icons,
              .help_png = pvz_res_help_buttons,
              .help_small_png = pvz_res_help_buttons_small,
              .icons_len = pvz_res_button_icons_size,
              .help_len = pvz_res_help_buttons_size,
              .help_small_len = pvz_res_help_buttons_small_size,
              .pad_png = {pvz_res_gamepad0, pvz_res_gamepad1},
              .pad_len = {pvz_res_gamepad0_size, pvz_res_gamepad1_size}},
      .log = setup_log,
      .working = english_working,
      .progress = english_progress,
      .no_button_pictures = !dcr_config()->button_pictures || fonts_failed(),
  };
  /* builds before the layer made only LawnStrings.txt (stamp "LawnStrings") */
  unsigned long old_crc = 0;
  if (rt_setup_stamp_get("LawnStrings", &old_crc, NULL) && old_crc) {
    char dst[320];
    rt_root_path(dst, sizeof dst, "data/files/properties/LawnStrings.txt");
    if (!dcr_config()->english)
      unlink(dst);
    rt_setup_stamp_set("LawnStrings", 0, 0);
  }
  if (!dcr_config()->english) {
    pvz_english_remove(&c);
    return;
  }
  pvz_english_apply(ctx->apk, &c);
}

/* ------------------------------------------------------------ the logo */
/* The Plants vs. Zombies Touch logo (logo.png at the port's top, sized by
 * tools/make_logo.py, built in: pvz_res.S) over the game's, on the main menu
 * and the title screen: the engine reads the files dir before the APK. What
 * was there (the English layer's picture) is kept as <name>.orig and put
 * back with [game] touch_logo off. After the English layer, which writes the
 * same names when it is made. */
extern const uint8_t pvz_res_logo_menu[], pvz_res_logo_title[];
extern const uint32_t pvz_res_logo_menu_size, pvz_res_logo_title_size;

static int put_logo(const char *rel, const uint8_t *png, size_t len, int on) {
  char dst[320], orig[330];
  rt_root_path(dst, sizeof dst, rel);
  snprintf(orig, sizeof orig, "%s.orig", dst);
  size_t have_len = 0;
  uint8_t *have = rt_read_whole(dst, &have_len);
  const int present = have != NULL, ours = present && have_len == len && !memcmp(have, png, len);
  free(have);
  if (on == ours)
    return 0;
  if (on) {
    unlink(orig); /* one from before is stale */
    if (present && rename(dst, orig) != 0)
      return -1;
    return rt_write_atomic(dst, png, len) ? 1 : -1;
  }
  unlink(dst);
  rename(orig, dst); /* none if the game's own was the one showing */
  return 1;
}

static void step_logo(RtSetupCtx *ctx) {
  (void)ctx;
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
static void step_hud(RtSetupCtx *ctx) {
  (void)ctx;
  char path[320];
  rt_root_path(path, sizeof path, "data/files/images/switch_hud.png");
  unlink(path);
}

/* ------------------------------------------------- adventure difficulty */
/* The mod's "Custom Difficulty": with its original-difficulty setting off,
 * the engine takes the adventure's levels from properties/levels.xml, and the
 * user files dir comes before the APK's own copy (the Transmension one). As
 * the mod's settings screen does, copy the chosen one from assets/levels/. */
#define PVZ_LEVELS_OUT "data/files/properties/levels.xml"

static void step_levels(RtSetupCtx *ctx) {
  static const char *const k_files[] = {"levels-normal.xml", "levels-middle.xml",
                                        "levels-hard.xml"};
  static const char *const k_names[] = {"tv_touch", "transmension", "toumai"};
  mz_zip_archive *zip = ctx->apk;
  const DcrConfig *c = dcr_config();
  int pick = c->adventure_levels;
  if (c->homura[HS_NORMAL_LEVEL] || pick == DCR_LEVELS_CUSTOM) {
    if (!c->homura[HS_NORMAL_LEVEL])
      debugPrintf("[setup] adventure difficulty: custom (%s, as you left it)\n", PVZ_LEVELS_OUT);
    return; /* original difficulty: the mod ignores levels.xml */
  }
  char dst[320], entry[64];
  rt_root_path(dst, sizeof dst, PVZ_LEVELS_OUT);
  snprintf(entry, sizeof entry, "assets/levels/%s", k_files[pick]);
  int i = mz_zip_reader_locate_file(zip, entry, NULL, 0);
  mz_zip_archive_file_stat st;
  if (i < 0 || !mz_zip_reader_file_stat(zip, (mz_uint)i, &st)) {
    debugPrintf("[setup] adventure difficulty: the game APK has no %s\n", entry);
    return;
  }
  unsigned long have_crc = 0, have_size = 0;
  if (rt_setup_stamp_get("levels.xml", &have_crc, &have_size) && have_crc == st.m_crc32 &&
      rt_file_size(dst) == (long)have_size)
    return;
  size_t len = 0;
  void *buf = mz_zip_reader_extract_to_heap(zip, (mz_uint)i, &len, 0);
  int ok = buf && rt_write_atomic(dst, buf, len);
  mz_free(buf);
  if (!ok) {
    debugPrintf("[setup] adventure difficulty: could not write %s\n", dst);
    return;
  }
  rt_setup_stamp_set("levels.xml", st.m_crc32, (unsigned long)len);
  debugPrintf("[setup] adventure difficulty: %s (%s)\n", k_names[pick], k_files[pick]);
}

/* ------------------------------------------------------ the controls */
/* The engine's options file (Sexy::GetEnv reads "setup.env" through its
 * file system: the files dir first). LAWN_GAMEPAD_MODE=1 (GamepadApp
 * +0x895, what HasGamepad() needs) is the Xbox 360 edition's control scheme:
 * buttons reach the board as gamepad buttons (GameButtonDown: L/R cycle the
 * seed packets, A plants, B digs, X butter, START pauses) and hints show
 * button pictures. Without it, the TV edition's remote scheme: A enters a seed
 * picker, and <A> in hints is spelled out. No APK ships the file. */
#define PVZ_SETUP_ENV "data/files/setup.env"
int b_setenv(const char *name, const char *value, int overwrite); /* bionic_core.c */
#define SETUP_ENV_MARK "# written by the Switch wrapper"

/* The engine's Xbox 360 gamepad mode: GamepadApp takes it from
 * Sexy::GetEnvOption("LAWN_GAMEPAD_MODE"). Its options file, setup.env, is read
 * through the pak file system, which does not look in the files folder, but a
 * name the file lacks falls back to getenv() -- so it goes in the environment,
 * before the engine loads. (Build 202609250408 wrote a setup.env the engine
 * never opened: removed.) */
static void step_gamepad_mode(RtSetupCtx *ctx) {
  (void)ctx;
  char dst[320];
  rt_root_path(dst, sizeof dst, PVZ_SETUP_ENV);
  size_t len = 0;
  uint8_t *have = rt_read_whole(dst, &len);
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

/* ------------------------------------------------------------- the plan */
/* In this order, with the APK open: the English layer first, as the logo
 * goes over its picture. */
static const RtSetupStep k_steps[] = {
    {"english", RT_STEP_WITH_ZIP, 200, 950, step_english},
    {"logo", RT_STEP_WITH_ZIP, 950, 950, step_logo},
    {"hud", RT_STEP_WITH_ZIP, 950, 950, step_hud},
    {"levels", RT_STEP_WITH_ZIP, 950, 950, step_levels},
    {"gamepad_mode", RT_STEP_WITH_ZIP, 950, 950, step_gamepad_mode},
};

const RtSetupPlan port_setup_plan = {
    .libs = k_libs,
    .nlibs = sizeof k_libs / sizeof k_libs[0],
    .libs_what = "Unpacking the game",
    .apk_requirement = "This port needs PvZ TV Touch 1.1.5 (com.trans.pvztv, armeabi-v7a):\n"
                       "put the APK of that version in the game folder (any file name).",
    .libs_p0 = 20,
    .libs_p1 = 140,
    .classes_p0 = 170,
    .classes_p1 = 200,
    .steps = k_steps,
    .nsteps = sizeof k_steps / sizeof k_steps[0],
};
