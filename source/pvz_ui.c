/* pvz_ui.c -- fixes to the game's own screens, applied once per frame.
 *
 * Runs on the engine's thread, between frames (eglSwapBuffers, gl_mesa.c):
 * nothing of the widget tree is in use there.
 *
 * HELP SCREENS UNDER ANOTHER SCREEN'S FOCUS. The mod shows its "first visit"
 * help (LawnApp::TryHelpTextScreen: the house / leaderboards, VS, co-op) from
 * the constructor of the screen it explains. LawnApp::ShowHelpTextScreen adds
 * it and gives it the focus -- and then the new screen is added and takes the
 * focus back. The help stays on top, drawn, but the controller's buttons go to
 * the screen under it: B (GameButtonDown 7, which closes a HelpTextScreen)
 * never reaches it, and only the mod's touch "Close" button works (hardware
 * run 9). So: a help screen that is up without the focus gets it, and when it
 * closes, the screen it covered gets the focus back if nothing else took it.
 *
 * THE ZOMBATAR SIGN. The mod turns the main menu's old "Unlock Full Game"
 * button (id 6) into the Zombatar sign, moves it into the extras scene and
 * points all four of its focus links at the back pot (id 15) -- but links no
 * button to it, so the controller can never land on it. So, while the menu is
 * up: the extras-scene button (store, zen garden, almanac, mail, back pot)
 * nearest the sign gets a link to it, in the sign's direction; the sign gets
 * the way back, and in that direction whatever the button used to lead to,
 * so nothing becomes unreachable. The layout is logged once.
 *
 * THE EXTRAS SCENE'S LEFT AND RIGHT. With the mod's rearrangement the links
 * there no longer follow the layout (mail's "right" is the back pot, left of
 * it; the pot has none): left and right in the extras scene follow the
 * buttons' order on screen, among those shown (hardware run 10).
 *
 * THE KEYBOARD'S FIELD. pvz_text.c holds an edit field's keyboard request
 * until A: it learns here, each frame, which widget has the focus. After the
 * Switch keyboard's OK it asks for a Return in the field, pressed here
 * (AndroidAppDriver::InjectKeyEvent: a key down and up, Sexy's KeyCode).
 *
 * Offsets (libGameMain 1.1.5): LawnApp::mWidgetManager +0x294, the app driver
 * +0x2ec (EditWidget::ShowKeyboard),
 * mGameSelector (the MainMenu) +0x8a8 (KillMainMenu), mHelpTextScreen +0x8b4
 * (ShowHelpTextScreen); WidgetManager::mFocusWidget +0xa0,
 * Widget::mWidgetManager +0x18 (WidgetManager::SetFocus); Widget mX mY mWidth
 * mHeight +0x34..+0x40, mWidgetId +0x70, mVisible +0x74, mDisabled +0x76,
 * mParent +0x1c, mFocusedChildWidget +0xdc,
 * mFocusLinks[up, down, left, right] +0xc0 (the mod's Widget.h). MIT.
 */
#include <stdint.h>
#include <string.h>

#include "pvz.h"
#include "so_util.h"
#include "util.h"

extern so_module g_mod_game;

typedef void (*fn_set_focus)(void *wm, void *widget);
typedef void *(*fn_find_widget)(void *container, int id);
typedef void (*fn_inject_key)(void *driver, int key, int flags);

#define APP_WIDGET_MANAGER 0x294
#define APP_DRIVER 0x2ec /* SexyAppBase::mAppDriver, the AndroidAppDriver */
#define APP_HELP_SCREEN 0x8b4
#define WM_FOCUS 0xa0
#define WIDGET_MANAGER 0x18

#define APP_BOARD 0x8a0
#define APP_GAME_SELECTOR 0x8a8
#define W_X 0x34
#define W_Y 0x38
#define W_W 0x3c
#define W_H 0x40
#define W_ID 0x70
#define W_VISIBLE 0x74
#define W_DISABLED 0x76
#define W_LINKS 0xc0
#define W_PARENT 0x1c        /* WidgetContainer::mParent */
#define W_FOCUSED_CHILD 0xdc /* WidgetContainer::mFocusedChildWidget */

#define AT(p, off) (*(void **)((uint8_t *)(p) + (off)))
#define INT(p, off) (*(int *)((uint8_t *)(p) + (off)))
#define BYTE(p, off) (*(uint8_t *)((uint8_t *)(p) + (off)))
#define LINK(w, d) (((void **)((uint8_t *)(w) + W_LINKS))[d])

