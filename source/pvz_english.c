/* pvz_english.c -- the English layer for the newest (Chinese) mod.
 *
 * PvZ TV Touch 1.1.5 (the newest mod, game.apk) exists only in Chinese: its
 * text, fonts and every picture with words on it. The older English builds of
 * the mod (4.0.5 "PvZTouch", RedStr1x) are the same engine with an English
 * data set, and they carry assets/paks/2.ChangeGameChina.zip: the Chinese
 * original of EVERY file their translators changed (the mod's settings screen
 * unpacks a pak over the files dir to switch language). So that pak is an
 * exact list of what to translate, and the English APK has each file's
 * English version. The user supplies that APK as english.apk (their own copy;
 * nothing of the game ships with the port), and this makes an English layer in
 * the engine's files dir, which the engine searches before game.apk:
 *
 *   text      properties/LawnStrings.txt, LawnOEMStrings.txt and the mod's
 *             addonFiles/properties/AddonStrings.txt: every key of the game's
 *             own tables, in English from, in order: the fixes in
 *             resources/english/ (lines the newest mod changed),
 *             english.apk's tables, the mod's own English table
 *             (addonFiles/properties/LawnStrings_EN.txt, in game.apk),
 *             resources/english/ (the newest mod's own lines, translated for
 *             this port). pvz_strings.c checks each for its printf formats.
 *             LawnOEMStrings.txt, which the engine loads after LawnStrings
 *             and which overrides it, is the TV-remote wording in the game
 *             ("press the Back key"); the layer's holds only the menu crow's
 *             lines, as the English builds' does.
 *   pictures  each file of the pak that game.apk has exactly as the pak has it
 *             (the same Chinese original): english.apk's version. Where the
 *             newest mod changed the file itself, a short list (k_take_en)
 *             says which still take the English one; the rest stay. Another
 *             (k_keep_game) keeps the game's own: English pictures with the
 *             Xbox 360 edition's prompts painted in.
 *   menus     the English builds keep the main menu's signs in an atlas
 *             (reanim/mainmenu3/menu-atlas1.tex) that game.apk does not use:
 *             it has one PNG per sign. Each sign whose English picture differs
 *             from the Chinese one (the pak has the Chinese atlas) is cut out
 *             and written as game.apk's PNG of that name and size.
 *   fonts     each font the pak changes: english.apk's descriptor
 *             (data/<font>.txt) with its pictures renamed <name>_en, so that
 *             none of game.apk's Chinese glyph sheets (a NAME.png in the APK
 *             next to an English NAME_.png alpha sheet) can mix in. The
 *             engine's compiled fonts (<app data>/cached/data/<font>.cfu2) are
 *             deleted, or it would keep drawing the old ones.
 *
 *   console   the help bar's button sheets, the Xbox 360 edition's that
 *             game.apk carries, with the Switch's names (x360_buttons); the
 *             versus screens' controllers, Switch Pro Controllers
 *             (console_controllers). With or without english.apk.
 *
 * Without english.apk only the text is made (the mod's English table and
 * this port's lines), in the Chinese fonts, which have Latin letters.
 *
 * The layer list (.english in the game folder) names every file made and a key
 * over game.apk, english.apk and the port's text: the layer is made again when
 * any of them changes, and removed file by file for language = chinese.
 * english.apk is needed only to make it: deleted afterwards, the layer stays
 * (until game.apk changes). MIT.
 */
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "pvz_english.h"
#include "pvz_strings.h"

/* Bump it with any change to what the layer makes: a layer is kept while its
 * key matches, and the fix to it never reaches a card that has one (hardware
 * run 16: the UTF-8 font fix, made with the version unchanged, never ran --
 * and the engine kept loading its compiled copy of the broken font). */
#define LAYER_VERSION 17
#define FILES "assets/files/"

/* Files the newest mod changed from the Chinese original that still take the
 * English picture (checked by eye against both: the logo, the "final wave" /
 * "zombies ate your brains" banners, Survival's sign, an almanac card, the
 * copyright notice shown at start-up). The
 * mod's other changes (the Zombatar screen's new background, its help menu)
 * stay. Lower case. */
static const char *const k_take_en[] = {
    "reanim/finalwave.png",
    "reanim/zombieswon.png",
    "reanim/mainmenu3/pvz_logo.png",
    "images/pvz_logo.png",
    "reanim/mainmenu3/survival button.png",
    "reanim/mainmenu3/survival pressed.png",
    "reanim/mainmenu3/survival selected.png",
    "images/survival_button.png",
    "reanim/mainmenu3/almanac plant 10.png",
    "images/guide.png", /* the start-up copyright notice */
    NULL,
};

/* English pictures never taken, though the game's are the Chinese original's.
 * The English build's Crazy Dave speech bubble has the Xbox 360 edition's
 * "PRESS (A) TO CONTINUE" painted in, and the game writes its own
 * [CLICK_TO_CONTINUE] in the same place: the prompt came out twice, in two
 * fonts (tester, 2026-09-30). The game's bubble is blank. Lower case. */
static const char *const k_keep_game[] = {
    "images/store_speechbubble2.png",
    NULL,
};

static const char *const k_img_ext[] = {".png", ".jpg", ".jpeg", ".gif", ".tga", ".bmp", NULL};

typedef struct {
  char *name; /* lower case, relative to the index's prefix */
  mz_uint idx;
} Ent;

typedef struct {
  Ent *v;
  int n;
} Index;

typedef struct {
  char name[160];
  char tex[336];
  int x, y, w, h;
} Region;

typedef struct {
  const PvzEnglishCfg *cfg;
  mz_zip_archive *g, e, c;
  int have_e, have_c; /* english.apk usable (its pak located); the pak read */
  int pak;
  void *c_buf;
  Index gi, ei, ci;
  char **made;
  int nmade, capmade;
  int fail;
  int n_swap, n_take, n_kept, n_font, n_fontimg, n_crop, n_same, n_made;
  int zb_tried, zb_ok; /* the Zombatar back slab (make_zombatar_back) */
  uint8_t *icons;      /* the buttons' row, RGBA (font_buttons); NULL: not decoded */
  int icons_w, icons_h, icons_tried;
  int ic_x[16], ic_w[16];          /* each button in that row: where, how wide */
  uint8_t *x360_big, *x360_small;  /* the game's own button sheets, relabelled (x360_buttons) */
  int n_icon_fonts, n_icon_fail; /* fonts given the buttons' layer; not */
  char kept[512];
} Ctx;

#define LOG(...) x->cfg->log(__VA_ARGS__)
#define PROGRESS(pm, what) (x->cfg->progress ? x->cfg->progress((pm), (what)) : (void)0)

static void lower_str(char *s) {
  for (; *s; s++)
    *s = (char)tolower((unsigned char)*s);
}

static int starts(const char *s, const char *p) { return !strncmp(s, p, strlen(p)); }

static int ends(const char *s, const char *p) {
  size_t n = strlen(s), m = strlen(p);
  return n >= m && !strcmp(s + n - m, p);
}

/* ---------------------------------------------------------------- index */
static int ent_cmp(const void *a, const void *b) {
  return strcmp(((const Ent *)a)->name, ((const Ent *)b)->name);
}

static int index_build(mz_zip_archive *z, const char *prefix, Index *ix) {
  mz_uint n = mz_zip_reader_get_num_files(z);
  size_t pl = strlen(prefix);
  ix->n = 0;
  ix->v = calloc(n ? n : 1, sizeof *ix->v);
  if (!ix->v)
    return -1;
  char name[512];
  for (mz_uint i = 0; i < n; i++) {
    if (mz_zip_reader_is_file_a_directory(z, i))
      continue;
    if (!mz_zip_reader_get_filename(z, i, name, sizeof name) || strncmp(name, prefix, pl))
      continue;
    char *s = strdup(name + pl);
    if (!s)
      return -1;
    lower_str(s);
    ix->v[ix->n].name = s;
    ix->v[ix->n].idx = i;
    ix->n++;
  }
  qsort(ix->v, (size_t)ix->n, sizeof *ix->v, ent_cmp);
  return 0;
}

static void index_free(Index *ix) {
  for (int i = 0; i < ix->n; i++)
    free(ix->v[i].name);
  free(ix->v);
  ix->v = NULL;
  ix->n = 0;
}

/* zip index of `rel` (any case), -1 if none */
static int index_find(const Index *ix, const char *rel) {
  char key[512];
  snprintf(key, sizeof key, "%s", rel);
  lower_str(key);
  Ent k = {key, 0};
  const Ent *e = ix->v ? bsearch(&k, ix->v, (size_t)ix->n, sizeof *ix->v, ent_cmp) : NULL;
  return e ? (int)e->idx : -1;
}

static mz_uint32 entry_crc(mz_zip_archive *z, int idx) {
  mz_zip_archive_file_stat st;
  return idx >= 0 && mz_zip_reader_file_stat(z, (mz_uint)idx, &st) ? st.m_crc32 : 0;
}

/* NUL-terminated copy of an entry (mz_free) */
static void *extract(mz_zip_archive *z, int idx, size_t *len) {
  if (idx < 0)
    return NULL;
  size_t n = 0;
  void *p = mz_zip_reader_extract_to_heap(z, (mz_uint)idx, &n, 0);
  if (!p)
    return NULL;
  void *q = realloc(p, n + 1);
  if (!q) {
    mz_free(p);
    return NULL;
  }
  ((char *)q)[n] = 0;
  if (len)
    *len = n;
  return q;
}

/* ---------------------------------------------------------------- output */
static void mkdirs_for(const char *path) {
  char p[600];
  snprintf(p, sizeof p, "%s", path);
  for (char *q = p + 1; *q; q++)
    if (*q == '/' && q[-1] != ':') {
      *q = 0;
      mkdir(p, 0777);
      *q = '/';
    }
}

static int made_has(const Ctx *x, const char *rel) {
  for (int i = 0; i < x->nmade; i++)
    if (!strcasecmp(x->made[i], rel))
      return 1;
  return 0;
}

static void made_add(Ctx *x, const char *rel) {
  if (made_has(x, rel))
    return;
  if (x->nmade == x->capmade) {
    int cap = x->capmade ? x->capmade * 2 : 256;
    char **v = realloc(x->made, sizeof *v * (size_t)cap);
    if (!v) {
      x->fail = 1;
      return;
    }
    x->made = v;
    x->capmade = cap;
  }
  char *s = strdup(rel);
  if (!s) {
    x->fail = 1;
    return;
  }
  x->made[x->nmade++] = s;
}

/* files_dir/rel, whole or not at all */
static int put_file(Ctx *x, const char *rel, const void *buf, size_t len) {
  char path[600], tmp[610];
  snprintf(path, sizeof path, "%s/%s", x->cfg->files_dir, rel);
  snprintf(tmp, sizeof tmp, "%s.part", path);
  mkdirs_for(path);
  FILE *f = fopen(tmp, "wb");
  int ok = f && (len == 0 || fwrite(buf, 1, len, f) == len);
  if (f && fclose(f) != 0)
    ok = 0;
  if (ok) {
    unlink(path);
    ok = rename(tmp, path) == 0;
  }
  if (!ok) {
    unlink(tmp);
    if (!x->fail)
      LOG("[english] could not write %s -- is the SD card full?\n", path);
    x->fail = 1;
    return -1;
  }
  made_add(x, rel);
  return 0;
}

/* english.apk's copy of an entry, written as `rel` */
static int copy_e(Ctx *x, int eidx, const char *rel) {
  size_t n = 0;
  void *d = extract(&x->e, eidx, &n);
  if (!d) {
    LOG("[english] could not read %s from the English APK\n", rel);
    return -1;
  }
  int r = put_file(x, rel, d, n);
  free(d);
  return r;
}

/* ---------------------------------------------------------------- the list */
static char *read_all(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *b = n >= 0 ? malloc((size_t)n + 1) : NULL;
  if (b && fread(b, 1, (size_t)n, f) != (size_t)n) {
    free(b);
    b = NULL;
  }
  fclose(f);
  if (b) {
    b[n] = 0;
    if (len)
      *len = (size_t)n;
  }
  return b;
}

/* the key the layer on the card was made for ("" if none) */
static void list_key(const PvzEnglishCfg *cfg, char *key, size_t cap) {
  key[0] = 0;
  char *t = read_all(cfg->layer_list, NULL);
  if (!t)
    return;
  char *nl = strchr(t, '\n');
  if (nl)
    *nl = 0;
  if (starts(t, "pvz-english "))
    snprintf(key, cap, "%s", t + 12);
  free(t);
}

/* delete every file of the layer and the list; the number deleted */
static int remove_listed(const PvzEnglishCfg *cfg) {
  char *t = read_all(cfg->layer_list, NULL);
  if (!t)
    return 0;
  int n = 0;
  char *line = strchr(t, '\n');
  while (line && *++line) {
    char *nl = strchr(line, '\n');
    if (nl)
      *nl = 0;
    if (*line && !strstr(line, "..")) {
      char path[600];
      snprintf(path, sizeof path, "%s/%s", cfg->files_dir, line);
      if (unlink(path) == 0)
        n++;
    }
    line = nl;
  }
  free(t);
  unlink(cfg->layer_list);
  return n;
}

static int write_list(Ctx *x, const char *key) {
  char tmp[600];
  snprintf(tmp, sizeof tmp, "%s.part", x->cfg->layer_list);
  FILE *f = fopen(tmp, "w");
  if (!f)
    return -1;
  fprintf(f, "pvz-english %s\n", key);
  for (int i = 0; i < x->nmade; i++)
    fprintf(f, "%s\n", x->made[i]);
  int ok = fclose(f) == 0;
  if (ok) {
    unlink(x->cfg->layer_list);
    ok = rename(tmp, x->cfg->layer_list) == 0;
  }
  if (!ok)
    unlink(tmp);
  return ok ? 0 : -1;
}

