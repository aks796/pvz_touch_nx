/* dcr_config.c -- <game folder>/config.ini, the user's settings.
 *
 * Written with every option, its default and a line of explanation on the
 * first start; an existing file is appended to (options a newer build adds,
 * at the end, with their defaults), so edits and comments survive updates.
 * Plain INI: [section], key = value, # comments; booleans take true/false,
 * yes/no, on/off, 1/0. Read once at start-up: changes apply the next time the
 * game starts. (The machinery is the Crossy Road port's; the options are
 * PvZ's.)
 *
 * [game] is the mod's own settings screen (Homura; the APK's
 * assets/defaultSetting.xml gives the defaults used here), [cheats] its cheat
 * menu (Preferences.Changes feature ids, see Main.cpp of the mod). MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <switch.h>

#include "dcr_build.h"
#include "dcr_config.h"
#include "util.h"

const char *dcr_game_root(void);        /* main.c */
void dcr_window_set_size(int w, int h); /* android_ndk.c */

static DcrConfig g_cfg = {
    .english = 1,
    .homura = {[HS_XBOX_PAUSE] = 1, [HS_NORMAL_LEVEL] = 1, [HS_CLASSIC_SHOVEL] = 1,
               [HS_IMITATER_GREY] = 1, [HS_SHOW_HOUSE] = 1, [HS_NEW_COB_CANNON] = 1,
               [HS_AUTO_FIX_POS] = 1, [HS_PIN_SEED_BANK] = 1, [HS_DYNAMIC_PREVIEW] = 1,
               [HS_PLAY_VIDEO] = 1},
    .second_touch = 1,
    .rumble = 1,
    .res_w = 1280,
    .res_h = 720,
    .boost = 1,
    .load_mod = 1,
};

const DcrConfig *dcr_config(void) { return &g_cfg; }

enum { K_BOOL, K_CHOICE };

typedef struct {
  const char *section, *key, *def, *help;
  int kind;
  const char *choices; /* K_CHOICE: comma-separated, index = value */
  int homura;          /* >= 0: an HS_ setting */
  int cheat;           /* > 0: a Preferences.Changes feature id */
} Opt;

#define H(i) i, 0
#define C(id) -1, id
#define N -1, 0

