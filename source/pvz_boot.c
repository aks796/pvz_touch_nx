/* pvz_boot.c -- plays the part of the game's Android activity.
 *
 * The APK's launcher activity is com.transmension.mobile.EnhanceActivity
 * (-> MainActivity -> NativeActivity). What it does, in Android's order, and
 * what this file does for it:
 *
 *   onCreate   NativeActivity: NativeApp.load() = System.load(libfmodex, not
 *              needed here) + System.load(libnative_code) -> JNI_OnLoad;
 *              NativeApp.loadNativeApp(<lib dir>/libGameMain.so, "android_main",
 *              activity, view, userDataPath, externalDataPath, sdk, assets)
 *              -> the native handle. libnative_code then dlopen()s the engine,
 *              registers a pipe on this thread's looper, and starts the app
 *              (game) thread.
 *              EnhanceActivity: System.loadLibrary("Homura") (its constructor
 *              already ran, pvz_loader.c), loadGameSettings(): one native per
 *              option (config.ini [game]), and the cheat menu's
 *              Preferences.Changes (config.ini [cheats]).
 *   onStart / onResume          -> NativeApp.onStartNative / onResumeNative
 *   surfaceCreated / Changed    -> NativeView.onSurfaceCreatedNative /
 *                                  onSurfaceChangedNative (the engine's thread
 *                                  then makes its EGL context)
 *   onWindowFocusChanged(true)  -> NativeView.onWindowFocusChangedNative
 *
 * After that this thread IS the UI thread: it runs its looper (the works
 * libnative_code posts to the UI thread, some of which the game thread waits
 * for), feeds input (pvz_input.c) and the keyboard applet (pvz_text.c), and
 * turns the Switch's focus changes into onPause/onResume. The game itself runs
 * on libnative_code's app thread (GameRender + eglSwapBuffers). MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "config.h"
#include "dcr_apkcache.h"
#include "dcr_boost.h"
#include "dcr_config.h"
#include "dcr_dircache.h"
#include "dcr_path.h"
#include "error.h"
#include "exc_handler.h"
#include "gl_layer.h"
#include "jni.h"
#include "pvz.h"
#include "rt_applet.h"
#include "so_util.h"
#include "util.h"
#include "watchdog.h"

extern so_module g_mod_native, g_mod_game, g_mod_homura;

void pvz_prof_start(void);                /* pvz_prof.c */
void pvz_setup_font_failed(const char *why); /* pvz_setup_plan.c */
void dcr_looper_run_main(int timeout_ms); /* the runtime's android_ndk.c */
void dcr_config_locale(const char *lang, const char *country, int density);

#define A_LIB_GAME "/data/app/" PVZ_PACKAGE "-1/lib/arm/" PVZ_LIB_GAME
#define A_FILES "/data/data/" PVZ_PACKAGE "/files"
#define A_EXT_FILES "/storage/emulated/0/Android/data/" PVZ_PACKAGE "/files"

/* ------------------------------------------------------------ natives */
typedef jint (*fn_onload)(void *vm, void *reserved);
typedef jlong (*fn_loadNativeApp)(void *env, void *clazz, void *lib, void *entry, void *activity,
                                  void *view, void *user_dir, void *ext_dir, jint sdk, void *am);
typedef void (*fn_h)(void *env, void *clazz, jlong h);
typedef void (*fn_hz)(void *env, void *obj, jlong h, jboolean z);
typedef void (*fn_hi)(void *env, void *obj, jlong h, jint i);
typedef void (*fn_hs)(void *env, void *obj, jlong h, void *s);
typedef void (*fn_surf_changed)(void *env, void *obj, jlong h, void *surface, jint fmt, jint w, jint hgt);
typedef void (*fn_v)(void *env, void *clazz);
typedef jboolean (*fn_z)(void *env, void *clazz);
typedef void (*fn_i)(void *env, void *clazz, jint i);
typedef void (*fn_zb)(void *env, void *clazz, jboolean b);
typedef void (*fn_prefs)(void *env, void *clazz, void *con, jint feat, void *name, jint value,
                         jboolean b, void *str);

static jlong g_handle;
static volatile int g_exit;
static fn_h n_processWorks;

int64_t pvz_native_handle(void) { return g_handle; }

void *pvz_native(const char *symbol) { return so_resolve_external(symbol); }

static void *need(const char *symbol) {
  void *p = pvz_native(symbol);
  if (!p)
    debugPrintf("[boot] MISSING native %s\n", symbol);
  return p;
}

