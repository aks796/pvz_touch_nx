/* pvz_loader.c -- loading PvZ TV Touch's modules, and the Homura mod's hooks.
 *
 * Modules (lib/armeabi-v7a of the user's APK), in Android's load order:
 *   libnative_code.so  Transmension MobileSDK (NativeApp, BridgeApp, AG* API)
 *   libGameMain.so     the engine (PopCap Sexy + Lawn, PakLib, Audiere, GLES1)
 *   libHomura.so       the "Touch" mod, loaded by EnhanceActivity after the engine
 * Not loaded: libfmodex.so (a DT_NEEDED of the engine, which imports nothing
 * from it -- its sound is Audiere) and libGameRegister.so (Transmension's
 * licence module, dlopen()ed only by CAuthenticationManager::registerNewUser).
 *
 * HOMURA'S HOOKS
 * libHomura's constructor (LibMain) hooks the engine at run time, the Android
 * way: Cydia Substrate inline hooks (589; mprotect(text, RWX), store a branch,
 * and trampolines built in mmap(RW) pages then mprotect()ed RX at the SAME
 * address), vtable and GOT slots (mprotect RW), and byte patches (Patcher:
 * mprotect RWX + memcpy + mprotect RX). Horizon has no RWX and never lets a
 * written page execute again, so none of that can happen in place on live code.
 * It does not have to: nothing in the engine runs until the hooks are in. So
 * the engine and a trampoline pool stay writable (not executable) at their
 * final addresses while LibMain runs (so_map_writable), and are sealed as code
 * afterwards (so_finalize / pool_seal) -- the same pages, the same addresses.
 * Checked on this build: none of the engine's 70 constructors reaches a hooked
 * function (depth 4), so running them after the seal is safe; LibMain itself
 * only dlsym()s, reads and writes the engine.
 *
 * After the seal the only code writes left are Homura's Patcher toggles
 * (settings changed in game). mprotect(W) on sealed text arms a window;
 * memcpy/memmove/memset into it go through so_patch_code (a temporary RW alias
 * and a cache flush); mprotect without W disarms it.
 *
 * Homura's __clear_cache is compiler-rt's inline Linux cacheflush syscall
 * (`movw r7,#2; mov r2,#0; movt r7,#0xf; svc #0`), an invalid SVC on Horizon.
 * It is rewritten to `mov r0,#0` before the module is sealed: every flush it
 * would have done is done by the seal or by so_patch_code. MIT.
 */
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "bionic.h"
#include "code_flush.h"
#include "codespace.h"
#include "config.h"
#include "dcr_config.h"
#include "error.h"
#include "imports.h"
#include "pvz_net.h"
#include "selfproc.h"
#include "so_util.h"
#include "util.h"

const char *dcr_game_root(void); /* main.c */

so_module g_mod_native, g_mod_game, g_mod_homura;

/* ================================================================ pool */
#define POOL_BYTES (512u * 1024)

static uint8_t *g_pool_stage; /* heap pages behind the pool */
static uint8_t *g_pool;       /* the final (and, in the hook phase, writable) address */
static void *g_pool_tmp;
static VirtmemReservation *g_pool_rv, *g_pool_tmp_rv;
static size_t g_pool_used;
static int g_hook_phase, g_pool_sealed;
static unsigned g_pool_allocs;
static Mutex g_cs_lock;

static int pool_init(void) {
  g_pool_stage = memalign(0x1000, POOL_BYTES);
  if (!g_pool_stage)
    return -1;
  memset(g_pool_stage, 0, POOL_BYTES);
  if (dcr_is_emulator()) {
    g_pool = g_pool_stage; /* heap pages execute under the emulator */
    return 0;
  }
  virtmemLock();
  g_pool = virtmemFindCodeMemory(POOL_BYTES, 0x1000);
  g_pool_rv = g_pool ? virtmemAddReservation(g_pool, POOL_BYTES) : NULL;
  g_pool_tmp = virtmemFindCodeMemory(POOL_BYTES, 0x1000);
  g_pool_tmp_rv = g_pool_tmp ? virtmemAddReservation(g_pool_tmp, POOL_BYTES) : NULL;
  virtmemUnlock();
  if (!g_pool_rv || !g_pool_tmp_rv)
    return -2;
  Result rc = svcMapProcessCodeMemory(dcr_self_process(), (u64)(uintptr_t)g_pool_tmp,
                                      (u64)(uintptr_t)g_pool_stage, POOL_BYTES);
  if (R_FAILED(rc)) {
    debugPrintf("[hook] pool: svcMapProcessCodeMemory 0x%x\n", rc);
    return -3;
  }
  rc = so_alias_map(g_pool, (uintptr_t)g_pool_tmp, POOL_BYTES);
  if (R_FAILED(rc)) {
    debugPrintf("[hook] pool: svcMapProcessMemory 0x%x\n", rc);
    return -4;
  }
  return 0;
}