static const Opt k_opts[] = {
    {"game", "language", "english",
     "english or chinese. The game is Chinese. english: the English text; and with\n"
     "# the English APK in this folder too (your copy of PvZTouch 4.0.5, any file\n"
     "# name) the English pictures, menu signs and fonts too. Without it the\n"
     "# pictures stay Chinese and some TV-only lines too. Made on the next start.",
     K_CHOICE, "english,chinese", N},
    /* ---- the mod's settings screen (its English labels in quotes) ---- */
    {"game", "xbox_pause_menu", "true", "\"Xbox Pause Menu\": the Xbox version's pause menu, with more options.",
     K_BOOL, NULL, H(HS_XBOX_PAUSE)},
    {"game", "classic_shovel", "true",
     "\"Use Original Shovel\": the shovel of the other versions, which can dig up a\n"
     "# Pumpkin on its own (the TV shovel can't).",
     K_BOOL, NULL, H(HS_CLASSIC_SHOVEL)},
    {"game", "original_adventure_difficulty", "true",
     "\"Original Difficulty\": Adventure as in the other versions, with the full\n"
     "# first and second playthrough. Off: adventure_difficulty below.",
     K_BOOL, NULL, H(HS_NORMAL_LEVEL)},
    {"game", "adventure_difficulty", "tv_touch",
     "Used only when original_adventure_difficulty = false (the mod's \"Custom\n"
     "# Difficulty\"; every level can be won without losing a lawn mower):\n"
     "#   tv_touch      extremely difficult: the mod's, by hhad8 and YC_Xiaoxuan\n"
     "#   transmension  difficult: the TV edition's own\n"
     "#   toumai        very difficult: by Japonisme Toumai (20 levels from 3-10)\n"
     "#   custom        your own file: data/files/properties/levels.xml",
     K_CHOICE, "tv_touch,transmension,toumai,custom", N},
    {"game", "grey_imitater_plants", "true", "\"Gray Imitater Plants\": Imitater copies drawn grey.",
     K_BOOL, NULL, H(HS_IMITATER_GREY)},
    {"game", "show_house_on_main_menu", "true", "\"Show House on Main Menu\".", K_BOOL, NULL,
     H(HS_SHOW_HOUSE)},
    {"game", "mobile_cob_cannon_target", "true",
     "\"Use Cannon Target of PvZ on Mobile Device\": the phone version's Cob\n"
     "# Cannon target instead of the TV one.",
     K_BOOL, NULL, H(HS_NEW_COB_CANNON)},
    {"game", "auto_center_cursor", "false",
     "\"Auto-centering Cursor\": the cursor keeps being pulled to the centre of\n"
     "# its lawn square -- even while the stick moves it, which makes the stick\n"
     "# feel sticky. Made for touch; off suits a controller.",
     K_BOOL, NULL, H(HS_AUTO_FIX_POS)},
    {"game", "seed_bank_on_top", "true",
     "\"Set Seed Bank Layer to Topmost\": nothing is drawn over the seed bank.", K_BOOL, NULL,
     H(HS_PIN_SEED_BANK)},
    {"game", "dynamic_planting_preview", "true",
     "\"Display Dynamic Planting Preview\": an animated plant under the cursor\n"
     "# where it will go.",
     K_BOOL, NULL, H(HS_DYNAMIC_PREVIEW)},
    {"game", "show_seed_cooldown", "false", "\"Show CD of Seed Packets\": their recharge time.",
     K_BOOL, NULL, H(HS_SHOW_COOLDOWN)},
    {"game", "manual_collect", "false",
     "\"Disable Auto-collection of Sunlight and Coins\": collect them with the\n"
     "# cursor (the TV edition collects them itself). Not the Zen Garden. With\n"
     "# the controller scheme, offline (VS too), they are collected the Xbox 360\n"
     "# edition's way whatever this says: the cursor passing near, and ZL / ZR\n"
     "# held draw every one on the lawn to the cursor (in VS the plants' sun, the\n"
     "# zombies' brains). Online the game collects them itself.",
     K_BOOL, NULL, H(HS_MANUAL_COLLECT)},
    {"game", "disable_item_bar", "false",
     "\"Disable Special Item Bar\": the TV edition's bar of special items, which\n"
     "# the mod makes free and unlimited. True removes it.",
     K_BOOL, NULL, H(HS_DISABLE_SHOP)},
    {"game", "hide_grass_and_poles", "false",
     "\"Hide Grass and Pole\": the foreground grass and power poles.", K_BOOL, NULL,
     H(HS_HIDE_COVER)},
    {"game", "no_trash_can_zombies", "false",
     "\"Disable Trash Bin Zombie\": the TV edition adds Trash Can Zombies to\n"
     "# Survival levels; without them the waves are nearly those of the PC.",
     K_BOOL, NULL, H(HS_NO_TRASH_BIN)},
    {"game", "xbox_music", "false",
     "\"Xbox BGMs\": the Xbox version's music, with the PC's drum variations (about\n"
     "# 0.2 s late). Off: the TV edition's music.",
     K_BOOL, NULL, H(HS_XBOX_MUSIC)},
    {"game", "skip_logo", "false", "Skip the PopCap logo animation at start-up.", K_BOOL, NULL,
     H(HS_SKIP_LOGO)},
    {"game", "button_pictures", "true",
     "The Switch's buttons drawn in the game's text (a layer the English layer\n"
     "# adds to each English font). False: the text names them (A, +, ZL...).",
     K_BOOL, NULL, N},
    {"game", "touch_logo", "true",
     "The Plants vs. Zombies Touch logo on the title screen and main menu.\n"
     "# False: the game's own.",
     K_BOOL, NULL, N},
    {"game", "intro_video", "true",
     "The intro video at start-up (PopCap's logo and the zombie hand, from\n"
     "# the game APK's movies/intro.mp4). Any button skips it.",
     K_BOOL, NULL, H(HS_PLAY_VIDEO)},
    {"game", "advanced_pause", "false",
     "\"Enable Advanced Pause\": + pauses the game but you can still plant, dig\n"
     "# and fire Cob Cannons. Off: the game's own pause.",
     K_BOOL, NULL, N},
    /* ---- the mod's cheat menu ---- */
    {"cheats", "unlimited_sun", "false",
     "The mod's cheat menu. The game has all of it too: pause menu > Help &\n"
     "# Options > Mod Menu, whose choices (kept in data/files/modmenu.txt) win\n"
     "# over these.",
     K_BOOL, NULL, C(1)},
    {"cheats", "place_anywhere", "false", "Plants can go anywhere (water, roof, on top of others).",
     K_BOOL, NULL, C(42)},
    {"cheats", "seeds_no_recharge", "false", NULL, K_BOOL, NULL, C(2)},
    {"cheats", "plants_reload_instantly", "false", NULL, K_BOOL, NULL, C(3)},
    {"cheats", "mushrooms_awake", "false", "Mushrooms awake in daytime (no Coffee Bean needed).",
     K_BOOL, NULL, C(4)},
    {"cheats", "game_speed", "off", "off, 1.2, 1.5, 2, 2.5, 3, 5 or 10 (times normal speed).",
     K_CHOICE, "off,1.2,1.5,2,2.5,3,5,10", C(8)},
    {"cheats", "show_plant_health", "false", NULL, K_BOOL, NULL, C(21)},
    {"cheats", "show_zombie_health", "false", NULL, K_BOOL, NULL, C(23)},
    {"cheats", "show_armor_health", "false", "Health of helmets and shields.", K_BOOL, NULL, C(24)},
    {"cheats", "clear_fog", "false", NULL, K_BOOL, NULL, C(6)},
    {"cheats", "transparent_vases", "false", "See inside Vasebreaker vases.", K_BOOL, NULL, C(43)},
    {"cheats", "zombies_cannot_enter_house", "false", NULL, K_BOOL, NULL, C(45)},
    {"cheats", "no_lawn_mowers", "false", NULL, K_BOOL, NULL, C(88)},
    /* ---- the Switch side ---- */
    {"controls", "swap_a_b", "false",
     "Swap A and B (true: the bottom button selects, the right one goes back).", K_BOOL, NULL, N},
    {"controls", "scheme", "xbox",
     "xbox: the Xbox 360 edition's controls (the engine's own gamepad mode):\n"
     "# A plants and collects, hold B to dig, L / R pick a seed packet, ZL / ZR\n"
     "# (the triggers) switch the sun magnet (auto-collect; shown top left), Y is\n"
     "# fast forward (2x, the button right of the progress meter), - opens the\n"
     "# special item bar (hidden otherwise), + the pause menu (Help & Options\n"
     "# there: Cheats, Mod Menu).\n"
     "# tv: the Android TV remote scheme (A opens a seed picker).",
     K_CHOICE, "xbox,tv", N},
    {"controls", "pointer", "true",
     "Click the right stick for a pointer: the right stick moves it and ZR taps\n"
     "# with it, for the screens made for touch (the Zombatar editor, the VS\n"
     "# lobbies). Click again, or leave it a few seconds, to put it away.\n"
     "# Otherwise the right stick scrolls and moves through menus.",
     K_BOOL, NULL, N},
    {"controls", "rumble", "true",
     "Rumble for explosions, the whack-a-zombie mallet and the like (the mod's\n"
     "# phone vibration).",
     K_BOOL, NULL, N},
    {"touch", "second_finger_on_lawn", "true",
     "A second finger on the lawn works as its own cursor (the mod's two-finger\n"
     "# play: plant with one hand, collect or shovel with the other).",
     K_BOOL, NULL, N},
    {"display", "resolution", "720",
     "Rendering resolution: 720, 1080 or auto (1080 if docked when the game\n"
     "# starts). The Switch scales the picture to the screen either way; the game's\n"
     "# art is made for 720.",
     K_CHOICE, NULL, N},
    {"performance", "boost_cpu_when_loading", "true",
     "CPU at 1785 MHz while the game starts (until its first picture) and\n"
     "# inside loading frames (those over 50 ms), normal otherwise.",
     K_BOOL, NULL, N},
    {"debug", "gl_selftest", "false", "Graphics self-test picture at start-up.", K_BOOL, NULL, N},
    {"debug", "boot_log_on_screen", "false",
     "Show the start-up log on screen at every launch. Off: the log appears only\n"
     "# while something is being set up (first launch, a new APK or NRO).",
     K_BOOL, NULL, N},
    {"debug", "log_java_calls", "false",
     "Write every Java method the game calls to debug.log (slow; for bug reports).", K_BOOL,
     NULL, N},
    {"debug", "profile_startup_seconds", "90",
     "For this test build: every thread sampled for this many seconds from the\n"
     "# first picture, with a report in debug.log every 10 s (where the loading\n"
     "# time goes). 0 turns it off.",
     K_CHOICE, NULL, N},
    {"debug", "load_touch_mod", "true",
     "false: start the plain TV edition, without the Touch mod (libHomura), its\n"
     "# settings and its cheats. Only for telling a problem in the mod from one in\n"
     "# the game itself; leave it true to play.",
     K_BOOL, NULL, N},
    {"debug", "mod_from_apk", "false",
     "true: the mod exactly as the game APK has it, instead of this port's build of\n"
     "# the same mod with the Switch controls (the built-in mod menu, the Cheats\n"
     "# button, controller support in the Zombatar and VS screens).",
     K_BOOL, NULL, N},
    {"config", "version", "1", "Settings file format; leave as it is.", K_CHOICE, NULL, N},
};
#define O_COUNT ((int)(sizeof k_opts / sizeof k_opts[0]))

