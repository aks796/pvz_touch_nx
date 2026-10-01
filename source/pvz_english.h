/* pvz_english.h -- the English layer (pvz_english.c). */
#ifndef PVZ_ENGLISH_H
#define PVZ_ENGLISH_H

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz/miniz.h>

/* The wrapper's own English text (resources/english/, NUL-terminated UTF-8). */
typedef struct {
  const char *addon_en; /* the newest mod's AddonStrings lines */
  const char *lawn_en;  /* LawnStrings lines no English table has */
  const char *lawn_fix; /* LawnStrings lines the newest mod changed */
  /* the Switch's buttons (resources/buttons/, tools/make_button_icons.py):
   * every one in a row, and the help bar's two sheets */
  const unsigned char *icons_png, *help_png, *help_small_png;
  size_t icons_len, help_len, help_small_len;
  /* the versus side picker's controllers, players 1 and 2: the Switch Pro
   * Controller (resources/controllers/, tools/make_controller_icons.py) */
  const unsigned char *pad_png[2];
  size_t pad_len[2];
} PvzEnglishRes;

typedef struct {
  const char *files_dir;     /* the engine's files dir: the layer goes here */
  const char *layer_list;    /* the file listing what the layer made */
  const char *english_apk;   /* the English APK (need not exist) */
  const char *app_dirs[2];   /* app data dirs: their cached/ fonts are cleared */
  PvzEnglishRes res;
  void (*log)(const char *fmt, ...);
  void (*working)(void);     /* called once before slow work */
  void (*progress)(int permille, const char *what); /* how far the work is (may be NULL) */
  int no_button_pictures;    /* [game] button_pictures = false: the text names the buttons */
} PvzEnglishCfg;

/* Make (or keep) the layer for the open game.apk. 0: done or already up to
 * date; -1: could not. */
int pvz_english_apply(mz_zip_archive *game, const PvzEnglishCfg *cfg);

/* Take the layer away (language: chinese). */
void pvz_english_remove(const PvzEnglishCfg *cfg);

#endif
