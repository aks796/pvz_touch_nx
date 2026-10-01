/* pvz.h -- the PvZ TV Touch side of the port: the pieces that stand in for the
 * game's Java activity, talking to each other. MIT. */
#ifndef PVZ_H
#define PVZ_H
#include <stddef.h>
#include <stdint.h>
#include "jni.h"

/* pvz_java.c: the Java objects the game is given */
extern JObj *g_activity, *g_view, *g_audio_output, *g_input_manager, *g_asset_manager, *g_surface;
void pvz_java_init(void);
JObj *pvz_list_new(void);                 /* java.util.ArrayList */
void pvz_list_add(JObj *list, JObj *item); /* the list takes a reference */

/* pvz_boot.c: the activity */
void pvz_native_processWorks(void);        /* NativeApp.processWorksNative(handle) */
void pvz_request_exit(void);               /* Activity.finish() */
void pvz_log_quit_state(void);             /* the engine closing: why (the log) */
float pvz_density(void);                   /* DisplayMetrics.density */
const char *pvz_language_code(void);       /* Locale.getDefault().getLanguage() */
int pvz_game_ready(void);                  /* the engine's LawnApp exists */
int64_t pvz_native_handle(void);           /* NativeActivity.mNativeHandle */
/* Look up a JNI export of the game's modules ("Java_com_..."), NULL if absent. */
void *pvz_native(const char *symbol);

/* pvz_text.c: NativeView.showTextInputDialog / the mod's showTextInputDialog2 /
 * showIme. mod: 0 -> reply with NativeView.onTextInputNative, 1 ->
 * onTextInputNative2, PVZ_TEXT_IME -> onTextChangedNative, then Return in the
 * field (pvz_text_frame). The keyboard opens from the UI thread loop
 * (pvz_text_input_poll). */
#define PVZ_TEXT_IME 2
void pvz_text_input_request(int mod, int mode, const char *title, const char *hint,
                            const char *initial);
/* the edit field's held request (pvz_text.c): hideIme; each frame on the
 * engine's thread, the focused widget and its rectangle (game pixels) (1:
 * press Return in it now); A down (1: the keyboard comes up for it, so the
 * game does not get the A); a touch down, in game pixels */
void pvz_text_ime_hidden(void);
int pvz_text_frame(void *focus, int x, int y, int w, int h);
int pvz_text_press_a(void);
void pvz_text_touch(float x, float y);
void pvz_text_input_poll(void);

/* pvz_audio.c: AudioOutput */
int pvz_audio_setup(int rate, int channels, int bits);
void pvz_audio_shutdown(void);
void pvz_audio_write(const void *data, int len);
uint32_t pvz_audio_writes(void);
void pvz_audio_pause(int paused); /* background: writes wait, like a paused AudioTrack */
void pvz_audio_close(void);        /* exit: writes are dropped from now on */

/* pvz_input.c: controllers and the touchscreen as Android input devices */
void pvz_input_init(void);
void pvz_input_poll(void);                  /* UI thread, once per display frame */
int pvz_input_device_ids(int *ids, int cap);
const char *pvz_input_device_name(int id);
int pvz_input_device_sources(int id);
JObj *pvz_input_motion_ranges(int id);      /* List<InputManager$MotionRange> */
void pvz_input_rumble(int kind);            /* EnhanceActivity.startVibration */

/* pvz_loader.c */
int pvz_load_modules(void);
void pvz_run_constructors(void);

/* pvz_ui.c: fixes to the game's screens, once per frame (eglSwapBuffers). */
void pvz_ui_frame(void);

/* pvz_pointer.c: the right-stick pointer (state from pvz_input.c, drawn before
 * each present). */
void pvz_pointer_init(void);
void pvz_pointer_set(float x, float y, int shown, int pressed);
void pvz_pointer_draw(void);

/* pvz_video.c: the intro video (EnhanceActivity.videoOpen/Play/IsPlaying/
 * Close), its pictures drawn before each present, its sound mixed into the
 * game's output. */
int pvz_video_open(const char *path);
int pvz_video_play(void);
int pvz_video_playing(void);
void pvz_video_skip(void);
int pvz_video_stop(void);
void pvz_video_draw(void);
void pvz_video_mix(int16_t *out, int frames, int out_rate);

/* pvz_boot.c: set one entry of the mod's menu (Preferences.Changes). */
void pvz_mod_feature(int num, int value, int on);
#define PVZ_FEATURE_CHEAT_CODE_DIALOG 41
#define PVZ_FEATURE_LEVEL_JUMP_DIALOG 81
/* pvz_ui.c: a level is up (LawnApp::mBoard) */
int pvz_ui_in_level(void);

#endif
