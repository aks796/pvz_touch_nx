/* pvz_input.c -- Switch controllers and the touchscreen as Android input.
 *
 * The game is Transmension's Android TV edition: it is played with gamepads.
 * Its engine maps Android gamepad keycodes onto its own gamepad keys
 * (AndroidAppDriver::InitKeyMap: BUTTON_A..START -> KEYCODE_GAMEPAD_A.. 0x130+,
 * D-pad -> the arrow keys, BACK -> ESCAPE) and reads the sticks as joystick
 * axes. So each controller is an Android gamepad, and events go where the
 * Java would send them:
 *   keys      MainActivity.onNativeKeyEvent -> AndroidInputManager.onKeyEvent
 *             -> NativeInputManager.onKeyInputEventNative(handle, im, KeyInputEvent)
 *   sticks    NativeView.onGenericMotionEvent -> ... -> onJoystickEventNative
 *             (handle, im, JoystickEvent: all of the device's axes)
 *   touch     EnhanceActivity's touch listener -> onTouchEventNative(handle,
 *             null, PointerEvent) for the first finger; a second finger that
 *             lands on the lawn goes to the mod's nativeSendSecondTouch
 *             (board-relative, 1280x720 units), exactly as the listener does
 * and EnhanceActivity's extras: L/R also call the mod's native1PButtonDown
 * (native2PButtonDown for player 2) with GAMEPAD_BUTTON_TL/TR, and with
 * "advanced pause" (+) toggles the mod's nativeGaoJiPause in a level.
 *
 * Buttons: A B X Y L R ZL ZR + - L3 R3 -> BUTTON_A B X Y L1 R1 L2 R2 C Z
 * THUMBL THUMBR; D-pad -> DPAD_UP/DOWN/LEFT/RIGHT; left stick -> AXIS_X
 * Y, right stick -> AXIS_Z RZ. + goes as BUTTON_C and - as BUTTON_Z: the
 * engine's pad map (AndroidInput::InitButtonMap) has BUTTON_START and
 * BUTTON_SELECT only for one TV box's branch, not the one this port takes,
 * so those are dropped; C and Z are its START and SELECT too
 * (Gamepad::MapFromkeyCode: GAMEPAD_C -> START, GAMEPAD_Z -> SELECT), and
 * the pause menu closes on C as on START. Player 1 is the handheld or
 * the first controller (device 1), player 2 the second (device 2). The
 * touchscreen is device 10. Events are made on the UI thread, twice per
 * display frame. The lawn with the Xbox scheme is the Xbox 360 edition's: A
 * plants, B digs up, L R pick the seed packet, ZL ZR (its triggers) switch
 * the sun magnet, + pauses; the mod adds Y fast forward and - the item bar
 * (GamepadControls.cpp, SwitchHud.cpp).
 *
 * The pointer (config.ini [controls] pointer): clicking player 1's right stick
 * brings it up; the right stick moves it and ZR taps with it (a touch, as a
 * finger's; pvz_pointer.c draws it); a click or a few idle seconds put it
 * away. Without it, the right stick outside a level is a second D-pad (with
 * repeat): menus, achievements and lists scroll with it.
 *
 * Cheat codes: L, R, ZL, ZR pressed in that order during a level (the Xbox 360
 * edition's LB RB LT RT) opens the game's cheat code dialog, as the mod's own
 * menu does (its entry 41; the mod took the button combination out). MIT.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "dcr_config.h"
#include "dcr_time.h"
#include "jni.h"
#include "pvz.h"
#include "util.h"

void dcr_window_size(int *w, int *h);

#define KEY_DOWN 0
#define KEY_UP 1
#define AM_DOWN 0
#define AM_UP 1
#define AM_MOVE 2
#define SRC_KEYBOARD 0x101
#define SRC_DPAD 0x201
#define SRC_GAMEPAD 0x401
#define SRC_TOUCH 0x1002
#define SRC_JOYSTICK 0x1000010
#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 11
#define AXIS_RZ 14

#define NPLAYERS 2
#define TOUCH_DEVICE 10

static const int k_axes[] = {AXIS_X, AXIS_Y, AXIS_Z, AXIS_RZ};
#define NAXES 4

typedef void (*fn_event)(void *env, void *clazz, jlong h, void *im, void *ev);
typedef void (*fn_i)(void *env, void *clazz, jint i);
typedef void (*fn_iii)(void *env, void *clazz, jint a, jint b, jint c);
typedef void (*fn_z)(void *env, void *clazz, jboolean z);
typedef jboolean (*fn_rz)(void *env, void *clazz);

static struct {
  fn_event key, touch, joystick;
  fn_iii second_touch;
  fn_i p1_button, p2_button;
  fn_z gaoji_pause;
  fn_rz gaoji_paused, in_game;
} N;

static PadState g_pad[NPLAYERS];
static float g_axis_sent[NPLAYERS][NAXES];
static int g_announced[NPLAYERS];
static int g_ready;
static void *g_nim_class;

static int64_t now_ms(void) { return (int64_t)(dcr_monotonic_ns() / 1000000ull); }

/* ---------------------------------------------------------------- setup */
/* one line in the log when a player's controller comes or goes, or changes */
static void log_controller(int p) {
  static u32 seen[NPLAYERS] = {0xffffffffu, 0xffffffffu};
  const u32 style = padIsConnected(&g_pad[p]) ? padGetStyleSet(&g_pad[p]) : 0;
  if (style == seen[p])
    return;
  seen[p] = style;
  if (!style) {
    debugPrintf("[input] player %d: no controller\n", p + 1);
    return;
  }
  char kinds[96] = "";
  static const struct { u32 tag; const char *name; } k[] = {
      {HidNpadStyleTag_NpadFullKey, "Pro Controller"}, {HidNpadStyleTag_NpadHandheld, "handheld Joy-Cons"},
      {HidNpadStyleTag_NpadJoyDual, "two Joy-Cons"},   {HidNpadStyleTag_NpadJoyLeft, "left Joy-Con"},
      {HidNpadStyleTag_NpadJoyRight, "right Joy-Con"},
  };
  for (unsigned i = 0; i < sizeof k / sizeof k[0]; i++)
    if (style & k[i].tag)
      snprintf(kinds + strlen(kinds), sizeof kinds - strlen(kinds), "%s%s", kinds[0] ? " + " : "",
               k[i].name);
  debugPrintf("[input] player %d: %s (style 0x%x)\n", p + 1, kinds[0] ? kinds : "a controller",
              (unsigned)style);
}

