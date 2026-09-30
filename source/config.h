/* config.h -- build-wide constants for the PvZ TV Touch Switch wrapper.
 *
 * Plants vs. Zombies TV Touch (com.trans.pvztv 1.1.5-260924), armeabi-v7a:
 * Transmension's Android TV engine + the Homura "Touch" mod. AArch32 host
 * process built against libnx32 (see the Makefile). MIT.
 */
#ifndef DCR_CONFIG_H
#define DCR_CONFIG_H

/* Where on the SD card the game files live, and the module names. Releases
 * before 2026-09-30 used /switch/pvztouch (and PvZTouch.nro): its contents
 * move here on the first start (pvz_apks.c). */
#define PVZ_ROOT_PATH   "/switch/pvz_touch_nx"
#define DCR_ROOT_PATH   PVZ_ROOT_PATH
#define PVZ_OLD_ROOT_PATH "/switch/pvztouch"
#define PVZ_OLD_LAUNCHER  "PvZTouch.nro"
#define PVZ_LIB_NATIVE  "libnative_code.so"
#define PVZ_LIB_GAME    "libGameMain.so"
#define PVZ_LIB_HOMURA  "libHomura.so"
/* The user's APKs go in the game folder under any name; they are told apart
 * by their contents (pvz_apks.c). These names only win a tie. */
#define DCR_APK_NAME    "game.apk"    /* the game: PvZ TV Touch 1.1.5 */
#define PVZ_ENGLISH_APK "english.apk" /* optional: the English build of the older mod (pvz_english.c) */
/* The English files the launcher NRO carries (its romfs:/english.apk, from
 * tools/make_english_pack.py), copied into the game folder under this name
 * when the folder has no English APK (dcr_setup.c) */
#define PVZ_NRO_ENGLISH_ROMFS "english.apk"
#define PVZ_NRO_ENGLISH_NAME  "PvZ Touch English.apk"
#define PVZ_PACKAGE     "com.trans.pvztv"

/* The reserved region each game module is mapped into. Measured from this
 * build's ELF program headers (highest p_vaddr+p_memsz):
 *   libGameMain     0x787000  (~7.5 MB)
 *   libHomura       0x2714e0  (~2.5 MB)
 *   libnative_code  0x53f74   (~0.3 MB)
 * A module larger than this is refused by so_load (-3), reported by name. */
#define SO_REGION_BYTES (32u * 1024 * 1024)

/* Left outside the heap for kernel-side allocations. GPU buffers come from
 * the heap (libdrm_nouveau memaligns them and hands them to nvmap). */
#define GFX_RESERVE_MB  16u

/* Render resolution (handheld and docked share one; the compositor scales).
 * The engine lays out a 1280x720 screen itself. */
#define DCR_FORCE_SCREEN_W 1280
#define DCR_FORCE_SCREEN_H 720

#define DEBUG_LOG 1

/* The renderer: 1 = mesa/nouveau (gl_mesa.c, portlibs32/ from
 * mesa32), 0 = null GL (gl_null.c: runs the game, draws
 * nothing). Set by the Makefile. */
#ifndef DCR_GL_MESA
#define DCR_GL_MESA 0
#endif

#endif /* DCR_CONFIG_H */
