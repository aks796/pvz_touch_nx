/* pvz_audio.c -- the game's AudioOutput, played through audout.
 *
 * libnative_code's Native::AudioOutput calls the Java AudioOutput:
 * setup(rate, channels, bits) once, then write(ByteBuffer, offset, length)
 * from the engine's sound thread (Audiere mixes, the AG audio layer pushes
 * ~8 KB blocks), and AudioTrack.write in the Java blocks until the device has
 * room -- which is what paces the mixer. This file does the same with audout:
 * the PCM is converted to the device's 48 kHz stereo s16 (linear resampling,
 * continuous across writes) and queued in three 1024-frame buffers; a write
 * blocks while all three are queued (~64 ms of sound ahead).
 *
 * The mod's alternative output, OpenSL ES, is not available here
 * (slCreateEngine fails), and nativeEnableOpenSL is never called.
 *
 * audout's buffer descriptor is an IPC structure with 64-bit fields for every
 * client, while libnx32's AudioOutBuffer has 32-bit pointers, so append and
 * get-released are issued with the right layout here (from the Crossy Road
 * port, where the self-test below proved it on hardware). MIT.
 */
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <switch.h>

#include "pvz.h"
#include "util.h"

typedef struct {
  u64 next, buffer, buffer_size, data_size, data_offset;
} AoBuf;
_Static_assert(sizeof(AoBuf) == 0x28, "audout buffer descriptor");

#define NBUF 3
#define FRAMES_PER_BUF 1024              /* 1024 * 4 bytes = one 0x1000 page */
#define BUF_BYTES (FRAMES_PER_BUF * 4)

static AoBuf g_bufs[NBUF] __attribute__((aligned(16)));
static int16_t *g_pcm[NBUF];
static int g_queued[NBUF];
static int g_ao_ready;
static u32 g_out_rate = 48000;

static Result ao_append(AoBuf *b) {
  u64 tag = (u64)(uintptr_t)b;
  const bool auto_ = hosversionAtLeast(3, 0, 0);
  return serviceDispatchIn(audoutGetServiceSession_AudioOut(), auto_ ? 7 : 3, tag,
                           .buffer_attrs = {auto_ ? (SfBufferAttr_HipcAutoSelect | SfBufferAttr_In)
                                                  : (SfBufferAttr_HipcMapAlias | SfBufferAttr_In)},
                           .buffers = {{b, sizeof(*b)}});
}

static Result ao_released(u64 *tags, u32 max, u32 *count) {
  const bool auto_ = hosversionAtLeast(3, 0, 0);
  return serviceDispatchOut(audoutGetServiceSession_AudioOut(), auto_ ? 8 : 5, *count,
                            .buffer_attrs = {auto_ ? (SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out)
                                                   : (SfBufferAttr_HipcMapAlias | SfBufferAttr_Out)},
                            .buffers = {{tags, max * sizeof(u64)}});
}

/* Mark the buffers the audio server has finished with as free again. */
static void reap(void) {
  u64 tags[NBUF] = {0};
  u32 n = 0;
  if (R_SUCCEEDED(ao_released(tags, NBUF, &n)))
    for (u32 k = 0; k < n && k < NBUF; k++)
      for (int i = 0; i < NBUF; i++)
        if (tags[k] == (u64)(uintptr_t)&g_bufs[i])
          g_queued[i] = 0;
}

/* Index of a free buffer (reaping only when none is), or -1. */
static int free_buffer(void) {
  for (int pass = 0; pass < 2; pass++) {
    for (int i = 0; i < NBUF; i++)
      if (!g_queued[i])
        return i;
    reap();
  }
  return -1;
}

