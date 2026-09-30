/* tools/host/miniz_host.c -- see miniz/miniz.h. Zip reading (no zip64) and
 * PNG writing over the system zlib. MIT. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "miniz/miniz.h"

static unsigned u16(const unsigned char *p) { return p[0] | p[1] << 8; }
static unsigned u32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }

static mz_bool parse_cd(mz_zip_archive *z) {
  const unsigned char *d = z->m_data;
  size_t n = z->m_size, e;
  if (n < 22)
    return 0;
  for (e = n - 22; u32(d + e) != 0x06054b50; e--)
    if (e == 0 || n - e > 65557)
      return 0;
  mz_uint cnt = u16(d + e + 10);
  size_t off = u32(d + e + 16);
  z->m_ents = calloc(cnt ? cnt : 1, sizeof *z->m_ents);
  z->m_n = 0;
  for (mz_uint k = 0; k < cnt; k++) {
    if (off + 46 > n || u32(d + off) != 0x02014b50)
      return 0;
    mz_zip_archive_file_stat *s = &z->m_ents[z->m_n++];
    unsigned fl = u16(d + off + 28), xl = u16(d + off + 30), cl = u16(d + off + 32);
    s->m_file_index = k;
    s->m_method = (int)u16(d + off + 10);
    s->m_crc32 = u32(d + off + 16);
    s->m_comp_size = u32(d + off + 20);
    s->m_uncomp_size = u32(d + off + 24);
    s->m_local_header_ofs = u32(d + off + 42);
    size_t l = fl < sizeof s->m_filename - 1 ? fl : sizeof s->m_filename - 1;
    memcpy(s->m_filename, d + off + 46, l);
    s->m_filename[l] = 0;
    s->m_is_directory = l && s->m_filename[l - 1] == '/';
    off += 46 + fl + xl + cl;
  }
  return 1;
}

mz_bool mz_zip_reader_init_mem(mz_zip_archive *z, const void *mem, size_t size, mz_uint flags) {
  (void)flags;
  memset(z, 0, sizeof *z);
  z->m_data = (unsigned char *)mem;
  z->m_size = size;
  return parse_cd(z);
}

mz_bool mz_zip_reader_init_file(mz_zip_archive *z, const char *path, mz_uint flags) {
  (void)flags;
  memset(z, 0, sizeof *z);
  FILE *f = fopen(path, "rb");
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  z->m_data = malloc((size_t)n);
  if (!z->m_data || fread(z->m_data, 1, (size_t)n, f) != (size_t)n) {
    fclose(f);
    return 0;
  }
  fclose(f);
  z->m_size = (size_t)n;
  z->m_owns = 1;
  snprintf(z->m_path, sizeof z->m_path, "%s", path);
  return parse_cd(z);
}

/* MINIZ_TRACE=<file>: each entry extracted from an archive opened by path is
 * appended to <file> as "<archive path>\t<entry>" (tools/make_english_pack.py) */
static void trace(const mz_zip_archive *z, const char *name) {
  const char *to = getenv("MINIZ_TRACE");
  if (!to || !z->m_path[0])
    return;
  FILE *f = fopen(to, "a");
  if (f) {
    fprintf(f, "%s\t%s\n", z->m_path, name);
    fclose(f);
  }
}

mz_bool mz_zip_reader_end(mz_zip_archive *z) {
  if (z->m_owns)
    free(z->m_data);
  free(z->m_ents);
  memset(z, 0, sizeof *z);
  return 1;
}

mz_uint mz_zip_reader_get_num_files(mz_zip_archive *z) { return z->m_n; }
mz_bool mz_zip_reader_is_file_a_directory(mz_zip_archive *z, mz_uint i) {
  return i < z->m_n && z->m_ents[i].m_is_directory;
}
mz_uint mz_zip_reader_get_filename(mz_zip_archive *z, mz_uint i, char *buf, mz_uint cap) {
  if (i >= z->m_n || !cap)
    return 0;
  snprintf(buf, cap, "%s", z->m_ents[i].m_filename);
  return (mz_uint)strlen(buf) + 1;
}
mz_bool mz_zip_reader_file_stat(mz_zip_archive *z, mz_uint i, mz_zip_archive_file_stat *st) {
  if (i >= z->m_n)
    return 0;
  *st = z->m_ents[i];
  return 1;
}
int mz_zip_reader_locate_file(mz_zip_archive *z, const char *name, const char *comment, mz_uint flags) {
  (void)comment, (void)flags;
  for (mz_uint i = 0; i < z->m_n; i++)
    if (!strcmp(z->m_ents[i].m_filename, name))
      return (int)i;
  return -1;
}