#define NA_ "Java_com_transmension_mobile_NativeApp_"
#define NV_ "Java_com_transmension_mobile_NativeView_"
#define EA_ "Java_com_transmension_mobile_EnhanceActivity_"

static void *cls(const char *name) { return jni_class(name)->obj; }
#define C_NATIVEAPP cls("com/transmension/mobile/NativeApp")
#define C_ENHANCE cls("com/transmension/mobile/EnhanceActivity")

static void call_h(const char *name) {
  fn_h f = (fn_h)pvz_native(name);
  if (f) {
    debugPrintf("[boot] %s\n", name + sizeof(NA_) - 1);
    f(g_jni_env, C_NATIVEAPP, g_handle);
  }
}

void pvz_native_processWorks(void) {
  if (n_processWorks)
    n_processWorks(g_jni_env, C_NATIVEAPP, g_handle);
}

void pvz_request_exit(void) { g_exit = 1; }


/* Why the engine is closing (called from Activity.finish): its close flags,
 * the resource manager's error, and the code addresses on this thread's
 * stack (which Shutdown path: LawnApp::UpdateApp for mCloseRequest,
 * BaseAppDriver::Process for mLoadingFailed, ...). Hardware runs 5 and 6
 * closed right after loading with nothing else in the log. */
void pvz_log_quit_state(void) {
  static void **lawn_app;
  if (!lawn_app)
    lawn_app = (void **)so_try_find_addr_rx(&g_mod_game, "gLawnApp");
  if (lawn_app && *lawn_app) {
    const uint8_t *app = *lawn_app;
    debugPrintf("[quit] mLoadingFailed %d, mCloseRequest %d, mAuthenticated %d, loading thread done %d\n",
                app[0x571], app[0x938], app[0x7cc], app[0x570]);
    const uint8_t *rm = *(const uint8_t *const *)(app + 0x6cc); /* mResourceManager */
    if (rm && dcr_readable((uint32_t)(uintptr_t)rm, 0x130) >= 0x130) {
      const char *err = *(const char *const *)(rm + 0x128); /* mError (COW std::string) */
      if (rm[0x12c] && err && dcr_readable((uint32_t)(uintptr_t)err, 1)) {
        debugPrintf("[quit] resource manager error: %.400s\n", err);
        if (strstr(err, "Failed to load font"))
          pvz_setup_font_failed(err); /* the next start: the fonts without the button pictures */
      }
      else
        debugPrintf("[quit] resource manager: no error\n");
    }
  }
  uint32_t sp = (uint32_t)(uintptr_t)__builtin_frame_address(0);
  size_t n = dcr_readable(sp & ~3u, 0x2000);
  char line[900], nm[64];
  int len = snprintf(line, sizeof line, "[quit] stack:");
  int found = 0;
  for (uint32_t a = sp & ~3u; a + 4 <= (sp & ~3u) + n && found < 28 && len < (int)sizeof line - 70; a += 4) {
    uint32_t v = *(const volatile uint32_t *)(uintptr_t)a;
    if (dcr_is_code_addr(v & ~1u)) {
      len += snprintf(line + len, sizeof line - len, " %s", dcr_addr_name(v, nm, sizeof nm));
      found++;
    }
  }
  debugPrintf("%s\n", line);
}

float pvz_density(void) { return (float)dcr_config()->res_h / 540.0f; }

const char *pvz_language_code(void) { return dcr_config()->english ? "en" : "zh"; }

/* The engine's LawnApp exists (the mod's natives dereference it unchecked). */
int pvz_game_ready(void) {
  static void **lawn_app;
  if (!lawn_app)
    lawn_app = (void **)so_try_find_addr_rx(&g_mod_game, "gLawnApp");
  return lawn_app && *lawn_app;
}

/* For the watchdog: frames presented. Whether a stop is expected (focus,
 * the keyboard up) is the runtime's (rt_applet.c). */
uint64_t dcr_boot_frames(void) { return dcr_gl_frames(); }

