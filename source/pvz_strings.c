/* pvz_strings.c -- the game's string tables in English.
 *
 * The engine's string tables (properties/LawnStrings.txt, LawnOEMStrings.txt,
 * the mod's addonFiles/properties/AddonStrings.txt) are "[KEY]" lines, each
 * followed by its text up to the next '['. TodStringListLoad reads them into
 * one map, later files overriding earlier ones. The engine looks for a file in
 * its files directory before the APK (PakLib: the "native" locations have
 * priority 0, the APK's zip 20), so a table written there replaces the game's.
 *
 * The game (1.1.5, the newest mod) is Chinese. pvz_strings_pick() makes a
 * table with exactly the Chinese table's keys, in its order, each one's text
 * taken from the first English source that has a usable one:
 *   - a source's text is usable if it is not blank (unless the Chinese one is
 *     blank too) and has the same printf conversions as the Chinese text (a
 *     "%d" where the engine passes a string would crash it; see usable());
 *   - no usable English text: the Chinese text stays (shown rather than a
 *     missing-string marker).
 * Keys only a source has can be added after them (PvzStrSrc.add_new). The
 * output has the Chinese file's line endings and no BOM.
 *
 * Pure C (no libnx): tools/test_english.py builds it on the host. MIT.
 */
#define _GNU_SOURCE /* memmem */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pvz_strings.h"

