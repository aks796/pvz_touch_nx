/* pvz_main.c -- Plants vs. Zombies Touch's part of the boot (the runtime's
 * main.c runs the rest: runtime/source/main.c).
 *
 * port_load() goes from the APK to the game's first code: setup (the
 * libraries, classes.txt, the English layer and the port's other files:
 * pvz_setup_plan.c), then the three modules loaded and bound (pvz_loader.c).
 * port_run() runs the constructors -- libnative_code, the mod's (which hooks
 * the engine while it is still writable), the seal, the engine's at its
 * dlopen -- and then the game (pvz_boot.c). MIT.
 */
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "dcr_config.h"
#include "dcr_path.h"
#include "dcr_setup.h"
#include "error.h"
#include "pvz.h"
#include "pvz_apks.h"
#include "rt_boot.h"
#include "util.h"

int pvz_boot_run(void);

/* ------------------------------------------------------------- the APKs */
/* The runtime finds them by what is in them (PORT_APK_ROLES): role 0 the
 * game, role 1 the English source. */
const char *pvz_game_apk(void) { return dcr_apk_path(); }

const char *pvz_english_apk(void) {
  static char none[300];
  const char *p = dcr_apk_role_path(1);
  if (p && *p)
    return p;
  snprintf(none, sizeof none, "%s/%s", dcr_game_root(), PVZ_ENGLISH_APK);
  return none;
}

int pvz_apks_have_english(void) {
  const char *p = dcr_apk_role_path(1);
  return p && *p;
}

const char *port_apk_help(void) {
  return "Copy the APK of your own Plants vs. Zombies TV Touch 1.1.5\n"
         "(com.trans.pvztv, armeabi-v7a) into that folder. Any file name works:\n"
         "the game reads its data from it, and its libraries are unpacked\n"
         "from it on the first launch.";
}

/* No English APK in the folder: the English files the launcher NRO carries
 * (romfs:/english.apk, the English APK cut down to what pvz_english.c reads:
 * tools/make_english_pack.py), copied in as "PvZ Touch English.apk" -- as if
 * the player had put an English APK there; from then on it is one, found as
 * role 1 by the search this asks for again. */
int port_after_apk_find(void) {
  if (!dcr_config()->english || pvz_apks_have_english())
    return 0;
  const int r = rt_setup_copy_from_nro(PVZ_NRO_ENGLISH_ROMFS, PVZ_NRO_ENGLISH_NAME, "",
                                       "Unpacking the English files", 0, 150);
  if (r == 0)
    debugPrintf("[setup] the English files: no launcher NRO in the game folder carries them\n");
  return r == 2;
}

/* ------------------------------------------------------------ the boot */
int port_load(const char *apk) {
  /* libnative_code / libGameMain / libHomura, classes.txt, the English layer
   * and the rest, from the game APK when they are missing or it has changed
   * (pvz_setup_plan.c) */
  dcr_setup_from_apk(apk);
  if (pvz_load_modules() != 0)
    fatal_error("Could not load the game libraries from %s.\n\n"
                "They are unpacked from the game APK (lib/armeabi-v7a/) on launch: delete\n"
                "libnative_code.so, libGameMain.so, libHomura.so and .setup there to\n"
                "unpack them again.",
                dcr_game_root());
  return 0;
}

/* Constructors: libnative_code, then the mod's (which hooks the engine while
 * it is still writable), the seal, then the engine's (pvz_loader.c); then the
 * activity and the frame loop (pvz_boot.c). */
void port_run(void) {
  pvz_run_constructors();
  pvz_boot_run();
}