/* The compiled fonts: <app dir>/cached/...*.cfu2 */
static int clear_cfu2(const char *dir, int depth) {
  DIR *d = opendir(dir);
  if (!d)
    return 0;
  int n = 0;
  struct dirent *e;
  char sub[600];
  while ((e = readdir(d))) {
    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
      continue;
    snprintf(sub, sizeof sub, "%s/%s", dir, e->d_name);
    char low[256];
    snprintf(low, sizeof low, "%s", e->d_name);
    lower_str(low);
    if (ends(low, ".cfu2")) {
      if (unlink(sub) == 0)
        n++;
    } else if (depth < 4) {
      struct stat st;
      if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode))
        n += clear_cfu2(sub, depth + 1);
    }
  }
  closedir(d);
  return n;
}

static int clear_font_caches(const PvzEnglishCfg *cfg) {
  int n = 0;
  for (int i = 0; i < 2; i++)
    if (cfg->app_dirs[i]) {
      char dir[600];
      snprintf(dir, sizeof dir, "%s/cached", cfg->app_dirs[i]);
      n += clear_cfu2(dir, 0);
    }
  return n;
}

/* ---------------------------------------------------------------- the key */
static uint64_t fnv(uint64_t h, const void *p, size_t n) {
  const unsigned char *b = p;
  for (size_t i = 0; i < n; i++)
    h = (h ^ b[i]) * 0x100000001b3ull;
  return h;
}

static uint64_t hash_zip(uint64_t h, mz_zip_archive *z) {
  mz_uint n = mz_zip_reader_get_num_files(z);
  for (mz_uint i = 0; i < n; i++) {
    mz_zip_archive_file_stat st;
    if (!mz_zip_reader_file_stat(z, i, &st))
      continue;
    h = fnv(h, &st.m_crc32, sizeof st.m_crc32);
    h = fnv(h, st.m_filename, strlen(st.m_filename));
  }
  return h;
}

/* ---------------------------------------------------------------- text */
static void make_strings(Ctx *x) {
  const PvzEnglishRes *r = &x->cfg->res;
  size_t zl = 0, el = 0, g16l = 0, gl = 0, out_l = 0;
  char *zh = extract(x->g, index_find(&x->gi, "properties/LawnStrings.txt"), &zl);
  if (!zh) {
    LOG("[english] the game APK has no properties/LawnStrings.txt: the text stays as it is\n");
    return;
  }
  char *en = x->have_e ? extract(&x->e, index_find(&x->ei, "properties/LawnStrings.txt"), &el) : NULL;
  uint8_t *g16 = extract(x->g, index_find(&x->gi, "addonFiles/properties/LawnStrings_EN.txt"), &g16l);
  char *gen = g16 ? pvz_utf16_to_utf8(g16, g16l, &gl) : NULL;
  free(g16);
  PvzStrSrc src[4] = {
      {r->lawn_fix, r->lawn_fix ? strlen(r->lawn_fix) : 0, 0, 0},
      {en, el, 1, 1}, /* its button glyphs: the mod's table's <A> tags instead */
      {gen, gl, 0, 0},
      {r->lawn_en, r->lawn_en ? strlen(r->lawn_en) : 0, 1, 0}, /* e.g. MENU_BUTTON */
  };
  PvzStrStats st;
  char *out = pvz_strings_pick(zh, zl, src, 4, &out_l, &st);
  int tags = 0;
  const int icons = x->n_icon_fonts > 0 && x->n_icon_fail == 0; /* every font has the pictures */
  if (out) { /* <A>, <S>... as the Switch's buttons: pictures, or names */
    size_t nl = 0;
    char *named = pvz_strings_button_names(out, out_l, &nl, &tags, icons);
    if (named) {
      free(out);
      out = named, out_l = nl;
    }
  }
  if (out && put_file(x, "properties/LawnStrings.txt", out, out_l) == 0) {
    LOG("[english] LawnStrings: %d buttons %s\n", tags,
        icons ? "shown as the Switch's (pictures in every font)" : "named as on the Switch (A, B, X, Y, +, L, R, ZL, ZR)");
    LOG("[english] LawnStrings: %d lines -- %d from the English APK, %d from the mod's English table, "
        "%d from this port (%d fixes), %d still Chinese; %d more from the English APK\n",
        st.keys, st.from[1], st.from[2], st.from[0] + st.from[3], st.from[0], st.chinese, st.added);
    /* the OEM table overrides LawnStrings: only the crow's lines, in English */
    size_t ol = 0;
    char *oem = pvz_strings_subset(out, out_l, "CROW_SELECTION", &ol);
    if (oem)
      put_file(x, "properties/LawnOEMStrings.txt", oem, ol);
    else
      put_file(x, "properties/LawnOEMStrings.txt", out, out_l);
    free(oem);
  } else if (!out) {
    LOG("[english] could not make LawnStrings.txt\n");
  }
  free(out);
  free(zh);
  free(en);
  free(gen);

  /* the mod's own lines */
  zh = extract(x->g, index_find(&x->gi, "addonFiles/properties/AddonStrings.txt"), &zl);
  if (!zh)
    return;
  en = x->have_e ? extract(&x->e, index_find(&x->ei, "addonFiles/properties/AddonStrings.txt"), &el)
                 : NULL;
  /* this port's lines also add the keys the game's table lacks: the newest
   * mod's code can use a key its assets never got ([VS_UI_TIMED_DRAFT]) */
  PvzStrSrc asrc[2] = {
      {r->addon_en, r->addon_en ? strlen(r->addon_en) : 0, 1, 0},
      {en, el, 1, 1},
  };
  out = pvz_strings_pick(zh, zl, asrc, 2, &out_l, &st);
  if (out) {
    size_t nl = 0;
    char *named = pvz_strings_button_names(out, out_l, &nl, NULL, icons);
    if (named) {
      free(out);
      out = named, out_l = nl;
    }
  }
  if (out && put_file(x, "addonFiles/properties/AddonStrings.txt", out, out_l) == 0)
    LOG("[english] AddonStrings: %d lines -- %d from this port, %d from the English APK, %d still "
        "Chinese\n",
        st.keys, st.from[0], st.from[1], st.chinese);
  free(out);
  free(zh);
  free(en);
}

/* ---------------------------------------------------------------- fonts */
/* english.apk's picture(s) for the font image NAME (data/NAME.png, the
 * alpha-only data/NAME_.png or data/_NAME.png, any image type), written as
 * NAME_en; the number found. */
static int font_images(Ctx *x, const char *name) {
  static const char *const forms[] = {"data/%s%s", "data/%s_%s", "data/_%s%s"};
  static const char *const outs[] = {"data/%s_en%s", "data/%s_en_%s", "data/_%s_en%s"};
  int found = 0;
  for (int f = 0; f < 3; f++)
    for (int k = 0; k_img_ext[k]; k++) {
      char rel[300], out[300];
      snprintf(rel, sizeof rel, forms[f], name, k_img_ext[k]);
      int i = index_find(&x->ei, rel);
      if (i < 0)
        continue;
      found++;
      snprintf(out, sizeof out, outs[f], name, k_img_ext[k]);
      if (made_has(x, out))
        continue;
      if (copy_e(x, i, out) == 0)
        x->n_fontimg++;
    }
  return found;
}

/* The quoted image name of a "LayerSetImage <layer> 'NAME';" line at p (a
 * line start): *q0 at its first character, *q1 at the closing quote. */
static int image_line(const char *p, const char **q0, const char **q1) {
  while (*p == ' ' || *p == '\t')
    p++;
  if (strncmp(p, "LayerSetImage", 13) || (p[13] != ' ' && p[13] != '\t'))
    return 0;
  const char *a = strchr(p, '\'');
  const char *nl = strchr(p, '\n');
  if (!a || (nl && a > nl))
    return 0;
  const char *b = strchr(a + 1, '\'');
  if (!b || (nl && b > nl) || b == a + 1 || b - a > 120)
    return 0;
  *q0 = a + 1;
  *q1 = b;
  return 1;
}

static int font_buttons(Ctx *x, const char *rel, char **desc, size_t *len);

static void make_font(Ctx *x, const char *rel) {
  int ei = index_find(&x->ei, rel), gi = index_find(&x->gi, rel);
  if (ei < 0 || gi < 0)
    return; /* a font the English build or game.apk does not have: the game's stays */
  size_t n = 0;
  char *t = extract(&x->e, ei, &n);
  if (!t)
    return;
  /* first pass: every image it names must be in english.apk */
  int names = 0, ok = 1;
  for (const char *line = t; line && *line; line = strchr(line, '\n'), line = line ? line + 1 : NULL) {
    const char *a, *b;
    if (!image_line(line, &a, &b))
      continue;
    char name[128];
    snprintf(name, sizeof name, "%.*s", (int)(b - a), a);
    names++;
    if (strchr(name, '/') || strchr(name, '\\') || font_images(x, name) == 0) {
      LOG("[english] %s: english.apk has no picture for '%s' -- keeping the game's font\n", rel, name);
      ok = 0;
    }
  }
  if (!ok || !names) {
    free(t);
    return;
  }
  /* second pass: the descriptor with each image name + "_en" */
  char *o = malloc(n + (size_t)names * 3 + 1), *w = o;
  if (!o) {
    free(t);
    return;
  }
  const char *p = t;
  for (const char *line = t; line && *line; line = strchr(line, '\n'), line = line ? line + 1 : NULL) {
    const char *a, *b;
    if (!image_line(line, &a, &b))
      continue;
    memcpy(w, p, (size_t)(b - p));
    w += b - p;
    memcpy(w, "_en", 3);
    w += 3;
    p = b;
  }
  memcpy(w, p, (size_t)(t + n - p));
  w += t + n - p;
  size_t on = (size_t)(w - o);
  if (font_buttons(x, rel, &o, &on) == 0)
    x->n_icon_fonts++;
  else
    x->n_icon_fail++;
  if (put_file(x, rel, o, on) == 0)
    x->n_font++;
  free(o);
  free(t);
}

/* ---------------------------------------------------------------- atlases */
static int attr(const char *tag, const char *end, const char *name, char *out, size_t cap) {
  char pat[32];
  snprintf(pat, sizeof pat, " %s=\"", name);
  const char *p = tag;
  size_t pl = strlen(pat);
  for (; p + pl < end; p++)
    if (!strncmp(p, pat, pl))
      break;
  if (p + pl >= end)
    return 0;
  p += pl;
  const char *q = memchr(p, '"', (size_t)(end - p));
  if (!q || (size_t)(q - p) >= cap)
    return 0;
  memcpy(out, p, (size_t)(q - p));
  out[q - p] = 0;
  return 1;
}

/* <atlas base=".." path=".."> blocks and their <image name x y w h/> */
static Region *parse_atlas(const char *xml, int *count) {
  int cap = 64, n = 0;
  Region *v = malloc(sizeof *v * (size_t)cap);
  char tex[336] = "";
  for (const char *p = xml; v && (p = strchr(p, '<'));) {
    const char *end = strchr(p, '>');
    if (!end)
      break;
    char a[160], b[160], xs[16], ys[16], ws[16], hs[16];
    if (!strncmp(p, "<atlas ", 7) && attr(p, end, "base", a, sizeof a) &&
        attr(p, end, "path", b, sizeof b)) {
      snprintf(tex, sizeof tex, "%s%s.tex", a, b);
    } else if (!strncmp(p, "<image ", 7) && tex[0] && attr(p, end, "name", a, sizeof a) &&
               attr(p, end, "x", xs, sizeof xs) && attr(p, end, "y", ys, sizeof ys) &&
               attr(p, end, "w", ws, sizeof ws) && attr(p, end, "h", hs, sizeof hs)) {
      if (n == cap) {
        cap *= 2;
        Region *nv = realloc(v, sizeof *v * (size_t)cap);
        if (!nv) {
          free(v);
          return NULL;
        }
        v = nv;
      }
      Region *r = &v[n++];
      snprintf(r->name, sizeof r->name, "%s", a);
      snprintf(r->tex, sizeof r->tex, "%s", tex);
      r->x = atoi(xs), r->y = atoi(ys), r->w = atoi(ws), r->h = atoi(hs);
    }
    p = end + 1;
  }
  *count = n;
  return v;
}

/* A SEXYTEX texture ("SEXYTEX\0", width @12, height @16, format @20, a zlib
 * stream @48) of format 2, a8r8g8b8, as RGBA bytes. */
static uint8_t *tex_rgba(mz_zip_archive *z, int idx, int *w, int *h) {
  size_t n = 0;
  uint8_t *t = extract(z, idx, &n);
  if (!t || n < 48 || memcmp(t, "SEXYTEX\0", 8)) {
    free(t);
    return NULL;
  }
  uint32_t tw = t[12] | t[13] << 8 | t[14] << 16 | (uint32_t)t[15] << 24;
  uint32_t th = t[16] | t[17] << 8 | t[18] << 16 | (uint32_t)t[19] << 24;
  uint32_t fmt = t[20] | t[21] << 8 | t[22] << 16 | (uint32_t)t[23] << 24;
  if (fmt != 2 || !tw || !th || tw > 8192 || th > 8192) {
    free(t);
    return NULL;
  }
  mz_ulong len = (mz_ulong)tw * th * 4;
  uint8_t *px = malloc(len);
  if (!px || mz_uncompress(px, &len, t + 48, (mz_ulong)(n - 48)) != MZ_OK || len != (mz_ulong)tw * th * 4) {
    free(px);
    free(t);
    return NULL;
  }
  free(t);
  for (mz_ulong i = 0; i < len; i += 4) { /* B G R A -> R G B A */
    uint8_t b = px[i];
    px[i] = px[i + 2];
    px[i + 2] = b;
  }
  *w = (int)tw;
  *h = (int)th;
  return px;
}

