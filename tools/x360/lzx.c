/* lzx.c -- an LZX decoder (the MS-LZX bitstream as XMemCompress writes it:
 * 16-bit little-endian words read MSB first, the decoder's window, trees and
 * repeat offsets kept across frames, the bit buffer reset at each frame).
 *
 *   lzx <in> <offset> <window_bits> <out> [maxframes]
 *     reads frames from <offset>: a 2-byte big-endian compressed size, or 0xFF
 *     followed by a 2-byte uncompressed size and a 2-byte compressed size; a
 *     zero size ends the stream. Each frame decompresses to 0x8000 bytes
 *     (or its given size). Prints where the stream ended.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_MATCH 2
#define NUM_CHARS 256
#define NUM_PRIMARY_LENGTHS 7
#define NUM_SECONDARY_LENGTHS 249
#define MAXSYMS 720

static uint32_t position_base[51];
static uint8_t extra_bits[52];

/* ------------------------------------------------------------- bits */
static const uint8_t *in_p, *in_end;
static uint32_t bitbuf;
static int bitsleft;

static void bits_init(const uint8_t *p, const uint8_t *end) {
  in_p = p, in_end = end, bitbuf = 0, bitsleft = 0;
}
static inline void ensure(int n) {
  while (bitsleft < n) {
    uint32_t lo = in_p < in_end ? *in_p : 0;
    in_p++;
    uint32_t hi = in_p < in_end ? *in_p : 0;
    in_p++;
    bitbuf |= ((hi << 8) | lo) << (32 - 16 - bitsleft);
    bitsleft += 16;
  }
}
static inline uint32_t peek(int n) { return n ? bitbuf >> (32 - n) : 0; }
static inline void drop(int n) { bitbuf <<= n; bitsleft -= n; }
static inline uint32_t readbits(int n) {
  if (!n) return 0;
  ensure(n);
  uint32_t v = peek(n);
  drop(n);
  return v;
}

/* --------------------------------------------------------- huffman */
typedef struct {
  int nsyms;
  uint8_t len[MAXSYMS];
  int count[17], first[17], offset[17];
  uint16_t sorted[MAXSYMS];
  int empty;
} Tree;

static int make_tree(Tree *t) {
  memset(t->count, 0, sizeof t->count);
  for (int i = 0; i < t->nsyms; i++)
    if (t->len[i] > 16) return -1;
    else t->count[t->len[i]]++;
  t->count[0] = 0;
  int code = 0, off = 0, any = 0;
  for (int l = 1; l <= 16; l++) {
    t->first[l] = code;
    t->offset[l] = off;
    off += t->count[l];
    code = (code + t->count[l]) << 1;
    any |= t->count[l];
  }
  t->empty = !any;
  int pos[17];
  for (int l = 1; l <= 16; l++) pos[l] = t->offset[l];
  for (int i = 0; i < t->nsyms; i++)
    if (t->len[i]) t->sorted[pos[t->len[i]]++] = (uint16_t)i;
  return 0;
}

static int read_sym(const Tree *t) {
  ensure(16);
  int code = 0;
  for (int l = 1; l <= 16; l++) {
    code = (code << 1) | (int)((bitbuf >> (31 - (l - 1))) & 1);
    int idx = code - t->first[l];
    if (t->count[l] && idx >= 0 && idx < t->count[l]) {
      drop(l);
      return t->sorted[t->offset[l] + idx];
    }
  }
  return -1;
}

/* ------------------------------------------------------------ state */
static uint8_t *window;
static uint32_t window_size, window_posn;
static uint32_t R0 = 1, R1 = 1, R2 = 1;
static int main_elements, header_read, block_type;
static uint32_t block_remaining, block_length;
static Tree pretree, maintree, lengthtree, alignedtree;
static int intel_started;
static int32_t intel_filesize;
static uint32_t frames_done;