/* UTF-16 (with or without a BOM, LE unless the BOM says BE) -> UTF-8. */
char *pvz_utf16_to_utf8(const uint8_t *in, size_t n, size_t *out_len) {
  int be = 0;
  size_t i = 0;
  if (n >= 2 && in[0] == 0xFF && in[1] == 0xFE)
    i = 2;
  else if (n >= 2 && in[0] == 0xFE && in[1] == 0xFF)
    i = 2, be = 1;
  char *out = malloc(n / 2 * 3 + 4), *o = out;
  if (!out)
    return NULL;
  for (; i + 1 < n; i += 2) {
    uint32_t c = be ? (uint32_t)(in[i] << 8 | in[i + 1]) : (uint32_t)(in[i + 1] << 8 | in[i]);
    if (c >= 0xD800 && c < 0xDC00 && i + 3 < n) {
      uint32_t d = be ? (uint32_t)(in[i + 2] << 8 | in[i + 3]) : (uint32_t)(in[i + 3] << 8 | in[i + 2]);
      if (d >= 0xDC00 && d < 0xE000) {
        c = 0x10000 + ((c - 0xD800) << 10) + (d - 0xDC00);
        i += 2;
      }
    }
    if (c < 0x80) {
      *o++ = (char)c;
    } else if (c < 0x800) {
      *o++ = (char)(0xC0 | c >> 6);
      *o++ = (char)(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
      *o++ = (char)(0xE0 | c >> 12);
      *o++ = (char)(0x80 | ((c >> 6) & 0x3F));
      *o++ = (char)(0x80 | (c & 0x3F));
    } else {
      *o++ = (char)(0xF0 | c >> 18);
      *o++ = (char)(0x80 | ((c >> 12) & 0x3F));
      *o++ = (char)(0x80 | ((c >> 6) & 0x3F));
      *o++ = (char)(0x80 | (c & 0x3F));
    }
  }
  *o = 0;
  *out_len = (size_t)(o - out);
  return out;
}

/* ------------------------------------------------------------- parsing */
/* A section starts at a line "[KEY]" (letters, digits, '_'); its text runs
 * from the next line to the next such line, without its leading and trailing
 * line breaks. */
typedef struct {
  const char *key, *val;
  size_t klen, vlen;
} Sect;

typedef struct {
  Sect *v;
  int n;
} Table;

static int key_line(const char *p, const char *end, const char **key, size_t *klen) {
  if (p >= end || *p != '[')
    return 0;
  const char *q = p + 1;
  while (q < end && (*q == '_' || (*q >= '0' && *q <= '9') || (*q >= 'A' && *q <= 'Z') ||
                     (*q >= 'a' && *q <= 'z')))
    q++;
  if (q == p + 1 || q >= end || *q != ']')
    return 0;
  const char *r = q + 1;
  while (r < end && (*r == ' ' || *r == '\t' || *r == '\r'))
    r++;
  if (r < end && *r != '\n')
    return 0;
  *key = p + 1;
  *klen = (size_t)(q - p - 1);
  return 1;
}

static void trim_breaks(Sect *s) {
  while (s->vlen && (s->val[0] == '\n' || s->val[0] == '\r'))
    s->val++, s->vlen--;
  while (s->vlen && (s->val[s->vlen - 1] == '\n' || s->val[s->vlen - 1] == '\r'))
    s->vlen--;
}

static int parse(const char *t, size_t n, Table *tab) {
  int cap = 256, c = 0;
  Sect *v = malloc(sizeof *v * (size_t)cap);
  if (!v)
    return -1;
  if (n >= 3 && !memcmp(t, "\xEF\xBB\xBF", 3)) /* UTF-8 BOM */
    t += 3, n -= 3;
  const char *end = t + n;
  for (const char *line = t; line < end;) {
    const char *nl = memchr(line, '\n', (size_t)(end - line));
    const char *next = nl ? nl + 1 : end;
    const char *key;
    size_t klen;
    if (key_line(line, end, &key, &klen)) {
      if (c)
        v[c - 1].vlen = (size_t)(line - v[c - 1].val);
      if (c == cap) {
        cap *= 2;
        Sect *nv = realloc(v, sizeof *v * (size_t)cap);
        if (!nv) {
          free(v);
          return -1;
        }
        v = nv;
      }
      v[c++] = (Sect){key, next, klen, 0};
    }
    line = next;
  }
  if (c)
    v[c - 1].vlen = (size_t)(end - v[c - 1].val);
  for (int i = 0; i < c; i++)
    trim_breaks(&v[i]);
  tab->v = v;
  tab->n = c;
  return 0;
}

static const Sect *find(const Table *t, const char *key, size_t klen) {
  for (int i = 0; i < t->n; i++)
    if (t->v[i].klen == klen && !memcmp(t->v[i].key, key, klen))
      return &t->v[i];
  return NULL;
}

static int blank(const char *s, size_t n) {
  for (size_t i = 0; i < n; i++)
    if (s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n')
      return 0;
  return 1;
}

/* The printf conversions of s, as "<length modifiers><conversion>" joined by
 * ','; "%%" is not one. */
static void conversions(const char *s, size_t n, char *out, size_t cap) {
  size_t o = 0;
  out[0] = 0;
  for (size_t i = 0; i < n; i++) {
    if (s[i] != '%')
      continue;
    size_t j = i + 1;
    if (j < n && s[j] == '%') {
      i = j;
      continue;
    }
    while (j < n && s[j] && strchr("-+ #0", s[j]))
      j++;
    while (j < n && ((s[j] >= '0' && s[j] <= '9') || s[j] == '*'))
      j++;
    if (j < n && s[j] == '.') {
      j++;
      while (j < n && ((s[j] >= '0' && s[j] <= '9') || s[j] == '*'))
        j++;
    }
    size_t m = j;
    while (j < n && s[j] && strchr("hlLqjzt", s[j]))
      j++;
    if (j >= n || !s[j] || !strchr("diouxXeEfFgGaAcspn", s[j]))
      continue; /* a lone '%': printf would print it */
    for (size_t k = m; k <= j && o + 2 < cap; k++)
      out[o++] = s[k];
    if (o + 1 < cap)
      out[o++] = ',';
    out[o] = 0;
    i = j;
  }
}

/* A Chinese text with conversions is a format: the English one must have the
 * same. One without may still reach printf ("60% of" reads as "% o"): no
 * conversion that reads a pointer, then. */
static int glyphs(const Sect *s) {
  for (size_t i = 0; i < s->vlen; i++)
    if ((uint8_t)s->val[i] == 0xC3 && i + 1 < s->vlen && (uint8_t)s->val[i + 1] >= 0x80)
      return 1;
  return 0;
}

static int usable(const Sect *en, const Sect *zh, int plain) {
  if (blank(en->val, en->vlen) && !blank(zh->val, zh->vlen))
    return 0;
  if (plain && glyphs(en))
    return 0;
  char a[128], b[128];
  conversions(en->val, en->vlen, a, sizeof a);
  conversions(zh->val, zh->vlen, b, sizeof b);
  if (b[0])
    return !strcmp(a, b);
  return !strchr(a, 's') && !strchr(a, 'n') && !strchr(a, 'p');
}

/* ------------------------------------------------------------- output */
typedef struct {
  char *p;
  size_t n, cap;
  int crlf, fail;
} Buf;

static void put(Buf *b, const char *s, size_t n) {
  if (b->fail)
    return;
  if (b->n + n + 1 > b->cap) {
    size_t cap = b->cap ? b->cap : 4096;
    while (b->n + n + 1 > cap)
      cap *= 2;
    char *p = realloc(b->p, cap);
    if (!p) {
      b->fail = 1;
      return;
    }
    b->p = p;
    b->cap = cap;
  }
  memcpy(b->p + b->n, s, n);
  b->n += n;
  b->p[b->n] = 0;
}

/* text with its line breaks made the output's */
static void put_text(Buf *b, const char *s, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s[i] == '\r')
      continue;
    if (s[i] == '\n' && b->crlf)
      put(b, "\r", 1);
    put(b, &s[i], 1);
  }
}

