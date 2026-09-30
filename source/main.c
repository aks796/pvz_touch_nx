/* main.c -- boot sequence for the PvZ TV Touch wrapper (32-bit).
 *
 * The order here matters; each step says why it is where it is. MIT.
 */
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "dcr_config.h"
#include "dcr_manifest.h"
#include "dcr_path.h"
#include "dcr_sched.h"
#include "dcr_time.h"
#include "error.h"
#include "nx_init.h"
#include "pvz.h"
#include "pvz_apks.h"
#include "selfproc.h"
#include "so_util.h"
#include "util.h"

int pvz_boot_run(void);
void dcr_setup_update_from_nro(void); /* dcr_setup.c */
void dcr_setup_from_apk(const char *apk);

static char g_root[256] = "sdmc:" PVZ_ROOT_PATH;
const char *dcr_game_root(void) { return g_root; }

extern volatile uint32_t __dcr_reloc_path __attribute__((visibility("hidden")));

static void report_boot(void) {
  const u64 MB = 1024 * 1024;
  debugPrintf("[boot] === pvz_nx: Plants vs. Zombies Touch (PvZ TV Touch: Transmension engine + Homura mod, "
              "armeabi-v7a) ===\n");
  static const char *const paths[] = {"none needed", "patched through a writable alias (hardware)",
                                      "direct writes (emulator: pseudo-handle refused)"};
  debugPrintf("[boot] text relocations: %s\n",
              __dcr_reloc_path < 3 ? paths[__dcr_reloc_path] : "?");
  debugPrintf("[heap] total %u MB, used %u MB at start, heap region %u MB, heap %u MB @ %p\n",
              (unsigned)(g_nxinit.total / MB), (unsigned)(g_nxinit.used / MB),
              (unsigned)(g_nxinit.heap_region / MB), (unsigned)(g_nxinit.heap / MB),
              (void *)g_nxinit.heap_base);
  debugPrintf("[svc] sm=%x applet=%x hid=%x time=%x fs=%x sdmc=%x\n", g_nxinit.rc_sm,
              g_nxinit.rc_applet, g_nxinit.rc_hid, g_nxinit.rc_time, g_nxinit.rc_fs,
              g_nxinit.rc_sdmc);
  if (R_FAILED(g_nxinit.rc_time))
    debugPrintf("[svc] time service unavailable: clocks fall back to the system tick\n");
}

int main(int argc, char *argv[]) {
  pvz_old_folder_move(g_root); /* /switch/pvztouch of older releases (pvz_apks.c) */
  mkdir(g_root, 0777);
  log_init(g_root);
  log_console_open(); /* blank: text only when asked or for setup work */
  report_boot();
  pvz_old_folder_report();

  if (chdir(g_root) != 0)
    debugPrintf("[boot] WARNING: chdir(%s) failed\n", g_root);
  dcr_config_load(); /* config.ini: the mod's settings, cheats, resolution, boost */
  void dcr_boost_launch_begin(void);
  dcr_boost_launch_begin(); /* CPU at 1785 MHz until the first picture (dcr_boost.c) */
  if (dcr_config()->boot_log)
    log_console_show_text();
  dcr_time_init();
  dcr_path_prepare_dirs();

  /* A newer build of this program in the launcher NRO: install it and
   * restart into it before anything else happens (dcr_setup.c). */
  dcr_setup_update_from_nro();

  /* the APKs in the folder, whatever their names: by their contents */
  if (pvz_apks_find(g_root) != 0)
    fatal_error("No Plants vs. Zombies TV Touch APK in %s\n(found: %s).\n\n"
                "Copy the APK of your own Plants vs. Zombies TV Touch 1.1.5\n"
                "(com.trans.pvztv, armeabi-v7a) into that folder. Any file name works:\n"
                "the game reads its data from it, and its libraries are unpacked\n"
                "from it on the first launch.",
                g_root, pvz_apks_summary());
  const char *apk = pvz_game_apk();
  void dcr_apkcache_set_path(const char *real);
  dcr_apkcache_set_path(apk); /* the engine's zip reads of it are cached (dcr_apkcache.c) */
  if (dcr_manifest_load(apk) != 0)
    fatal_error("%s could not be read.\n\n"
                "Copy the APK of your own Plants vs. Zombies TV Touch (com.trans.pvztv,\n"
                "1.1.5, armeabi-v7a) into %s again: the game reads its data from it,\n"
                "and its libraries are unpacked from it on the first launch.",
                apk, g_root);
  if (strcmp(dcr_manifest_package(), PVZ_PACKAGE))
    debugPrintf("[boot] WARNING: the game APK is %s, not %s\n", dcr_manifest_package(), PVZ_PACKAGE);

  if (dcr_self_process() == INVALID_HANDLE)
    fatal_error("Could not obtain a handle to this process.\n"
                "The loader needs it to map the game's code.");

  /* libnative_code / libGameMain / libHomura, classes.txt and the English
   * text, from game.apk when they are missing or it has changed */
  dcr_setup_from_apk(apk);
  if (pvz_load_modules() != 0)
    fatal_error("Could not load the game libraries from %s.\n\n"
                "They are unpacked from the game APK (lib/armeabi-v7a/) on launch: delete\n"
                "libnative_code.so, libGameMain.so, libHomura.so and .setup there to\n"
                "unpack them again.",
                g_root);

  /* The main thread becomes a guest thread like the game's own: priority
   * 59 on cores 0-2, where the kernel time-slices (dcr_sched.c). */
  dcr_sched_init();
  {
    void dcr_audio_selftest(void);
    dcr_audio_selftest();
    void dcr_pthread_selftest(void);
    dcr_pthread_selftest();
    void dcr_io_selftest(void);
    dcr_io_selftest();
  }
#if DCR_GL_MESA
  if (dcr_is_emulator() || dcr_config()->gl_selftest) {
    int dcr_gl_selftest(void);
    dcr_gl_selftest();
  }
#endif

  /* Constructors: libnative_code, then the mod's (which hooks the engine
   * while it is still writable), the seal, then the engine's (pvz_loader.c). */
  pvz_run_constructors();

  pvz_boot_run();
  debugPrintf("[boot] exiting\n");
  log_flush_ring();
  return 0;
}
