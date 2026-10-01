/* pvz_java.c -- the Java side of PvZ TV Touch, answered in C.
 *
 * The game's Java (com.transmension.mobile.*: EnhanceActivity -> MainActivity
 * -> NativeActivity, NativeView, AudioOutput, AndroidInputManager and the
 * InputManager event classes) does not run here. libnative_code and the mod
 * call ~70 of its methods through JNI; each has a handler below, doing what
 * the decompiled Java does (or what it does on a device with no network, no
 * sensors and no web browser). Unhandled calls are logged once (jni_core.c).
 *
 * The objects: one activity (EnhanceActivity), one view (NativeView), one
 * AudioOutput, one AndroidInputManager, one AssetManager (whose mObject field
 * libnative_code reads as the native AAssetManager pointer). Event objects
 * (InputManager$KeyInputEvent / PointerEvent / JoystickEvent / MotionRange)
 * are built per event by pvz_input.c with jni_set_field, and read back by
 * libnative_code with Get<Type>Field. MIT.
 */
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include "config.h"
#include "dcr_config.h"
#include "dcr_manifest.h"
#include "jni.h"
#include "pvz.h"
#include "rt_settings.h"
#include "so_util.h"
#include "util.h"

#define PKG PVZ_PACKAGE
#define A_APK "/data/app/" PKG "-1/base.apk"
#define A_LIBDIR "/data/app/" PKG "-1/lib/arm"
#define A_DATA "/data/data/" PKG
#define A_FILES "/data/data/" PKG "/files"
#define A_EXT_FILES "/storage/emulated/0/Android/data/" PKG "/files"

#define H(fn) static jvalue fn(JObj *self, const jvalue *a, const JMethod *m)

/* ============================================================ objects */
JObj *g_activity, *g_view, *g_audio_output, *g_input_manager, *g_asset_manager, *g_surface;

static const JClass *cls_of_obj(const void *o) { return o ? ((const JObj *)o)->cls : NULL; }

/* ---- java/util/ArrayList: p = an 'L' array (capacity), v[0] = size ---- */
static void list_finalize(JObj *l) { jni_release(l->p); }

JObj *pvz_list_new(void) {
  JObj *l = jni_new("java/util/ArrayList");
  l->p = jni_array('L', 8);
  l->v[0] = 0;
  l->finalize = list_finalize;
  return l;
}

void pvz_list_add(JObj *l, JObj *item) {
  JObj *arr = l->p;
  if (l->v[0] >= arr->a.len) {
    JObj *bigger = jni_array('L', arr->a.len * 2);
    for (jsize i = 0; i < arr->a.len; i++)
      ((JObj **)bigger->a.data)[i] = jni_retain(((JObj **)arr->a.data)[i]);
    jni_release(arr);
    l->p = arr = bigger;
  }
  ((JObj **)arr->a.data)[l->v[0]++] = jni_retain(item);
}

H(h_list_size) { return jv_i(self && self->p ? (jint)self->v[0] : 0); }
H(h_list_get) {
  if (!self || !self->p || a[0].i < 0 || a[0].i >= self->v[0])
    return jv_l(NULL);
  return jv_l(jni_retain(((JObj **)((JObj *)self->p)->a.data)[a[0].i]));
}

/* ============================================================ strings */
static JObj *str_or_null(const char *s) { return s ? jni_str(s) : NULL; }

H(h_str_getBytes) {
  const char *u = jni_utf(self);
  size_t n = strlen(u);
  JObj *arr = jni_array('B', (jsize)n);
  memcpy(arr->a.data, u, n);
  return jv_l(arr);
}
/* new String(byte[], charset): the constructor hands back a real string */
H(h_str_init_bytes) {
  JObj *arr = a[0].l;
  if (!arr || arr->kind != JK_ARRAY)
    return jv_l(jni_str(""));
  char *tmp = malloc((size_t)arr->a.len + 1);
  memcpy(tmp, arr->a.data, (size_t)arr->a.len);
  tmp[arr->a.len] = 0;
  JObj *s = jni_str(tmp);
  free(tmp);
  return jv_l(s);
}
H(h_str_substring) {
  const char *u = jni_utf(self);
  /* indices are UTF-16 units; the strings passed here are the IME's text */
  size_t n = strlen(u);
  jint b = a[0].i < 0 ? 0 : a[0].i, e = a[1].i;
  if ((size_t)e > n)
    e = (jint)n;
  if (b > e)
    b = e;
  char *tmp = strndup(u + b, (size_t)(e - b));
  JObj *s = jni_str(tmp ? tmp : "");
  free(tmp);
  return jv_l(s);
}