static void reset_state(void) {
  memset(window, 0, window_size);
  window_posn = 0;
  R0 = R1 = R2 = 1;
  header_read = 0;
  block_type = 0;
  block_remaining = block_length = 0;
  memset(maintree.len, 0, sizeof maintree.len);
  memset(lengthtree.len, 0, sizeof lengthtree.len);
  intel_started = 0;
  intel_filesize = 0;
  frames_done = 0;
}

static int read_lengths(uint8_t *lens, int first, int last) {
  pretree.nsyms = 20;
  for (int x = 0; x < 20; x++) pretree.len[x] = (uint8_t)readbits(4);
  if (make_tree(&pretree)) return -1;
  for (int x = first; x < last;) {
    int z = read_sym(&pretree);
    if (z < 0) return -1;
    if (z == 17) {
      int y = (int)readbits(4) + 4;
      while (y-- && x < last) lens[x++] = 0;
    } else if (z == 18) {
      int y = (int)readbits(5) + 20;
      while (y-- && x < last) lens[x++] = 0;
    } else if (z == 19) {
      int y = (int)readbits(1) + 4;
      z = read_sym(&pretree);
      if (z < 0) return -1;
      z = lens[x] - z;
      if (z < 0) z += 17;
      while (y-- && x < last) lens[x++] = (uint8_t)z;
    } else {
      z = lens[x] - z;
      if (z < 0) z += 17;
      lens[x++] = (uint8_t)z;
    }
  }
  return 0;
}