typedef struct {
  char rel[336];
  uint8_t *px;
  int w, h;
} Tex;

static Tex *tex_get(Tex *cache, int ncache, mz_zip_archive *z, const Index *ix, const char *rel) {
  int free_slot = -1;
  for (int i = 0; i < ncache; i++) {
    if (cache[i].px && !strcasecmp(cache[i].rel, rel))
      return &cache[i];
    if (!cache[i].px && free_slot < 0)
      free_slot = i;
  }
  if (free_slot < 0) { /* evict the first */
    free(cache[0].px);
    cache[0].px = NULL;
    free_slot = 0;
  }
  Tex *t = &cache[free_slot];
  int idx = index_find(ix, rel);
  if (idx < 0)
    return NULL;
  t->px = tex_rgba(z, idx, &t->w, &t->h);
  if (!t->px)
    return NULL;
  snprintf(t->rel, sizeof t->rel, "%s", rel);
  return t;
}

static int in_tex(const Tex *t, const Region *r) {
  return r->x >= 0 && r->y >= 0 && r->w > 0 && r->h > 0 && r->x + r->w <= t->w && r->y + r->h <= t->h;
}

static int region_equal(const Tex *a, const Region *ra, const Tex *b, const Region *rb) {
  if (ra->w != rb->w || ra->h != rb->h)
    return 0;
  for (int y = 0; y < ra->h; y++)
    if (memcmp(a->px + ((size_t)(ra->y + y) * a->w + ra->x) * 4,
               b->px + ((size_t)(rb->y + y) * b->w + rb->x) * 4, (size_t)ra->w * 4))
      return 0;
  return 1;
}

/* width and height of a PNG in game.apk; 0 if it is not one */
static int png_size(mz_zip_archive *z, int idx, int *w, int *h) {
  size_t n = 0;
  uint8_t *p = extract(z, idx, &n);
  int ok = p && n >= 24 && !memcmp(p, "\x89PNG\r\n\x1a\n", 8) && !memcmp(p + 12, "IHDR", 4);
  if (ok) {
    *w = p[16] << 24 | p[17] << 16 | p[18] << 8 | p[19];
    *h = p[20] << 24 | p[21] << 16 | p[22] << 8 | p[23];
  }
  free(p);
  return ok;
}

static void make_atlas(Ctx *x, const char *xml_rel) {
  char *ex = extract(&x->e, index_find(&x->ei, xml_rel), NULL);
  char *cx = extract(&x->c, index_find(&x->ci, xml_rel), NULL);
  int ne = 0, nc = 0;
  Region *re = ex ? parse_atlas(ex, &ne) : NULL;
  Region *rc = cx ? parse_atlas(cx, &nc) : NULL;
  free(ex);
  free(cx);
  Tex te[2], tc[2];
  memset(te, 0, sizeof te);
  memset(tc, 0, sizeof tc);
  for (int i = 0; re && i < ne && !x->fail; i++) {
    const Region *r = &re[i];
    const char *slash = strrchr(r->tex, '/');
    char out[400];
    snprintf(out, sizeof out, "%.*s%s", slash ? (int)(slash + 1 - r->tex) : 0, r->tex, r->name);
    int gi = index_find(&x->gi, out);
    if (gi < 0 || made_has(x, out))
      continue; /* the game has no picture of that name, or it is done already */
    Tex *e = tex_get(te, 2, &x->e, &x->ei, r->tex);
    if (!e || !in_tex(e, r))
      continue;
    const Region *c = NULL;
    for (int k = 0; rc && k < nc && !c; k++)
      if (!strcmp(rc[k].name, r->name))
        c = &rc[k];
    if (c) {
      /* a texture the pak does not have is the English build's unchanged */
      Tex *ct = tex_get(tc, 2, &x->c, &x->ci, c->tex);
      if (!ct)
        ct = tex_get(tc, 2, &x->e, &x->ei, c->tex);
      if (ct && in_tex(ct, c) && region_equal(e, r, ct, c)) {
        x->n_same++;
        continue; /* no words on it */
      }
    }
    int gw = 0, gh = 0;
    if (!png_size(x->g, gi, &gw, &gh) || gw != r->w || gh != r->h) {
      LOG("[english] %s: game.apk's picture is %dx%d, the English one %dx%d -- kept\n", out, gw, gh,
          r->w, r->h);
      continue;
    }
    uint8_t *crop = malloc((size_t)r->w * r->h * 4);
    if (!crop)
      break;
    for (int y = 0; y < r->h; y++)
      memcpy(crop + (size_t)y * r->w * 4, e->px + ((size_t)(r->y + y) * e->w + r->x) * 4,
             (size_t)r->w * 4);
    size_t png_len = 0;
    void *png = tdefl_write_image_to_png_file_in_memory_ex(crop, r->w, r->h, 4, &png_len, 6, MZ_FALSE);
    free(crop);
    if (png && put_file(x, out, png, png_len) == 0)
      x->n_crop++;
    mz_free(png);
  }
  for (int i = 0; i < 2; i++) {
    free(te[i].px);
    free(tc[i].px);
  }
  free(re);
  free(rc);
}

/* ------------------------------------------------- the Zombatar back slab */
/* The mod's Zombatar screen (a night graveyard of its own, which the English
 * build does not have) has "返回" (back) carved on the slab its back button
 * lights: in the background and in the lit copy drawn over it. Here both get
 * the English build's "BACK" instead, made from the two APKs: the Chinese
 * carving is filled with the slab's own shading (a quadratic fit of the stone
 * around it, with its grain), and the English stone's lettering is laid on,
 * slanted as the slab lies -- black in the background, lit in the copy. */
#define ZB_BG "addonFiles/images/ZombatarWidget/zombatar_main_bg.png"
#define ZB_HL "addonFiles/images/ZombatarWidget/zombatar_mainmenuback_highlight.png"
#define ZB_X 471 /* where the lit copy goes on the background */
#define ZB_Y 629

static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

static int paeth(int a, int b, int c) {
  int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
  return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

/* 8-bit RGBA or RGB, not interlaced -> RGBA (free) */
static uint8_t *png_rgba(const uint8_t *p, size_t n, int *w, int *h) {
  if (n < 33 || memcmp(p, "\x89PNG\r\n\x1a\n", 8))
    return NULL;
  int width = 0, height = 0, ch = 0;
  uint8_t *z = NULL;
  size_t zn = 0;
  for (size_t o = 8; o + 12 <= n;) {
    uint32_t len = be32(p + o);
    const uint8_t *type = p + o + 4, *d = p + o + 8;
    if (o + 12 + len > n)
      break;
    if (!memcmp(type, "IHDR", 4)) {
      width = (int)be32(d);
      height = (int)be32(d + 4);
      ch = d[9] == 6 ? 4 : d[9] == 2 ? 3 : 0;
      if (d[8] != 8 || !ch || d[12] != 0 || width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        free(z);
        return NULL;
      }
    } else if (!memcmp(type, "IDAT", 4)) {
      uint8_t *nz = realloc(z, zn + len);
      if (!nz) {
        free(z);
        return NULL;
      }
      z = nz;
      memcpy(z + zn, d, len);
      zn += len;
    }
    o += 12 + len;
  }
  const size_t stride = (size_t)width * ch;
  mz_ulong raw_n = (mz_ulong)((stride + 1) * (size_t)height);
  uint8_t *raw = z && ch ? malloc(raw_n) : NULL;
  if (raw && mz_uncompress(raw, &raw_n, z, (mz_ulong)zn) != MZ_OK)
    raw_n = 0;
  free(z);
  if (!raw || raw_n < (stride + 1) * (size_t)height) {
    free(raw);
    return NULL;
  }
  uint8_t *line = malloc(stride * 2), *out = malloc((size_t)width * height * 4);
  if (!line || !out) {
    free(line);
    free(out);
    free(raw);
    return NULL;
  }
  uint8_t *prev = line, *cur = line + stride;
  memset(prev, 0, stride);
  for (int y = 0; y < height; y++) {
    const uint8_t *src = raw + y * (stride + 1) + 1;
    const int f = raw[y * (stride + 1)];
    for (size_t i = 0; i < stride; i++) {
      int a = i >= (size_t)ch ? cur[i - ch] : 0, b = prev[i], c = i >= (size_t)ch ? prev[i - ch] : 0;
      int v = src[i];
      switch (f) {
      case 1: v += a; break;
      case 2: v += b; break;
      case 3: v += (a + b) / 2; break;
      case 4: v += paeth(a, b, c); break;
      default: break;
      }
      cur[i] = (uint8_t)v;
    }
    uint8_t *d = out + (size_t)y * width * 4;
    for (int x0 = 0; x0 < width; x0++) {
      d[x0 * 4 + 0] = cur[x0 * ch + 0];
      d[x0 * 4 + 1] = cur[x0 * ch + 1];
      d[x0 * 4 + 2] = cur[x0 * ch + 2];
      d[x0 * 4 + 3] = ch == 4 ? cur[x0 * ch + 3] : 255;
    }
    uint8_t *t = prev;
    prev = cur;
    cur = t;
  }
  free(line);
  free(raw);
  *w = width;
  *h = height;
  return out;
}

static uint8_t *zip_png(mz_zip_archive *z, const Index *ix, const char *rel, int *w, int *h) {
  size_t n = 0;
  uint8_t *p = extract(z, index_find(ix, rel), &n);
  uint8_t *px = p ? png_rgba(p, n, w, h) : NULL;
  free(p);
  return px;
}

/* a w x h byte mask grown by r (square) */
static void mask_grow(const uint8_t *in, uint8_t *out, int w, int h, int r) {
  for (int y = 0; y < h; y++)
    for (int x0 = 0; x0 < w; x0++) {
      int v = 0;
      for (int dy = -r; dy <= r && !v; dy++)
        for (int dx = -r; dx <= r && !v; dx++) {
          int yy = y + dy, xx = x0 + dx;
          v = yy >= 0 && yy < h && xx >= 0 && xx < w && in[yy * w + xx];
        }
      out[y * w + x0] = (uint8_t)v;
    }
}

/* the English stone's "BACK": its dark carving, closed (the bevel's grey
 * specks are ink too), without the stone's crack; cut to its box */
static float *zb_letters(const uint8_t *en, int ew, int *lw, int *lh) {
  enum { X0 = 500, Y0 = 652, W = 190, H = 40 };
  if (ew < X0 + W)
    return NULL;
  uint8_t m[W * H], t[W * H], keep[W * H];
  for (int y = 0; y < H; y++)
    for (int x0 = 0; x0 < W; x0++) {
      const uint8_t *p = en + ((size_t)(Y0 + y) * ew + X0 + x0) * 4;
      m[y * W + x0] = p[0] + p[1] + p[2] < 200;
    }
  mask_grow(m, t, W, H, 1); /* closing: grow, then shrink */
  for (int y = 0; y < H; y++)
    for (int x0 = 0; x0 < W; x0++) {
      int v = 1;
      for (int dy = -1; dy <= 1 && v; dy++)
        for (int dx = -1; dx <= 1 && v; dx++) {
          int yy = y + dy, xx = x0 + dx;
          v = yy < 0 || yy >= H || xx < 0 || xx >= W || t[yy * W + xx];
        }
      m[y * W + x0] = (uint8_t)v;
    }
  static int stack[W * H];
  memset(keep, 0, sizeof keep);
  memset(t, 0, sizeof t); /* seen */
  for (int i = 0; i < W * H; i++) {
    if (!m[i] || t[i])
      continue;
    int sp = 0, n = 0;
    stack[sp++] = i;
    t[i] = 1;
    static int part[W * H];
    while (sp) {
      int k = stack[--sp];
      part[n++] = k;
      const int ky = k / W, kx = k % W;
      const int nb[4] = {ky > 0 ? k - W : -1, ky < H - 1 ? k + W : -1, kx > 0 ? k - 1 : -1, kx < W - 1 ? k + 1 : -1};
      for (int j = 0; j < 4; j++)
        if (nb[j] >= 0 && m[nb[j]] && !t[nb[j]]) {
          t[nb[j]] = 1;
          stack[sp++] = nb[j];
        }
    }
    if (n >= 150)
      for (int j = 0; j < n; j++)
        keep[part[j]] = 1;
  }
  int bx0 = W, by0 = H, bx1 = -1, by1 = -1;
  for (int y = 0; y < H; y++)
    for (int x0 = 0; x0 < W; x0++)
      if (keep[y * W + x0]) {
        bx0 = x0 < bx0 ? x0 : bx0;
        bx1 = x0 > bx1 ? x0 : bx1;
        by0 = y < by0 ? y : by0;
        by1 = y > by1 ? y : by1;
      }
  if (bx1 < 0)
    return NULL;
  *lw = bx1 - bx0 + 1;
  *lh = by1 - by0 + 1;
  float *out = malloc(sizeof *out * (size_t)*lw * *lh);
  for (int y = 0; out && y < *lh; y++)
    for (int x0 = 0; x0 < *lw; x0++)
      out[y * *lw + x0] = keep[(by0 + y) * W + bx0 + x0];
  return out;
}

static float bilinear(const float *m, int w, int h, float x, float y) {
  if (x < 0 || y < 0 || x > w - 1 || y > h - 1)
    return 0;
  int x0 = (int)x, y0 = (int)y, x1 = x0 + 1 < w ? x0 + 1 : x0, y1 = y0 + 1 < h ? y0 + 1 : y0;
  float fx = x - x0, fy = y - y0;
  return (m[y0 * w + x0] * (1 - fx) + m[y0 * w + x1] * fx) * (1 - fy) +
         (m[y1 * w + x0] * (1 - fx) + m[y1 * w + x1] * fx) * fy;
}

/* the normal equations of a quadratic fit (1 x y xx yy xy | sum), solved in
 * place: 0 if singular */
static int solve6(double ata[6][7], double coef[6]) {
  for (int i = 0; i < 6; i++) { /* Gauss-Jordan with partial pivoting */
    int piv = i;
    for (int r = i + 1; r < 6; r++)
      if (fabs(ata[r][i]) > fabs(ata[piv][i]))
        piv = r;
    for (int k = 0; k < 7; k++) {
      double tmp = ata[i][k];
      ata[i][k] = ata[piv][k];
      ata[piv][k] = tmp;
    }
    if (fabs(ata[i][i]) < 1e-12)
      return 0;
    for (int r = 0; r < 6; r++)
      if (r != i) {
        double f = ata[r][i] / ata[i][i];
        for (int k = i; k < 7; k++)
          ata[r][k] -= f * ata[i][k];
      }
  }
  for (int i = 0; i < 6; i++)
    coef[i] = ata[i][6] / ata[i][i];
  return 1;
}

/* hole's pixels (in a w-wide RGBA picture, area box) given a quadratic fit
 * of the grey stone around them, with its grain */
static void zb_refill(uint8_t *px, int w, int h, const uint8_t *hole, const int box[4], uint32_t seed) {
  double ata[6][7];
  for (int c = 0; c < 3; c++) {
    memset(ata, 0, sizeof ata);
    double ss = 0;
    int n = 0;
    for (int y = box[1]; y < box[3]; y++)
      for (int x0 = box[0]; x0 < box[2]; x0++) {
        const uint8_t *p = px + ((size_t)y * w + x0) * 4;
        int sum = p[0] + p[1] + p[2];
        if (hole[y * w + x0] || abs(p[0] - p[2]) >= 25 || sum <= 240 || sum >= 600)
          continue;
        const double t[6] = {1, x0, y, (double)x0 * x0, (double)y * y, (double)x0 * y};
        for (int i = 0; i < 6; i++) {
          for (int j = 0; j < 6; j++)
            ata[i][j] += t[i] * t[j];
          ata[i][6] += t[i] * p[c];
        }
        n++;
      }
    double coef[6];
    if (n < 12 || !solve6(ata, coef))
      return;
    for (int y = box[1]; y < box[3]; y++) /* the grain: the fit's residual */
      for (int x0 = box[0]; x0 < box[2]; x0++) {
        const uint8_t *p = px + ((size_t)y * w + x0) * 4;
        int sum = p[0] + p[1] + p[2];
        if (hole[y * w + x0] || abs(p[0] - p[2]) >= 25 || sum <= 240 || sum >= 600)
          continue;
        double f = coef[0] + coef[1] * x0 + coef[2] * y + coef[3] * x0 * x0 + coef[4] * y * y + coef[5] * x0 * y;
        ss += (p[c] - f) * (p[c] - f);
      }
    const double sd = sqrt(ss / n) * 0.6;
    for (int y = 0; y < h; y++)
      for (int x0 = 0; x0 < w; x0++) {
        if (!hole[y * w + x0])
          continue;
        seed = seed * 1664525u + 1013904223u;
        double u1 = ((seed >> 8) + 1) / 16777217.0;
        seed = seed * 1664525u + 1013904223u;
        double u2 = (seed >> 8) / 16777216.0;
        double noise = sqrt(-2 * log(u1)) * cos(6.283185307 * u2) * sd;
        double f = coef[0] + coef[1] * x0 + coef[2] * y + coef[3] * x0 * x0 + coef[4] * y * y + coef[5] * x0 * y + noise;
        px[((size_t)y * w + x0) * 4 + c] = (uint8_t)(f < 0 ? 0 : f > 255 ? 255 : f);
      }
  }
}

static void zb_put_png(Ctx *x, const char *rel, const uint8_t *px, int w, int h) {
  size_t len = 0;
  void *png = tdefl_write_image_to_png_file_in_memory_ex(px, w, h, 4, &len, 6, MZ_FALSE);
  if (png)
    put_file(x, rel, png, len);
  mz_free(png);
}

/* ---------------------------------------------------------------- buttons */
/* The Switch's buttons in the text as pictures (hardware run 14: the game's
 * tags came out as written). The console editions put them in their fonts as
 * glyphs; of the English fonts only HouseofTerror20 has any (Xbox ones, at
 * letters other fonts use as letters). So every English font gets a layer of
 * its own with the port's pictures (resources/buttons/icons.png) at U+E000 on
 * (pvz_button_icons.h), scaled to the font: the engine's ImageFont takes a
 * character's advance as the widest of its layers' and draws each layer's
 * glyph, so wrapping, centring and measuring all count them. The layer adds
 * white to the text's colour (LayerSetColorAdd, alpha 0) so the pictures keep
 * their own colours and only take the text's transparency; the font's other
 * layers get the same widths and no picture. The text gets the characters
 * only if every font got the layer (make_strings). */
#include "pvz_button_icons.h"

/* src (an RGBA row sw_total wide) at sx..sx+sw, sh high, into dw x dh at dst
 * (stride dstride bytes): area average over premultiplied alpha */
static void icon_scale(const uint8_t *src, int sw_total, int sx, int sw, int sh, uint8_t *dst,
                       size_t dstride, int dw, int dh) {
  for (int y = 0; y < dh; y++) {
    const double y0 = (double)y * sh / dh, y1 = (double)(y + 1) * sh / dh;
    for (int xx = 0; xx < dw; xx++) {
      const double x0 = sx + (double)xx * sw / dw, x1 = sx + (double)(xx + 1) * sw / dw;
      double r = 0, g = 0, b = 0, a = 0, area = 0;
      for (int ys = (int)y0; ys < sh && ys < y1; ys++) {
        const double fy = fmin(y1, ys + 1) - fmax(y0, ys);
        for (int xs = (int)x0; xs < sx + sw && xs < x1; xs++) {
          const double f = fy * (fmin(x1, xs + 1) - fmax(x0, xs));
          const uint8_t *q = src + ((size_t)ys * sw_total + xs) * 4;
          const double qa = q[3] / 255.0 * f;
          r += q[0] * qa, g += q[1] * qa, b += q[2] * qa, a += qa, area += f;
        }
      }
      uint8_t *d = dst + (size_t)y * dstride + (size_t)xx * 4;
      if (a > 0) {
        d[0] = (uint8_t)fmin(255, r / a + 0.5);
        d[1] = (uint8_t)fmin(255, g / a + 0.5);
        d[2] = (uint8_t)fmin(255, b / a + 0.5);
      }
      d[3] = area > 0 ? (uint8_t)fmin(255, a / area * 255 + 0.5) : 0;
    }
  }
}

/* ------------------------------------------------- the console's buttons */
/* The game APK carries the Xbox 360 edition's own button pictures: the help
 * bar's sheets, images/help_buttons.png (13 cels of 42 pixels: A B X Y Start
 * RB LB D-pad RT LT Back RS LS) and help_buttons_small.png (21 pixels). They
 * are what the port shows -- in the help bar, and in the text as the fonts'
 * button layer -- with the Switch's names on the six the Switch calls
 * otherwise: the bumpers say R and L (their own R and L, the B wiped), the
 * triggers ZR and ZL (their own R and L moved over, beside a Z in the same
 * one-pixel stroke), Start and Back a + and a - in the arrows' own shading.
 * The steps below were measured on exactly these pictures (CRC): any other
 * sheet, and the port's own drawings are used (resources/buttons/). The small
 * sheet's changed cels are the big ones halved, as the game's own are. */
#define X360_W 546
#define X360_CEL 42
#define X360_BIG_CRC 0xc85bb103u
#define X360_SMALL_CRC 0xb3d002f0u
#define X360_NOISE 14 /* a moved letter's difference from its face below this is the face's */

static uint8_t *x360_px(uint8_t *a, int x, int y) { return a + ((size_t)y * X360_W + (size_t)x) * 4; }

static uint8_t clamp8(long v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); }

