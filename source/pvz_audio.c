/* pvz_audio.c -- the game's AudioOutput, played through audout.
 *
 * libnative_code's Native::AudioOutput calls the Java AudioOutput:
 * setup(rate, channels, bits) once, then write(ByteBuffer, offset, length)
 * from the engine's sound thread (Audiere mixes, the AG audio layer pushes
 * ~8 KB blocks), and AudioTrack.write in the Java blocks until the device has
 * room -- which is what paces the mixer. This file does the same with audout:
 * the PCM is converted to the device's 48 kHz stereo s16 (linear resampling,
 * continuous across writes) and queued through the runtime's rt_audout.c in
 * three 1024-frame buffers; a write blocks while all three are queued (~64 ms
 * of sound ahead).
 *
 * The mod's alternative output, OpenSL ES, is not available here
 * (slCreateEngine fails), and nativeEnableOpenSL is never called. The start-up
 * self-test of the output is the runtime's (rt_audout_selftest). MIT.
 */
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <switch.h>

#include "pvz.h"
#include "rt_audout.h"
#include "util.h"

/* ------------------------------------------------------ AudioOutput */
static Mutex g_lock;
static int g_rate, g_ch, g_bits, g_setup;
static volatile uint32_t g_writes;
static volatile int g_paused, g_closing;
static int16_t g_out[RT_AUDOUT_FRAMES * 2];
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
  if (rt_audout_open() != 0)
    return 0;
  mutexLock(&g_lock);
  g_rate = rate, g_ch = channels, g_bits = bits;
  g_pos = 0.0;
  g_prev[0] = g_prev[1] = 0;
  g_out_n = 0;
  g_setup = 1;
  mutexUnlock(&g_lock);
  debugPrintf("[audio] AudioOutput.setup(%d Hz, %d ch, %d bit) -> audout %u Hz stereo\n", rate,
              channels, bits, rt_audout_rate());
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
  const double step = (double)g_rate / (double)rt_audout_rate();
  int16_t a[2], b[2];
  while (g_pos < (double)(frames - 1)) {
    int i0 = (int)floor(g_pos);
    double t = g_pos - (double)i0;
    frame_at(in, i0, a);
    frame_at(in, i0 + 1, b);
    g_out[g_out_n * 2] = (int16_t)((double)a[0] + (double)(b[0] - a[0]) * t);
    g_out[g_out_n * 2 + 1] = (int16_t)((double)a[1] + (double)(b[1] - a[1]) * t);
    if (++g_out_n == RT_AUDOUT_FRAMES) {
      pvz_video_mix(g_out, RT_AUDOUT_FRAMES, (int)rt_audout_rate()); /* the intro's sound, if it plays */
      rt_audout_submit(g_out); /* blocks while every buffer is queued: the pacing */
      g_out_n = 0;
    }
    g_pos += step;
  }
  g_pos -= (double)frames;
  frame_at(in, frames - 1, g_prev);
  if (g_writes % 2000 == 0) {
    double secs = (double)armTicksToNs(armGetSystemTick() - g_t_first) / 1e9;
    RtAudoutStats st;
    rt_audout_stats(&st);
    debugPrintf("[audio] %lu writes; pulled at %.0f Hz over %.0f s (setup %d Hz); %lu underruns, "
                "%lu failed submits (%lu dropped)\n",
                (unsigned long)g_writes, secs > 0 ? (double)g_frames_in / secs : 0.0, secs, g_rate,
                st.underruns, st.append_fails, st.dropped);
  }
  mutexUnlock(&g_lock);
}