enum { UP, DOWN, LEFT, RIGHT };
static const int k_opposite[4] = {DOWN, UP, RIGHT, LEFT};
static const char *const k_dir[4] = {"up", "down", "left", "right"};
#define ZOMBATAR_BUTTON 6
static const int k_extras[] = {15, 16, 17, 18, 19}; /* back pot, store, zen, almanac, mail */

static fn_find_widget find_widget;
static volatile int g_in_level;

int pvz_ui_in_level(void) { return g_in_level; }

static int shown(void *w) { return w && BYTE(w, W_VISIBLE) && !BYTE(w, W_DISABLED); }

/* left/right among the extras scene's shown buttons, in screen order */
static void extras_order(void *menu) {
  void *v[8];
  int n = 0;
  for (unsigned i = 0; i < sizeof k_extras / sizeof k_extras[0]; i++) {
    void *w = find_widget(menu, k_extras[i]);
    if (shown(w))
      v[n++] = w;
  }
  for (int i = 1; i < n; i++) /* by x (a handful: insertion sort) */
    for (int j = i; j > 0 && INT(v[j], W_X) < INT(v[j - 1], W_X); j--) {
      void *s = v[j];
      v[j] = v[j - 1];
      v[j - 1] = s;
    }
  for (int i = 0; i < n; i++) {
    void *l = i > 0 ? v[i - 1] : NULL, *r = i + 1 < n ? v[i + 1] : NULL;
    if (LINK(v[i], LEFT) != l)
      LINK(v[i], LEFT) = l;
    if (LINK(v[i], RIGHT) != r)
      LINK(v[i], RIGHT) = r;
  }
}

static int id_of(void *menu, void *w) {
  if (!w)
    return -1;
  for (int id = 0; id <= 21; id++)
    if (find_widget(menu, id) == w)
      return id;
  return -2; /* not one of the menu's buttons */
}

static void dump_menu(void *menu) {
  debugPrintf("[ui] main menu buttons (id: x y w h, links up/down/left/right):\n");
  for (int id = 0; id <= 21; id++) {
    void *w = find_widget(menu, id);
    if (!w)
      continue;
    debugPrintf("[ui]   %2d: %5d %5d %4d %4d %s%s  %d/%d/%d/%d\n", id, INT(w, W_X), INT(w, W_Y),
                INT(w, W_W), INT(w, W_H), BYTE(w, W_VISIBLE) ? "" : "hidden ",
                BYTE(w, W_DISABLED) ? "disabled" : "", id_of(menu, LINK(w, UP)),
                id_of(menu, LINK(w, DOWN)), id_of(menu, LINK(w, LEFT)), id_of(menu, LINK(w, RIGHT)));
  }
}

/* one inserted link: from `from` in direction `dir` to the sign; `old` is
 * where `from` led that way before */
static struct {
  void *menu, *sign, *from, *old;
  int dir;
} Z;

static void zombatar_link(void *app) {
  void *menu = AT(app, APP_GAME_SELECTOR);
  if (!menu || !find_widget) {
    Z.menu = NULL;
    return;
  }
  void *sign = find_widget(menu, ZOMBATAR_BUTTON);
  if (menu != Z.menu || sign != Z.sign) { /* a new menu */
    memset(&Z, 0, sizeof Z);
    Z.menu = menu;
    Z.sign = sign;
    if (sign)
      dump_menu(menu);
  }
  extras_order(menu);
  if (!sign || !BYTE(sign, W_VISIBLE) || BYTE(sign, W_DISABLED))
    return;
  if (Z.from) {
    /* keep it: the mod resets the sign's links whenever the menu syncs */
    if (LINK(Z.from, Z.dir) == sign) {
      LINK(sign, k_opposite[Z.dir]) = Z.from;
      if (Z.old && Z.old != sign)
        LINK(sign, Z.dir) = Z.old;
    }
    return;
  }
  for (unsigned i = 0; i < sizeof k_extras / sizeof k_extras[0]; i++) {
    void *w = find_widget(menu, k_extras[i]);
    for (int d = 0; w && d < 4; d++)
      if (LINK(w, d) == sign)
        return; /* something leads there already */
  }
  const float sx = INT(sign, W_X) + INT(sign, W_W) * 0.5f, sy = INT(sign, W_Y) + INT(sign, W_H) * 0.5f;
  void *best = NULL;
  float best_d = 0, bdx = 0, bdy = 0;
  for (unsigned i = 0; i < sizeof k_extras / sizeof k_extras[0]; i++) {
    void *w = find_widget(menu, k_extras[i]);
    if (!w || !BYTE(w, W_VISIBLE))
      continue;
    const float dx = sx - (INT(w, W_X) + INT(w, W_W) * 0.5f), dy = sy - (INT(w, W_Y) + INT(w, W_H) * 0.5f);
    const float dist = dx * dx + dy * dy;
    if (!best || dist < best_d)
      best = w, best_d = dist, bdx = dx, bdy = dy;
  }
  if (!best)
    return;
  const int dir = (bdx * bdx > bdy * bdy) ? (bdx > 0 ? RIGHT : LEFT) : (bdy > 0 ? DOWN : UP);
  Z.from = best;
  Z.dir = dir;
  Z.old = LINK(best, dir);
  LINK(best, dir) = sign;
  LINK(sign, k_opposite[dir]) = best;
  if (Z.old)
    LINK(sign, dir) = Z.old;
  debugPrintf("[ui] Zombatar sign: reachable by controller from button %d (%s)%s\n",
              id_of(menu, best), k_dir[dir], Z.old ? "; what that led to is next along" : "");
}

