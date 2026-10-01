/* pvz_apks.h -- the game's APK and the English source, as the runtime found
 * them (PORT_APK_ROLES in port_config.h: role 0 the game, role 1 the English
 * source; runtime/source/rt_boot.c). pvz_main.c. MIT.
 */
#ifndef PVZ_APKS_H
#define PVZ_APKS_H

/* The game APK's path (dcr_apk_path()). */
const char *pvz_game_apk(void);
/* The English APK's path; "<root>/english.apk" (absent) when none was found. */
const char *pvz_english_apk(void);
/* An English source was found (pvz_english_apk() names it). */
int pvz_apks_have_english(void);

#endif
