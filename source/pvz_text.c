/* pvz_text.c -- text entry (the player's name, cheat codes) on the Switch keyboard.
 *
 * The game asks for text two ways, both through its Java view:
 *   NativeView.showTextInputDialog(mode, title, hint, initial)   the engine
 *       (AndroidTextInputManager: an AlertDialog with an EditText; OK ->
 *       NativeView.onTextInputNative(handle, text), Back/cancel -> (..., null))
 *   NativeView.showTextInputDialog2(mode, title, hint, initial)  the mod
 *       (OK -> NativeView.onTextInputNative2(text), cancel -> nothing)
 * and an edit field (the cheat code dialog's) asks for the soft keyboard:
 *   NativeView.showIme(flags)   the typing goes to its hidden EditText, whose
 *       changes reach the field as onTextChangedNative(handle, text, selection
 *       start, end, time); Android's keyboard's Done then sends Enter, which
 *       submits it (EditWidget: Return -> EditListener::EditWidgetText; the
 *       new player dialog takes it as its OK).
 * Here both open the system keyboard applet. The request arrives on the UI
 * thread inside libnative_code's work callback; the keyboard is shown from the
 * UI thread loop right after (pvz_text_input_poll), not inside the callback.
 *
 * An edit field's request is held, though: the engine asks whenever the field
 * gets the focus -- as its dialog opens (the cheat code dialog puts the focus
 * back on the field each time it gets it) and whenever the controller moves
 * onto it -- and the Switch keyboard takes the whole screen. So it comes up
 * when A is pressed while the field has the focus -- between the engine's
 * showIme and hideIme (the field lost it), which is all that tells (hardware
 * run 13: comparing the focused widget as well never matched) -- and A there
 * is the keyboard's, not the game's, as often as it is pressed. A tap inside
 * the focused widget's rectangle (followed on the engine's thread between
 * frames: pvz_text_frame, from pvz_ui_frame) opens it too.
 *
 * The Switch keyboard's OK is that Done: the text, then Return in the field
 * (AndroidAppDriver::InjectKeyEvent, on the engine's thread: pvz_ui.c), as the
 * engine does itself with a dialog's answer (AndroidAppDriver::
 * HandleInputEvents). Without it the new player's name stayed in the field,
 * and A there only brought the keyboard back (tester, 2026-09-30).
 * mode is the Java one: 1 password, 2 URI, 3 e-mail, 4 visible password,
 * else plain text. MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "jni.h"
#include "pvz.h"
#include "rt_applet.h"
#include "util.h"

static Mutex g_lock;
static struct {
  int pending, mod, mode;
  char title[128], hint[128], initial[256];
} R;

/* the edit field's request, held until A (see the top) */
static struct {
  volatile int held;     /* R has it, not yet shown */
  volatile int capture;  /* the field is the focus the next frame finds */
  void *field;           /* that widget (engine thread only) */
  volatile int on_field; /* it still has the focus */
  volatile int x, y, w, h; /* its rectangle, in game pixels */
  volatile int submit;     /* frames until Return in it (the keyboard's OK) */
} H;

/* frames between the text and the Return: the text event is the engine's
 * next frame's, the Return comes after it */
#define SUBMIT_FRAMES 3

void pvz_text_input_request(int mod, int mode, const char *title, const char *hint,
                            const char *initial) {
  mutexLock(&g_lock);
  if (mod == PVZ_TEXT_IME) {
    R.pending = 0;
    H.on_field = 0;
    H.capture = 1;
    H.held = 1;
  } else {
    H.held = 0;
    R.pending = 1;
  }
  R.mod = mod;
  R.mode = mode;
  snprintf(R.title, sizeof R.title, "%s", title ? title : "");
  snprintf(R.hint, sizeof R.hint, "%s", hint ? hint : "");
  snprintf(R.initial, sizeof R.initial, "%s", initial ? initial : "");
  mutexUnlock(&g_lock);
  debugPrintf("[text] %s(mode %d, \"%s\", hint \"%s\", \"%s\")%s\n",
              mod == PVZ_TEXT_IME ? "showIme" : mod ? "showTextInputDialog2" : "showTextInputDialog", mode,
              R.title, R.hint, R.initial, mod == PVZ_TEXT_IME ? ": the keyboard on A (or a tap) on the field" : "");
}

void pvz_text_ime_hidden(void) {
  mutexLock(&g_lock);
  H.held = 0;
  mutexUnlock(&g_lock);
}

int pvz_text_frame(void *focus, int x, int y, int w, int h) {
  int ret = 0;
  if (H.submit > 0 && --H.submit == 0) {
    ret = focus != NULL && focus == H.field;
    debugPrintf("[text] %s\n", ret ? "Return in the field" : "the field lost the focus: no Return");
  }
  if (!H.held)
    return ret;
  if (H.capture) {
    H.field = focus;
    H.capture = 0;
  }
  H.x = x, H.y = y, H.w = w, H.h = h;
  H.on_field = focus != NULL && focus == H.field;
  return ret;
}