/* ---------------------------------------------------- the mod's settings */
static void apply_homura_settings(void) {
  static const char *const natives[HS_COUNT] = {
      [HS_XBOX_MUSIC] = "nativeUseXboxMusics",
      [HS_MANUAL_COLLECT] = "nativeEnableManualCollect",
      [HS_DISABLE_SHOP] = "nativeDisableShop",
      [HS_XBOX_PAUSE] = "nativeEnableNewOptionsDialog",
      [HS_HIDE_COVER] = "nativeHideCoverLayer",
      [HS_SHOW_COOLDOWN] = "nativeShowCoolDown",
      [HS_NORMAL_LEVEL] = "nativeEnableNormalLevelMode",
      [HS_CLASSIC_SHOVEL] = "nativeEnableNewShovel",
      [HS_IMITATER_GREY] = "nativeEnableImitater",
      [HS_NO_TRASH_BIN] = "nativeDisableTrashBinZombie",
      [HS_SHOW_HOUSE] = "nativeShowHouse",
      [HS_NEW_COB_CANNON] = "nativeUseNewCobCannon",
      [HS_AUTO_FIX_POS] = "nativeAutoFixPosition",
      [HS_PIN_SEED_BANK] = "nativeSeedBankPin",
      [HS_DYNAMIC_PREVIEW] = "nativeDynamicPreview",
      [HS_SKIP_LOGO] = "nativeJumpLogo",
      [HS_PLAY_VIDEO] = "nativePlayVideo",
  };
  /* EnhanceActivity.loadGameSettings's order */
  static const int order[] = {HS_XBOX_MUSIC,     HS_MANUAL_COLLECT, HS_DISABLE_SHOP,
                              HS_XBOX_PAUSE,     HS_HIDE_COVER,     HS_SHOW_COOLDOWN,
                              HS_NORMAL_LEVEL,   HS_CLASSIC_SHOVEL, HS_IMITATER_GREY,
                              HS_NO_TRASH_BIN,   HS_SHOW_HOUSE,     HS_NEW_COB_CANNON,
                              HS_AUTO_FIX_POS,   HS_PIN_SEED_BANK,  HS_DYNAMIC_PREVIEW,
                              HS_SKIP_LOGO,      HS_PLAY_VIDEO};
  char sym[96];
  int n = 0;
  for (unsigned i = 0; i < sizeof order / sizeof order[0]; i++) {
    if (!dcr_config()->homura[order[i]])
      continue;
    snprintf(sym, sizeof sym, EA_ "%s", natives[order[i]]);
    fn_v f = (fn_v)need(sym);
    if (f) {
      f(g_jni_env, C_ENHANCE);
      n++;
    }
  }
  debugPrintf("[boot] loadGameSettings: %d of the mod's options on\n", n);

  const DcrConfig *c = dcr_config();
  fn_prefs changes = (fn_prefs)pvz_native("Java_com_android_support_Preferences_Changes");
  if (c->ncheats && changes) {
    JObj *empty = jni_str("");
    for (int i = 0; i < c->ncheats; i++) {
      const CheatSetting *s = &c->cheats[i];
      changes(g_jni_env, cls("com/android/support/Preferences"), g_activity, s->id, empty,
              s->is_value ? s->value : 0, s->is_value ? 0 : (jboolean)s->value, empty);
    }
    jni_release(empty);
    debugPrintf("[boot] %d cheat%s set\n", c->ncheats, c->ncheats > 1 ? "s" : "");
  }
}

/* One entry of the mod's own menu (Preferences.Changes): e.g. 41, its cheat
 * code dialog, which LawnApp::UpdateApp opens on the next frame. */
void pvz_mod_feature(int num, int value, int on) {
  fn_prefs changes = (fn_prefs)pvz_native("Java_com_android_support_Preferences_Changes");
  if (!changes || !pvz_game_ready())
    return;
  JObj *empty = jni_str("");
  changes(g_jni_env, cls("com/android/support/Preferences"), g_activity, num, empty, value,
          (jboolean)(on != 0), empty);
  jni_release(empty);
}

/* ------------------------------------------------------------- lifecycle */
static void window_focus(int on) {
  fn_hz f = (fn_hz)pvz_native(NV_ "onWindowFocusChangedNative");
  if (f)
    f(g_jni_env, g_view, g_handle, (jboolean)on);
}

/* The runtime (rt_applet.c) takes the HOME menu and sleep messages, writes
 * out the log and holds the clocks; the activity is paused and resumed here.
 * Both run from rt_applet_poll() in the frame loop below, on this thread. */
void port_focus_lost(void) {
  window_focus(0);
  call_h(NA_ "onPauseNative");
  pvz_audio_pause(1);
}

void port_focus_gained(void) {
  pvz_audio_pause(0);
  call_h(NA_ "onResumeNative");
  window_focus(1);
}

/* A backstop for the shutdown: if the game has not closed in 5 s (an engine
 * thread that never quits, say), end the process rather than hang on the
 * HOME menu's "closing" screen. */