/* The box x0..x1-1, y0..y1-1 of cel b in a, as its face would be without the
 * letters: each edge (the pixels just outside the box) blended in, the corners
 * out (a Coons patch). Into bg (RGB, the box's size). */
static void x360_face(uint8_t *a, int b, int x0, int y0, int x1, int y1, int *bg) {
  const double w = x1 - x0 + 1, h = y1 - y0 + 1;
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) {
      const double u = (x - x0 + 1) / w, v = (y - y0 + 1) / h;
      for (int c = 0; c < 3; c++) {
        const double l = x360_px(a, b + x0 - 1, y)[c], r = x360_px(a, b + x1, y)[c];
        const double t = x360_px(a, b + x, y0 - 1)[c], d = x360_px(a, b + x, y1)[c];
        const double c00 = x360_px(a, b + x0 - 1, y0 - 1)[c], c10 = x360_px(a, b + x1, y0 - 1)[c];
        const double c01 = x360_px(a, b + x0 - 1, y1)[c], c11 = x360_px(a, b + x1, y1)[c];
        const double p = (1 - u) * l + u * r + (1 - v) * t + v * d -
                         ((1 - u) * (1 - v) * c00 + u * (1 - v) * c10 + (1 - u) * v * c01 + u * v * c11);
        bg[((y - y0) * (x1 - x0) + (x - x0)) * 3 + c] = clamp8(lround(p));
      }
    }
}

/* the same box as rows of one colour each: the face at column from_x */
static void x360_rows(uint8_t *a, int b, int x0, int y0, int x1, int y1, int from_x, int *bg) {
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++)
      for (int c = 0; c < 3; c++)
        bg[((y - y0) * (x1 - x0) + (x - x0)) * 3 + c] = x360_px(a, b + from_x, y)[c];
}

static void x360_put_box(uint8_t *img, int b, int x0, int y0, int x1, int y1, const int *bg) {
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++)
      for (int c = 0; c < 3; c++)
        x360_px(img, b + x, y)[c] = (uint8_t)bg[((y - y0) * (x1 - x0) + (x - x0)) * 3 + c];
}

/* The letter in columns l0..l1-1 of the box (its difference from the face,
 * dark strokes and light bevel alike) added dx to the right, but the pixels
 * in skip (cel x, y pairs, ending -1). */
static void x360_move(uint8_t *img, const uint8_t *src, int b, int x0, int y0, int x1, int y1, const int *bg,
                      int l0, int l1, int dx, const int *skip) {
  for (int y = y0; y < y1; y++)
    for (int x = l0; x < l1; x++) {
      int skipped = 0;
      for (const int *s = skip; s && s[0] >= 0; s += 2)
        skipped |= s[0] == x && s[1] == y;
      int d[3], big = 0;
      for (int c = 0; c < 3; c++) {
        d[c] = x360_px((uint8_t *)src, b + x, y)[c] - bg[((y - y0) * (x1 - x0) + (x - x0)) * 3 + c];
        big |= abs(d[c]) >= X360_NOISE;
      }
      if (skipped || !big)
        continue;
      uint8_t *p = x360_px(img, b + x + dx, y);
      for (int c = 0; c < 3; c++)
        p[c] = clamp8(p[c] + d[c]);
    }
}

/* the triggers' Z: 6 x 9, the letters' one-pixel stroke, darkness in quarters */
static const uint8_t k_x360_z[9][6] = {
    {4, 4, 4, 4, 4, 4}, {0, 0, 0, 1, 4, 2}, {0, 0, 0, 3, 3, 0}, {0, 0, 1, 4, 1, 0}, {0, 0, 3, 3, 0, 0},
    {0, 1, 4, 1, 0, 0}, {0, 3, 3, 0, 0, 0}, {2, 4, 1, 0, 0, 0}, {4, 4, 4, 4, 4, 4},
};

/* + and - in the arrows' shading: their face, rising by row from 130 to 221,
 * a darker outline (103 to 135), and a softer ring inside it */
static int x360_fill(int y) { return (int)lround(130 + (y - 10) * (221 - 130) / 16.0); }
static int x360_edge(int y) { return (int)lround(103 + (y - 10) * (135 - 103) / 17.0); }

static int x360_in(const int (*r)[4], int n, int x, int y) {
  for (int i = 0; i < n; i++)
    if (x >= r[i][0] && x < r[i][2] && y >= r[i][1] && y < r[i][3])
      return 1;
  return 0;
}

static void x360_symbol(uint8_t *img, int b, const int (*r)[4], int n) {
  for (int y = 1; y < X360_CEL - 2; y++)
    for (int x = 2; x < X360_CEL - 2; x++) {
      if (!x360_in(r, n, x, y))
        continue;
      const int edge = !x360_in(r, n, x + 1, y) || !x360_in(r, n, x - 1, y) || !x360_in(r, n, x, y + 1) ||
                       !x360_in(r, n, x, y - 1);
      const int ring = !x360_in(r, n, x + 1, y + 1) || !x360_in(r, n, x - 1, y - 1) ||
                       !x360_in(r, n, x + 1, y - 1) || !x360_in(r, n, x - 1, y + 1) ||
                       !x360_in(r, n, x + 2, y) || !x360_in(r, n, x - 2, y) || !x360_in(r, n, x, y + 2) ||
                       !x360_in(r, n, x, y - 2);
      const int v = edge ? x360_edge(y) : ring ? (x360_edge(y) + x360_fill(y)) / 2 + 8 : x360_fill(y);
      uint8_t *p = x360_px(img, b + x, y);
      p[0] = p[1] = p[2] = clamp8(v);
    }
}