static void put_entry(Buf *b, const Sect *key, const Sect *val) {
  const char *nl = b->crlf ? "\r\n" : "\n";
  put(b, "[", 1);
  put(b, key->key, key->klen);
  put(b, "]", 1);
  put(b, nl, strlen(nl));
  put_text(b, val->val, val->vlen);
  put(b, nl, strlen(nl));
  put(b, nl, strlen(nl));
}

char *pvz_strings_pick(const char *zh, size_t zh_len, const PvzStrSrc *src, int nsrc,
                       size_t *out_len, PvzStrStats *st) {
  Table z = {0}, t[PVZ_STR_MAX_SRC] = {{0}};
  PvzStrStats stats;
  memset(&stats, 0, sizeof stats);
  Buf b = {0};
  b.crlf = memmem(zh, zh_len, "\r\n", 2) != NULL;
  if (nsrc > PVZ_STR_MAX_SRC || parse(zh, zh_len, &z) != 0)
    return NULL;
  for (int i = 0; i < nsrc; i++)
    if (src[i].text && parse(src[i].text, src[i].len, &t[i]) != 0)
      b.fail = 1;
  for (int k = 0; k < z.n && !b.fail; k++) {
    const Sect *zk = &z.v[k], *pick = zk;
    int from = -1;
    for (int i = 0; i < nsrc && from < 0; i++) {
      const Sect *e = find(&t[i], zk->key, zk->klen);
      if (!e)
        continue;
      if (usable(e, zk, src[i].plain)) {
        pick = e;
        from = i;
      } else {
        stats.rejected++;
      }
    }
    if (from >= 0)
      stats.from[from]++;
    else if (!blank(zk->val, zk->vlen))
      stats.chinese++;
    put_entry(&b, zk, pick);
  }
  /* keys only an English source has (with add_new), once each */
  for (int i = 0; i < nsrc && !b.fail; i++) {
    if (!src[i].add_new)
      continue;
    for (int k = 0; k < t[i].n; k++) {
      const Sect *e = &t[i].v[k];
      int seen = find(&z, e->key, e->klen) != NULL;
      for (int j = 0; j < i && !seen; j++)
        seen = src[j].add_new && find(&t[j], e->key, e->klen) != NULL;
      /* a key twice in one file: the first */
      if (!seen && find(&t[i], e->key, e->klen) == e && !(src[i].plain && glyphs(e))) {
        put_entry(&b, e, e);
        stats.added++;
      }
    }
  }
  free(z.v);
  for (int i = 0; i < nsrc; i++)
    free(t[i].v);
  if (b.fail) {
    free(b.p);
    return NULL;
  }
  if (!b.p)
    put(&b, "", 0);
  if (b.fail)
    return NULL;
  stats.keys = z.n;
  if (st)
    *st = stats;
  *out_len = b.n;
  return b.p;
}

char *pvz_strings_subset(const char *table, size_t len, const char *prefix, size_t *out_len) {
  Table t = {0};
  Buf b = {0};
  b.crlf = memmem(table, len, "\r\n", 2) != NULL;
  if (parse(table, len, &t) != 0)
    return NULL;
  size_t pl = strlen(prefix);
  for (int k = 0; k < t.n; k++)
    if (t.v[k].klen >= pl && !memcmp(t.v[k].key, prefix, pl))
      put_entry(&b, &t.v[k], &t.v[k]);
  free(t.v);
  if (b.fail || !b.p) {
    free(b.p);
    return NULL;
  }
  *out_len = b.n;
  return b.p;
}