static void exit_guard(void *arg) {
  (void)arg;
  svcSleepThread(5000000000ll);
  debugPrintf("[boot] the game did not close within 5 s: ending the process\n");
  log_flush_ring();
  svcExitProcess();
}

static void exit_guard_start(void) {
  static Thread t;
  if (R_FAILED(threadCreate(&t, exit_guard, NULL, NULL, 0x4000, 0x2B, -2)) ||
      R_FAILED(threadStart(&t)))
    debugPrintf("[boot] no exit backstop thread\n");
}

static void surface_up(void) {
  int w = dcr_config()->res_w, h = dcr_config()->res_h;
  fn_hs created = (fn_hs)need(NV_ "onSurfaceCreatedNative");
  fn_surf_changed changed = (fn_surf_changed)need(NV_ "onSurfaceChangedNative");
  fn_hi keyboard = (fn_hi)pvz_native(NV_ "onKeyboardFrameNative");
  debugPrintf("[boot] surfaceCreated\n");
  if (created)
    created(g_jni_env, g_view, g_handle, g_surface);
  debugPrintf("[boot] surfaceChanged %dx%d\n", w, h);
  if (changed)
    changed(g_jni_env, g_view, g_handle, g_surface, 1 /* PixelFormat.RGBA_8888 */, w, h);
  if (keyboard)
    keyboard(g_jni_env, g_view, g_handle, 0); /* onGlobalLayout: no IME */
}

static void report(void) {
  static u64 last_tick;
  static unsigned long last_presented;
  u64 tick = armGetSystemTick();
  unsigned long presented = (unsigned long)dcr_gl_frames();
  double fps = last_tick ? (double)(presented - last_presented) * 1e9 /
                               (double)armTicksToNs(tick - last_tick)
                         : 0.0;
  last_tick = tick;
  last_presented = presented;
  debugPrintf("[boot] %lu frames presented (%.1f fps), %lu audio writes, %d Java objects\n",
              presented, fps, (unsigned long)pvz_audio_writes(), jni_live_objects());
  dcr_boost_report();
  dcr_apkcache_report();
  dcr_dircache_report();
  /* The engine's update pacing (BaseAppDriver::Process: mFrameTime ms per
   * update, divided by mUpdateMultiplier): the loading screen ran at 20 fps. */
  static void **lawn_app;
  if (!lawn_app)
    lawn_app = (void **)so_try_find_addr_rx(&g_mod_game, "gLawnApp");
  if (lawn_app && *lawn_app) {
    const uint8_t *app = *lawn_app;
    double mult;
    memcpy(&mult, app + 0x4c0, sizeof mult);
    debugPrintf("[boot] engine: frame time %d ms, update multiplier %.2f, flag@0x486 %d\n",
                *(const int *)(app + 0x480), mult, (int)app[0x486]);
  }
}

/* before each present: the intro video over the game's picture, then the
 * right-stick pointer over both */
static void present(void) {
  pvz_video_draw();
  pvz_pointer_draw();
}