void pvz_input_init(void) {
  /* players 1-2 and handheld, any standard controller (the supported-id list
   * needs this project's libnx32: the image's sent it garbled) */
  padConfigureInput(NPLAYERS, HidNpadStyleSet_NpadStandard);
  debugPrintf("[input] controllers accepted: players 1-2 and handheld (Pro Controller, Joy-Cons)\n");
  padInitialize(&g_pad[0], HidNpadIdType_No1, HidNpadIdType_Handheld);
  padInitialize(&g_pad[1], HidNpadIdType_No2);
  hidInitializeTouchScreen();
  if (dcr_config()->pointer)
    pvz_pointer_init();
  N.key = (fn_event)pvz_native("Java_com_transmension_mobile_NativeInputManager_onKeyInputEventNative");
  N.touch = (fn_event)pvz_native("Java_com_transmension_mobile_NativeInputManager_onTouchEventNative");
  N.joystick = (fn_event)pvz_native("Java_com_transmension_mobile_NativeInputManager_onJoystickEventNative");
#define EA_ "Java_com_transmension_mobile_EnhanceActivity_"
  N.second_touch = (fn_iii)pvz_native(EA_ "nativeSendSecondTouch");
  N.p1_button = (fn_i)pvz_native(EA_ "native1PButtonDown");
  N.p2_button = (fn_i)pvz_native(EA_ "native2PButtonDown");
  N.gaoji_pause = (fn_z)pvz_native(EA_ "nativeGaoJiPause");
  N.gaoji_paused = (fn_rz)pvz_native(EA_ "nativeIsGaoJiPaused");
  N.in_game = (fn_rz)pvz_native(EA_ "nativeIsInGame");
  g_nim_class = jni_class("com/transmension/mobile/NativeInputManager")->obj;
  g_ready = N.key && N.touch && N.joystick;
  debugPrintf("[input] natives: key %p touch %p joystick %p, mod: second touch %p, 1P %p 2P %p\n",
              (void *)N.key, (void *)N.touch, (void *)N.joystick, (void *)N.second_touch,
              (void *)N.p1_button, (void *)N.p2_button);
}