H(h_locale_getDefault) { return jv_l(jni_singleton("java/util/Locale")); }
H(h_locale_getLanguage) { return jv_l(jni_str(pvz_language_code())); }

/* ===================================================== NativeActivity */
H(h_getPackageSource) { return jv_l(jni_str(A_APK)); }
H(h_getPackageName) { return jv_l(jni_str(PKG)); }
H(h_getDataDir) { return jv_l(jni_str(A_DATA)); }
H(h_getNativeLibraryDir) { return jv_l(jni_str(A_LIBDIR)); }
H(h_getInternalDataPath) { return jv_l(jni_str(A_FILES)); }
H(h_getExternalDataPath) { return jv_l(jni_str(A_EXT_FILES)); }
H(h_getVersionName) {
  const char *v = dcr_manifest_version_name();
  return jv_l(jni_str(v && *v ? v : "1.1.5-260924"));
}
H(h_getVersionCode) {
  int c = dcr_manifest_version_code();
  return jv_i(c > 0 ? c : 2);
}
H(h_processWorks) {
  pvz_native_processWorks();
  return jv_none();
}
H(h_getManufacturer) { return jv_l(jni_str("Nintendo")); }
/* Build.MODEL, as the engine sees it (AGGetModel). "IDEA TV", for two reasons
 * the engine itself keys on the model (hardware run 7):
 *  - licensing: Sexy::CommonAuthManager::DoAuthenticate passes a model with
 *    "IDEA TV"/"IDEATV" in it ("Auth Pass in IDEA TV"); any other device must
 *    activate once with Transmension's server, auth.nisouwosou.com, whose
 *    domain no longer exists. A failed activation is SexyAppBase::AuthFinished
 *    (false) -> Shutdown, right after loading: the game closed there.
 *  - controls: Sexy::AndroidInput::InitButtonMap maps the standard gamepad
 *    keys (BUTTON_A..START 96-108, BACK, ENTER), which pvz_input.c sends, only
 *    for a model with "IDEA" or "TV" in it; otherwise just HID BUTTON_1..12. */
H(h_getModel) { return jv_l(jni_str("IDEA TV")); }
H(h_getProduct) { return jv_l(jni_str("switch")); }
H(h_getMacAddress) { return jv_l(jni_str("ABC")); } /* what the Java returns */
H(h_empty_string) { return jv_l(jni_str("")); }
H(h_null) { return jv_l(NULL); }
H(h_false) { return jv_z(0); }
H(h_zero) { return jv_i(0); }
H(h_void) { return jv_none(); }
H(h_getDensity) { return jv_f(pvz_density()); }
H(h_getAllAppMetaData) {
  JObj *arr = jni_array('L', 2);
  arr->cls = jni_class("[Ljava/lang/String;");
  ((JObj **)arr->a.data)[0] = jni_str("android.max_aspect");
  ((JObj **)arr->a.data)[1] = jni_str("2.4");
  return jv_l(arr);
}
/* NativeActivity.showMessageBox is a stub on this build: no dialog, this id. */
H(h_showMessageBox) {
  debugPrintf("[java] showMessageBox(\"%s\", \"%s\", %d)\n", jni_utf(a[0].l), jni_utf(a[1].l),
              a[2].i);
  return jv_i(1048576);
}
H(h_notifyStartupFinished) {
  debugPrintf("[java] notifyStartupFinished\n");
  return jv_none();
}
H(h_finish) {
  debugPrintf("[java] %s(): the game asked to close\n", m->name);
  pvz_log_quit_state();
  pvz_request_exit();
  return jv_none();
}
static char *g_clip;
H(h_setClipboard) {
  free(g_clip);
  g_clip = strdup(jni_utf(a[1].l));
  return jv_none();
}
H(h_getClipboard) { return jv_l(jni_str(g_clip ? g_clip : "")); }
H(h_openURL) {
  debugPrintf("[java] %s(%s): no web browser here\n", m->name, jni_utf(a[0].l));
  return m->ret == 'Z' ? jv_z(0) : jv_none();
}

