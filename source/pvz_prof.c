/* pvz_prof.c -- where the start-up's time goes, on every thread.
 *
 * Hardware run 4 (2026-09-24): the title screen came up, but the engine's
 * resource loading took 187 s ("Resource Loading Time: 187585") at a steady 20
 * frames a second, and the per-thread CPU of the long frames (dcr_boost.c) was
 * a few ms in frames of hundreds: the loading thread mostly WAITS. On what
 * cannot be read off the log, so from the first frame, for [debug]
 * profile_startup_seconds, a thread of its own pauses each game thread every
 * 10 ms (under b_pause_lock, one at a time, as the watchdog does), reads its
 * pc and the code addresses on the mapped top of its stack, and classifies it
 * by the syscall it sits in (none: running; SendSyncRequest: file/IPC; else
 * waiting). Unlike the Crossy Road port's frame profiler, waiting threads keep
 * their stacks: what a thread waits in is the question here. Every 10 s the
 * log gets, for each thread that did anything:
 *   its start routine (module+offset) and kernel CPU time,
 *   run/io/wait shares of the samples,
 *   self   the busiest code while running (64-byte buckets),
 *   wait   the code a waiting thread's stack goes through (who waits),
 *   incl   the call sites most often on its stack overall.
 * Offsets are named offline against the libraries' symbols. MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include "bionic_pthread.h"
#include "dcr_config.h"
#include "util.h"

const char *dcr_addr_name(uint32_t a, char *buf, size_t cap); /* exc_handler.c */
int dcr_is_code_addr(uint32_t a);                               /* exc_handler.c */
size_t dcr_readable(uint32_t p, size_t want);                   /* exc_handler.c */

#define PERIOD_NS 10000000ll
#define WINDOW_NS 10000000000ull
#define MAX_SAMPLES 20000
#define MAX_THREADS 48
#define DEPTH 20

typedef struct {
  uint64_t r[29];
  uint64_t fp, lr, sp, pc;
  uint32_t psr, _pad;
  uint8_t v[32][16];
  uint32_t fpcr, fpsr;
  uint64_t tpidr;
} KCtx;

static Result get_ctx(KCtx *ctx, Handle h) { /* svcGetThreadContext3 */
  register uint32_t r0 __asm__("r0") = (uint32_t)(uintptr_t)ctx;
  register uint32_t r1 __asm__("r1") = h;
  __asm__ volatile("svc 0x33" : "+r"(r0), "+r"(r1) : : "r2", "r3", "r12", "lr", "memory");
  return r0;
}

enum { ST_RUN, ST_IO, ST_WAIT };

typedef struct {
  uint32_t pc;
  uint8_t thread, state, n, _pad;
  uint32_t ret[DEPTH];
} Sample;

typedef struct {
  BThread *t;
  int tid, main;
  char name[16];
  uint32_t start;
  uint32_t n[3];
  u64 ticks0, ticks1; /* kernel CPU ticks at the window's start / at the report */
} Seen;

static Sample *g_s;
static volatile uint32_t g_n;
static Seen g_seen[MAX_THREADS];
static int g_nseen;
static Handle g_main_handle;
static Thread g_thread;
static u64 g_end_tick;

/* The syscall a paused thread sits in: a thread blocked in the kernel shows
 * its pc ON the svc; the instruction before is checked for one just back. */
static uint32_t svc_at(uint32_t a, int thumb) {
  if (!dcr_is_code_addr(a & ~1u))
    return ~0u;
  if (thumb) {
    uint16_t op = *(const volatile uint16_t *)(uintptr_t)(a & ~1u);
    return (op & 0xFF00) == 0xDF00 ? (uint32_t)(op & 0xFF) : ~0u;
  }
  if (a & 3)
    return ~0u;
  uint32_t op = *(const volatile uint32_t *)(uintptr_t)a;
  return (op & 0x0F000000) == 0x0F000000 ? (op & 0xFFFFFF) : ~0u;
}

static int state_of(const KCtx *c) {
  uint32_t pc = (uint32_t)c->pc;
  int thumb = (c->psr & 0x20) != 0;
  uint32_t svc = svc_at(pc, thumb);
  if (svc == ~0u)
    svc = svc_at(pc - (thumb ? 2 : 4), thumb);
  if (svc == ~0u)
    return ST_RUN;
  if (svc == 0x21 || svc == 0x22)
    return ST_IO;
  return ST_WAIT;
}