/* --------------------------------------------------------- the devices */
int pvz_input_device_ids(int *ids, int cap) {
  int n = 0;
  for (int p = 0; p < NPLAYERS && n < cap; p++)
    ids[n++] = p + 1;
  if (n < cap)
    ids[n++] = TOUCH_DEVICE;
  return n;
}

const char *pvz_input_device_name(int id) {
  if (id == TOUCH_DEVICE)
    return "touchscreen";
  if (id >= 1 && id <= NPLAYERS)
    return id == 1 ? "Nintendo Switch Controller 1" : "Nintendo Switch Controller 2";
  return NULL;
}

int pvz_input_device_sources(int id) {
  if (id == TOUCH_DEVICE)
    return SRC_TOUCH;
  if (id >= 1 && id <= NPLAYERS)
    return SRC_GAMEPAD | SRC_DPAD | SRC_JOYSTICK;
  return 0;
}

JObj *pvz_input_motion_ranges(int id) {
  if (id < 1 || id > NPLAYERS)
    return NULL;
  JObj *list = pvz_list_new();
  for (int i = 0; i < NAXES; i++) {
    JObj *r = jni_new("com/transmension/mobile/InputManager$MotionRange");
    jni_set_field(r, "mAxis", jv_i(k_axes[i]), 0);
    jni_set_field(r, "mSource", jv_i(SRC_JOYSTICK), 0);
    jni_set_field(r, "mMin", jv_f(-1.0f), 0);
    jni_set_field(r, "mMax", jv_f(1.0f), 0);
    jni_set_field(r, "mRange", jv_f(2.0f), 0);
    jni_set_field(r, "mFlat", jv_f(0.12f), 0);
    jni_set_field(r, "mFuzz", jv_f(0.0f), 0);
    jni_set_field(r, "mResolution", jv_f(0.0f), 0);
    pvz_list_add(list, r);
    jni_release(r);
  }
  return list;
}

/* ---------------------------------------------------------- the events */
static void base_fields(JObj *e, int device, int action, int source, int64_t down_ms) {
  int64_t t = now_ms();
  jni_set_field(e, "mDeviceId", jv_i(device), 0);
  jni_set_field(e, "mEventTime", jv_j(t), 0);
  jni_set_field(e, "mDownTime", jv_j(down_ms ? down_ms : t), 0);
  jni_set_field(e, "mAction", jv_i(action), 0);
  jni_set_field(e, "mSource", jv_i(source), 0);
}

/* The event is the native's local reference for the call: it may
 * DeleteLocalRef it itself; if not, the reference dies when it returns. */
static void local_ref_done(JObj *e, int32_t refs_before_call) {
  if (e->refs >= refs_before_call)
    jni_release(e);  /* the native's local, still alive at return */
  jni_release(e);    /* ours */
}
#define CALL_WITH_LOCAL(e, call)        \
  do {                                  \
    jni_retain(e);                      \
    int32_t r_ = (e)->refs;             \
    call;                               \
    local_ref_done((e), r_);            \
  } while (0)

static void send_key(int player, int action, int keycode) {
  if (!g_ready)
    return;
  static int64_t down_at[NPLAYERS][256];
  int64_t t = now_ms();
  if (action == KEY_DOWN && keycode < 256)
    down_at[player][keycode] = t;
  JObj *e = jni_new("com/transmension/mobile/InputManager$KeyInputEvent");
  int src = (keycode >= 19 && keycode <= 23) ? (SRC_DPAD | SRC_GAMEPAD) : (SRC_GAMEPAD | SRC_KEYBOARD);
  base_fields(e, player + 1, action, src, keycode < 256 ? down_at[player][keycode] : t);
  jni_set_field(e, "mFlags", jv_i(8 /* FLAG_FROM_SYSTEM */), 0);
  jni_set_field(e, "mMetaState", jv_i(0), 0);
  jni_set_field(e, "mKeyCode", jv_i(keycode), 0);
  jni_set_field(e, "mKeyChar", jv_i(0), 0);
  jni_set_field(e, "mRepeatCount", jv_i(0), 0);
  jni_set_field(e, "mScanCode", jv_i(0), 0);
  CALL_WITH_LOCAL(e, N.key(g_jni_env, g_nim_class, pvz_native_handle(), g_input_manager, e));
}