static int decompress_frame(const uint8_t *in, size_t inlen, uint8_t *out, uint32_t outlen) {
  bits_init(in, in + inlen);
  if (!header_read) {
    if (readbits(1)) {
      uint32_t i = readbits(16), j = readbits(16);
      intel_filesize = (int32_t)((i << 16) | j);
    }
    header_read = 1;
  }
  uint32_t togo = outlen;
  while (togo > 0) {
    if (block_remaining == 0) {
      if (block_type == 3) {
        if (block_length & 1) in_p++; /* realign */
        bits_init(in_p, in + inlen);
      }
      block_type = (int)readbits(3);
      uint32_t i = readbits(16), j = readbits(8);
      block_remaining = block_length = (i << 8) | j;
      switch (block_type) {
      case 2:
        alignedtree.nsyms = 8;
        for (int k = 0; k < 8; k++) alignedtree.len[k] = (uint8_t)readbits(3);
        if (make_tree(&alignedtree)) return -1;
        /* fall through */
      case 1:
        if (read_lengths(maintree.len, 0, 256) || read_lengths(maintree.len, 256, main_elements)) return -1;
        maintree.nsyms = main_elements;
        if (make_tree(&maintree)) return -1;
        if (maintree.len[0xE8]) intel_started = 1;
        if (read_lengths(lengthtree.len, 0, NUM_SECONDARY_LENGTHS)) return -1;
        lengthtree.nsyms = NUM_SECONDARY_LENGTHS;
        if (make_tree(&lengthtree)) return -1;
        break;
      case 3: {
        intel_started = 1;
        ensure(16);
        if (bitsleft > 16) in_p -= 2; /* realign to the bytes */
        const uint8_t *p = in_p;
        R0 = p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
        R1 = p[4] | p[5] << 8 | p[6] << 16 | (uint32_t)p[7] << 24;
        R2 = p[8] | p[9] << 8 | p[10] << 16 | (uint32_t)p[11] << 24;
        in_p += 12;
        break;
      }
      default:
        return -2;
      }
    }
    uint32_t this_run;
    while ((this_run = block_remaining) > 0 && togo > 0) {
      if (this_run > togo) this_run = togo;
      togo -= this_run;
      block_remaining -= this_run;
      window_posn &= window_size - 1;
      if (window_posn + this_run > window_size) return -3;
      if (block_type == 3) {
        if (in_p + this_run > in + inlen) return -4;
        memcpy(window + window_posn, in_p, this_run);
        in_p += this_run;
        window_posn += this_run;
        continue;
      }
      while ((int32_t)this_run > 0) {
        int me = read_sym(&maintree);
        if (me < 0) return -5;
        if (me < NUM_CHARS) {
          window[window_posn++] = (uint8_t)me;
          this_run--;
          continue;
        }
        me -= NUM_CHARS;
        uint32_t match_length = me & NUM_PRIMARY_LENGTHS;
        if (match_length == NUM_PRIMARY_LENGTHS) {
          int lf = read_sym(&lengthtree);
          if (lf < 0) return -6;
          match_length += lf;
        }
        match_length += MIN_MATCH;
        uint32_t match_offset = (uint32_t)me >> 3;
        if (match_offset > 2) {
          if (block_type == 1) {
            if (match_offset != 3) {
              int extra = extra_bits[match_offset];
              match_offset = position_base[match_offset] - 2 + readbits(extra);
            } else {
              match_offset = 1;
            }
          } else {
            int extra = extra_bits[match_offset];
            match_offset = position_base[match_offset] - 2;
            if (extra > 3) {
              extra -= 3;
              match_offset += readbits(extra) << 3;
              int ab = read_sym(&alignedtree);
              if (ab < 0) return -7;
              match_offset += ab;
            } else if (extra == 3) {
              int ab = read_sym(&alignedtree);
              if (ab < 0) return -7;
              match_offset += ab;
            } else if (extra > 0) {
              match_offset += readbits(extra);
            } else {
              match_offset = 1;
            }
          }
          R2 = R1, R1 = R0, R0 = match_offset;
        } else if (match_offset == 0) {
          match_offset = R0;
        } else if (match_offset == 1) {
          match_offset = R1, R1 = R0, R0 = match_offset;
        } else {
          match_offset = R2, R2 = R0, R0 = match_offset;
        }
        uint32_t rundest = window_posn, runsrc;
        this_run -= match_length;
        if (window_posn >= match_offset) {
          runsrc = rundest - match_offset;
        } else {
          runsrc = rundest + (window_size - match_offset);
          uint32_t copy_length = match_offset - window_posn;
          if (copy_length < match_length) {
            match_length -= copy_length;
            window_posn += copy_length;
            while (copy_length--) window[rundest++] = window[runsrc++];
            runsrc = 0;
          }
        }
        window_posn += match_length;
        if (window_posn > window_size) return -8;
        while (match_length--) window[rundest++] = window[runsrc++];
      }
    }
  }
  uint32_t start = window_posn == 0 ? window_size : window_posn;
  start -= outlen;
  memcpy(out, window + start, outlen);
  /* the E8 call translation, when on */
  if (intel_started && intel_filesize && outlen > 10) {
    /* frames are 0x8000: the translation is done per frame, by position */
    uint8_t *d = out, *dend = out + outlen - 10;
    int32_t curpos = (int32_t)(frames_done * 0x8000);
    while (d < dend) {
      if (*d++ != 0xE8) { curpos++; continue; }
      int32_t abs_off = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
      if (abs_off >= -curpos && abs_off < intel_filesize) {
        int32_t rel_off = abs_off >= 0 ? abs_off - curpos : abs_off + intel_filesize;
        d[0] = (uint8_t)rel_off, d[1] = (uint8_t)(rel_off >> 8), d[2] = (uint8_t)(rel_off >> 16),
        d[3] = (uint8_t)(rel_off >> 24);
      }
      d += 4;
      curpos += 5;
    }
    frames_done++;
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 5) {
    fprintf(stderr, "lzx <in> <offset> <window_bits> <out> [maxframes]\n");
    return 2;
  }
  FILE *f = fopen(argv[1], "rb");
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t *d = malloc((size_t)n + 16);
  if (fread(d, 1, (size_t)n, f) != (size_t)n) return 1;
  memset(d + n, 0, 16);
  fclose(f);
  long p = strcmp(argv[2], "seg") ? strtol(argv[2], NULL, 0) : 0;
  int wbits = atoi(argv[3]);
  long maxframes = argc > 5 ? atol(argv[5]) : -1;

  for (int i = 0, j = 0; i <= 50; i += 2) {
    extra_bits[i] = extra_bits[i + 1] = (uint8_t)j;
    if (i != 0 && j < 17) j++;
  }
  for (int i = 0, j = 0; i <= 50; i++) {
    position_base[i] = (uint32_t)j;
    j += 1 << extra_bits[i];
  }
  window_size = 1u << wbits;
  window = calloc(window_size, 1);
  int posn_slots = wbits == 20 ? 42 : wbits == 21 ? 50 : wbits << 1;
  main_elements = NUM_CHARS + (posn_slots << 3);

  FILE *o = fopen(argv[4], "wb");
  static uint8_t out[0x10000];
  if (!strcmp(argv[2], "seg")) {
    /* PopCap's XBLA main.pak: 0x0FF512ED, version, a u32, a u32, then one big-endian
     * uncompressed size per 0x20000-byte block of compressed data (the first block
     * starts after the table); each block is its own LZX stream */
    long nseg = 0;
    long hdr_end = 0;
    for (long k = 0;; k++) {
      long q = 16 + 4 * k;
      hdr_end = q;
      if ((q & ~0x1FFFFL) != 0) break;
      nseg = k;
    }
    nseg = (n + 0x1FFFF) / 0x20000;
    hdr_end = 16 + 4 * nseg;
    long total = 0;
    for (long k = 0; k < nseg; k++) {
      uint32_t usize = (uint32_t)(d[16 + 4 * k] << 24 | d[17 + 4 * k] << 16 | d[18 + 4 * k] << 8 | d[19 + 4 * k]);
      long q = k ? k * 0x20000L : hdr_end;
      long qend = (k + 1) * 0x20000L;
      reset_state();
      uint32_t left = usize;
      if (d[q] == 0 && d[q + 1] == 0 && q + 2 + (long)usize <= qend) {
        /* stored: a zero word, then the bytes as they are (incompressible data) */
        fwrite(d + q + 2, 1, usize, o);
        total += usize;
        continue;
      }
      while (left) {
        uint32_t fs = left < 0x8000 ? left : 0x8000, bs;
        if (d[q] == 0xFF) {
          fs = (uint32_t)(d[q + 1] << 8 | d[q + 2]);
          bs = (uint32_t)(d[q + 3] << 8 | d[q + 4]);
          q += 5;
        } else {
          bs = (uint32_t)(d[q] << 8 | d[q + 1]);
          q += 2;
        }
        if (!bs || q + (long)bs > qend + 0x10) {
          printf("segment %ld: bad frame at 0x%lx (%u)\n", k, q, bs);
          return 1;
        }
        int rc = decompress_frame(d + q, bs, out, fs);
        if (rc) {
          printf("segment %ld: frame at 0x%lx: error %d\n", k, q, rc);
          return 1;
        }
        fwrite(out, 1, fs, o);
        q += bs;
        left -= fs;
        total += fs;
      }
    }
    fclose(o);
    printf("%ld segments, %ld bytes\n", nseg, total);
    return 0;
  }
  long frames = 0, total = 0;
  while (p < n && (maxframes < 0 || frames < maxframes)) {
    uint32_t fs = 0x8000, bs;
    if (d[p] == 0xFF) {
      fs = (uint32_t)(d[p + 1] << 8 | d[p + 2]);
      bs = (uint32_t)(d[p + 3] << 8 | d[p + 4]);
      p += 5;
    } else {
      bs = (uint32_t)(d[p] << 8 | d[p + 1]);
      p += 2;
    }
    if (!bs || !fs) {
      printf("end marker at 0x%lx after %ld frames, %ld bytes out\n", p - 2, frames, total);
      break;
    }
    if (p + (long)bs > n) {
      printf("frame %ld runs past the end (0x%lx + %u)\n", frames, p, bs);
      break;
    }
    int rc = decompress_frame(d + p, bs, out, fs);
    if (rc) {
      printf("frame %ld at 0x%lx: error %d (%ld bytes out)\n", frames, p, rc, total);
      break;
    }
    fwrite(out, 1, fs, o);
    total += fs;
    p += bs;
    frames++;
  }
  printf("stopped at 0x%lx: %ld frames, %ld bytes\n", p, frames, total);
  fclose(o);
  return 0;
}