static int dcr_audio_open(void) {
  if (g_ao_ready)
    return 0;
  Result rc = audoutInitialize();
  if (R_FAILED(rc)) {
    debugPrintf("[audio] audoutInitialize failed 0x%x\n", rc);
    return -1;
  }
  rc = audoutStartAudioOut();
  if (R_FAILED(rc)) {
    debugPrintf("[audio] audoutStartAudioOut failed 0x%x\n", rc);
    audoutExit();
    return -1;
  }
  g_out_rate = audoutGetSampleRate() ? audoutGetSampleRate() : 48000;
  for (int i = 0; i < NBUF; i++) {
    g_pcm[i] = memalign(0x1000, BUF_BYTES);
    if (!g_pcm[i])
      return -1;
    memset(g_pcm[i], 0, BUF_BYTES);
    g_bufs[i].buffer = (u64)(uintptr_t)g_pcm[i];
    g_bufs[i].buffer_size = BUF_BYTES;
    g_bufs[i].data_size = BUF_BYTES;
  }
  g_ao_ready = 1;
  debugPrintf("[audio] audout open: %u Hz, %u ch\n", (unsigned)g_out_rate,
              (unsigned)audoutGetChannelCount());
  return 0;
}

static unsigned long g_underruns, g_append_fails, g_dropped, g_submits;

/* Queue one full buffer of 48 kHz stereo s16; blocks while all are in use. */
static void submit(const int16_t *frames) {
  int i;
  reap();
  int queued = 0;
  for (int k = 0; k < NBUF; k++)
    queued += g_queued[k];
  if (!queued && g_submits > NBUF)
    g_underruns++; /* audout had nothing left to play */
  while ((i = free_buffer()) < 0)
    svcSleepThread(2000000ll);
  memcpy(g_pcm[i], frames, BUF_BYTES);
  armDCacheFlush(g_pcm[i], BUF_BYTES);
  g_bufs[i].data_size = BUF_BYTES;
  g_bufs[i].data_offset = 0;
  for (int attempt = 0; attempt < 5; attempt++) {
    Result rc = ao_append(&g_bufs[i]);
    if (R_SUCCEEDED(rc)) {
      g_queued[i] = 1;
      g_submits++;
      return;
    }
    if (g_append_fails++ < 3)
      debugPrintf("[audio] audout append failed 0x%x (retrying)\n", (unsigned)rc);
    svcSleepThread(2000000ll);
    reap();
  }
  g_dropped++;
}

/* Self-check of the descriptor layout, before the game runs: two buffers of
 * silence must come back from the audio server. */
void dcr_audio_selftest(void) {
  if (dcr_audio_open() != 0)
    return;
  static int16_t silence[FRAMES_PER_BUF * 2];
  submit(silence);
  submit(silence);
  u64 t0 = armGetSystemTick();
  int back = 0;
  while (armTicksToNs(armGetSystemTick() - t0) < 500000000ull) {
    reap();
    back = 0;
    for (int i = 0; i < NBUF; i++)
      back += !g_queued[i];
    if (back == NBUF)
      break;
    svcSleepThread(5000000ll);
  }
  debugPrintf("[audio] self-test: %s (%d/%d buffers returned in %llu ms)\n",
              back == NBUF ? "OK" : "FAILED -- buffer descriptor not accepted", back, NBUF,
              (unsigned long long)(armTicksToNs(armGetSystemTick() - t0) / 1000000ull));
}

/* ------------------------------------------------------ AudioOutput */
static Mutex g_lock;
static int g_rate, g_ch, g_bits, g_setup;
static volatile uint32_t g_writes;
static volatile int g_paused, g_closing;
static int16_t g_out[FRAMES_PER_BUF * 2];
static int g_out_n;
static double g_pos;           /* resampler position, relative to the current input */
static int16_t g_prev[2];      /* the previous input's last frame */
static unsigned long g_frames_in;
static u64 g_t_first;

uint32_t pvz_audio_writes(void) { return g_writes; }
void pvz_audio_pause(int paused) { g_paused = paused; }
void pvz_audio_close(void) { g_closing = 1; }