static void pool_seal(void) {
  if (g_pool_sealed)
    return;
  g_pool_sealed = 1;
  if (dcr_is_emulator()) {
    dcr_code_flush(g_pool, POOL_BYTES);
    return;
  }
  Result rc = so_alias_unmap(g_pool, (uintptr_t)g_pool_tmp, POOL_BYTES);
  if (R_SUCCEEDED(rc))
    rc = svcUnmapProcessCodeMemory(dcr_self_process(), (u64)(uintptr_t)g_pool_tmp,
                                   (u64)(uintptr_t)g_pool_stage, POOL_BYTES);
  if (R_SUCCEEDED(rc))
    rc = svcMapProcessCodeMemory(dcr_self_process(), (u64)(uintptr_t)g_pool,
                                 (u64)(uintptr_t)g_pool_stage, POOL_BYTES);
  if (R_SUCCEEDED(rc))
    rc = svcSetProcessMemoryPermission(dcr_self_process(), (u64)(uintptr_t)g_pool, POOL_BYTES,
                                       Perm_Rx);
  if (R_FAILED(rc)) {
    log_flush_ring();
    fatal_error("Sealing the hook trampolines failed: 0x%x.", rc);
  }
  virtmemLock();
  virtmemRemoveReservation(g_pool_tmp_rv);
  virtmemUnlock();
  dcr_code_flush(g_pool, POOL_BYTES);
}

static int in_pool(const void *p) {
  return g_pool && (const uint8_t *)p >= g_pool && (const uint8_t *)p < g_pool + POOL_BYTES;
}
int pvz_in_pool(const void *p) { return in_pool(p); } /* crash reports */

/* ============================================================ mm policy */
void *cs_mmap(size_t len, int prot, const void *caller) {
  if (!g_hook_phase || !len || len > 64 * 1024)
    return NULL;
  mutexLock(&g_cs_lock);
  size_t need = (len + 15) & ~(size_t)15;
  void *p = NULL;
  if (g_pool_used + need <= POOL_BYTES) {
    p = g_pool + g_pool_used;
    g_pool_used += need;
    g_pool_allocs++;
  }
  mutexUnlock(&g_cs_lock);
  if (!p)
    debugPrintf("[hook] trampoline pool full (%u bytes asked)\n", (unsigned)len);
  return p;
}

int cs_munmap(void *addr, size_t len) {
  return in_pool(addr); /* trampolines are never reused: nothing to free */
}

/* Sealed text armed for writing by its owner's mprotect(W). */
typedef struct {
  uintptr_t start, end;
} Window;
#define MAX_WINDOWS 16
static Window g_win[MAX_WINDOWS];
volatile int g_cs_armed;

static int is_text(so_module *m, uintptr_t a) {
  const Elf32_Phdr *p = so_segment_of(m, a);
  return p && (p->p_flags & PF_X);
}