static void x360_relabel(uint8_t *img, const uint8_t *src) {
  int bg[42 * 42 * 3];
  /* the bumpers: RB -> R (cel 5), LB -> L (cel 6) */
  static const int k_bump[2][8] = {{5, 10, 12, 29, 27, 10, 19, 5}, {6, 15, 12, 34, 27, 15, 23, 5}};
  for (int i = 0; i < 2; i++) {
    const int *k = k_bump[i], b = k[0] * X360_CEL;
    x360_face((uint8_t *)src, b, k[1], k[2], k[3], k[4], bg);
    x360_put_box(img, b, k[1], k[2], k[3], k[4], bg);
    x360_move(img, src, b, k[1], k[2], k[3], k[4], bg, k[5], k[6], k[7], NULL);
  }
  /* the triggers: RT -> ZR (cel 8), LT -> ZL (cel 9): the face's left column
   * across, the R / L six to the right (LT's T touches the L at one corner),
   * the Z where the R / L was */
  static const int k_trig[2][9] = {{8, 14, 15, 25, 28, 34, 15, 21, 15}, {9, 15, 16, 24, 29, 33, 16, 22, 16}};
  static const int k_skip_lt[] = {21, 24, 21, 25, -1};
  for (int i = 0; i < 2; i++) {
    const int *k = k_trig[i], b = k[0] * X360_CEL;
    x360_rows((uint8_t *)src, b, k[2], k[3], k[4], k[5], k[1], bg);
    x360_put_box(img, b, k[2], k[3], k[4], k[5], bg);
    x360_move(img, src, b, k[2], k[3], k[4], k[5], bg, k[6], k[7], 6, i ? k_skip_lt : NULL);
    for (int y = 0; y < 9; y++)
      for (int x = 0; x < 6; x++) {
        uint8_t *p = x360_px(img, b + k[8] + x, k[3] + y);
        for (int c = 0; c < 3; c++)
          p[c] = clamp8(p[c] - k_x360_z[y][x] * 180 / 4);
      }
  }
  /* Start -> + (cel 4), Back -> - (cel 10): the arrow gone (its face is one
   * colour a row), the symbol drawn */
  static const int k_plus[2][4] = {{11, 15, 31, 21}, {18, 8, 24, 28}}, k_minus[1][4] = {{11, 15, 31, 21}};
  x360_rows(img, 4 * X360_CEL, 12, 7, 36, 30, 10, bg);
  x360_put_box(img, 4 * X360_CEL, 12, 7, 36, 30, bg);
  x360_symbol(img, 4 * X360_CEL, k_plus, 2);
  x360_rows(img, 10 * X360_CEL, 5, 7, 30, 30, 32, bg);
  x360_put_box(img, 10 * X360_CEL, 5, 7, 30, 30, bg);
  x360_symbol(img, 10 * X360_CEL, k_minus, 1);
}

/* the game's sheets, relabelled into x->x360_big / x360_small: 1 when they
 * are the ones these steps were made for */
static int x360_buttons(Ctx *x) {
  const int bi = index_find(&x->gi, "images/help_buttons.png");
  const int si = index_find(&x->gi, "images/help_buttons_small.png");
  if (bi < 0 || si < 0 || entry_crc(x->g, bi) != X360_BIG_CRC || entry_crc(x->g, si) != X360_SMALL_CRC)
    return 0;
  size_t bl = 0, sl = 0;
  uint8_t *bp = extract(x->g, bi, &bl), *sp = extract(x->g, si, &sl);
  int bw = 0, bh = 0, sw = 0, sh = 0;
  uint8_t *src = bp ? png_rgba(bp, bl, &bw, &bh) : NULL, *small = sp ? png_rgba(sp, sl, &sw, &sh) : NULL;
  free(bp);
  free(sp);
  uint8_t *img = src ? malloc((size_t)bw * bh * 4) : NULL;
  if (!src || !small || !img || bw != X360_W || bh != X360_CEL || sw != X360_W / 2 || sh != X360_CEL / 2) {
    free(src);
    free(small);
    free(img);
    return 0;
  }
  memcpy(img, src, (size_t)bw * bh * 4);
  x360_relabel(img, src);
  free(src);
  /* the small sheet's changed cels: the big ones halved (premultiplied) */
  static const int k_changed[] = {4, 5, 6, 8, 9, 10};
  for (unsigned k = 0; k < sizeof k_changed / sizeof k_changed[0]; k++)
    for (int y = 0; y < sh; y++)
      for (int xx = 0; xx < X360_CEL / 2; xx++) {
        const int bx = k_changed[k] * X360_CEL + xx * 2, sx = k_changed[k] * (X360_CEL / 2) + xx;
        double r = 0, g = 0, b = 0, a = 0;
        for (int dy = 0; dy < 2; dy++)
          for (int dx = 0; dx < 2; dx++) {
            const uint8_t *q = x360_px(img, bx + dx, y * 2 + dy);
            const double qa = q[3] / 255.0;
            r += q[0] * qa, g += q[1] * qa, b += q[2] * qa, a += qa;
          }
        uint8_t *d = small + ((size_t)y * sw + (size_t)sx) * 4;
        d[0] = a > 0 ? clamp8(lround(r / a)) : 0;
        d[1] = a > 0 ? clamp8(lround(g / a)) : 0;
        d[2] = a > 0 ? clamp8(lround(b / a)) : 0;
        d[3] = clamp8(lround(a / 4 * 255));
      }
  x->x360_big = img;
  x->x360_small = small;
  return 1;
}

/* The buttons for the fonts, once: the game's own relabelled (x360_buttons),
 * else the port's drawings. Each in its own box of x->icons (a row of
 * x->icons_h pixels), in PVZ_ICON_* order. 1 when there are pictures. */
static int buttons_load(Ctx *x) {
  if (x->icons_tried)
    return x->icons != NULL;
  x->icons_tried = 1;
  if (x360_buttons(x)) {
    /* PVZ_ICON_* order: A B X Y + - L R ZL ZR LS RS D-pad, from the cels A B X
     * Y Start RB LB D-pad RT LT Back RS LS; all one height (the rows any cel
     * uses), each as wide as it is */
    static const int k_cel[PVZ_ICON_COUNT] = {0, 1, 2, 3, 4, 10, 6, 5, 9, 8, 12, 11, 7};
    int top = X360_CEL, bot = -1, x0[PVZ_ICON_COUNT], x1[PVZ_ICON_COUNT], wsum = 0;
    for (int i = 0; i < PVZ_ICON_COUNT; i++) {
      x0[i] = X360_CEL, x1[i] = -1;
      for (int y = 0; y < X360_CEL; y++)
        for (int xx = 0; xx < X360_CEL; xx++)
          if (x360_px(x->x360_big, k_cel[i] * X360_CEL + xx, y)[3]) {
            top = y < top ? y : top, bot = y > bot ? y : bot;
            x0[i] = xx < x0[i] ? xx : x0[i], x1[i] = xx > x1[i] ? xx : x1[i];
          }
      if (x1[i] < x0[i])
        x0[i] = 0, x1[i] = 0;
      wsum += x1[i] - x0[i] + 1;
    }
    const int h = bot - top + 1;
    x->icons = calloc((size_t)wsum * h, 4);
    if (x->icons) {
      int at = 0;
      for (int i = 0; i < PVZ_ICON_COUNT; i++) {
        const int w = x1[i] - x0[i] + 1;
        for (int y = 0; y < h; y++)
          memcpy(x->icons + ((size_t)y * wsum + at) * 4, x360_px(x->x360_big, k_cel[i] * X360_CEL + x0[i], top + y),
                 (size_t)w * 4);
        x->ic_x[i] = at, x->ic_w[i] = w;
        at += w;
      }
      x->icons_w = wsum, x->icons_h = h;
      LOG("[english] buttons: the game's own (the Xbox 360 edition's), with the Switch's names\n");
      return 1;
    }
  }
  const PvzEnglishRes *res = &x->cfg->res;
  if (res->icons_png && res->icons_len)
    x->icons = png_rgba(res->icons_png, res->icons_len, &x->icons_w, &x->icons_h);
  if (!x->icons || x->icons_h != PVZ_ICON_H) {
    free(x->icons);
    x->icons = NULL;
    LOG("[english] the buttons' pictures could not be read: the text names them\n");
    return 0;
  }
  for (int i = 0; i < PVZ_ICON_COUNT; i++)
    x->ic_x[i] = k_pvz_icons[i].x, x->ic_w[i] = k_pvz_icons[i].w;
  LOG("[english] buttons: this port's drawings (the game's sheet is not the one they are made from)\n");
  return 1;
}

/* the integer after "<cmd> <layer>" on its line (cmd at a line start); the
 * largest if several; -1 if none. layer_out (if given) gets that line's layer. */
static int desc_value(const char *t, const char *cmd, char *layer_out, size_t cap) {
  int best = -1;
  const size_t cl = strlen(cmd);
  for (const char *line = t; line && *line; line = strchr(line, '\n'), line = line ? line + 1 : NULL) {
    const char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (strncmp(p, cmd, cl) || (p[cl] != ' ' && p[cl] != '\t'))
      continue;
    p += cl;
    while (*p == ' ' || *p == '\t')
      p++;
    const char *l0 = p;
    while (*p && *p != ' ' && *p != '\t' && *p != ';' && *p != '\n')
      p++;
    const char *l1 = p;
    while (*p == ' ' || *p == '\t')
      p++;
    if (!((*p >= '0' && *p <= '9') || *p == '-'))
      continue;
    const int v = atoi(p);
    if (v > best) {
      best = v;
      if (layer_out)
        snprintf(layer_out, cap, "%.*s", (int)(l1 - l0), l0);
    }
  }
  return best;
}

static void put_utf8_icon(char *out, int i) { /* U+E000 + i */
  const unsigned cp = 0xE000u + (unsigned)i;
  out[0] = (char)(0xE0 | (cp >> 12));
  out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[2] = (char)(0x80 | (cp & 0x3F));
  out[3] = 0;
}

/* The buttons' layer for the font rel (data/NAME.txt): its picture written
 * as data/NAME_buttons_en.png, the descriptor *desc (*len bytes, malloc'd)
 * extended. 0 done; -1 not (the font stays as it was). */
static int font_buttons(Ctx *x, const char *rel, char **desc, size_t *len) {
  if (x->cfg->no_button_pictures || !buttons_load(x))
    return -1;
  char *t = malloc(*len + 1);
  if (!t)
    return -1;
  memcpy(t, *desc, *len);
  t[*len] = 0;
  char top[64] = "";
  const int ascent = desc_value(t, "LayerSetAscent", top, sizeof top);
  int point = desc_value(t, "LayerSetPointSize", NULL, 0);
  if (point <= 0)
    point = desc_value(t, "SetDefaultPointSize", NULL, 0);
  /* its layers */
  char layers[8][64];
  int nl = 0;
  for (const char *line = t; line && *line && nl < 8; line = strchr(line, '\n'), line = line ? line + 1 : NULL) {
    const char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (strncmp(p, "CreateLayer", 11) || (p[11] != ' ' && p[11] != '\t'))
      continue;
    p += 11;
    while (*p == ' ' || *p == '\t')
      p++;
    const char *q = p;
    while (*q && *q != ' ' && *q != '\t' && *q != ';' && *q != '\r' && *q != '\n')
      q++;
    if (q > p && q - p < 63)
      snprintf(layers[nl++], sizeof layers[0], "%.*s", (int)(q - p), p);
  }
  free(t);
  if (ascent < 6 || !nl) {
    LOG("[english] %s: no layer ascent -- its text names the buttons\n", rel);
    return -1;
  }

  /* the pictures: about the capitals' height, centred on them */
  const int size = (int)lround(ascent * 0.9), top_off = (int)lround(ascent * 0.67 - size / 2.0);
  int wsum = 0, wi[PVZ_ICON_COUNT];
  for (int i = 0; i < PVZ_ICON_COUNT; i++) {
    wi[i] = (int)lround((double)x->ic_w[i] * size / x->icons_h);
    if (wi[i] < 1)
      wi[i] = 1;
    wsum += wi[i] + 2;
  }
  uint8_t *strip = calloc((size_t)wsum * size, 4);
  if (!strip)
    return -1;
  char base[160];
  const char *slash = strrchr(rel, '/');
  snprintf(base, sizeof base, "%s", slash ? slash + 1 : rel);
  char *dot = strrchr(base, '.');
  if (dot)
    *dot = 0;

  /* The engine reads a descriptor as UTF-8 only after a byte-order mark
   * (Sexy::EncodingParser), else a byte a character: english.apk's
   * HouseofTerror28 is Latin-1, and the buttons' characters written into it
   * as UTF-8 came out as three each -- the font failed to load and the game
   * closed (hardware run 15). So such a descriptor becomes UTF-8 first. */
  if (*len < 3 || memcmp(*desc, "\xEF\xBB\xBF", 3) != 0) {
    char *u = malloc(*len * 2 + 4);
    if (!u) {
      free(strip);
      return -1;
    }
    size_t un = 0;
    memcpy(u, "\xEF\xBB\xBF", 3);
    un = 3;
    for (size_t i = 0; i < *len; i++) {
      const unsigned char b = (unsigned char)(*desc)[i];
      if (b < 0x80) {
        u[un++] = (char)b;
      } else {
        u[un++] = (char)(0xC0 | (b >> 6));
        u[un++] = (char)(0x80 | (b & 0x3F));
      }
    }
    free(*desc);
    *desc = u;
    *len = un;
  }
  /* the descriptor's lines */
  const int crlf = memmem(*desc, *len, "\r\n", 2) != NULL;
  const char *nlc = crlf ? "\r\n" : "\n";
  size_t cap = *len + 4096 + (size_t)nl * 128;
  char *o = malloc(cap);
  if (!o) {
    free(strip);
    return -1;
  }
  memcpy(o, *desc, *len);
  size_t n = *len;
#define ADD(...) (n += (size_t)snprintf(o + n, cap - n, __VA_ARGS__))
  ADD("%s%sDefine SwitchButtonChars%s (", nlc, nlc, nlc);
  for (int i = 0; i < PVZ_ICON_COUNT; i++) {
    char u[4];
    put_utf8_icon(u, i);
    ADD("%s'%s'", i ? ", " : " ", u);
  }
  ADD(");%s%sDefine SwitchButtonWidths%s (", nlc, nlc, nlc);
  for (int i = 0; i < PVZ_ICON_COUNT; i++)
    ADD("%s%d", i ? ", " : " ", wi[i] + 2);
  ADD(");%s%sDefine SwitchButtonRects%s (", nlc, nlc, nlc);
  int xs = 0;
  for (int i = 0; i < PVZ_ICON_COUNT; i++) {
    icon_scale(x->icons, x->icons_w, x->ic_x[i], x->ic_w[i], x->icons_h,
               strip + (size_t)(xs + 1) * 4, (size_t)wsum * 4, wi[i], size);
    ADD("%s(%d, 0, %d, %d)", i ? ", " : " ", xs + 1, wi[i], size);
    xs += wi[i] + 2;
  }
  ADD(");%s%sDefine SwitchButtonOffsets%s (", nlc, nlc, nlc);
  for (int i = 0; i < PVZ_ICON_COUNT; i++)
    ADD("%s(1, %d)", i ? ", " : " ", top_off);
  ADD(");%s%s", nlc, nlc);
  ADD("CreateLayer               SwitchButtons;%s", nlc);
  ADD("LayerSetImage             SwitchButtons '%s_buttons_en';%s", base, nlc);
  ADD("LayerSetAscent            SwitchButtons %d;%s", ascent, nlc);
  ADD("LayerSetCharWidths        SwitchButtons SwitchButtonChars SwitchButtonWidths;%s", nlc);
  ADD("LayerSetImageMap          SwitchButtons SwitchButtonChars SwitchButtonRects;%s", nlc);
  ADD("LayerSetCharOffsets       SwitchButtons SwitchButtonChars SwitchButtonOffsets;%s", nlc);
  ADD("LayerSetColorAdd          SwitchButtons (1, 1, 1, 0);%s", nlc);
  if (point > 0)
    ADD("LayerSetPointSize         SwitchButtons %d;%s", point, nlc);
  for (int l = 0; l < nl; l++)
    ADD("LayerSetCharWidths        %s SwitchButtonChars SwitchButtonWidths;%s", layers[l], nlc);
#undef ADD
  if (n >= cap) {
    free(o);
    free(strip);
    return -1;
  }
  char pic[200];
  snprintf(pic, sizeof pic, "data/%s_buttons_en.png", base);
  size_t pl = 0;
  void *png = tdefl_write_image_to_png_file_in_memory_ex(strip, wsum, size, 4, &pl, 6, MZ_FALSE);
  free(strip);
  if (!png || put_file(x, pic, png, pl) != 0) {
    mz_free(png);
    free(o);
    return -1;
  }
  mz_free(png);
  free(*desc);
  *desc = o;
  *len = n;
  return 0;
}