void *mz_zip_reader_extract_to_heap(mz_zip_archive *z, mz_uint i, size_t *len, mz_uint flags) {
  (void)flags;
  if (i >= z->m_n)
    return NULL;
  const mz_zip_archive_file_stat *s = &z->m_ents[i];
  trace(z, s->m_filename);
  const unsigned char *lh = z->m_data + s->m_local_header_ofs;
  if (u32(lh) != 0x04034b50)
    return NULL;
  const unsigned char *src = lh + 30 + u16(lh + 26) + u16(lh + 28);
  unsigned char *out = malloc(s->m_uncomp_size ? s->m_uncomp_size : 1);
  if (!out)
    return NULL;
  if (s->m_method == 0) {
    memcpy(out, src, s->m_uncomp_size);
  } else if (s->m_method == 8) {
    z_stream st;
    memset(&st, 0, sizeof st);
    inflateInit2(&st, -15);
    st.next_in = (unsigned char *)src;
    st.avail_in = (uInt)s->m_comp_size;
    st.next_out = out;
    st.avail_out = (uInt)s->m_uncomp_size;
    int r = inflate(&st, Z_FINISH);
    inflateEnd(&st);
    if (r != Z_STREAM_END) {
      free(out);
      return NULL;
    }
  } else {
    free(out);
    return NULL;
  }
  if ((mz_uint32)crc32(0, out, (uInt)s->m_uncomp_size) != s->m_crc32) {
    free(out);
    return NULL;
  }
  *len = s->m_uncomp_size;
  return out;
}

void mz_free(void *p) { free(p); }

int mz_uncompress(unsigned char *dst, mz_ulong *dst_len, const unsigned char *src, mz_ulong src_len) {
  uLongf n = *dst_len;
  int r = uncompress(dst, &n, src, src_len);
  *dst_len = n;
  return r == Z_OK ? MZ_OK : -1;
}

static void chunk(unsigned char **o, const char *type, const unsigned char *d, size_t n) {
  unsigned char *p = *o;
  p[0] = n >> 24, p[1] = n >> 16, p[2] = n >> 8, p[3] = n;
  memcpy(p + 4, type, 4);
  if (n)
    memcpy(p + 8, d, n);
  uLong c = crc32(0, p + 4, (uInt)(n + 4));
  p[8 + n] = c >> 24, p[9 + n] = c >> 16, p[10 + n] = c >> 8, p[11 + n] = c;
  *o = p + 12 + n;
}

void *tdefl_write_image_to_png_file_in_memory_ex(const void *img, int w, int h, int chans, size_t *len,
                                                 mz_uint level, mz_bool flip) {
  size_t row = (size_t)w * chans, raw_n = (row + 1) * h;
  unsigned char *raw = malloc(raw_n);
  for (int y = 0; y < h; y++) {
    raw[y * (row + 1)] = 0;
    memcpy(raw + y * (row + 1) + 1, (const unsigned char *)img + (flip ? h - 1 - y : y) * row, row);
  }
  uLongf zn = compressBound(raw_n);
  unsigned char *zd = malloc(zn);
  compress2(zd, &zn, raw, raw_n, (int)level);
  free(raw);
  unsigned char *out = malloc(zn + 64), *o = out;
  memcpy(o, "\x89PNG\r\n\x1a\n", 8);
  o += 8;
  static const int ctype[] = {0, 0, 4, 2, 6};
  unsigned char ihdr[13] = {w >> 24, w >> 16, w >> 8, w, h >> 24, h >> 16, h >> 8, h, 8, ctype[chans], 0, 0, 0};
  chunk(&o, "IHDR", ihdr, 13);
  chunk(&o, "IDAT", zd, zn);
  chunk(&o, "IEND", NULL, 0);
  free(zd);
  *len = (size_t)(o - out);
  return out;
}