int cs_mprotect(void *addr, size_t len, int prot, const void *caller) {
  const uintptr_t a = (uintptr_t)addr;
  if (in_pool(addr))
    return 1; /* trampolines: writable until sealed, then RX; nothing to do */
  so_module *m = so_find_module_by_addr(addr);
  if (!m)
    return 0;
  /* Inside a module: never change real permissions (data stays RW, as the
   * game expects of RELRO it has just made writable; text stays RX). */
  if (m->state != SO_SEALED || !is_text(m, a) || dcr_is_emulator())
    return 1;
  const uintptr_t s = a & ~0xFFFu, e = (a + len + 0xFFF) & ~0xFFFu;
  mutexLock(&g_cs_lock);
  if (prot & L_PROT_WRITE) {
    int slot = -1;
    for (int i = 0; i < MAX_WINDOWS; i++)
      if (g_win[i].start == s && g_win[i].end == e)
        slot = i;
    for (int i = 0; slot < 0 && i < MAX_WINDOWS; i++)
      if (!g_win[i].end)
        slot = i;
    if (slot >= 0) {
      g_win[slot] = (Window){s, e};
      g_cs_armed = 1;
    }
  } else {
    int any = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
      if (g_win[i].end && g_win[i].start < e && s < g_win[i].end)
        g_win[i] = (Window){0, 0};
      any |= g_win[i].end != 0;
    }
    g_cs_armed = any;
  }
  mutexUnlock(&g_cs_lock);
  return 1;
}

int cs_write(void *dst, const void *src, size_t n, int c, int kind) {
  const uintptr_t d = (uintptr_t)dst;
  int hit = 0;
  mutexLock(&g_cs_lock);
  for (int i = 0; i < MAX_WINDOWS && !hit; i++)
    hit = g_win[i].end && d < g_win[i].end && d + n > g_win[i].start; /* overlap: the
                                                     * mod's mprotect length counts from the page */
  mutexUnlock(&g_cs_lock);
  if (!hit || !n)
    return 0;
  uint8_t tmp[256];
  uint8_t *buf = n <= sizeof tmp ? tmp : malloc(n);
  if (!buf)
    return 0;
  if (kind == 2)
    memset(buf, c, n);
  else
    memcpy(buf, src, n);
  int rc = so_patch_code(dst, buf, n);
  if (buf != tmp)
    free(buf);
  static unsigned logged;
  if (logged++ < 32)
    debugPrintf("[hook] code write %p+%u (%s)\n", dst, (unsigned)n, rc ? "FAILED" : "patched");
  return 1;
}

/* ========================================================= fix-ups */
/* compiler-rt __clear_cache on ARM Linux: an inline cacheflush syscall. */
static int fix_linux_cacheflush(so_module *m) {
  int n = 0;
  for (int i = 0; i < m->phnum; i++) {
    const Elf32_Phdr *ph = &m->phdr[i];
    if (ph->p_type != PT_LOAD || !(ph->p_flags & PF_X))
      continue;
    uint8_t *seg = (uint8_t *)m->load_base + ph->p_vaddr;
    /* ARM: svc #0 with movw r7,#2 and movt r7,#0xf among the 4 before it */
    uint32_t *w = (uint32_t *)((uintptr_t)(seg + 3) & ~3u);
    size_t nw = ph->p_filesz / 4;
    for (size_t k = 4; k < nw; k++) {
      if (w[k] != 0xEF000000u)
        continue;
      int lo = 0, hi = 0;
      for (size_t j = k - 4; j < k; j++) {
        lo |= w[j] == 0xE3007002u; /* movw r7, #2   */
        hi |= w[j] == 0xE340700Fu; /* movt r7, #0xf */
      }
      if (lo && hi) {
        w[k] = 0xE3A00000u; /* mov r0, #0 */
        debugPrintf("[hook] %s+0x%x: Linux cacheflush syscall -> mov r0, #0\n", m->base_name,
                    (unsigned)((uintptr_t)&w[k] - (uintptr_t)m->load_base));
        n++;
      }
    }
    /* Thumb: DF00 after movw r7,#2 (F240 0702) and movt r7,#0xf (F2C0 070F) */
    uint16_t *h = (uint16_t *)((uintptr_t)(seg + 1) & ~1u);
    size_t nh = ph->p_filesz / 2;
    for (size_t k = 8; k < nh; k++) {
      if (h[k] != 0xDF00)
        continue;
      int lo = 0, hi = 0;
      for (size_t j = k - 8; j + 1 < k; j++) {
        lo |= h[j] == 0xF240 && h[j + 1] == 0x0702;
        hi |= h[j] == 0xF2C0 && h[j + 1] == 0x070F;
      }
      if (lo && hi) {
        h[k] = 0xBF00; /* nop: r0 keeps the start address, which is ignored */
        debugPrintf("[hook] %s+0x%x: Linux cacheflush syscall (Thumb) -> nop\n", m->base_name,
                    (unsigned)((uintptr_t)&h[k] - (uintptr_t)m->load_base));
        n++;
      }
    }
  }
  return n;
}