static void send_axes(int player, const float *v) {
  if (!g_ready)
    return;
  JObj *e = jni_new("com/transmension/mobile/InputManager$JoystickEvent");
  base_fields(e, player + 1, AM_MOVE, SRC_JOYSTICK, 0);
  jni_set_field(e, "mFlags", jv_i(SRC_JOYSTICK), 0); /* the Java copies the source here */
  jni_set_field(e, "mActionIndex", jv_i(0), 0);
  JObj *axes = pvz_list_new();
  for (int i = 0; i < NAXES; i++) {
    JObj *a = jni_new("com/transmension/mobile/InputManager$JoystickEvent$Axis");
    jni_set_field(a, "mAxis", jv_i(k_axes[i]), 0);
    jni_set_field(a, "mValue", jv_f(v[i]), 0);
    pvz_list_add(axes, a);
    jni_release(a);
  }
  jni_set_field(e, "mAxes", jv_l(axes), 1);
  jni_release(axes);
  CALL_WITH_LOCAL(e, N.joystick(g_jni_env, g_nim_class, pvz_native_handle(), g_input_manager, e));
}

static void send_touch(int action, int id, float x, float y, int64_t down_ms) {
  if (!g_ready)
    return;
  JObj *e = jni_new("com/transmension/mobile/InputManager$PointerEvent");
  base_fields(e, TOUCH_DEVICE, action, SRC_TOUCH, down_ms);
  jni_set_field(e, "mFlags", jv_i(SRC_TOUCH), 0);
  jni_set_field(e, "mActionIndex", jv_i(0), 0);
  JObj *ptrs = pvz_list_new();
  JObj *p = jni_new("com/transmension/mobile/InputManager$PointerEvent$Pointer");
  jni_set_field(p, "mId", jv_i(id), 0);
  jni_set_field(p, "mX", jv_f(x), 0);
  jni_set_field(p, "mY", jv_f(y), 0);
  jni_set_field(p, "mPressure", jv_f(action == AM_UP ? 0.0f : 1.0f), 0);
  pvz_list_add(ptrs, p);
  jni_release(p);
  jni_set_field(e, "mPointers", jv_l(ptrs), 1);
  jni_release(ptrs);
  CALL_WITH_LOCAL(e, N.touch(g_jni_env, g_nim_class, pvz_native_handle(), NULL, e));
}

/* ------------------------------------------------------------ touch */
static struct {
  int first_id, second_id; /* finger ids, -1 = none */
  int64_t down_ms;
  float last_x, last_y;    /* first finger */
  float x2, y2;            /* second finger */
} T = {-1, -1, 0, 0, 0, 0, 0};

/* EnhanceActivity.refreshNativeViewBorders: the lawn, in view pixels */
static int on_board(float x, float y, float sx, float sy) {
  return x > 240.0f * sx && x < 1040.0f * sx && y > 60.0f * sy && y < 660.0f * sy;
}

static void second_touch(float x, float y, float sx, float sy, int action) {
  if (N.second_touch && pvz_game_ready())
    N.second_touch(g_jni_env, jni_class("com/transmension/mobile/EnhanceActivity")->obj,
                   (jint)lroundf((x - 240.0f * sx) / sx), (jint)lroundf((y - 60.0f * sy) / sy), action);
}