/* MainActivity */
H(h_getAudioOutput) { return jv_l(jni_retain(g_audio_output)); }
H(h_createInputManager) { return jv_l(jni_retain(g_input_manager)); }

/* EnhanceActivity (called by the mod) */
/* the intro video (the mod's TitleScreen): pvz_video.c */
H(h_videoOpen) { return jv_z(pvz_video_open(jni_utf(a[0].l)) ? 1 : 0); }
H(h_videoPlay) { return jv_z(pvz_video_play() ? 1 : 0); }
H(h_videoStop) { return jv_z(pvz_video_stop() ? 1 : 0); }
H(h_videoIsPlaying) { return jv_z(pvz_video_playing() ? 1 : 0); }
H(h_startVibration) {
  pvz_input_rumble(a[0].i);
  return jv_none();
}
H(h_replayPicker) {
  debugPrintf("[java] %s: replay files are in %s/data/files/replays on the SD card\n", m->name,
              PORT_ROOT_PATH);
  return jv_none();
}

/* ========================================================== NativeView */
H(h_showTextInputDialog) {
  pvz_text_input_request(0, a[0].i, jni_utf(a[1].l), jni_utf(a[2].l), jni_utf(a[3].l));
  return jv_none();
}
H(h_showTextInputDialog2) {
  pvz_text_input_request(1, a[0].i, jni_utf(a[1].l), jni_utf(a[2].l), jni_utf(a[3].l));
  return jv_none();
}
H(h_hideTextInputDialog) { return jv_none(); }

static char *g_ime_text;
H(h_setText) {
  free(g_ime_text);
  g_ime_text = strdup(jni_utf(a[0].l));
  return jv_none();
}
H(h_hideIme) {
  pvz_text_ime_hidden();
  return jv_none();
}

H(h_showIme) {
  /* An edit field asked for the keyboard: same path as the dialog, with the
   * field's current text. */
  pvz_text_input_request(PVZ_TEXT_IME, 0, "", "", g_ime_text ? g_ime_text : "");
  return jv_none();
}
H(h_ime_log) {
  if (g_jni_log)
    debugPrintf("[java] NativeView.%s%s\n", m->name, m->sig);
  return jv_none();
}

/* ========================================================= AudioOutput */
H(h_ao_rate) { return jv_i(48000); }
H(h_ao_frames) { return jv_i(512); }
H(h_ao_setup) { return jv_z(pvz_audio_setup(a[0].i, a[1].i, a[2].i)); }
H(h_ao_shutdown) {
  pvz_audio_shutdown();
  return jv_none();
}
H(h_ao_write) {
  JObj *buf = a[0].l;
  if (buf && buf->p && a[2].i > 0)
    pvz_audio_write((const uint8_t *)buf->p + a[1].i, a[2].i);
  return jv_none();
}

/* ============================================== AndroidInputManager */
H(h_im_name) { return jv_l(jni_str("Android")); }
H(h_im_devices) {
  int ids[8];
  int n = pvz_input_device_ids(ids, 8);
  JObj *arr = jni_array('I', n);
  memcpy(arr->a.data, ids, sizeof(int) * (size_t)n);
  return jv_l(arr);
}
H(h_im_devname) { return jv_l(str_or_null(pvz_input_device_name(a[0].i))); }
H(h_im_devvendor) { return jv_l(pvz_input_device_name(a[0].i) ? jni_str("") : NULL); }
H(h_im_devsources) { return jv_i(pvz_input_device_sources(a[0].i)); }
H(h_im_ranges) { return jv_l(pvz_input_motion_ranges(a[0].i)); }
H(h_im_sensors) { return jv_l(jni_array('I', 0)); }
H(h_im_sensor_type) { return jv_i(-1); }