/* libgcc's __sync_* on ARM Linux call the kernel's user helpers through
 * literal pools (kuser.S). Point every such literal in the module's code at
 * ours; any other helper address is reported, not guessed at. */
void dcr_kuser_cmpxchg(void);
void dcr_kuser_memory_barrier(void);

static void fix_kuser_helpers(so_module *m) {
  int cmpxchg = 0, barrier = 0, other = 0;
  for (int i = 0; i < m->phnum; i++) {
    const Elf32_Phdr *ph = &m->phdr[i];
    if (ph->p_type != PT_LOAD || !(ph->p_flags & PF_X))
      continue;
    uint32_t *w = (uint32_t *)((uintptr_t)((uint8_t *)m->load_base + ph->p_vaddr + 3) & ~3u);
    size_t nw = ph->p_filesz / 4;
    for (size_t k = 0; k < nw; k++) {
      if ((w[k] & 0xfffff000u) != 0xffff0000u || (w[k] & 0xfff) < 0xf60)
        continue;
      if (w[k] == 0xffff0fc0u) {
        w[k] = (uint32_t)(uintptr_t)dcr_kuser_cmpxchg;
        cmpxchg++;
      } else if (w[k] == 0xffff0fa0u) {
        w[k] = (uint32_t)(uintptr_t)dcr_kuser_memory_barrier;
        barrier++;
      } else if (w[k] == 0xffff0f60u || w[k] == 0xffff0fe0u || w[k] == 0xffff0ffcu) {
        if (other++ < 4)
          debugPrintf("[boot] %s+0x%x: kernel helper 0x%08x not provided\n", m->base_name,
                      (unsigned)((uintptr_t)&w[k] - (uintptr_t)m->load_base), (unsigned)w[k]);
      }
    }
  }
  if (cmpxchg || barrier || other)
    debugPrintf("[boot] %s: libgcc atomics -> kuser.S (%d cmpxchg, %d barrier%s)\n",
                m->base_name, cmpxchg, barrier, other ? ", others NOT handled" : "");
}

/* ============================================================= loading */
static int load_one(so_module *mod, const char *name) {
  char path[512];
  snprintf(path, sizeof path, "%s/%s", dcr_game_root(), name);
  int rc = so_load(mod, path, NULL, SO_REGION_BYTES);
  if (rc < 0) {
    const char *why = rc == -1 ? "cannot open it, or it is not a 32-bit ARM ELF"
                    : rc == -2 ? "out of memory"
                    : rc == -3 ? "larger than SO_REGION_BYTES"
                    : rc == -4 ? "too many program headers" : "?";
    debugPrintf("[boot] so_load(%s) failed rc=%d: %s\n", path, rc, why);
    return -1;
  }
  so_relocate(mod);
  return 0;
}

/* [debug] load_touch_mod = false: the plain TV edition, without libHomura,
 * to tell a problem in the mod from one in the engine. (Not an emulator
 * path: Ryujinx 1.1.1098 lacks Thumb LDREX, and the engine's inline atomics
 * use it too.) */
static int with_mod(void) { return dcr_config()->load_mod; }

/* pvz_net_imports, then the generic table (so_resolve takes the first match). */
static DynLibFunction *with_net_imports(int *count) {
  static DynLibFunction *t;
  if (!t) {
    t = malloc(sizeof *t * (size_t)(pvz_net_imports_count + dcr_imports_count));
    if (!t) {
      *count = dcr_imports_count;
      return dcr_imports;
    }
    memcpy(t, pvz_net_imports, sizeof *t * (size_t)pvz_net_imports_count);
    memcpy(t + pvz_net_imports_count, dcr_imports, sizeof *t * (size_t)dcr_imports_count);
  }
  *count = pvz_net_imports_count + dcr_imports_count;
  return t;
}