static u64 thread_ticks(Handle h) {
  u64 v = 0;
  if (R_FAILED(svcGetInfo(&v, InfoType_ThreadTickCount, h, TickCountInfo_Total)))
    svcGetInfo(&v, InfoType_ThreadTickCountDeprecated, h, TickCountInfo_Total);
  return v;
}

static int seen_index(BThread *t) {
  for (int i = 0; i < g_nseen; i++)
    if (g_seen[i].t == t && g_seen[i].tid == t->tid)
      return i;
  if (g_nseen == MAX_THREADS)
    return -1;
  Seen *e = &g_seen[g_nseen];
  memset(e, 0, sizeof *e);
  e->t = t;
  e->tid = t->tid;
  e->main = t->handle == g_main_handle;
  e->start = (uint32_t)(uintptr_t)t->start;
  memcpy(e->name, t->name, sizeof e->name);
  e->name[sizeof e->name - 1] = 0;
  e->ticks0 = thread_ticks(t->handle);
  return g_nseen++;
}

static void sample_one(BThread *t, void *arg) {
  (void)arg;
  if (t->handle == INVALID_HANDLE || t->finished || g_n >= MAX_SAMPLES)
    return;
  if (t->handle == threadGetCurHandle())
    return;
  int ti = seen_index(t);
  if (ti < 0)
    return;
  b_pause_lock();
  int paused = R_SUCCEEDED(svcSetThreadActivity(t->handle, ThreadActivity_Paused));
  KCtx ctx;
  if (paused && R_SUCCEEDED(get_ctx(&ctx, t->handle))) {
    int st = state_of(&ctx);
    g_seen[ti].n[st]++;
    Sample *s = &g_s[g_n];
    s->pc = (uint32_t)ctx.pc | ((ctx.psr & 0x20) ? 1u : 0u);
    s->thread = (uint8_t)ti;
    s->state = (uint8_t)st;
    s->n = 0;
    uint32_t sp = (uint32_t)ctx.r[13];
    uintptr_t lo = (uintptr_t)t->stack_base, hi = lo + t->stack_size;
    if ((uint32_t)ctx.r[14] && dcr_is_code_addr((uint32_t)ctx.r[14] & ~1u))
      s->ret[s->n++] = (uint32_t)ctx.r[14];
    uintptr_t end = (sp & ~3u) + dcr_readable(sp & ~3u, 0x4000);
    if (sp >= 0x1000)
      for (uintptr_t a = sp & ~3u; a + 4 <= end && (!lo || a + 4 <= hi) && s->n < DEPTH; a += 4) {
        uint32_t v = *(const volatile uint32_t *)a;
        if (dcr_is_code_addr(v & ~1u))
          s->ret[s->n++] = v;
      }
    g_n++;
  }
  if (paused)
    svcSetThreadActivity(t->handle, ThreadActivity_Runnable);
  b_pause_unlock();
}

/* ---- aggregation ---- */
#define TAB 2048
typedef struct {
  uint32_t key, count;
} Ent;

static void add(Ent *t, uint32_t key) {
  for (uint32_t h = (key * 2654435761u) % TAB, k = 0; k < TAB; k++, h = (h + 1) % TAB) {
    if (t[h].count && t[h].key != key)
      continue;
    t[h].key = key;
    t[h].count++;
    return;
  }
}

static void top_line(const char *label, Ent *t, uint32_t total, int want) {
  char line[1000];
  int n = snprintf(line, sizeof line, "[prof]     %s:", label);
  int any = 0;
  for (int k = 0; k < want; k++) {
    int best = -1;
    for (int i = 0; i < TAB; i++)
      if (t[i].count && (best < 0 || t[i].count > t[best].count))
        best = i;
    if (best < 0 || n > (int)sizeof line - 80 || t[best].count * 100 < total * 2)
      break;
    char a[64];
    n += snprintf(line + n, sizeof line - n, " %s %u%%", dcr_addr_name(t[best].key, a, sizeof a),
                  t[best].count * 100 / total);
    t[best].count = 0;
    any = 1;
  }
  if (any)
    debugPrintf("%s\n", line);
}

/* CPU at the report, read from live threads only (an exited thread's BThread
 * may be gone): matched by pointer and tid, as seen_index does. */
static void refresh_cpu(BThread *t, void *arg) {
  (void)arg;
  if (t->handle == INVALID_HANDLE || t->finished)
    return;
  for (int i = 0; i < g_nseen; i++)
    if (g_seen[i].t == t && g_seen[i].tid == t->tid)
      g_seen[i].ticks1 = thread_ticks(t->handle);
}