/* The help bar's button sheets (images/help_buttons*.png, drawn by
 * HelpBarWidget and Board::DrawShovel): the game's own relabelled for the
 * Switch (x360_buttons), whatever the text does; else the port's drawings,
 * when the text has them too (own_too). */
static void help_sheets(Ctx *x, int own_too) {
  buttons_load(x);
  if (x->x360_big && x->x360_small) {
    zb_put_png(x, "images/help_buttons.png", x->x360_big, X360_W, X360_CEL);
    zb_put_png(x, "images/help_buttons_small.png", x->x360_small, X360_W / 2, X360_CEL / 2);
    return;
  }
  const PvzEnglishRes *res = &x->cfg->res;
  if (!own_too)
    return;
  if (res->help_png && res->help_len)
    put_file(x, "images/help_buttons.png", res->help_png, res->help_len);
  if (res->help_small_png && res->help_small_len)
    put_file(x, "images/help_buttons_small.png", res->help_small_png, res->help_small_len);
}

/* ---------------------------------------------- the console's controllers */
/* The versus screens show the players' controllers: the side picker's
 * images/gamepad0.png and gamepad1.png (player 1's and 2's, in a glow of their
 * colour) and the ones the sunflower and the zombie hold, plant_side_selected,
 * zombie_side_selected and help_menu_image_vs_controllers -- every one the
 * Xbox 360 edition's white controller. The port shows Switch Pro Controllers:
 * the picker's are the port's pictures (resources/controllers/, made by
 * tools/make_controller_icons.py with the game's own outline, glow and
 * colours); the held ones are the game's own, recoloured. Their layout is the
 * Pro Controller's already (a stick, the D-pad below it, the four buttons, the
 * other stick below them), so they take its colours, as measured on
 * resources/controllers/controller.png: the white plastic its dark grey (a
 * tone curve, so the painting's light and shade stay), the coloured buttons
 * its black ones, the zombie's drool still over it; the Xbox guide button is
 * painted over with the plastic round it. Each picture's areas were measured
 * on exactly that picture (CRC); if any of the three is another, every
 * controller stays the game's, so the screens never show both kinds. */
typedef struct {
  short x0, y0, x1, y1;
} PadRect;

typedef struct {
  PadRect r;
  unsigned char sat_max; /* its greys up to this saturation are the plastic */
  unsigned char cool;    /* only the cool or neutral ones: the warm are teeth, eyes, a hand */
  unsigned char slime;   /* the zombie's drool: its pale blue kept over the dark plastic */
} PadArea;

typedef struct {
  PadRect r;
  unsigned char sat; /* the buttons: coloured more than this */
} PadButtons;

typedef struct {
  float cx, cy, rx, ry; /* the guide button and its rim: an ellipse */
  PadRect clip;         /* within this (not the thumb pressing on it) */
} PadFill;

typedef struct {
  const char *rel;
  mz_uint32 crc;
  PadArea area[2];        /* the first that holds a pixel decides for it */
  PadRect keep[1];        /* never touched (the zombie's teeth, on the controller's edge) */
  PadRect guide[2];       /* the guide button's glow: plastic, whatever its tint */
  PadButtons buttons[2];
  PadFill fill[2];        /* painted over with the plastic round it */
} HeldPad;

/* unused entries are all zero (an empty rectangle) */
static const HeldPad k_held_pads[] = {
    {"images/plant_side_selected.png", 0xf730b895u,
     {{{0, 0, 291, 304}, 30, 0, 0}}, {{0}}, {{226, 186, 249, 200}},
     {{{192, 196, 225, 216}, 30}}, {{238.5f, 192.2f, 12.8f, 7.5f, {0, 0, 291, 304}}}},
    {"images/zombie_side_selected.png", 0x11216defu,
     {{{0, 0, 304, 313}, 40, 1, 1}}, {{0}}, {{77, 166, 107, 188}, {94, 188, 100, 189}},
     {{{117, 150, 154, 186}, 40}}, {{90.5f, 178.6f, 11.5f, 10.3f, {0, 0, 304, 189}}}},
    {"images/help_menu_image_vs_controllers.png", 0x5fd92484u,
     {{{8, 47, 66, 83}, 30, 0, 0}, {{55, 77, 119, 119}, 40, 1, 1}}, {{80, 78, 100, 87}},
     {{52, 56, 61, 60}, {82, 88, 93, 98}},
     {{{36, 58, 50, 67}, 25}, {{98, 79, 115, 97}, 40}},
     {{56.0f, 57.5f, 4.3f, 2.3f, {0, 0, 119, 119}}, {88.5f, 92.2f, 4.6f, 4.6f, {0, 0, 119, 97}}}},
};
#define N_HELD_PADS (int)(sizeof k_held_pads / sizeof k_held_pads[0])
#define PAD_FILL_PASSES 500

static const char *const k_pad_rel[2] = {"images/gamepad0.png", "images/gamepad1.png"};

/* the Pro Controller's plastic for the 360's, by lightness */
static const double k_pad_in[] = {0, 40, 70, 100, 140, 190, 230, 255};
static const double k_pad_out[] = {0, 24, 32, 42, 56, 76, 90, 104};
static const int k_pad_tint[3] = {-2, 1, 3};

static int pad_in(const PadRect *r, int x, int y) { return x >= r->x0 && x < r->x1 && y >= r->y0 && y < r->y1; }

static int pad_sat(const uint8_t *q) {
  const int hi = q[0] > q[1] ? (q[0] > q[2] ? q[0] : q[2]) : (q[1] > q[2] ? q[1] : q[2]);
  const int lo = q[0] < q[1] ? (q[0] < q[2] ? q[0] : q[2]) : (q[1] < q[2] ? q[1] : q[2]);
  return hi - lo;
}

static double pad_curve(double l) {
  for (int i = 1; i < (int)(sizeof k_pad_in / sizeof k_pad_in[0]); i++)
    if (l <= k_pad_in[i])
      return k_pad_out[i - 1] + (l - k_pad_in[i - 1]) * (k_pad_out[i] - k_pad_out[i - 1]) / (k_pad_in[i] - k_pad_in[i - 1]);
  return k_pad_out[sizeof k_pad_out / sizeof k_pad_out[0] - 1];
}

static int dbl_cmp(const void *a, const void *b) {
  const double x = *(const double *)a, y = *(const double *)b;
  return x < y ? -1 : x > y;
}

/* the median of n values (reordered), 0 for none */
static double median(double *v, int n) {
  if (n <= 0)
    return 0;
  qsort(v, (size_t)n, sizeof *v, dbl_cmp);
  return n % 2 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2;
}

/* The guide button painted over, as the plastic round it would go on: the
 * surface's shading from a quadratic fit of a wide band of that plastic (twice:
 * the second without what the first found far off it, the outlines and
 * creases), carried into the hole's edge by the difference there (each hole
 * pixel's difference the mean of its neighbours', over and over: Poisson) --
 * only from plastic, never the thumb on it. No grain: the plastic is smooth
 * paint (with its measured grain added, the patch showed as speckle). The hole
 * takes in the button's glow round it too. */
static void pad_fill(const HeldPad *p, const uint8_t *src, uint8_t *dst, const uint8_t *kind, int w, int h) {
  const size_t n = (size_t)w * h;
  uint8_t *hole = malloc(n);
  double *fit = malloc(n * 3 * sizeof *fit), *d = malloc(n * 3 * sizeof *d), *tmp = malloc(n * sizeof *tmp);
  int *bx = malloc(n * sizeof *bx), *by = malloc(n * sizeof *by);
  if (!hole || !fit || !d || !tmp || !bx || !by)
    goto out;
  static const int k_dx[4] = {1, -1, 0, 0}, k_dy[4] = {0, 0, 1, -1};
  for (int f = 0; f < 2; f++) {
    const PadFill *e = &p->fill[f];
    if (e->rx <= 0)
      continue;
    /* the hole: the ellipse, and the glow round it (yellow-green, light) */
    int nb = 0;
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        const size_t i = (size_t)y * w + x;
        const uint8_t *q = src + i * 4;
        const double qx = (x - e->cx) / e->rx, qy = (y - e->cy) / e->ry, qq = qx * qx + qy * qy;
        const int in = q[3] && pad_in(&e->clip, x, y);
        const int glow = in && qq <= 1.4 * 1.4 && q[1] >= q[2] + 10 && (q[0] + q[1] + q[2]) / 3.0 >= 110 &&
                         kind[i] != 2 && !pad_in(&p->keep[0], x, y);
        hole[i] = (in && qq <= 1) || glow;
      }
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        const size_t i = (size_t)y * w + x;
        const double qx = (x - e->cx) / e->rx, qy = (y - e->cy) / e->ry;
        if (kind[i] == 1 && !hole[i] && qx * qx + qy * qy <= 2.4 * 2.4)
          bx[nb] = x, by[nb] = y, nb++;
      }
    if (nb < 12)
      continue;
    /* the surface: a quadratic fit, each channel */
    for (int c = 0; c < 3; c++) {
      double coef[6] = {0}, sig = 0;
      for (int pass = 0; pass < 2; pass++) {
        double ata[6][7];
        memset(ata, 0, sizeof ata);
        int used = 0;
        for (int k = 0; k < nb; k++) {
          const double x = bx[k], y = by[k], v = dst[((size_t)by[k] * w + bx[k]) * 4 + c];
          const double t[6] = {1, x, y, x * x, y * y, x * y};
          if (pass && fabs(v - (coef[0] + coef[1] * x + coef[2] * y + coef[3] * x * x + coef[4] * y * y +
                                coef[5] * x * y)) > 2.5 * sig)
            continue;
          for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++)
              ata[i][j] += t[i] * t[j];
            ata[i][6] += t[i] * v;
          }
          used++;
        }
        if (used < 12 || !solve6(ata, coef))
          goto next;
        if (!pass) {
          for (int k = 0; k < nb; k++) {
            const double x = bx[k], y = by[k], v = dst[((size_t)by[k] * w + bx[k]) * 4 + c];
            tmp[k] = fabs(v - (coef[0] + coef[1] * x + coef[2] * y + coef[3] * x * x + coef[4] * y * y +
                               coef[5] * x * y));
          }
          sig = 1.4826 * median(tmp, nb) + 1e-6;
        }
      }
      for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
          fit[((size_t)y * w + x) * 3 + c] = coef[0] + coef[1] * x + coef[2] * y + coef[3] * (double)x * x +
                                             coef[4] * (double)y * y + coef[5] * (double)x * y;
    }
    /* the edge's difference from the fit, carried in */
    for (size_t i = 0; i < n; i++)
      for (int c = 0; c < 3; c++)
        d[i * 3 + c] = hole[i] ? 0 : dst[i * 4 + c] - fit[i * 3 + c];
    for (int pass = 0; pass < PAD_FILL_PASSES; pass++)
      for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
          const size_t i = (size_t)y * w + x;
          if (!hole[i])
            continue;
          double acc[3] = {0};
          int m = 0;
          for (int k = 0; k < 4; k++) {
            const int qx = x + k_dx[k], qy = y + k_dy[k];
            if (qx < 0 || qy < 0 || qx >= w || qy >= h)
              continue;
            const size_t j = (size_t)qy * w + qx;
            if (!hole[j] && kind[j] != 1)
              continue;
            for (int c = 0; c < 3; c++)
              acc[c] += d[j * 3 + c];
            m++;
          }
          if (m)
            for (int c = 0; c < 3; c++)
              d[i * 3 + c] = acc[c] / m;
        }
    for (size_t i = 0; i < n; i++)
      if (hole[i])
        for (int c = 0; c < 3; c++)
          dst[i * 4 + c] = clamp8(lround(fit[i * 3 + c] + d[i * 3 + c]));
  next:;
  }