/* The game's button tags in text, as the Switch's buttons: pictures (with
 * icons: U+E000 + n, the fonts' button layer, pvz_english.c font_buttons) or
 * names. In the Xbox 360 scheme the engine leaves the tags in the text
 * (TodDrawString spells <A> <B> <X> out only for the TV remote), so "Press <S>
 * to join!" came out as it is written (hardware run 13). With icons the
 * button names the engine puts in for <A> and <B> (OK_BUTTON, BACK_BUTTON:
 * the title's "Press A to start") become pictures too. Not MENU_BUTTON: it is
 * also the in-game pause button's own label ("Menu"; hardware run 20 showed a
 * lone X there). Key lines ([<A>] and the like) stay. */
#include "pvz_button_icons.h"
static const struct {
  const char *tag, *name;
  int icon;
} k_buttons[] = {
    {"<LT>", "ZL", PVZ_ICON_ZL},        {"<RT>", "ZR", PVZ_ICON_ZR},       {"<A>", "A", PVZ_ICON_A},
    {"<B>", "B", PVZ_ICON_B},           {"<X>", "X", PVZ_ICON_X},          {"<Y>", "Y", PVZ_ICON_Y},
    {"<S>", "+", PVZ_ICON_PLUS},        {"<L>", "L", PVZ_ICON_L},          {"<R>", "R", PVZ_ICON_R},
    {"<D>", "D-Pad", PVZ_ICON_DPAD},    {"<l>", "Left Stick", PVZ_ICON_LSTICK},
    {"<r>", "Right Stick", PVZ_ICON_RSTICK}, {"<b>", "-", PVZ_ICON_MINUS},
};
static const struct {
  const char *key;
  int icon;
} k_button_keys[] = {{"[OK_BUTTON]", PVZ_ICON_A}, {"[BACK_BUTTON]", PVZ_ICON_B}};

static size_t put_icon(char *out, int i) { /* U+E000 + i in UTF-8 */
  const unsigned cp = 0xE000u + (unsigned)i;
  out[0] = (char)(0xE0 | (cp >> 12));
  out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[2] = (char)(0x80 | (cp & 0x3F));
  return 3;
}

char *pvz_strings_button_names(const char *table, size_t len, size_t *out_len, int *replaced, int icons) {
  char *out = malloc(len * 4 + 1); /* no name is more than 4 times its tag */
  if (!out)
    return NULL;
  size_t n = 0;
  int count = 0, key_icon = -1; /* the icon a button-name key's value becomes */
  const char *p = table, *end = table + len;
  while (p < end) {
    const char *eol = memchr(p, '\n', (size_t)(end - p));
    const char *le = eol ? eol + 1 : end;
    const char *te = le;
    while (te > p && (te[-1] == '\n' || te[-1] == '\r'))
      te--;
    const int key = te > p && p[0] == '[' && te[-1] == ']';
    if (key) {
      key_icon = -1;
      for (unsigned i = 0; icons && i < sizeof k_button_keys / sizeof k_button_keys[0]; i++)
        if ((size_t)(te - p) == strlen(k_button_keys[i].key) && !memcmp(p, k_button_keys[i].key, (size_t)(te - p)))
          key_icon = k_button_keys[i].icon;
    } else if (key_icon >= 0 && te > p) { /* the value: the picture alone */
      n += put_icon(out + n, key_icon);
      memcpy(out + n, te, (size_t)(le - te));
      n += (size_t)(le - te);
      p = le, key_icon = -1, count++;
      continue;
    }
    while (p < le) {
      int done = 0;
      if (!key && *p == '<')
        for (unsigned i = 0; i < sizeof k_buttons / sizeof k_buttons[0]; i++) {
          const size_t tl = strlen(k_buttons[i].tag);
          if ((size_t)(le - p) >= tl && !memcmp(p, k_buttons[i].tag, tl)) {
            if (icons) {
              n += put_icon(out + n, k_buttons[i].icon);
            } else {
              const size_t nl = strlen(k_buttons[i].name);
              memcpy(out + n, k_buttons[i].name, nl);
              n += nl;
            }
            p += tl, count++, done = 1;
            break;
          }
        }
      if (!done)
        out[n++] = *p++;
    }
  }
  out[n] = 0;
  *out_len = n;
  if (replaced)
    *replaced = count;
  return out;
}