static void touch_poll(void) {
  HidTouchScreenState ts = {0};
  if (!hidGetTouchScreenStates(&ts, 1))
    return;
  int vw, vh;
  dcr_window_size(&vw, &vh);
  const float px = (float)vw / 1280.0f, py = (float)vh / 720.0f; /* panel -> view */
  const float sx = (float)vw / 1280.0f, sy = (float)vh / 720.0f;  /* view / 1280x720 */
  int first_seen = 0, second_seen = 0;
  for (int k = 0; k < ts.count && k < 16; k++) {
    const int id = (int)ts.touches[k].finger_id;
    const float x = (float)ts.touches[k].x * px, y = (float)ts.touches[k].y * py;
    if (id == T.first_id) {
      first_seen = 1;
      if (x != T.last_x || y != T.last_y) {
        T.last_x = x, T.last_y = y;
        send_touch(AM_MOVE, 0, x, y, T.down_ms);
      }
    } else if (id == T.second_id) {
      second_seen = 1;
      if (x != T.x2 || y != T.y2) {
        T.x2 = x, T.y2 = y;
        second_touch(x, y, sx, sy, 1);
      }
    } else if (T.first_id < 0) {
      T.first_id = id;
      first_seen = 1;
      T.down_ms = now_ms();
      T.last_x = x, T.last_y = y;
      pvz_text_touch(x / sx, y / sy);
      send_touch(AM_DOWN, 0, x, y, T.down_ms);
    } else if (T.second_id < 0 && dcr_config()->second_touch && on_board(x, y, sx, sy)) {
      T.second_id = id;
      second_seen = 1;
      T.x2 = x, T.y2 = y;
      second_touch(x, y, sx, sy, 0);
    }
  }
  if (T.first_id >= 0 && !first_seen) {
    send_touch(AM_UP, 0, T.last_x, T.last_y, T.down_ms);
    T.first_id = -1;
  }
  if (T.second_id >= 0 && !second_seen) {
    second_touch(T.x2, T.y2, sx, sy, 2);
    T.second_id = -1;
  }
}

/* ---------------------------------------------------------- controllers */
static const struct {
  u64 button;
  int keycode;
} k_keys[] = {
    {HidNpadButton_A, 96},       /* BUTTON_A */
    {HidNpadButton_B, 97},       /* BUTTON_B */
    {HidNpadButton_X, 99},       /* BUTTON_X */
    {HidNpadButton_Y, 100},      /* BUTTON_Y */
    {HidNpadButton_L, 102},      /* BUTTON_L1 */
    {HidNpadButton_R, 103},      /* BUTTON_R1 */
    {HidNpadButton_ZL, 104},     /* BUTTON_L2 */
    {HidNpadButton_ZR, 105},     /* BUTTON_R2 */
    {HidNpadButton_StickL, 106}, /* BUTTON_THUMBL */
    {HidNpadButton_StickR, 107}, /* BUTTON_THUMBR */
    {HidNpadButton_Plus, 98},    /* BUTTON_C: the engine's START (see the top) */
    {HidNpadButton_Minus, 101},  /* BUTTON_Z: the engine's SELECT (see the top) */
    /* the D-pad last (pad_poll's repeat) */
    {HidNpadButton_Up, 19},      /* DPAD_UP */
    {HidNpadButton_Down, 20},    /* DPAD_DOWN */
    {HidNpadButton_Left, 21},    /* DPAD_LEFT */
    {HidNpadButton_Right, 22},   /* DPAD_RIGHT */
};

/* ---------------------------------------------------------------- pointer */
#define POINTER_KEEP_MS 8000
static struct {
  float x, y;
  int shown, pressed, placed;
  int64_t active_ms, down_ms, last_ms;
} P;

static int pointer_on(void) { return dcr_config()->pointer; }

/* A click of the right stick shows or hides it; ZR is its while it shows,
 * and until a tap it began has ended. */
static int pointer_takes(u64 button, int down) {
  if (!pointer_on())
    return 0;
  const int64_t t = now_ms();
  int vw, vh;
  dcr_window_size(&vw, &vh);
  if (button == HidNpadButton_StickR) {
    if (down) {
      if (P.shown && !P.pressed) {
        P.shown = 0;
      } else if (!P.shown) {
        if (!P.placed) {
          P.x = vw * 0.5f, P.y = vh * 0.5f;
          P.placed = 1;
        }
        P.shown = 1;
        P.active_ms = t;
      }
      pvz_pointer_set(P.x, P.y, P.shown, P.pressed);
    }
    return 1; /* the click is the pointer's either way */
  }
  if (button != HidNpadButton_ZR)
    return 0;
  if (down) {
    if (!P.shown || T.first_id >= 0)
      return 0; /* not showing, or a finger is on the screen */
    P.pressed = 1;
    P.down_ms = P.active_ms = t;
    send_touch(AM_DOWN, 0, P.x, P.y, P.down_ms);
  } else {
    if (!P.pressed)
      return 0;
    P.pressed = 0;
    P.active_ms = t;
    send_touch(AM_UP, 0, P.x, P.y, P.down_ms);
  }
  pvz_pointer_set(P.x, P.y, P.shown, P.pressed);
  return 1;
}

