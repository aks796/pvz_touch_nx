/* pvz_pointer.c -- a pointer on the right stick, for the screens made for touch.
 *
 * Much of the newest mod is touch-only: the Zombatar editor (its KeyDown only
 * knows B), the VS lobbies and server lists (no key handling at all), the
 * replay manager, the mod's own dialogs. On a phone that is fine; docked it
 * leaves them unusable (hardware run 9). So the right stick (which the game
 * does not use) moves a pointer, drawn with the game's own arrow
 * (images/cursor_pointer.png from game.apk), and ZR taps -- a touch at the
 * pointer, held while ZR is held, so drags work too. ZR is taken only while
 * the pointer shows (moved in the last few seconds); otherwise it is the
 * game's ZR as before. pvz_input.c moves it and sends the touches; this file
 * holds the state and draws it.
 *
 * Drawing: once per frame, just before eglSwapBuffers, on the engine's thread
 * with its context current, by gl_blit.c: a quad as the engine draws its own,
 * every state it changes put back exactly, since the engine caches its GL
 * state from frame to frame. MIT.
 */
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz/miniz.h>

#include "config.h"
#include "pvz_apks.h"
#include "gl_blit.h"
#include "gl_layer.h"
#include "util.h"

const char *dcr_game_root(void); /* main.c */
void dcr_window_size(int *w, int *h);

/* ---------------------------------------------------------------- state */
static volatile float g_x, g_y;
static volatile int g_shown, g_pressed;

void pvz_pointer_set(float x, float y, int shown, int pressed) {
  g_x = x;
  g_y = y;
  g_pressed = pressed;
  g_shown = shown;
}

/* ---------------------------------------------------------------- image */
static uint8_t *g_rgba;
static int g_w, g_h, g_hot_x, g_hot_y;

static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

static int paeth(int a, int b, int c) {
  int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
  return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

/* 8-bit RGBA, not interlaced (what the game's cursors are) */
static uint8_t *png_rgba(const uint8_t *p, size_t n, int *w, int *h) {
  if (n < 33 || memcmp(p, "\x89PNG\r\n\x1a\n", 8))
    return NULL;
  int width = 0, height = 0;
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
      if (d[8] != 8 || d[9] != 6 || d[12] != 0 || width <= 0 || height <= 0 || width > 512 || height > 512)
        return NULL;
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
  size_t raw_n = 0;
  uint8_t *raw = z ? tinfl_decompress_mem_to_heap(z, zn, &raw_n, TINFL_FLAG_PARSE_ZLIB_HEADER) : NULL;
  free(z);
  const size_t stride = (size_t)width * 4;
  if (!raw || raw_n < (stride + 1) * (size_t)height) {
    mz_free(raw);
    return NULL;
  }
  uint8_t *out = malloc(stride * (size_t)height);
  for (int y = 0; out && y < height; y++) {
    const uint8_t *s = raw + y * (stride + 1) + 1;
    uint8_t *d = out + y * stride, *up = y ? d - stride : NULL;
    const int f = raw[y * (stride + 1)];
    for (size_t i = 0; i < stride; i++) {
      int a = i >= 4 ? d[i - 4] : 0, b = up ? up[i] : 0, c = (up && i >= 4) ? up[i - 4] : 0;
      int v = s[i];
      switch (f) {
      case 1: v += a; break;
      case 2: v += b; break;
      case 3: v += (a + b) / 2; break;
      case 4: v += paeth(a, b, c); break;
      default: break;
      }
      d[i] = (uint8_t)v;
    }
  }
  mz_free(raw);
  *w = width;
  *h = height;
  return out;
}

/* On the UI thread at start-up: the arrow out of game.apk, and its tip (the
 * first solid pixel, top-down). */
void pvz_pointer_init(void) {
  char apk[300];
  snprintf(apk, sizeof apk, "%s", pvz_game_apk());
  mz_zip_archive zip;
  memset(&zip, 0, sizeof zip);
  if (!mz_zip_reader_init_file(&zip, apk, 0))
    return;
  size_t n = 0;
  void *png = mz_zip_reader_extract_file_to_heap(&zip, "assets/files/images/cursor_pointer.png", &n, 0);
  mz_zip_reader_end(&zip);
  if (png)
    g_rgba = png_rgba(png, n, &g_w, &g_h);
  mz_free(png);
  if (!g_rgba) {
    debugPrintf("[pointer] no cursor picture in game.apk: the pointer is off\n");
    return;
  }
  for (int i = 0; i < g_w * g_h; i++)
    if (g_rgba[i * 4 + 3] > 128) {
      g_hot_x = i % g_w;
      g_hot_y = i / g_w;
      break;
    }
}

/* ---------------------------------------------------------------- drawing */
typedef void (*t_geti)(GLenum, GLint *);
typedef void (*t_bind)(GLenum, GLuint);
typedef void (*t_gen)(GLsizei, GLuint *);
typedef void (*t_teximage)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
typedef void (*t_texparami)(GLenum, GLenum, GLint);
typedef void (*t_pixelstorei)(GLenum, GLint);

static struct {
  t_geti GetIntegerv;
  t_bind BindTexture;
  t_gen GenTextures;
  t_teximage TexImage2D;
  t_texparami TexParameteri;
  t_pixelstorei PixelStorei;
} G;
static GLuint g_tex;
static int g_gl_state; /* 0 not set up, 1 ready, -1 unavailable */

#define L(name) (G.name = (void *)dcr_gl_lookup("gl" #name))
static int gl_setup(void) {
  L(GetIntegerv), L(BindTexture), L(GenTextures), L(TexImage2D), L(TexParameteri), L(PixelStorei);
  if (!G.GetIntegerv || !G.BindTexture || !G.GenTextures || !G.TexImage2D || !G.TexParameteri ||
      !G.PixelStorei || dcr_blit_setup("pointer") < 0) {
    debugPrintf("[pointer] the GL driver lacks a basic call: the pointer is not drawn\n");
    return -1;
  }
  GLint bound = 0, align = 4;
  G.GetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
  G.GetIntegerv(GL_UNPACK_ALIGNMENT, &align);
  G.GenTextures(1, &g_tex);
  G.BindTexture(GL_TEXTURE_2D, g_tex);
  G.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
  G.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_w, g_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, g_rgba);
  G.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  G.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  G.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  G.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  G.PixelStorei(GL_UNPACK_ALIGNMENT, align);
  G.BindTexture(GL_TEXTURE_2D, (GLuint)bound);
  return 1;
}

/* Before eglSwapBuffers, on the engine's thread: the picture blended over
 * the frame (gl_blit.c keeps the game's GL state). */
void pvz_pointer_draw(void) {
  if (!g_shown || !g_rgba)
    return;
  if (!g_gl_state)
    g_gl_state = gl_setup();
  if (g_gl_state < 0)
    return;
  GLint fbo = 0;
  G.GetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &fbo);
  if (fbo)
    return; /* not drawing to the screen right now */
  int vw, vh;
  dcr_window_size(&vw, &vh);
  const float s = (float)vh / 720.0f; /* the picture is sized for 720p */
  const int w = (int)(g_w * s), h = (int)(g_h * s);
  const int x = (int)(g_x - g_hot_x * s), y = (int)(g_y - g_hot_y * s);
  dcr_blit(g_tex, x, y, w, h, 1, 0);
}