out:
  free(hole);
  free(fit);
  free(d);
  free(tmp);
  free(bx);
  free(by);
}

/* src (w x h RGBA) recoloured into dst, as p says */
static void pad_recolour(const HeldPad *p, const uint8_t *src, uint8_t *dst, int w, int h) {
  const size_t n = (size_t)w * h;
  uint8_t *core = calloc(n, 1), *kind = calloc(n, 1); /* kind: 1 plastic, 2 a button */
  if (!core || !kind) {
    free(core);
    free(kind);
    return;
  }
  memcpy(dst, src, n * 4);
  /* the buttons: their colour, then the rims next to it */
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      const uint8_t *q = src + ((size_t)y * w + x) * 4;
      const int sat = pad_sat(q);
      for (int k = 0; k < 2; k++)
        if (q[3] && pad_in(&p->buttons[k].r, x, y) && sat > p->buttons[k].sat)
          core[(size_t)y * w + x] = 1;
    }
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      const uint8_t *q = src + ((size_t)y * w + x) * 4;
      uint8_t *d = dst + ((size_t)y * w + x) * 4;
      if (!q[3] || pad_in(&p->keep[0], x, y))
        continue;
      const int r = q[0], g = q[1], b = q[2];
      const int sat = pad_sat(q);
      const double l = (r + g + b) / 3.0;
      const size_t i = (size_t)y * w + x;
      const int in_buttons = pad_in(&p->buttons[0].r, x, y) || pad_in(&p->buttons[1].r, x, y);
      const int near = (x > 0 && core[i - 1]) || (x + 1 < w && core[i + 1]) || (y > 0 && core[i - w]) ||
                       (y + 1 < h && core[i + w]);
      if (core[i] || (near && in_buttons && sat > 18 && l >= 40)) {
        for (int c = 0; c < 3; c++)
          d[c] = clamp8(lround(22 + 0.3 * l + k_pad_tint[c]));
        kind[i] = 2;
        continue;
      }
      const PadArea *a = NULL;
      for (int k = 0; k < 2 && !a; k++)
        if (pad_in(&p->area[k].r, x, y))
          a = &p->area[k];
      if (!a || l < 40)
        continue;
      const int guide = pad_in(&p->guide[0], x, y) || pad_in(&p->guide[1], x, y);
      /* (b - g: not the backdrop's blue, which is never light) */
      const int grey = sat <= a->sat_max && (b - g <= 12 || l >= 150) &&
                       (!a->cool || b >= (r > g ? r : g) - 6 || (sat <= 10 && l >= 100));
      if (!grey && !guide)
        continue;
      /* the drool: its own pale blue (its colour away from grey, 1.8 times,
       * so it keeps its tint over the dark plastic), three quarters over it */
      const double m = pad_curve(l), s = a->slime ? fmin(1, fmax(0, (b - r - 6) / 12.0)) * 0.75 : 0;
      for (int c = 0; c < 3; c++) {
        const double v = fmin(255, fmax(0, m + k_pad_tint[c]));
        const double drool = fmin(255, fmax(0, l + 1.8 * (q[c] - l)));
        d[c] = clamp8(lround(v + (drool - v) * s));
      }
      kind[i] = 1;
    }
  free(core);
  pad_fill(p, src, dst, kind, w, h);
  free(kind);
}

/* the held controllers recoloured, into the layer; the side picker's the
 * port's. All or none. */
static void console_controllers(Ctx *x) {
  const PvzEnglishRes *res = &x->cfg->res;
  uint8_t *img[N_HELD_PADS] = {0};
  int w[N_HELD_PADS] = {0}, h[N_HELD_PADS] = {0}, ok = 1;
  for (int i = 0; i < 2; i++)
    ok &= res->pad_png[i] && res->pad_len[i] && index_find(&x->gi, k_pad_rel[i]) >= 0;
  for (int i = 0; i < N_HELD_PADS && ok; i++) {
    const int idx = index_find(&x->gi, k_held_pads[i].rel);
    if (idx < 0 || entry_crc(x->g, idx) != k_held_pads[i].crc) {
      LOG("[english] controllers: game.apk's %s is not the one known here: every controller stays "
          "the game's\n", k_held_pads[i].rel);
      ok = 0;
      break;
    }
    size_t len = 0;
    uint8_t *png = extract(x->g, idx, &len);
    uint8_t *src = png ? png_rgba(png, len, &w[i], &h[i]) : NULL;
    free(png);
    img[i] = src ? malloc((size_t)w[i] * h[i] * 4) : NULL;
    if (img[i])
      pad_recolour(&k_held_pads[i], src, img[i], w[i], h[i]);
    free(src);
    ok = img[i] != NULL;
  }
  if (ok) {
    for (int i = 0; i < 2; i++)
      put_file(x, k_pad_rel[i], res->pad_png[i], res->pad_len[i]);
    for (int i = 0; i < N_HELD_PADS; i++)
      zb_put_png(x, k_held_pads[i].rel, img[i], w[i], h[i]);
    LOG("[english] controllers: the Switch Pro Controller's (the side picker's, and the game's own "
        "held ones recoloured)\n");
  }
  for (int i = 0; i < N_HELD_PADS; i++)
    free(img[i]);
}

/* The help screen's Controls page (menu/HelpMenu.menu.txt, page 2): the
 * Xbox 360 edition's four lines (A plant, B dig, the stick, X butter) in the
 * small body type, at a layout scaled twice over by the engine and its type
 * not. The layer's copy of the file has the Switch's controls there instead,
 * seven lines ([SWITCH_HELP_2_1..7], resources/english/AddonStrings_en.txt)
 * in the How To Play page's own BrianneTod, bigger: its 32, which the mod
 * draws at 62% for these (HelpTextWidget_Draw). The rest of the file is
 * game.apk's. */
static const char k_help_page2[] =
    "#Page 2 (the Switch port: its controls)\n"
    "AddWidget HelpImageWidget PAGE_2_BACK;\n"
    "SetHelpImage 'images/help_menu_paper01';\n"
    "Resize 1315 45 730 514;\n"
    "\n"
    "AddWidget HelpTextWidget PAGE_2_TITLE;\n"
    "SetText '[HELP_TEXT_2_TITLE]';\n"
    "SetHelpFont 'FONT_HOUSEOFTERROR28';\n"
    "Resize 1465 55 800 50;\n"
    "SetHelpColor 255 255 255 255;\n"
    "\n"
    "%s"
    "AddWidget HelpTextWidget PAGE_2_PAGE;\n"
    "SetText '[HELP_TEXT_PAGE_2_OF_X]';\n"
    "SetHelpFont 'FONT_HOUSEOFTERROR16';\n"
    "SetHelpColor 255 255 255 255;\n"
    "Resize 1345 255 300 60;\n"
    "\n";

static void help_menu(Ctx *x) {
  const char *rel = "menu/HelpMenu.menu.txt";
  size_t len = 0;
  char *menu = extract(x->g, index_find(&x->gi, rel), &len);
  char *from = menu ? strstr(menu, "#Page 2") : NULL;
  char *to = from ? strstr(from, "#Page 3") : NULL;
  if (!to) {
    LOG("[english] help screen: game.apk's %s is not the one known here: its Controls page stays\n", rel);
    free(menu);
    return;
  }
  char lines[1400];
  size_t n = 0;
  for (int i = 0; i < 7; i++) /* 22 apart: 44 pixels once the engine has scaled the layout */
    n += (size_t)snprintf(lines + n, sizeof lines - n,
                          "AddWidget HelpTextWidget PAGE_2_SWITCH_%d;\n"
                          "SetText '[SWITCH_HELP_2_%d]';\n"
                          "SetHelpFont 'FONT_BRIANNETOD32';\n"
                          "Resize 1370 %d 800 60;\n"
                          "SetHelpColor 0 0 0 255;\n\n",
                          i + 1, i + 1, 93 + 22 * i);
  char page[sizeof k_help_page2 + sizeof lines];
  const int pl = snprintf(page, sizeof page, k_help_page2, lines);
  const size_t head = (size_t)(from - menu), tail = len - (size_t)(to - menu);
  char *out = malloc(head + (size_t)pl + tail);
  if (out) {
    memcpy(out, menu, head);
    memcpy(out + head, page, (size_t)pl);
    memcpy(out + head + pl, to, tail);
    if (put_file(x, rel, out, head + (size_t)pl + tail) == 0)
      LOG("[english] help screen: the Controls page shows the Switch's controls\n");
  }
  free(out);
  free(menu);
}

/* 1 if both pictures were made */
static int make_zombatar_back(Ctx *x) {
  int bw, bh, hw, hh, ew, eh, lw, lh;
  uint8_t *bg = zip_png(x->g, &x->gi, ZB_BG, &bw, &bh);
  uint8_t *hl = zip_png(x->g, &x->gi, ZB_HL, &hw, &hh);
  uint8_t *en = zip_png(&x->e, &x->ei, ZB_BG, &ew, &eh);
  float *letters = en ? zb_letters(en, ew, &lw, &lh) : NULL;
  uint8_t *lit = NULL, *glow = NULL, *hole = NULL, *tmp = NULL;
  float *m = NULL;
  int ok = 0;
  if (!bg || !hl || !letters || bw != 1280 || bh != 720 || hw != 322 || hh != 85 || eh != 720)
    goto done;
  const int n = hw * hh;
  lit = calloc(n, 1), glow = calloc(n, 1), hole = calloc(n, 1), tmp = calloc(n, 1);
  m = calloc(n, sizeof *m);
  if (!lit || !glow || !hole || !tmp || !m)
    goto done;
  /* the carving, lit in the copy: bright, or its violet halo */
  int gx0 = hw, gy0 = hh, gx1 = -1, gy1 = -1;
  for (int i = 0; i < n; i++) {
    const uint8_t *p = hl + (size_t)i * 4;
    glow[i] = p[0] + p[1] + p[2] > 480 && p[3] > 200;
    lit[i] = glow[i] || (p[0] - p[1] > 25 && p[2] - p[1] > 25 && p[3] > 100);
    if (glow[i]) {
      int gx = i % hw, gy = i / hw;
      gx0 = gx < gx0 ? gx : gx0;
      gx1 = gx > gx1 ? gx : gx1;
      gy0 = gy < gy0 ? gy : gy0;
      gy1 = gy > gy1 ? gy : gy1;
    }
  }
  /* it must be the picture this was made for: the copy matches the
   * background where it is not lit */
  double diff = 0;
  int cnt = 0;
  for (int i = 0; i < n; i++) {
    const uint8_t *p = hl + (size_t)i * 4, *q = bg + ((size_t)(ZB_Y + i / hw) * bw + ZB_X + i % hw) * 4;
    if (p[3] > 230 && !glow[i]) {
      diff += abs(p[0] - q[0]) + abs(p[1] - q[1]) + abs(p[2] - q[2]);
      cnt++;
    }
  }
  if (gx1 < 0 || !cnt || diff / cnt > 20) {
    LOG("[english] the Zombatar back slab is not the expected picture (%.1f) -- kept\n", cnt ? diff / cnt : -1.0);
    goto done;
  }
  /* the English letters: 138 x 33, slanted, where the Chinese ones were */
  const float LW = 138, LH = 33, shear = 0.42f;
  const float cx = (gx0 + gx1) / 2.0f + 2, cy = (gy0 + gy1) / 2.0f + 1;
  for (int y = 0; y < hh; y++)
    for (int x0 = 0; x0 < hw; x0++) {
      float v = y - (cy - LH / 2), u = x0 - (cx - LW / 2) + shear * (v - LH / 2);
      if (v < 0 || v > LH - 1 || u < 0 || u > LW - 1)
        continue;
      m[y * hw + x0] = bilinear(letters, lw, lh, u * (lw - 1) / (LW - 1), v * (lh - 1) / (LH - 1));
    }
  const int box[4] = {60, 6, 250, 64};

  /* the background: the carving (lit area, and the dark around it) filled,
   * the letters black */
  uint8_t *patch = malloc((size_t)n * 4);
  if (!patch)
    goto done;
  for (int y = 0; y < hh; y++)
    memcpy(patch + (size_t)y * hw * 4, bg + ((size_t)(ZB_Y + y) * bw + ZB_X) * 4, (size_t)hw * 4);
  mask_grow(lit, hole, hw, hh, 1);
  mask_grow(glow, tmp, hw, hh, 4);
  for (int i = 0; i < n; i++)
    if (tmp[i] && patch[i * 4] + patch[i * 4 + 1] + patch[i * 4 + 2] < 260)
      hole[i] = 1;
  zb_refill(patch, hw, hh, hole, box, 7);
  for (int i = 0; i < n; i++) {
    const float a = m[i];
    const float ink[3] = {24, 22, 30};
    for (int c = 0; c < 3; c++)
      patch[i * 4 + c] = (uint8_t)(patch[i * 4 + c] * (1 - a) + ink[c] * a + 0.5f);
  }
  for (int y = 0; y < hh; y++)
    memcpy(bg + ((size_t)(ZB_Y + y) * bw + ZB_X) * 4, patch + (size_t)y * hw * 4, (size_t)hw * 4);
  free(patch);

  /* the lit copy: filled, a violet halo, pale letters */
  memset(hole, 0, n);
  mask_grow(lit, hole, hw, hh, 3);
  zb_refill(hl, hw, hh, hole, box, 11);
  float kern[21], ksum = 0; /* Gaussian, sigma 3.2 */
  for (int k = -10; k <= 10; k++)
    ksum += kern[k + 10] = expf(-(float)(k * k) / (2 * 3.2f * 3.2f));
  float *row = calloc(n, sizeof *row), *halo = calloc(n, sizeof *halo);
  if (!row || !halo) {
    free(row);
    free(halo);
    goto done;
  }
  for (int y = 0; y < hh; y++)
    for (int x0 = 0; x0 < hw; x0++) {
      float s = 0;
      for (int k = -10; k <= 10; k++) {
        int xx = x0 + k < 0 ? 0 : x0 + k >= hw ? hw - 1 : x0 + k;
        s += m[y * hw + xx] * kern[k + 10];
      }
      row[y * hw + x0] = s / ksum;
    }
  for (int y = 0; y < hh; y++)
    for (int x0 = 0; x0 < hw; x0++) {
      float s = 0;
      for (int k = -10; k <= 10; k++) {
        int yy = y + k < 0 ? 0 : y + k >= hh ? hh - 1 : y + k;
        s += row[yy * hw + x0] * kern[k + 10];
      }
      s = s / ksum * 1.8f;
      halo[y * hw + x0] = s > 1 ? 1 : s;
    }
  for (int i = 0; i < n; i++) {
    const float g = halo[i], a = m[i];
    const float violet[3] = {214, 120, 236}, pale[3] = {255, 236, 255};
    for (int c = 0; c < 3; c++) {
      float v = hl[i * 4 + c] * (1 - g) + violet[c] * g;
      hl[i * 4 + c] = (uint8_t)(v * (1 - a) + pale[c] * a + 0.5f);
    }
    if (hl[i * 4 + 3] && g * 255 > hl[i * 4 + 3])
      hl[i * 4 + 3] = (uint8_t)(g * 255);
  }
  free(row);
  free(halo);
  zb_put_png(x, ZB_BG, bg, bw, bh);
  zb_put_png(x, ZB_HL, hl, hw, hh);
  ok = !x->fail;
done:
  free(bg);
  free(hl);
  free(en);
  free(letters);
  free(lit);
  free(glow);
  free(hole);
  free(tmp);
  free(m);
  return ok;
}