int pvz_audio_setup(int rate, int channels, int bits) {
  if (rate < 8000 || rate > 192000 || (channels != 1 && channels != 2) || (bits != 8 && bits != 16)) {
    debugPrintf("[audio] AudioOutput.setup(%d, %d, %d): not a format this output takes\n", rate,
                channels, bits);
    return 0;
  }
  if (dcr_audio_open() != 0)
    return 0;
  mutexLock(&g_lock);
  g_rate = rate, g_ch = channels, g_bits = bits;
  g_pos = 0.0;
  g_prev[0] = g_prev[1] = 0;
  g_out_n = 0;
  g_setup = 1;
  mutexUnlock(&g_lock);
  debugPrintf("[audio] AudioOutput.setup(%d Hz, %d ch, %d bit) -> audout %u Hz stereo\n", rate,
              channels, bits, (unsigned)g_out_rate);
  return 1;
}

void pvz_audio_shutdown(void) {
  mutexLock(&g_lock);
  g_setup = 0;
  mutexUnlock(&g_lock);
  debugPrintf("[audio] AudioOutput.shutdown()\n");
}

static inline void frame_at(const uint8_t *in, int i, int16_t out[2]) {
  if (i < 0) {
    out[0] = g_prev[0], out[1] = g_prev[1];
    return;
  }
  if (g_bits == 16) {
    const int16_t *s = (const int16_t *)in + i * g_ch;
    out[0] = s[0];
    out[1] = g_ch == 2 ? s[1] : s[0];
  } else {
    const uint8_t *s = in + i * g_ch;
    out[0] = (int16_t)(((int)s[0] - 128) << 8);
    out[1] = g_ch == 2 ? (int16_t)(((int)s[1] - 128) << 8) : out[0];
  }
}

void pvz_audio_write(const void *data, int len) {
  /* The engine's sound thread (audiere) mixes and writes without looking at
   * the activity: on Android the paused AudioTrack's blocking write holds it,
   * and this does the same while the game is in the background. When the game
   * is closing, writes are dropped instead: its shutdown may join this thread. */
  while (g_paused && !g_closing)
    svcSleepThread(10000000ll);
  if (g_closing) {
    svcSleepThread(5000000ll);
    return;
  }
  mutexLock(&g_lock);
  if (!g_setup) {
    mutexUnlock(&g_lock);
    return;
  }
  const uint8_t *in = data;
  const int frame_bytes = g_ch * (g_bits / 8);
  const int frames = len / frame_bytes;
  if (frames <= 0) {
    mutexUnlock(&g_lock);
    return;
  }
  if (!g_writes++) {
    g_t_first = armGetSystemTick();
    debugPrintf("[audio] first write: %d bytes\n", len);
  }
  g_frames_in += (unsigned long)frames;
  /* Linear resampling across write boundaries: index -1 is the previous
   * write's last frame, so pos runs over [-1, frames - 1). */
  const double step = (double)g_rate / (double)g_out_rate;
  int16_t a[2], b[2];
  while (g_pos < (double)(frames - 1)) {
    int i0 = (int)floor(g_pos);
    double t = g_pos - (double)i0;
    frame_at(in, i0, a);
    frame_at(in, i0 + 1, b);
    g_out[g_out_n * 2] = (int16_t)((double)a[0] + (double)(b[0] - a[0]) * t);
    g_out[g_out_n * 2 + 1] = (int16_t)((double)a[1] + (double)(b[1] - a[1]) * t);
    if (++g_out_n == FRAMES_PER_BUF) {
      pvz_video_mix(g_out, FRAMES_PER_BUF, (int)g_out_rate); /* the intro's sound, if it plays */
      submit(g_out); /* blocks while every buffer is queued: the pacing */
      g_out_n = 0;
    }
    g_pos += step;
  }
  g_pos -= (double)frames;
  frame_at(in, frames - 1, g_prev);
  if (g_writes % 2000 == 0) {
    double secs = (double)armTicksToNs(armGetSystemTick() - g_t_first) / 1e9;
    debugPrintf("[audio] %lu writes; pulled at %.0f Hz over %.0f s (setup %d Hz); %lu underruns, "
                "%lu failed submits (%lu dropped)\n",
                (unsigned long)g_writes, secs > 0 ? (double)g_frames_in / secs : 0.0, secs, g_rate,
                g_underruns, g_append_fails, g_dropped);
  }
  mutexUnlock(&g_lock);
}
