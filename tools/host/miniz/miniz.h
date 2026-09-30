/* tools/host/miniz/miniz.h -- the few miniz calls pvz_english.c makes, over the
 * system zlib, so tools/test_english.py can run it on a PC (libnx32's miniz is
 * a prebuilt ARM library). Not miniz: just enough of its interface. MIT. */
#ifndef HOST_MINIZ_H
#define HOST_MINIZ_H
#include <stddef.h>
#include <stdint.h>

typedef unsigned int mz_uint;
typedef uint32_t mz_uint32;
typedef uint64_t mz_uint64;
typedef unsigned long mz_ulong;
typedef int mz_bool;
#define MZ_FALSE 0
#define MZ_TRUE 1
#define MZ_OK 0

typedef struct {
  mz_uint32 m_file_index;
  mz_uint32 m_crc32;
  mz_uint64 m_comp_size, m_uncomp_size, m_local_header_ofs;
  int m_method;
  mz_bool m_is_directory;
  char m_filename[512];
} mz_zip_archive_file_stat;

typedef struct {
  unsigned char *m_data;
  size_t m_size;
  int m_owns;
  mz_zip_archive_file_stat *m_ents;
  mz_uint m_n;
  char m_path[512]; /* init_file's path: MINIZ_TRACE names the entries read from it */
} mz_zip_archive;

mz_bool mz_zip_reader_init_file(mz_zip_archive *z, const char *path, mz_uint flags);
mz_bool mz_zip_reader_init_mem(mz_zip_archive *z, const void *mem, size_t size, mz_uint flags);
mz_bool mz_zip_reader_end(mz_zip_archive *z);
mz_uint mz_zip_reader_get_num_files(mz_zip_archive *z);
mz_bool mz_zip_reader_is_file_a_directory(mz_zip_archive *z, mz_uint i);
mz_uint mz_zip_reader_get_filename(mz_zip_archive *z, mz_uint i, char *buf, mz_uint cap);
mz_bool mz_zip_reader_file_stat(mz_zip_archive *z, mz_uint i, mz_zip_archive_file_stat *st);
void *mz_zip_reader_extract_to_heap(mz_zip_archive *z, mz_uint i, size_t *len, mz_uint flags);
int mz_zip_reader_locate_file(mz_zip_archive *z, const char *name, const char *comment, mz_uint flags);
void mz_free(void *p);
int mz_uncompress(unsigned char *dst, mz_ulong *dst_len, const unsigned char *src, mz_ulong src_len);
void *tdefl_write_image_to_png_file_in_memory_ex(const void *img, int w, int h, int chans, size_t *len,
                                                 mz_uint level, mz_bool flip);
#endif
