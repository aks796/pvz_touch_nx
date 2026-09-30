/* pvz_strings.h -- the string tables in English (pvz_strings.c). */
#ifndef PVZ_STRINGS_H
#define PVZ_STRINGS_H
#include <stddef.h>
#include <stdint.h>

#define PVZ_STR_MAX_SRC 6

/* One English source, UTF-8 (a BOM is skipped). NULL text: none. */
typedef struct {
  const char *text;
  size_t len;
  int add_new; /* also add the keys the Chinese table does not have */
  int plain;   /* refuse its texts with a Latin-1 letter (U+00C0..U+00FF: the
                * English builds' button glyphs, which only some fonts draw as
                * buttons) */
} PvzStrSrc;

typedef struct {
  int keys;                  /* the Chinese table's */
  int from[PVZ_STR_MAX_SRC]; /* texts taken from each source */
  int chinese;               /* left Chinese: no source had usable text */
  int rejected;              /* source texts skipped (blank or other conversions) */
  int added;                 /* keys only a source had */
} PvzStrStats;

/* The Chinese table `zh` (UTF-8) with each key's text from the first source
 * that has a usable one (pvz_strings.c). malloc'd, NUL-terminated; NULL on
 * failure. */
char *pvz_strings_pick(const char *zh, size_t zh_len, const PvzStrSrc *src, int nsrc,
                       size_t *out_len, PvzStrStats *st);

/* The entries of `table` whose key starts with `prefix`; NULL if none. */
char *pvz_strings_subset(const char *table, size_t len, const char *prefix, size_t *out_len);
/* table with the game's button tags in its text (<A>, <S>, <LT>...) as the
 * Switch's buttons: with icons, their pictures' characters (U+E000 + n: the
 * fonts' button layer) and OK_BUTTON/BACK_BUTTON/MENU_BUTTON too; else their
 * names (A, +, ZL...). Key lines kept. */
char *pvz_strings_button_names(const char *table, size_t len, size_t *out_len, int *replaced, int icons);

/* UTF-16 (BOM optional) -> malloc'd UTF-8. */
char *pvz_utf16_to_utf8(const uint8_t *in, size_t n, size_t *out_len);

#endif