/* InputManager$MotionRange getters read its fields */
H(h_mr_int) { return jv_i(jni_get_field(self, m->name[3] == 'A' ? "mAxis" : "mSource").i); }
H(h_mr_float) {
  const char *f = !strcmp(m->name, "getMin") ? "mMin" : !strcmp(m->name, "getMax") ? "mMax"
                : !strcmp(m->name, "getFlat") ? "mFlat" : !strcmp(m->name, "getFuzz") ? "mFuzz"
                : !strcmp(m->name, "getRange") ? "mRange" : "mResolution";
  return jv_f(jni_get_field(self, f).f);
}

/* android.view.KeyEvent(downTime, eventTime, action, code, repeat, meta,
 * deviceId, scancode, flags, source) -- built by dispatchUnhandledKeyEvent */
H(h_keyevent_init) {
  jni_set_field(self, "mKeyCode", jv_i(a[3].i), 0);
  jni_set_field(self, "mAction", jv_i(a[2].i), 0);
  return jv_none();
}
H(h_dispatchUnhandledKeyEvent) {
  if (g_jni_log)
    debugPrintf("[java] unhandled key %d (action %d) -- ignored\n",
                jni_get_field(a[0].l, "mKeyCode").i, jni_get_field(a[0].l, "mAction").i);
  return jv_none();
}

/* =============================================================== table */
#define NA "com/transmension/mobile/NativeActivity"
#define MA "com/transmension/mobile/MainActivity"
#define EA "com/transmension/mobile/EnhanceActivity"
#define NV "com/transmension/mobile/NativeView"
#define AO "com/transmension/mobile/AudioOutput"
#define AIM "com/transmension/mobile/AndroidInputManager"
#define MR "com/transmension/mobile/InputManager$MotionRange"
#define S "Ljava/lang/String;"