/* the right stick moves it (faster the further it is pushed) */
static void pointer_move(float rx, float ry) {
  const int64_t t = now_ms();
  float dt = P.last_ms ? (float)(t - P.last_ms) / 1000.0f : 0.0f;
  P.last_ms = t;
  if (dt > 0.05f)
    dt = 0.05f;
  int vw, vh;
  dcr_window_size(&vw, &vh);
  const float mag = sqrtf(rx * rx + ry * ry);
  if (P.shown && mag > 0.15f) {
    P.active_ms = t;
    const float m = (mag - 0.15f) / 0.85f;
    const float speed = (float)vh / 720.0f * (150.0f + 1100.0f * m * m); /* px/s */
    P.x += rx / mag * speed * dt;
    P.y += ry / mag * speed * dt;
    if (P.x < 0) P.x = 0;
    if (P.y < 0) P.y = 0;
    if (P.x > vw - 1) P.x = (float)(vw - 1);
    if (P.y > vh - 1) P.y = (float)(vh - 1);
    if (P.pressed)
      send_touch(AM_MOVE, 0, P.x, P.y, P.down_ms);
  } else if (P.shown && !P.pressed && t - P.active_ms > POINTER_KEEP_MS) {
    P.shown = 0;
  }
  pvz_pointer_set(P.x, P.y, P.shown, P.pressed);
}

/* ------------------------------------------------ the right stick as a D-pad */
static void stick_dpad(int p, float rx, float ry) {
  static int held[NPLAYERS];         /* keycode held, 0 none */
  static int64_t next_ms[NPLAYERS];
  const float mag = sqrtf(rx * rx + ry * ry);
  int want = 0;
  if (mag > 0.55f)
    want = fabsf(rx) > fabsf(ry) ? (rx > 0 ? 22 : 21) : (ry > 0 ? 20 : 19);
  else if (mag > 0.35f && held[p])
    want = held[p]; /* a little hysteresis */
  const int64_t t = now_ms();
  if (want != held[p]) {
    if (held[p])
      send_key(p, KEY_UP, held[p]);
    held[p] = want;
    if (want) {
      send_key(p, KEY_DOWN, want);
      next_ms[p] = t + 380;
    }
  } else if (want && t >= next_ms[p]) { /* repeat while held */
    send_key(p, KEY_UP, want);
    send_key(p, KEY_DOWN, want);
    next_ms[p] = t + 110;
  }
}

/* ---------------------------------------------------- the cheat code combo */
static void cheat_combo(int keycode) {
  static const int seq[] = {102, 103, 104, 105}; /* L1 R1 L2 R2 */
  static int at;
  static int64_t start;
  const int64_t t = now_ms();
  if (at && t - start > 3000)
    at = 0;
  if (keycode == seq[at]) {
    if (!at)
      start = t;
    if (++at == 4) {
      at = 0;
      if (pvz_ui_in_level()) {
        debugPrintf("[input] L R ZL ZR: the cheat code dialog\n");
        pvz_mod_feature(PVZ_FEATURE_CHEAT_CODE_DIALOG, 0, 1);
      }
    }
  } else {
    at = keycode == seq[0] ? 1 : 0;
    if (at)
      start = t;
  }
}

static int keycode_of(int i) {
  int k = k_keys[i].keycode;
  if (dcr_config()->swap_ab && (k == 96 || k == 97))
    k = k == 96 ? 97 : 96;
  return k;
}

static void key_down_extras(int player, int keycode, int *swallow) {
  /* A on an edit field that asked for the keyboard: the keyboard, not the A */
  if (keycode == 96 && pvz_text_press_a()) {
    *swallow = 1;
    return;
  }

  /* EnhanceActivity.onNativeKeyEvent: L1/R1 also tell the mod (seed bank
   * cycling in its touch-era controls), then go on as normal keys. The Xbox
   * scheme's engine makes them gamepad buttons itself (Board::KeyDown): there
   * the mod's copy would move two packets per press. */
  if ((keycode == 102 || keycode == 103) && !dcr_config()->xbox_scheme && pvz_game_ready()) {
    fn_i f = player == 0 ? N.p1_button : N.p2_button;
    if (f)
      f(g_jni_env, jni_class("com/transmension/mobile/EnhanceActivity")->obj,
        keycode == 102 ? 10 : 11 /* GAMEPAD_BUTTON_TL / TR */);
  }

  /* The stop button / pause key with "advanced pause": the mod's own pause. */
  void *ea = jni_class("com/transmension/mobile/EnhanceActivity")->obj;
  if (keycode == 98 && dcr_config()->special_pause && N.gaoji_pause && N.in_game &&
      pvz_game_ready() && N.in_game(g_jni_env, ea)) {
    N.gaoji_pause(g_jni_env, ea, N.gaoji_paused ? !N.gaoji_paused(g_jni_env, ea) : 1);
    *swallow = 1;
  }
}

