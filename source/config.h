/* config.h -- the PvZ TV Touch port's own constants (the runtime's settings
 * are in port_config.h).
 *
 * Plants vs. Zombies TV Touch (com.trans.pvztv 1.1.5-260924/260925),
 * armeabi-v7a: Transmension's Android TV engine + the Homura "Touch" mod. MIT.
 */
#ifndef PVZ_CONFIG_H
#define PVZ_CONFIG_H

/* The package, as the engine's own paths spell it (/data/data/<pkg>/files). */
#define PVZ_PACKAGE     "com.trans.pvztv"

/* The game's modules, unpacked from the APK into the game folder. */
#define PVZ_LIB_NATIVE  "libnative_code.so"
#define PVZ_LIB_GAME    "libGameMain.so"
#define PVZ_LIB_HOMURA  "libHomura.so"

/* The English files the launcher NRO carries (its romfs:/english.apk, from
 * tools/make_english_pack.py), copied into the game folder under this name
 * when the folder has no English APK (pvz_main.c port_after_apk_find). */
#define PVZ_NRO_ENGLISH_ROMFS "english.apk"
#define PVZ_NRO_ENGLISH_NAME  "PvZ Touch English.apk"
/* The English APK's name when there is none (pvz_english.c then makes the
 * text only). */
#define PVZ_ENGLISH_APK "english.apk"

#endif /* PVZ_CONFIG_H */