static char g_val[O_COUNT][24];
static int g_have[O_COUNT];

static int opt_index(const char *section, const char *key) {
  for (int i = 0; i < O_COUNT; i++)
    if (!strcmp(k_opts[i].section, section) && !strcmp(k_opts[i].key, key))
      return i;
  return -1;
}

static void path_of(char *out, size_t cap, const char *name) {
  snprintf(out, cap, "%s/%s", dcr_game_root(), name);
}

static char *trim(char *s) {
  while (*s == ' ' || *s == '\t')
    s++;
  char *e = s + strlen(s);
  while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n'))
    *--e = 0;
  return s;
}

static void parse(FILE *f) {
  char line[256], section[32] = "";
  while (fgets(line, sizeof line, f)) {
    char *s = trim(line);
    if (!*s || *s == '#' || *s == ';')
      continue;
    if (*s == '[') {
      char *e = strchr(s, ']');
      if (e) {
        *e = 0;
        snprintf(section, sizeof section, "%s", trim(s + 1));
      }
      continue;
    }
    char *eq = strchr(s, '=');
    if (!eq)
      continue;
    *eq = 0;
    char *key = trim(s), *val = trim(eq + 1);
    char *hash = strpbrk(val, "#;");
    if (hash) {
      *hash = 0;
      val = trim(val);
    }
    for (int i = 0; i < O_COUNT; i++)
      if (!strcasecmp(section, k_opts[i].section) && !strcasecmp(key, k_opts[i].key)) {
        snprintf(g_val[i], sizeof g_val[i], "%s", val);
        g_have[i] = 1;
      }
  }
}