static void pad_poll(int p) {
  padUpdate(&g_pad[p]);
  log_controller(p);
  if (!padIsConnected(&g_pad[p]))
    return;
  u64 down = padGetButtonsDown(&g_pad[p]), up = padGetButtonsUp(&g_pad[p]);
  static u64 swallowed[NPLAYERS];
  static int sent[NPLAYERS][sizeof k_keys / sizeof k_keys[0]]; /* keycode each went down as */
  for (unsigned i = 0; i < sizeof k_keys / sizeof k_keys[0]; i++) {
    const u64 b = k_keys[i].button;
    const int kc = (up & b) && sent[p][i] ? sent[p][i] : keycode_of((int)i);
    if ((down & b) && p == 0 && pointer_takes(b, 1)) {
      swallowed[p] |= b;
      continue;
    }
    if ((up & b) && p == 0 && (swallowed[p] & b) && pointer_takes(b, 0)) {
      swallowed[p] &= ~b;
      continue;
    }
    if (down & b) {
      int swallow = 0;
      if (p == 0)
        cheat_combo(k_keys[i].keycode);
      key_down_extras(p, kc, &swallow);
      if (swallow)
        swallowed[p] |= b;
      else
        send_key(p, KEY_DOWN, kc);
      sent[p][i] = kc;
    }
    if (up & b) {
      if (swallowed[p] & b)
        swallowed[p] &= ~b;
      else
        send_key(p, KEY_UP, kc);
      sent[p][i] = 0;
    }
  }
  /* Menus: a D-pad direction held repeats, as a key held on a keyboard (and
   * as the right stick does there): lists go on scrolling. */
  static int64_t dpad_next[NPLAYERS][4];
  const u64 held = padGetButtons(&g_pad[p]);
  for (unsigned d = 0; d < 4; d++) {
    const unsigned i = sizeof k_keys / sizeof k_keys[0] - 4 + d; /* the table ends with the D-pad */
    if (!(held & k_keys[i].button) || !sent[p][i] || (swallowed[p] & k_keys[i].button) || pvz_ui_in_level()) {
      dpad_next[p][d] = 0;
      continue;
    }
    const int64_t t = now_ms();
    if (!dpad_next[p][d]) {
      dpad_next[p][d] = t + 380; /* its press moved once already */
    } else if (t >= dpad_next[p][d]) {
      send_key(p, KEY_UP, sent[p][i]);
      send_key(p, KEY_DOWN, sent[p][i]);
      dpad_next[p][d] = t + 110;
    }
  }
  HidAnalogStickState ls = padGetStickPos(&g_pad[p], 0), rs = padGetStickPos(&g_pad[p], 1);
  float v[NAXES] = {(float)ls.x / 32767.0f, -(float)ls.y / 32767.0f, (float)rs.x / 32767.0f,
                    -(float)rs.y / 32767.0f};
  if (p == 0 && pointer_on())
    pointer_move(v[2], v[3]);
  if (p == 0 && P.shown) {
    v[2] = v[3] = 0.0f; /* the right stick is the pointer's */
  } else if (!pvz_ui_in_level()) {
    stick_dpad(p, v[2], v[3]); /* menus: a second D-pad */
    v[2] = v[3] = 0.0f;
  }
  for (int i = 0; i < NAXES; i++) {
    if (fabsf(v[i]) < 0.12f)
      v[i] = 0.0f; /* the range's flat */
    v[i] = roundf(v[i] * 64.0f) / 64.0f;
  }
  if (!g_announced[p] || memcmp(v, g_axis_sent[p], sizeof v) != 0) {
    if (!g_announced[p])
      debugPrintf("[input] player %d controller connected (device %d)\n", p + 1, p + 1);
    g_announced[p] = 1;
    memcpy(g_axis_sent[p], v, sizeof v);
    send_axes(p, v);
  }
}