/* the held request, shown now (and again on the next A, until hideIme); not
 * while the last text is still going in (the A is the field's all the same) */
static int show_held(void) {
  mutexLock(&g_lock);
  const int held = H.held, go = held && H.submit == 0;
  if (go)
    R.pending = 1;
  mutexUnlock(&g_lock);
  if (go)
    debugPrintf("[text] the keyboard for the field\n");
  return held;
}

int pvz_text_press_a(void) { return show_held(); }

void pvz_text_touch(float x, float y) {
  if (H.held && H.on_field && H.w > 0 && x >= H.x && x < H.x + H.w && y >= H.y && y < H.y + H.h)
    show_held();
}

typedef void (*fn_text)(void *env, void *view, jlong h, void *str);
typedef void (*fn_text2)(void *env, void *view, void *str);
typedef void (*fn_changed)(void *env, void *view, jlong h, void *str, jint start, jint end, jlong time);

/* UTF-16 length of UTF-8 text (the selection is in Java chars) */
static jint utf16_len(const char *s) {
  jint n = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++)
    if ((*p & 0xC0) != 0x80)
      n += *p >= 0xF0 ? 2 : 1;
  return n;
}

static void reply(int mod, const char *text) {
  if (mod == PVZ_TEXT_IME) { /* the field's text, then Return in it (the top) */
    if (!text)
      return;
    fn_changed f = (fn_changed)pvz_native("Java_com_transmension_mobile_NativeView_onTextChangedNative");
    JObj *s = jni_str(text);
    const jint end = utf16_len(text);
    if (f)
      f(g_jni_env, g_view, pvz_native_handle(), s, end, end,
        (jlong)(armTicksToNs(armGetSystemTick()) / 1000000ull));
    jni_release(s);
    if (f)
      H.submit = SUBMIT_FRAMES;
    return;
  }
  JObj *s = text ? jni_str(text) : NULL;
  if (mod == 0) {
    fn_text f = (fn_text)pvz_native("Java_com_transmension_mobile_NativeView_onTextInputNative");
    if (f)
      f(g_jni_env, g_view, pvz_native_handle(), s);
  } else if (text) {
    fn_text2 f = (fn_text2)pvz_native("Java_com_transmension_mobile_NativeView_onTextInputNative2");
    if (f)
      f(g_jni_env, g_view, s);
  }
  jni_release(s);
}

void pvz_text_input_poll(void) {
  if (!R.pending)
    return;
  mutexLock(&g_lock);
  int mod = R.mod, mode = R.mode;
  char title[128], hint[128], initial[256];
  memcpy(title, R.title, sizeof title);
  memcpy(hint, R.hint, sizeof hint);
  memcpy(initial, R.initial, sizeof initial);
  R.pending = 0;
  mutexUnlock(&g_lock);

  SwkbdConfig kbd;
  Result rc = swkbdCreate(&kbd, 0);
  if (R_FAILED(rc)) {
    debugPrintf("[text] keyboard unavailable (0x%x): answering with \"%s\"\n", rc,
                initial[0] ? initial : "Player");
    reply(mod, initial[0] ? initial : "Player");
    return;
  }
  /* AndroidTextInputManager: mode 1 is a password (input type 129, masked),
   * mode 4 visible text (145); both accept ASCII letters and digits only. */
  int alnum = mode == 1 || mode == 4;
  if (mode == 1) {
    swkbdConfigMakePresetPassword(&kbd);
  } else {
    swkbdConfigMakePresetDefault(&kbd);
    if (alnum)
      swkbdConfigSetType(&kbd, SwkbdType_QWERTY);
  }
  if (title[0])
    swkbdConfigSetHeaderText(&kbd, title);
  if (hint[0])
    swkbdConfigSetGuideText(&kbd, hint);
  if (initial[0])
    swkbdConfigSetInitialText(&kbd, initial);
  swkbdConfigSetStringLenMax(&kbd, 64);
  char out[256] = {0};
  dcr_applet_busy(1); /* no frames while it is up, and nothing wrong (watchdog, boost) */
  rc = swkbdShow(&kbd, out, sizeof out);
  dcr_applet_busy(0);
  swkbdClose(&kbd);
  if (R_SUCCEEDED(rc)) {
    if (alnum) { /* the Java InputFilter: anything else is refused */
      char *w = out;
      for (const char *c = out; *c; c++)
        if ((*c >= '0' && *c <= '9') || (*c >= 'A' && *c <= 'Z') || (*c >= 'a' && *c <= 'z'))
          *w++ = *c;
      *w = 0;
    }
    debugPrintf("[text] entered \"%s\"\n", mode == 1 ? "(password)" : out);
    reply(mod, out);
  } else {
    debugPrintf("[text] cancelled (0x%x)\n", rc);
    reply(mod, NULL);
  }
}