int pvz_load_modules(void) {
  if (load_one(&g_mod_native, PVZ_LIB_NATIVE) < 0 || load_one(&g_mod_game, PVZ_LIB_GAME) < 0 ||
      (with_mod() && load_one(&g_mod_homura, PVZ_LIB_HOMURA) < 0))
    return -1;
  /* Imports are resolved once every module is in the list, so the engine's
   * AG* imports find libnative_code's exports. */
  so_module *mods[] = {&g_mod_native, &g_mod_game, &g_mod_homura};
  int nmods = with_mod() ? 3 : 2;
  for (int i = 0; i < nmods; i++) {
    DynLibFunction *table = dcr_imports;
    int count = dcr_imports_count;
    if (mods[i] == &g_mod_homura) /* the mod's multiplayer: real sockets (pvz_net.c) */
      table = with_net_imports(&count);
    int missing = so_resolve(mods[i], table, count, 1);
    debugPrintf("[boot] %-18s %6u KB  staged %p -> %p  (%d unresolved imports)\n",
                mods[i]->base_name, (unsigned)(mods[i]->load_size >> 10), mods[i]->load_base,
                mods[i]->load_virtbase, missing);
  }
  fix_kuser_helpers(&g_mod_native);
  fix_kuser_helpers(&g_mod_game);
  if (!with_mod()) {
    debugPrintf("[boot] config.ini [debug] load_touch_mod = false: the TV edition without the mod\n");
    so_finalize(&g_mod_native);
    so_finalize(&g_mod_game);
    so_flush_caches(&g_mod_native);
    so_flush_caches(&g_mod_game);
    return 0;
  }
  if (!fix_linux_cacheflush(&g_mod_homura))
    debugPrintf("[hook] note: no inline cacheflush syscall found in %s\n", PVZ_LIB_HOMURA);

  so_finalize(&g_mod_native);
  so_finalize(&g_mod_homura);
  so_flush_caches(&g_mod_native);
  so_flush_caches(&g_mod_homura);
  so_map_writable(&g_mod_game);
  int prc = pool_init();
  if (prc)
    fatal_error("Could not set up memory for the mod's hooks (%d).", prc);
  debugPrintf("[boot] modules mapped: engine writable for the hook phase, trampolines at %p\n",
              g_pool);
  return 0;
}

/* libGameMain's constructors run where Android runs them: inside the dlopen
 * of it in NativeApp::load (libnative_code+0x196bc), after loadNativeApp has
 * put ANDROID_SOURCE_DIR and the other paths in the environment. One of them
 * (p_addResource -> PakLib::FileSystemManager::addDefaultLocations, which
 * runs once) registers the APK as the game's data from that variable: run at
 * start-up instead, it found no APK and the game would have had no data
 * (hardware run 2, 2026-09-24). Only armed after the seal: the mod's
 * constructor dlopens libGameMain itself (RTLD_NOLOAD) while its code is
 * still a writable alias. */
static void defer_game_constructors(void) {
  g_mod_game.init_on_dlopen = 1;
  debugPrintf("[boot] %s constructors: at its dlopen in NativeApp::load, as on Android\n",
              PVZ_LIB_GAME);
}

/* Android runs a library's constructors when it is loaded. The order here
 * differs in one place, on purpose: Homura's run BEFORE the engine's (see the
 * top of this file). */
void pvz_run_constructors(void) {
  so_execute_init_array(&g_mod_native);
  debugPrintf("[boot] %s constructors done\n", PVZ_LIB_NATIVE);

  if (!with_mod()) {
    defer_game_constructors();
    return;
  }

  extern int g_so_trace_ctors;
  g_so_trace_ctors = dcr_is_emulator();
  u64 t0 = armGetSystemTick();
  g_hook_phase = 1;
  so_execute_init_array(&g_mod_homura);
  g_hook_phase = 0;
  u64 t1 = armGetSystemTick();
  dcr_stdio_flush_repeats(); /* "SubstrateHookFunctionThumb" x hundreds */
  debugPrintf("[hook] %s constructors done in %llu ms: %u trampolines, %u bytes of pool\n",
              PVZ_LIB_HOMURA, (unsigned long long)(armTicksToNs(t1 - t0) / 1000000ull),
              g_pool_allocs, (unsigned)g_pool_used);
  log_flush_ring();

  so_finalize(&g_mod_game); /* the seal: same pages, same address, now code */
  pool_seal();
  so_flush_caches(&g_mod_game);
  debugPrintf("[hook] sealed %s and the trampolines as code\n", PVZ_LIB_GAME);
  log_flush_ring();
  defer_game_constructors();
}