/* ---------------------------------------------------------------- rumble */
static HidVibrationDeviceHandle g_vib[2];
static int g_vib_ok, g_vib_style;
static u64 g_vib_stop;

void pvz_input_rumble(int kind) {
  if (!dcr_config()->rumble)
    return;
  u32 style = padGetStyleSet(&g_pad[0]);
  HidNpadIdType id = (style & HidNpadStyleTag_NpadHandheld) ? HidNpadIdType_Handheld : HidNpadIdType_No1;
  if (!g_vib_ok || g_vib_style != (int)style) {
    HidNpadStyleTag tag = (style & HidNpadStyleTag_NpadHandheld) ? HidNpadStyleTag_NpadHandheld
                        : (style & HidNpadStyleTag_NpadFullKey)  ? HidNpadStyleTag_NpadFullKey
                                                                 : HidNpadStyleTag_NpadJoyDual;
    g_vib_ok = R_SUCCEEDED(hidInitializeVibrationDevices(g_vib, 2, id, tag));
    g_vib_style = (int)style;
  }
  if (!g_vib_ok)
    return;
  /* EnhanceActivity's HAPITIC_* kinds */
  static const struct { float amp; int ms; } k[] = {
      {0.45f, 70},  /* 0 thump */          {0.90f, 260}, /* 1 explosion */
      {0.40f, 60},  /* 2 bowling */        {0.30f, 60},  /* 3 slot machine */
      {0.50f, 70},  /* 4 whack hit */      {0.20f, 40},  /* 5 whack miss */
      {0.50f, 120}, /* 6 ice trap */       {0.30f, 60},  /* 7 jump */
      {0.35f, 150}, /* 8 rise from grave */{0.35f, 150}, /* 9 rise from pool */
      {0.45f, 90},  /* 10 bungee landing */{0.30f, 90},  /* 11 bungee rising */
      {0.80f, 200}, /* 12 boss hit */
  };
  const int i = (kind >= 0 && kind < (int)(sizeof k / sizeof k[0])) ? kind : 0;
  HidVibrationValue v = {k[i].amp, 160.0f, k[i].amp, 320.0f};
  HidVibrationValue vv[2] = {v, v};
  hidSendVibrationValues(g_vib, vv, 2);
  g_vib_stop = armGetSystemTick() + armNsToTicks((u64)k[i].ms * 1000000ull);
}

static void rumble_poll(void) {
  if (g_vib_stop && armGetSystemTick() >= g_vib_stop) {
    g_vib_stop = 0;
    HidVibrationValue off = {0.0f, 160.0f, 0.0f, 320.0f};
    HidVibrationValue vv[2] = {off, off};
    hidSendVibrationValues(g_vib, vv, 2);
  }
}

/* ------------------------------------------------------------------ poll */
/* While the intro video plays, a button or a touch ends it (and goes no
 * further: the title screen is not under it yet). */
static void video_skip_poll(void) {
  int any = 0;
  for (int p = 0; p < NPLAYERS; p++) {
    padUpdate(&g_pad[p]);
    if (padIsConnected(&g_pad[p]) && padGetButtonsDown(&g_pad[p]) & ~(HidNpadButton_StickLLeft | HidNpadButton_StickLRight |
          HidNpadButton_StickLUp | HidNpadButton_StickLDown | HidNpadButton_StickRLeft | HidNpadButton_StickRRight |
          HidNpadButton_StickRUp | HidNpadButton_StickRDown))
      any = 1;
  }
  HidTouchScreenState ts = {0};
  static int touching;
  const int now = hidGetTouchScreenStates(&ts, 1) && ts.count > 0;
  if (now && !touching)
    any = 1;
  touching = now;
  if (any)
    pvz_video_skip();
}

void pvz_input_poll(void) {
  if (!g_ready)
    return;
  if (pvz_video_playing()) {
    video_skip_poll();
    return;
  }
  touch_poll();
  for (int p = 0; p < NPLAYERS; p++)
    pad_poll(p);
  rumble_poll();
}