static void write_opts(FILE *f, int only_missing) {
  const char *last = NULL;
  for (int i = 0; i < O_COUNT; i++) {
    if (only_missing && g_have[i])
      continue;
    if (!last || strcmp(last, k_opts[i].section)) {
      fprintf(f, "\n[%s]\n", k_opts[i].section);
      if (!strcmp(k_opts[i].section, "cheats"))
        fputs("# The mod's cheat menu. They change the game as it plays; saves made with\n"
              "# them on keep what they got.\n",
              f);
    }
    last = k_opts[i].section;
    if (k_opts[i].help)
      fprintf(f, "# %s\n", k_opts[i].help);
    fprintf(f, "%s = %s\n", k_opts[i].key, g_val[i]);
  }
}

static int as_bool(int i) {
  const char *v = g_val[i];
  if (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || !strcasecmp(v, "on") || !strcmp(v, "1"))
    return 1;
  if (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcasecmp(v, "off") || !strcmp(v, "0"))
    return 0;
  debugPrintf("[config] %s = %s: not true/false, using %s\n", k_opts[i].key, v, k_opts[i].def);
  return !strcmp(k_opts[i].def, "true");
}

/* index of the value in the option's choice list, 0 (the first) if unknown */
static int as_choice(int i) {
  const char *v = g_val[i];
  const char *c = k_opts[i].choices;
  for (int idx = 0; c && *c; idx++) {
    const char *e = strchr(c, ',');
    size_t n = e ? (size_t)(e - c) : strlen(c);
    if (strlen(v) == n && !strncasecmp(v, c, n))
      return idx;
    if (!e)
      break;
    c = e + 1;
  }
  if (strcasecmp(v, k_opts[i].def))
    debugPrintf("[config] %s = %s: not one of %s, using %s\n", k_opts[i].key, v,
                k_opts[i].choices, k_opts[i].def);
  return 0;
}

