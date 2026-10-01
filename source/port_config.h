/* port_config.h -- Plants vs. Zombies Touch's settings for the android32
 * runtime.
 *
 * Macros only: the runtime's C files, its assembly and the launcher all read
 * this (runtime/source/rt_settings.h). What each setting does is next to its
 * default in the runtime; runtime/docs/ lists them all. MIT.
 */
#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

/* ------------------------------------------------------------------ the game */
#define PORT_TITLE   "Plants vs. Zombies Touch"
#define PORT_NAME    "pvz_touch_nx"
#define PORT_PACKAGE "com.trans.pvztv"
#define PORT_BANNER  "pvz_nx: Plants vs. Zombies Touch (PvZ TV Touch: Transmension engine + Homura mod, armeabi-v7a)"
/* releases before 2026-09-30 used /switch/pvztouch (and PvZTouch.nro): its
 * contents move here on the first start; the old NRO stays, and .moved there
 * says it is done */
#define PORT_OLD_ROOT_PATHS    "/switch/pvztouch"
#define RT_MIGRATE_ONCE_MARKER ".moved"

/* The APKs, by what is in them, whatever they are called: the game (the
 * engine library, no translation pak), and optionally the English source
 * (the older mod's English build, or the pack the NRO carries: it has the
 * mod's assets/paks/2.ChangeGameChina.zip). pvz_english.c reads role 1. */
#define PORT_APK_DESC "PvZ TV Touch 1.1.5 (com.trans.pvztv, armeabi-v7a)"
#define PORT_APK_ROLES                                                                        \
  {.what = "the game", .name = "game.apk",                                                  \
   .need = (const char *const[]){"lib/armeabi-v7a/libGameMain.so", NULL},                   \
   .reject = (const char *const[]){"assets/paks/2.ChangeGameChina.zip", NULL}},              \
  {.what = "the English source", .name = "english.apk",                                     \
   .need = (const char *const[]){"assets/paks/2.ChangeGameChina.zip", NULL},                \
   .flags = RT_APK_OPTIONAL}
#define PORT_LAUNCHER_START_NOTE "(the first start unpacks the game's libraries from the APK)"

/* ------------------------------------------------------------------ JNI, NDK */
/* The game is the Chinese edition: AConfiguration says zh-CN until
 * config.ini's language sets it (dcr_config_locale, pvz_boot.c). */
#define RT_ACONFIG_LANG    "zh"
#define RT_ACONFIG_COUNTRY "CN"

/* ------------------------------------------------------------------ frames */
#define RT_GL_BLIT         1 /* the intro video and the pointer (pvz_video.c, pvz_pointer.c) */
#define RT_PAD_MAX_PLAYERS 2

#endif