static u64 cpu_of(const Seen *e) { return e->ticks1 > e->ticks0 ? e->ticks1 - e->ticks0 : 0; }

static void report_thread(int ti, uint32_t ticks, u64 window_ns) {
  static Ent self[TAB], wait[TAB], incl[TAB];
  memset(self, 0, sizeof self);
  memset(wait, 0, sizeof wait);
  memset(incl, 0, sizeof incl);
  Seen *e = &g_seen[ti];
  uint32_t n = g_n;
  for (uint32_t i = 0; i < n; i++) {
    const Sample *s = &g_s[i];
    if (s->thread != ti)
      continue;
    if (s->state == ST_RUN)
      add(self, (s->pc & ~1u) & ~63u);
    for (int k = 0; k < s->n; k++) {
      uint32_t a = s->ret[k] & ~1u;
      int dup = 0;
      for (int j = 0; j < k && !dup; j++)
        dup = (s->ret[j] & ~1u) == a;
      if (dup)
        continue;
      add(incl, a);
      if (s->state != ST_RUN)
        add(wait, a);
    }
  }
  char st[64];
  u64 cpu = cpu_of(e);
  debugPrintf("[prof]   %s \"%s\" (tid %d, start %s): CPU %llu ms of %llu; samples: running %u%%, "
              "file/IPC %u%%, waiting %u%%\n",
              e->main ? "main thread" : "thread", e->name, e->tid,
              e->start ? dcr_addr_name(e->start, st, sizeof st) : "-",
              (unsigned long long)(armTicksToNs(cpu) / 1000000ull),
              (unsigned long long)(window_ns / 1000000ull), e->n[ST_RUN] * 100 / ticks,
              e->n[ST_IO] * 100 / ticks, e->n[ST_WAIT] * 100 / ticks);
  top_line("self", self, ticks, 10);
  top_line("wait", wait, ticks, 14);
  top_line("incl", incl, ticks, 14);
}

static void report_window(u64 window_ns) {
  uint32_t ticks = 0;
  for (int i = 0; i < g_nseen; i++) {
    uint32_t t = g_seen[i].n[0] + g_seen[i].n[1] + g_seen[i].n[2];
    if (t > ticks)
      ticks = t;
  }
  if (ticks < 10)
    return;
  b_thread_foreach(refresh_cpu, NULL);
  debugPrintf("[prof] --- %llu ms, %u samples of %d threads ---\n",
              (unsigned long long)(window_ns / 1000000ull), (unsigned)g_n, g_nseen);
  for (int i = 0; i < g_nseen; i++) {
    u64 cpu = cpu_of(&g_seen[i]);
    uint32_t busy = g_seen[i].n[ST_RUN] + g_seen[i].n[ST_IO];
    /* every thread that used 2% of a core or was seen busy in 2% of samples */
    if (armTicksToNs(cpu) * 50 >= window_ns || busy * 50 >= ticks)
      report_thread(i, ticks, window_ns);
  }
}

static void sampler(void *arg) {
  (void)arg;
  u64 window_start = armGetSystemTick();
  for (;;) {
    b_thread_foreach(sample_one, NULL);
    u64 now = armGetSystemTick();
    u64 ns = armTicksToNs(now - window_start);
    int last = now >= g_end_tick;
    if (ns >= WINDOW_NS || last || g_n >= MAX_SAMPLES) {
      report_window(ns);
      g_n = 0;
      g_nseen = 0;
      window_start = now;
    }
    if (last)
      break;
    svcSleepThread(PERIOD_NS);
  }
  debugPrintf("[prof] done\n");
  free(g_s);
  g_s = NULL;
}

/* From the first frame, for [debug] profile_startup_seconds (0: off). */
void pvz_prof_start(void) {
  int secs = dcr_config()->profile_secs;
  if (secs <= 0 || g_s)
    return;
  g_main_handle = envGetMainThreadHandle();
  g_s = malloc(sizeof(Sample) * MAX_SAMPLES);
  if (!g_s)
    return;
  g_end_tick = armGetSystemTick() + armNsToTicks((u64)secs * 1000000000ull);
  if (R_FAILED(threadCreate(&g_thread, sampler, NULL, NULL, 0x8000, 0x2C, -2)) ||
      R_FAILED(threadStart(&g_thread))) {
    free(g_s);
    g_s = NULL;
    return;
  }
  debugPrintf("[prof] sampling every thread every %lld ms for %d s; a report every %llu s\n",
              PERIOD_NS / 1000000, secs, WINDOW_NS / 1000000000ull);
}