/* For pvz_text.c's held keyboard request: the widget with the focus (down
 * the focused children) and its rectangle on the screen; and the Return it
 * asks for. */
static void text_field_focus(void *app, void *wm, fn_inject_key inject_key) {
  void *w = AT(wm, WM_FOCUS);
  for (int i = 0; w && AT(w, W_FOCUSED_CHILD) && i < 16; i++)
    w = AT(w, W_FOCUSED_CHILD);
  int x = 0, y = 0;
  void *p = w;
  for (int i = 0; p && p != wm && i < 16; i++, p = AT(p, W_PARENT))
    x += INT(p, W_X), y += INT(p, W_Y);
  if (pvz_text_frame(w, x, y, w ? INT(w, W_W) : 0, w ? INT(w, W_H) : 0) && inject_key &&
      AT(app, APP_DRIVER))
    inject_key(AT(app, APP_DRIVER), 0x0d /* KEYCODE_RETURN */, 0);
}

void pvz_ui_frame(void) {
  static void **lawn_app;
  static fn_set_focus set_focus;
  static fn_inject_key inject_key;
  static int looked;
  static void *help_seen, *covered;
  if (!looked) {
    looked = 1;
    lawn_app = (void **)so_try_find_addr_rx(&g_mod_game, "gLawnApp");
    set_focus = (fn_set_focus)so_try_find_addr_rx(&g_mod_game,
                                                  "_ZN4Sexy13WidgetManager8SetFocusEPNS_6WidgetE");
    find_widget = (fn_find_widget)so_try_find_addr_rx(&g_mod_game,
                                                      "_ZN4Sexy15WidgetContainer10FindWidgetEi");
    inject_key = (fn_inject_key)so_try_find_addr_rx(&g_mod_game,
                                                    "_ZN4Sexy16AndroidAppDriver14InjectKeyEventEii");
    if (!inject_key)
      debugPrintf("[ui] AndroidAppDriver::InjectKeyEvent missing: the keyboard's OK does not submit\n");
    if (!lawn_app || !set_focus)
      debugPrintf("[ui] engine symbols missing: help-screen focus fix off\n");
  }
  g_in_level = lawn_app && *lawn_app && AT(*lawn_app, APP_BOARD) != NULL;
  if (!lawn_app || !set_focus || !*lawn_app)
    return;
  void *app = *lawn_app, *wm = AT(app, APP_WIDGET_MANAGER), *help = AT(app, APP_HELP_SCREEN);
  if (!wm)
    return;
  text_field_focus(app, wm, inject_key);
  zombatar_link(app);
  if (!help) {
    if (help_seen) {
      /* it closed: the screen it covered, if it is still up and unfocused */
      if (covered && !AT(wm, WM_FOCUS) && AT(covered, WIDGET_MANAGER) == wm) {
        set_focus(wm, covered);
        debugPrintf("[ui] help screen closed: focus back to the screen under it\n");
      }
      help_seen = covered = NULL;
    }
    return;
  }
  if (help == help_seen || AT(help, WIDGET_MANAGER) != wm)
    return; /* dealt with, or not added yet */
  help_seen = help;
  void *focus = AT(wm, WM_FOCUS);
  if (focus == help)
    return;
  covered = focus;
  set_focus(wm, help);
  debugPrintf("[ui] help screen up under another screen's focus: focus moved to it (B closes it)\n");
}