const JMethodDef jni_method_defs[] = {
    {"java/util/ArrayList", "size", "()I", h_list_size},
    {"java/util/ArrayList", "get", "(I)Ljava/lang/Object;", h_list_get},
    {"java/util/List", "size", "()I", h_list_size},
    {"java/util/List", "get", "(I)Ljava/lang/Object;", h_list_get},
    {"java/lang/String", "getBytes", "(" S ")[B", h_str_getBytes},
    {"java/lang/String", "<init>", "([B" S ")V", h_str_init_bytes},
    {"java/lang/String", "substring", "(II)" S, h_str_substring},
    {"java/util/Locale", "getDefault", "()Ljava/util/Locale;", h_locale_getDefault},
    {"java/util/Locale", "getLanguage", "()" S, h_locale_getLanguage},

    {NA, "getPackageSource", "()" S, h_getPackageSource},
    {NA, "getPackageName", "()" S, h_getPackageName},
    {NA, "getDataDir", "()" S, h_getDataDir},
    {NA, "getNativeLibraryDir", "()" S, h_getNativeLibraryDir},
    {NA, "getInternalDataPath", "()" S, h_getInternalDataPath},
    {NA, "getUserDataPath", "()" S, h_getInternalDataPath},
    {NA, "getExternalDataPath", "()" S, h_getExternalDataPath},
    {NA, "getVersionName", "()" S, h_getVersionName},
    {NA, "getVersionCode", "()I", h_getVersionCode},
    {NA, "processWorks", "()V", h_processWorks},
    {NA, "getProduct", "()" S, h_getProduct},
    {NA, "getManufacturer", "()" S, h_getManufacturer},
    {NA, "getModel", "()" S, h_getModel},
    {NA, "getAndroidId", "()" S, h_empty_string},
    {NA, "getDeviceId", "()" S, h_empty_string},
    {NA, "getMacAddress", "()" S, h_getMacAddress},
    {NA, "getSerialNumber", "()" S, h_empty_string},
    {NA, "isOnline", "()Z", h_false},
    {NA, "getHeadsetState", "()Z", h_false},
    {NA, "setAudioMode", "(I)V", h_void},
    {NA, "openURL", "(" S ")V", h_openURL},
    {NA, "openURL", "(" S "I)V", h_openURL},
    {NA, "startWebBrowser", "(" S "I)Z", h_openURL},
    {NA, "startWebBrowser", "(" S "I" S S ")Z", h_openURL},
    {NA, "hideWebBrowser", "()V", h_void},
    {NA, "getDensity", "()F", h_getDensity},
    {NA, "isTablet", "()Z", h_false},
    {NA, "getIntentURI", "()" S, h_null},
    {NA, "getIntentExtras", "()[" S, h_null},
    {NA, "getAllMetaData", "()[" S, h_null},
    {NA, "getAllAppMetaData", "()[" S, h_getAllAppMetaData},
    {NA, "notifyStartupFinished", "()V", h_notifyStartupFinished},
    {NA, "restartApp", "()V", h_finish},
    {NA, "finish", "()V", h_finish},
    {NA, "showMessageBox", "(" S S "I)I", h_showMessageBox},
    {NA, "closeMessageBox", "(I)V", h_void},
    {NA, "installPackage", "(" S ")V", h_void},
    {NA, "setClipboard", "(" S S ")V", h_setClipboard},
    {NA, "getClipboard", "()" S, h_getClipboard},
    {MA, "getAudioOutput", "()Lcom/transmension/mobile/AudioOutput;", h_getAudioOutput},
    {MA, "createInputManager", "()Lcom/transmension/mobile/InputManager;", h_createInputManager},
    {EA, "startVibration", "(I)V", h_startVibration},
    {EA, "showReplayImportPicker", "(" S ")V", h_replayPicker},
    {EA, "showReplayExportPicker", "(" S S ")V", h_replayPicker},
    {EA, "startOrientationListener", "()V", h_void},
    {EA, "stopOrientationListener", "()V", h_void},
    {EA, "videoOpen", "(" S ")Z", h_videoOpen},
    {EA, "videoPlay", "()Z", h_videoPlay},
    {EA, "videoStop", "()Z", h_videoStop},
    {EA, "videoClose", "()Z", h_videoStop},
    {EA, "videoShow", "(Z)V", h_void},
    {EA, "videoIsPlaying", "()Z", h_videoIsPlaying},

    {NV, "showTextInputDialog", "(I" S S S ")V", h_showTextInputDialog},
    {NV, "showTextInputDialog2", "(I" S S S ")V", h_showTextInputDialog2},
    {NV, "hideTextInputDialog", "()V", h_hideTextInputDialog},
    {NV, "hideTextInputDialog", "(Z)V", h_hideTextInputDialog},
    {NV, "hideTextInputDialog2", "()V", h_hideTextInputDialog},
    {NV, "showIme", "(I)V", h_showIme},
    {NV, "hideIme", "(I)V", h_hideIme},
    {NV, "setText", "(" S ")V", h_setText},
    {NV, "setText", "(" S "II)V", h_setText},
    {NV, "setSelection", "(II)V", h_ime_log},
    {NV, "setInputCookie", "(J)V", h_ime_log},
    {NV, "setInputType", "(I)V", h_ime_log},
    {NV, "setImeOptions", "(I)V", h_ime_log},
    {NV, "getDensity", "()F", h_getDensity},
    {NV, "dispatchUnhandledKeyEvent", "(Landroid/view/KeyEvent;)V", h_dispatchUnhandledKeyEvent},
    {"android/view/KeyEvent", "<init>", "(JJIIIIIIII)V", h_keyevent_init},

    {AO, "getPreferredSampleRate", "()I", h_ao_rate},
    {AO, "getPreferredFramesPerBuffer", "()I", h_ao_frames},
    {AO, "setup", "(III)Z", h_ao_setup},
    {AO, "shutdown", "()V", h_ao_shutdown},
    {AO, "write", "(Ljava/nio/ByteBuffer;II)V", h_ao_write},
    {AO, "onPause", "()V", h_void},
    {AO, "onResume", "()V", h_void},

    {AIM, "getName", "()" S, h_im_name},
    {AIM, "getFeatures", "()I", h_zero},
    {AIM, "getDeviceList", "()[I", h_im_devices},
    {AIM, "getDeviceName", "(I)" S, h_im_devname},
    {AIM, "getDeviceVendor", "(I)" S, h_im_devvendor},
    {AIM, "getDeviceSources", "(I)I", h_im_devsources},
    {AIM, "getDeviceMotionRanges", "(I)Ljava/util/List;", h_im_ranges},
    {AIM, "getSensorList", "()[I", h_im_sensors},
    {AIM, "hasSensor", "(I)Z", h_false},
    {AIM, "startSensor", "(II)Z", h_false},
    {AIM, "stopSensor", "(II)Z", h_false},
    {AIM, "getSensorType", "(I)I", h_im_sensor_type},
    {AIM, "getSensorName", "(II)" S, h_empty_string},
    {AIM, "getSensorVendor", "(II)" S, h_empty_string},
    {AIM, "getSensorMaxRange", "(II)F", h_zero},
    {AIM, "getSensorResolution", "(II)F", h_zero},
    {MR, "getAxis", "()I", h_mr_int},
    {MR, "getSource", "()I", h_mr_int},
    {MR, "getMin", "()F", h_mr_float},
    {MR, "getMax", "()F", h_mr_float},
    {MR, "getFlat", "()F", h_mr_float},
    {MR, "getFuzz", "()F", h_mr_float},
    {MR, "getRange", "()F", h_mr_float},
    {MR, "getResolution", "()F", h_mr_float},
    {NULL, NULL, NULL, NULL},
};