void dcr_config_load(void) {
  for (int i = 0; i < O_COUNT; i++)
    snprintf(g_val[i], sizeof g_val[i], "%s", k_opts[i].def);
  char path[300];
  path_of(path, sizeof path, "config.ini");
  FILE *f = fopen(path, "r");
  if (f) {
    parse(f);
    fclose(f);
    int missing = 0;
    for (int i = 0; i < O_COUNT; i++)
      missing += !g_have[i];
    if (missing && (f = fopen(path, "a"))) {
      fprintf(f, "\n# Added by build %llu (new options, at their defaults):\n",
              (unsigned long long)DCR_BUILD);
      write_opts(f, 1);
      fclose(f);
      debugPrintf("[config] added %d new option%s to config.ini\n", missing, missing > 1 ? "s" : "");
    }
  } else if ((f = fopen(path, "w"))) {
    fputs("# Plants vs. Zombies Touch for Switch -- settings.\n"
          "# Changes apply the next time the game starts. Delete this file to get\n"
          "# the defaults back.\n",
          f);
    write_opts(f, 0);
    fclose(f);
    debugPrintf("[config] wrote config.ini with the defaults\n");
  }

  g_cfg.ncheats = 0;
  for (int i = 0; i < O_COUNT; i++) {
    const Opt *o = &k_opts[i];
    if (o->homura >= 0)
      g_cfg.homura[o->homura] = as_bool(i);
    if (o->cheat > 0 && g_cfg.ncheats < (int)(sizeof g_cfg.cheats / sizeof g_cfg.cheats[0])) {
      int v = o->kind == K_CHOICE ? as_choice(i) : as_bool(i);
      if (v)
        g_cfg.cheats[g_cfg.ncheats++] = (CheatSetting){o->cheat, v, o->kind == K_CHOICE};
    }
  }
  g_cfg.english = as_choice(opt_index("game", "language")) == 0;
  g_cfg.touch_logo = as_bool(opt_index("game", "touch_logo"));
  g_cfg.button_pictures = as_bool(opt_index("game", "button_pictures"));
  g_cfg.special_pause = as_bool(opt_index("game", "advanced_pause"));
  g_cfg.adventure_levels = as_choice(opt_index("game", "adventure_difficulty"));
  g_cfg.swap_ab = as_bool(opt_index("controls", "swap_a_b"));
  g_cfg.rumble = as_bool(opt_index("controls", "rumble"));
  g_cfg.pointer = as_bool(opt_index("controls", "pointer"));
  g_cfg.mod_from_apk = as_bool(opt_index("debug", "mod_from_apk"));
  g_cfg.xbox_scheme = as_choice(opt_index("controls", "scheme")) == 0;
  g_cfg.second_touch = as_bool(opt_index("touch", "second_finger_on_lawn"));
  g_cfg.boost = as_bool(opt_index("performance", "boost_cpu_when_loading"));
  g_cfg.gl_selftest = as_bool(opt_index("debug", "gl_selftest"));
  g_cfg.boot_log = as_bool(opt_index("debug", "boot_log_on_screen"));
  g_cfg.log_jni = as_bool(opt_index("debug", "log_java_calls"));
  g_cfg.load_mod = as_bool(opt_index("debug", "load_touch_mod"));
  g_cfg.profile_secs = atoi(g_val[opt_index("debug", "profile_startup_seconds")]);
  if (g_cfg.profile_secs < 0 || g_cfg.profile_secs > 3600)
    g_cfg.profile_secs = 0;

  const char *r = g_val[opt_index("display", "resolution")];
  int docked = appletGetOperationMode() == AppletOperationMode_Console;
  int h = !strcmp(r, "720") ? 720 : !strcmp(r, "1080") ? 1080 : !strcasecmp(r, "auto") ? (docked ? 1080 : 720) : 0;
  if (!h) {
    debugPrintf("[config] resolution = %s: not 720, 1080 or auto, using 720\n", r);
    h = 720;
  }
  g_cfg.res_h = h;
  g_cfg.res_w = h * 16 / 9;
  dcr_window_set_size(g_cfg.res_w, g_cfg.res_h);

  char on[512];
  int n = 0;
  for (int i = 0; i < O_COUNT && n < (int)sizeof on - 48; i++)
    if (k_opts[i].homura >= 0 && g_cfg.homura[k_opts[i].homura])
      n += snprintf(on + n, sizeof on - n, " %s", k_opts[i].key);
  debugPrintf("[config] game settings on:%s\n", n ? on : " (none)");
  n = 0;
  on[0] = 0;
  for (int i = 0; i < g_cfg.ncheats && n < (int)sizeof on - 32; i++)
    n += snprintf(on + n, sizeof on - n, " %d=%d", g_cfg.cheats[i].id, g_cfg.cheats[i].value);
  debugPrintf("[config] %s; cheats:%s; %dx%d (%s, %s), A/B %s, second finger %s, CPU boost %s\n",
              g_cfg.english ? "English" : "Chinese", g_cfg.ncheats ? on : " none", g_cfg.res_w,
              g_cfg.res_h, r,
              docked ? "docked" : "handheld", g_cfg.swap_ab ? "swapped" : "normal",
              g_cfg.second_touch ? "on" : "off", g_cfg.boost ? "on" : "off");
}