int pvz_boot_run(void) {
  pvz_java_init();
  dcr_config_locale(dcr_config()->english ? "en" : "zh", dcr_config()->english ? "US" : "CN",
                    dcr_config()->res_h >= 1080 ? 320 : 213);
  dcr_watchdog_start();
  rt_watchdog_add_counter("audio writes", pvz_audio_writes);
  pvz_input_init();
  dcr_frame_hook = pvz_ui_frame;        /* pvz_ui.c */
  dcr_present_hook = present;           /* the intro video, the pointer */

  /* ---- NativeActivity.onCreate ---- */
  fn_onload onload = (fn_onload)so_try_find_addr_rx(&g_mod_native, "JNI_OnLoad");
  if (onload)
    debugPrintf("[boot] libnative_code JNI_OnLoad -> 0x%lx\n", (unsigned long)onload(g_jni_vm, NULL));
  fn_loadNativeApp load = (fn_loadNativeApp)need(NA_ "loadNativeApp");
  n_processWorks = (fn_h)need(NA_ "processWorksNative");
  if (!load)
    fatal_error("libnative_code.so has no NativeApp.loadNativeApp: is it from PvZ TV Touch 1.1.5?");
  JObj *lib = jni_str(A_LIB_GAME), *entry = jni_str("android_main");
  JObj *user_dir = jni_str(A_FILES), *ext_dir = jni_str(A_EXT_FILES);
  debugPrintf("[boot] NativeApp.loadNativeApp(%s)\n", A_LIB_GAME);
  g_handle = load(g_jni_env, C_NATIVEAPP, lib, entry, g_activity, g_view, user_dir, ext_dir, 28,
                  g_asset_manager);
  debugPrintf("[boot] native handle 0x%llx\n", (unsigned long long)g_handle);
  if (!g_mod_game.inited) { /* NativeApp::load's dlopen should have run them */
    debugPrintf("[boot] WARNING: %s was not dlopened by loadNativeApp: running its "
                "constructors late\n", PVZ_LIB_GAME);
    so_execute_init_array(&g_mod_game);
  }
  jni_release(lib);
  jni_release(entry);
  jni_release(user_dir);
  jni_release(ext_dir);
  if (!g_handle)
    fatal_error("NativeApp.loadNativeApp failed (see debug.log).");

  /* ---- EnhanceActivity.onCreate, after super.onCreate ---- */
  if (dcr_config()->load_mod)
    apply_homura_settings();

  /* ---- onStart, onResume, the surface, focus ---- */
  /* The focus messages (HOME, sleep) are the runtime's from boot on
   * (rt_applet.c); they reach the activity from the frame loop. */
  call_h(NA_ "onStartNative");
  call_h(NA_ "onResumeNative");
  surface_up();
  window_focus(1);
  debugPrintf("[boot] activity up; this thread is the UI thread now\n");
  log_flush_ring();

  /* ---- the UI thread ---- */
  u64 last_input = 0, last_report = armGetSystemTick();
  int launch_done = 0;
  unsigned long quiet_at = 0;
  const u64 input_period = armNsToTicks(8000000ull); /* 8 ms: twice per display frame */
  while (!g_exit && !rt_exit_requested() && appletMainLoop()) {
    rt_applet_poll();
    if (!rt_focused()) {
      dcr_looper_run_main(50);
      continue;
    }
    dcr_looper_run_main(4);
    u64 now = armGetSystemTick();
    if (now - last_input >= input_period) {
      last_input = now;
      pvz_input_poll();
    }
    pvz_text_input_poll();
    dcr_boost_poll();
    unsigned long frames = (unsigned long)dcr_gl_frames();
    if (!launch_done && frames > 0) {
      launch_done = 1;
      dcr_boost_launch_end();
      debugPrintf("[boot] first frame presented\n");
      pvz_prof_start();
      quiet_at = frames + 180;
    }
    /* From ~3 s after the first picture the log goes to a RAM ring (util.c),
     * written out every 10 s and by the watchdog: a line per call on the SD
     * card costs real frame time. */
    if (quiet_at && frames >= quiet_at) {
      quiet_at = 0;
      log_set_quiet(1);
    }
    if (armTicksToNs(now - last_report) >= 10000000000ull) {
      last_report = now;
      report();
      log_flush_ring();
      log_console_update();
    }
  }

  /* Android's way out: onPause, onStop, surfaceDestroyed, then onDestroy's
   * NativeApp.unloadNativeApp. NativeApp::free (libnative_code+0x18940) runs
   * the main-thread work itself (10 ms selects) until the app thread has run
   * GameUninit and quit, joins it, and frees the app; the game saves on the
   * pause. Only then may exit() take fs, audout and nv away. */
  debugPrintf("[boot] leaving: onPause, onStop, surfaceDestroyed, unloadNativeApp\n");
  log_set_quiet(0);
  /* the hook off, no more hang reports, and the clocks running again if the
   * game was last told it lost focus (frozen clocks would stall any timed
   * wait in the shutdown) */
  rt_applet_stop();
  if (rt_focused()) {
    window_focus(0);
    call_h(NA_ "onPauseNative");
  }
  call_h(NA_ "onStopNative");
  pvz_audio_close();
  fn_h destroyed = (fn_h)pvz_native(NV_ "onSurfaceDestroyedNative");
  if (destroyed) {
    debugPrintf("[boot] surfaceDestroyed\n");
    destroyed(g_jni_env, g_view, g_handle);
  }
  exit_guard_start();
  fn_h unload = (fn_h)pvz_native(NA_ "unloadNativeApp");
  if (unload) {
    debugPrintf("[boot] unloadNativeApp\n");
    unload(g_jni_env, C_NATIVEAPP, g_handle);
    g_handle = 0;
  }
  debugPrintf("[boot] the game has closed\n");
  log_flush_ring();
  return 0;
}