const JFieldDef jni_field_defs[] = {
    {NULL, NULL, NULL, 0, NULL},
};

const char *const jni_class_supers[][2] = {
    {EA, MA},
    {MA, NA},
    {NA, "android/app/Activity"},
    {"android/app/Activity", "android/view/ContextThemeWrapper"},
    {"android/view/ContextThemeWrapper", "android/content/ContextWrapper"},
    {"android/content/ContextWrapper", "android/content/Context"},
    {NV, "android/view/SurfaceView"},
    {"android/view/SurfaceView", "android/view/View"},
    {AIM, "com/transmension/mobile/InputManager"},
    {"java/util/ArrayList", "java/util/List"},
    {"java/nio/DirectByteBuffer", "java/nio/ByteBuffer"},
    {"com/transmension/mobile/InputManager$KeyInputEvent", "com/transmension/mobile/InputManager$InputEvent"},
    {"com/transmension/mobile/InputManager$PointerEvent", "com/transmension/mobile/InputManager$InputEvent"},
    {"com/transmension/mobile/InputManager$JoystickEvent", "com/transmension/mobile/InputManager$InputEvent"},
    {NULL, NULL},
};

/* Only used when classes.txt (the APK's class list) is missing. */
const char *const jni_missing_classes[] = {
    "com/transmension/mobile/GameCenter*", "com/transmension/mobile/ShareManager*",
    "com/transmension/mobile/NotificationManager*", "com/transmension/mobile/ImagePicker*",
    "com/transmension/mobile/SocialManager*", "com/transmension/mobile/AudioRecorder*",
    NULL,
};

/* ================================================================ setup */
void pvz_java_init(void) {
  jni_init();
  g_activity = jni_singleton(EA);
  g_view = jni_singleton(NV);
  g_audio_output = jni_singleton(AO);
  g_input_manager = jni_singleton(AIM);
  g_surface = jni_singleton("android/view/Surface");
  g_asset_manager = jni_singleton("android/content/res/AssetManager");
  /* AssetManager.mObject: the native AAssetManager. libnative_code only hands
   * it to AConfiguration_fromAssetManager, which does not look inside. */
  static uint32_t fake_aasset_manager[16];
  jni_set_field(g_asset_manager, "mObject", jv_j((jlong)(uintptr_t)fake_aasset_manager), 0);
  (void)cls_of_obj;
  debugPrintf("[java] objects ready: activity %p view %p audio %p input %p\n", (void *)g_activity,
              (void *)g_view, (void *)g_audio_output, (void *)g_input_manager);
}

