/* pvz_apks.h -- the game folder: moved from its old name, and the APKs in it
 * told apart by what is in them (pvz_apks.c). MIT. */
#ifndef PVZ_APKS_H
#define PVZ_APKS_H

/* Before anything is written to the game folder: an older release's folder
 * (/switch/pvztouch) moves into this one, all but its old launcher NRO. The
 * outcome is kept for pvz_old_folder_report(), which logs it once the log is
 * open. */
void pvz_old_folder_move(const char *theRoot);
void pvz_old_folder_report(void);

/* The APKs in the game folder, whatever their names: the game (an APK with
 * the engine library and no English translation pak) and the optional English
 * source (an APK with the mod's assets/paks/2.ChangeGameChina.zip). Logs what
 * it found. 0: a game APK was found. */
int pvz_apks_find(const char *theRoot);
/* The game APK's path; "<root>/game.apk" when none was found. */
const char *pvz_game_apk(void);
/* The English APK's path; "<root>/english.apk" (absent) when none was found. */
const char *pvz_english_apk(void);
/* What was found, for an error screen: "a.apk (English), b.apk (not the game)". */
const char *pvz_apks_summary(void);

#endif