static int zombatar_back(Ctx *x) {
  if (!x->zb_tried) {
    x->zb_tried = 1;
    x->zb_ok = make_zombatar_back(x);
    if (x->zb_ok)
      x->n_made += 2;
  }
  return x->zb_ok;
}

/* ---------------------------------------------------------------- pictures */
static int in_list(const char *lrel, const char *const *list) {
  for (int i = 0; list[i]; i++)
    if (!strcmp(lrel, list[i]))
      return 1;
  return 0;
}

static void make_assets(Ctx *x) {
  mz_uint n = mz_zip_reader_get_num_files(&x->c);
  char name[512], low[512];
  char **atlases = NULL;
  int natl = 0;
  /* pictures and other files, then fonts, then the menu atlases (whose signs
   * must not replace a picture taken whole) */
  for (int pass = 0; pass < 2 && !x->fail; pass++)
    for (mz_uint i = 0; i < n && !x->fail; i++) {
      PROGRESS((int)((pass * (long)n + i) * 850 / (2 * (long)n + 1)),
               pass ? "English fonts and the Switch's buttons" : "English pictures");
      if (mz_zip_reader_is_file_a_directory(&x->c, i) ||
          !mz_zip_reader_get_filename(&x->c, i, name, sizeof name) || !strchr(name, '/'))
        continue; /* the pak's own readme and picture */
      snprintf(low, sizeof low, "%s", name);
      lower_str(low);
      if (starts(low, "properties/") || starts(low, "addonfiles/properties/"))
        continue; /* the text: make_strings */
      if (starts(low, "data/")) {
        if (pass == 1 && ends(low, ".txt"))
          make_font(x, name);
        continue; /* font pictures come with their descriptor */
      }
      if (pass == 1)
        continue;
      int gi = index_find(&x->gi, name);
      if (gi < 0) {
        if (ends(low, ".xml") && index_find(&x->ei, name) >= 0) {
          char **v = realloc(atlases, sizeof *v * (size_t)(natl + 1));
          if (v && (v[natl] = strdup(name)))
            atlases = v, natl++;
          else if (v)
            atlases = v;
        }
        continue; /* not a file the game has */
      }
      int ei = index_find(&x->ei, name);
      if (ei < 0)
        continue;
      mz_uint32 cc = entry_crc(&x->c, (int)i), ec = entry_crc(&x->e, ei), gc = entry_crc(x->g, gi);
      if (ec == cc || in_list(low, k_keep_game))
        continue; /* the English build left it Chinese too; or the game's is right */
      if (gc == cc) {
        if (copy_e(x, ei, name) == 0)
          x->n_swap++;
      } else if (in_list(low, k_take_en)) {
        if (copy_e(x, ei, name) == 0)
          x->n_take++;
      } else if ((!strcasecmp(name, ZB_BG) || !strcasecmp(name, ZB_HL)) && zombatar_back(x)) {
        /* made, both at once */
      } else {
        x->n_kept++;
        size_t kl = strlen(x->kept);
        if (kl + strlen(name) + 3 < sizeof x->kept)
          snprintf(x->kept + kl, sizeof x->kept - kl, "%s%s", kl ? ", " : "", name);
      }
    }
  for (int i = 0; i < natl; i++) {
    PROGRESS(850 + i * 100 / (natl + 1), "English menu signs");
    if (!x->fail)
      make_atlas(x, atlases[i]);
    free(atlases[i]);
  }
  free(atlases);
}

/* ---------------------------------------------------------------- main */
static int find_china_pak(mz_zip_archive *z) {
  mz_uint n = mz_zip_reader_get_num_files(z);
  int best = -1;
  char name[512];
  for (mz_uint i = 0; i < n; i++) {
    if (!mz_zip_reader_get_filename(z, i, name, sizeof name) || !starts(name, "assets/paks/"))
      continue;
    lower_str(name);
    if (!ends(name, ".zip"))
      continue;
    if (strstr(name, "changegamechina"))
      return (int)i;
    if (best < 0 && strstr(name, "china"))
      best = (int)i;
  }
  return best;
}

static void open_english(Ctx *x) {
  const char *path = x->cfg->english_apk;
  struct stat st;
  if (!path || stat(path, &st) != 0)
    return;
  if (!mz_zip_reader_init_file(&x->e, path, 0)) {
    LOG("[english] %s is not a readable APK -- ignored\n", path);
    return;
  }
  x->have_e = 1;
  if (index_build(&x->e, FILES, &x->ei) != 0 ||
      index_find(&x->ei, "properties/LawnStrings.txt") < 0) {
    LOG("[english] %s has no assets/files/properties/LawnStrings.txt: not an English PvZ TV build "
        "-- ignored\n", path);
    goto drop;
  }
  x->pak = find_china_pak(&x->e);
  if (x->pak < 0) {
    LOG("[english] %s has no assets/paks/...China...zip (the list of translated files): "
        "use the English PvZ TV Touch 4.0.5 or RedStr1x APK -- ignored\n", path);
    goto drop;
  }
  return;
drop:
  index_free(&x->ei);
  mz_zip_reader_end(&x->e);
  x->have_e = 0;
}

/* the China pak, into memory: 0 if it could be read */
static int load_pak(Ctx *x) {
  size_t n = 0;
  x->c_buf = mz_zip_reader_extract_to_heap(&x->e, (mz_uint)x->pak, &n, 0);
  if (!x->c_buf || !mz_zip_reader_init_mem(&x->c, x->c_buf, n, 0)) {
    mz_free(x->c_buf);
    x->c_buf = NULL;
    return -1;
  }
  x->have_c = 1;
  if (index_build(&x->c, "", &x->ci) != 0)
    return -1;
  return 0;
}

static void close_all(Ctx *x) {
  if (x->have_c) {
    mz_zip_reader_end(&x->c);
    index_free(&x->ci);
  }
  mz_free(x->c_buf);
  if (x->have_e) {
    mz_zip_reader_end(&x->e);
    index_free(&x->ei);
  }
  index_free(&x->gi);
  for (int i = 0; i < x->nmade; i++)
    free(x->made[i]);
  free(x->made);
  free(x->icons);
  free(x->x360_big);
  free(x->x360_small);
}

int pvz_english_apply(mz_zip_archive *game, const PvzEnglishCfg *cfg) {
  Ctx ctx, *x = &ctx;
  memset(x, 0, sizeof *x);
  x->cfg = cfg;
  x->g = game;
  if (index_build(game, FILES, &x->gi) != 0)
    return -1;
  open_english(x);

  /* key: "<game.apk + this port's text> <english.apk or 0> <full|text>" */
  uint64_t h = 0xcbf29ce484222325ull, he = 0;
  int ver = LAYER_VERSION;
  h = fnv(h, &ver, sizeof ver);
  h = hash_zip(h, game);
  const char *res[3] = {cfg->res.addon_en, cfg->res.lawn_en, cfg->res.lawn_fix};
  for (int i = 0; i < 3; i++)
    if (res[i])
      h = fnv(h, res[i], strlen(res[i]));
  if (cfg->res.icons_png && !cfg->no_button_pictures)
    h = fnv(h, cfg->res.icons_png, cfg->res.icons_len);
  if (cfg->res.help_png)
    h = fnv(h, cfg->res.help_png, cfg->res.help_len);
  for (int i = 0; i < 2; i++)
    if (cfg->res.pad_png[i])
      h = fnv(h, cfg->res.pad_png[i], cfg->res.pad_len[i]);
  if (x->have_e)
    he = hash_zip(0xcbf29ce484222325ull, &x->e);
  char key[80], have[80], game_part[24];
  snprintf(key, sizeof key, "%016llx %016llx %s", (unsigned long long)h, (unsigned long long)he,
           x->have_e ? "full" : "text");
  snprintf(game_part, sizeof game_part, "%016llx ", (unsigned long long)h);
  list_key(cfg, have, sizeof have);
  char probe[600];
  struct stat st;
  snprintf(probe, sizeof probe, "%s/properties/LawnStrings.txt", cfg->files_dir);
  int present = stat(probe, &st) == 0;
  if (present && !strcmp(key, have)) {
    cfg->log("[english] English layer up to date (%s)\n", x->have_e ? "from english.apk" : "text only");
    close_all(x);
    return 0;
  }
  /* english.apk is only the source: once its layer is made for this
   * game.apk, it can be deleted */
  if (present && !x->have_e && starts(have, game_part) && ends(have, " full")) {
    cfg->log("[english] English layer up to date (made from the English APK, which is no longer "
             "needed)\n");
    close_all(x);
    return 0;
  }
  /* a layer from english.apk that this build (or a new game.apk) would make
   * differently: without english.apk it is kept as it is, never replaced by
   * the text-only one */
  if (present && !x->have_e && ends(have, " full")) {
    cfg->log("[english] the English layer was made by an older build or for another game APK: "
             "kept as it is (put the English APK back in the game folder to refresh it)\n");
    close_all(x);
    return 0;
  }

  if (cfg->working)
    cfg->working();
  int removed = remove_listed(cfg);
  if (x->have_e && load_pak(x) != 0) {
    LOG("[english] could not read the China pak in %s\n", cfg->english_apk);
    x->fail = 1; /* made again next start */
  }
  if (x->have_c)
    LOG("[english] making the English layer from %s (the pictures, fonts and text)...\n",
        cfg->english_apk);
  else
    LOG("[english] making the English text (no %s: pictures and fonts stay Chinese; see the "
        "README)...\n", cfg->english_apk ? cfg->english_apk : "the English APK");
  if (x->have_c && !x->fail) {
    make_assets(x); /* first: the text's buttons are pictures only if every font has them */
    help_sheets(x, x->n_icon_fonts > 0 && x->n_icon_fail == 0);
    if (x->cfg->no_button_pictures)
      LOG("[english] buttons: named in the text (button_pictures = false)\n");
    else
      LOG("[english] buttons: pictures in %d fonts%s\n", x->n_icon_fonts,
          x->n_icon_fail ? " -- not in every one: the text names them" : "");
    LOG("[english] pictures: %d swapped, %d the mod changed but English taken, %d menu signs cut "
        "from the English atlas (%d without words), %d made (the Zombatar back slab), %d the mod "
        "changed kept%s%s%s\n",
        x->n_swap, x->n_take, x->n_crop, x->n_same, x->n_made, x->n_kept, x->n_kept ? " (" : "",
        x->kept, x->n_kept ? ")" : "");
    LOG("[english] fonts: %d English fonts, %d pictures\n", x->n_font, x->n_fontimg);
  }
  PROGRESS(950, "English text");
  if (!x->have_c || x->fail)
    help_sheets(x, 0); /* the game's own sheets, relabelled, with the text alone too */
  console_controllers(x);
  make_strings(x);
  help_menu(x);
  PROGRESS(1000, "English text");
  int cleared = clear_font_caches(cfg);
  if (write_list(x, x->fail ? "0 incomplete" : key) != 0)
    LOG("[english] could not write %s\n", cfg->layer_list);
  LOG("[english] %s: %d files%s; %d old files removed, %d compiled fonts cleared\n",
      x->fail ? "incomplete (it is tried again next start)" : "done", x->nmade,
      x->have_c ? "" : " (text only)", removed, cleared);
  int rc = x->fail ? -1 : 0;
  close_all(x);
  return rc;
}

void pvz_english_remove(const PvzEnglishCfg *cfg) {
  struct stat st;
  if (stat(cfg->layer_list, &st) != 0)
    return;
  int n = remove_listed(cfg);
  int cleared = clear_font_caches(cfg);
  cfg->log("[english] language: chinese -- the English layer removed (%d files, %d compiled "
           "fonts)\n", n, cleared);
}
