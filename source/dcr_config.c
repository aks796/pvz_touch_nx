/* dcr_config.c -- <game folder>/config.ini, the player's settings: PvZ's
 * options, on the runtime's INI engine (runtime/source/rt_cfg.c).
 *
 * Written with every option, its default and a line of explanation on the
 * first start; an existing file is appended to (options a newer build adds,
 * at the end, with their defaults), so edits and comments survive updates.
 * Plain INI: [section], key = value, # comments; booleans take true/false,
 * yes/no, on/off, 1/0. Read once at start-up: changes apply the next time the
 * game starts.
 *
 * [game] is the mod's own settings screen (Homura; the APK's
 * assets/defaultSetting.xml gives the defaults used here), [cheats] its cheat
 * menu (Preferences.Changes feature ids, see Main.cpp of the mod): a row's
 * tag says which (TAG_HS + an HS_ index, or a cheat's feature id). MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "dcr_config.h"
#include "rt_cfg.h"
#include "util.h"

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

/* A row's tag: a cheat's Preferences.Changes feature id (1..), or TAG_HS + an
 * HS_ setting of the mod's settings screen. */
#define TAG_HS 1000
#define H(i) &g_cfg.homura[i], 0, 0, 0, TAG_HS + (i)
#define C(id) NULL, 0, 0, 0, (id)
#define N NULL, 0, 0, 0, 0

static const CfgOpt k_opts[] = {
    {"game", "language", "english",
     "english or chinese. The game is Chinese. english: the English text,\n"
     "# pictures, menu signs and fonts, from the English files the NRO carries\n"
     "# (copied here as \"PvZ Touch English.apk\"; an English APK of your own in\n"
     "# this folder is used instead). Without either, the pictures stay Chinese\n"
     "# and some TV-only lines too. Made on the next start.",
     CFG_CHOICE, "english,chinese", N},
    /* ---- the mod's settings screen (its English labels in quotes) ---- */
    {"game", "xbox_pause_menu", "true", "\"Xbox Pause Menu\": the Xbox version's pause menu, with more options.",
     CFG_BOOL, NULL, H(HS_XBOX_PAUSE)},
    {"game", "classic_shovel", "true",
     "\"Use Original Shovel\": the shovel of the other versions, which can dig up a\n"
     "# Pumpkin on its own (the TV shovel can't).",
     CFG_BOOL, NULL, H(HS_CLASSIC_SHOVEL)},
    {"game", "original_adventure_difficulty", "true",
     "\"Original Difficulty\": Adventure as in the other versions, with the full\n"
     "# first and second playthrough. Off: adventure_difficulty below.",
     CFG_BOOL, NULL, H(HS_NORMAL_LEVEL)},
    {"game", "adventure_difficulty", "tv_touch",
     "Used only when original_adventure_difficulty = false (the mod's \"Custom\n"
     "# Difficulty\"; every level can be won without losing a lawn mower):\n"
     "#   tv_touch      extremely difficult: the mod's, by hhad8 and YC_Xiaoxuan\n"
     "#   transmension  difficult: the TV edition's own\n"
     "#   toumai        very difficult: by Japonisme Toumai (20 levels from 3-10)\n"
     "#   custom        your own file: data/files/properties/levels.xml",
     CFG_CHOICE, "tv_touch,transmension,toumai,custom", N},
    {"game", "grey_imitater_plants", "true", "\"Gray Imitater Plants\": Imitater copies drawn grey.",
     CFG_BOOL, NULL, H(HS_IMITATER_GREY)},
    {"game", "show_house_on_main_menu", "true", "\"Show House on Main Menu\".", CFG_BOOL, NULL,
     H(HS_SHOW_HOUSE)},
    {"game", "mobile_cob_cannon_target", "true",
     "\"Use Cannon Target of PvZ on Mobile Device\": the phone version's Cob\n"
     "# Cannon target instead of the TV one.",
     CFG_BOOL, NULL, H(HS_NEW_COB_CANNON)},
    {"game", "auto_center_cursor", "false",
     "\"Auto-centering Cursor\": the cursor keeps being pulled to the centre of\n"
     "# its lawn square -- even while the stick moves it, which makes the stick\n"
     "# feel sticky. Made for touch; off suits a controller.",
     CFG_BOOL, NULL, H(HS_AUTO_FIX_POS)},
    {"game", "seed_bank_on_top", "true",
     "\"Set Seed Bank Layer to Topmost\": nothing is drawn over the seed bank.", CFG_BOOL, NULL,
     H(HS_PIN_SEED_BANK)},
    {"game", "dynamic_planting_preview", "true",
     "\"Display Dynamic Planting Preview\": an animated plant under the cursor\n"
     "# where it will go.",
     CFG_BOOL, NULL, H(HS_DYNAMIC_PREVIEW)},
    {"game", "show_seed_cooldown", "false", "\"Show CD of Seed Packets\": their recharge time.",
     CFG_BOOL, NULL, H(HS_SHOW_COOLDOWN)},
    {"game", "manual_collect", "false",
     "\"Disable Auto-collection of Sunlight and Coins\": collect them with the\n"
     "# cursor (the TV edition collects them itself). Not the Zen Garden. With\n"
     "# the controller scheme, offline (VS too), they are collected the Xbox 360\n"
     "# edition's way whatever this says: the cursor passing near, and ZL / ZR\n"
     "# held draw every one on the lawn to the cursor (in VS the plants' sun, the\n"
     "# zombies' brains). Online the game collects them itself.",
     CFG_BOOL, NULL, H(HS_MANUAL_COLLECT)},
    {"game", "disable_item_bar", "false",
     "\"Disable Special Item Bar\": the TV edition's bar of special items, which\n"
     "# the mod makes free and unlimited. True removes it.",
     CFG_BOOL, NULL, H(HS_DISABLE_SHOP)},
    {"game", "hide_grass_and_poles", "false",
     "\"Hide Grass and Pole\": the foreground grass and power poles.", CFG_BOOL, NULL,
     H(HS_HIDE_COVER)},
    {"game", "no_trash_can_zombies", "false",
     "\"Disable Trash Bin Zombie\": the TV edition adds Trash Can Zombies to\n"
     "# Survival levels; without them the waves are nearly those of the PC.",
     CFG_BOOL, NULL, H(HS_NO_TRASH_BIN)},
    {"game", "xbox_music", "false",
     "\"Xbox BGMs\": the Xbox version's music, with the PC's drum variations (about\n"
     "# 0.2 s late). Off: the TV edition's music.",
     CFG_BOOL, NULL, H(HS_XBOX_MUSIC)},
    {"game", "skip_logo", "false", "Skip the PopCap logo animation at start-up.", CFG_BOOL, NULL,
     H(HS_SKIP_LOGO)},
    {"game", "button_pictures", "true",
     "The Switch's buttons drawn in the game's text (a layer the English layer\n"
     "# adds to each English font). False: the text names them (A, +, ZL...).",
     CFG_BOOL, NULL, N},
    {"game", "touch_logo", "true",
     "The Plants vs. Zombies Touch logo on the title screen and main menu.\n"
     "# False: the game's own.",
     CFG_BOOL, NULL, N},
    {"game", "intro_video", "true",
     "The intro video at start-up (PopCap's logo and the zombie hand, from\n"
     "# the game APK's movies/intro.mp4). Any button skips it.",
     CFG_BOOL, NULL, H(HS_PLAY_VIDEO)},
    {"game", "advanced_pause", "false",
     "\"Enable Advanced Pause\": + pauses the game but you can still plant, dig\n"
     "# and fire Cob Cannons. Off: the game's own pause.",
     CFG_BOOL, NULL, N},
    /* ---- the mod's cheat menu ---- */
    {"cheats", "unlimited_sun", "false",
     "The mod's cheat menu. The game has all of it too: pause menu > Help &\n"
     "# Options > Mod Menu, whose choices (kept in data/files/modmenu.txt) win\n"
     "# over these.",
     CFG_BOOL, NULL, C(1)},
    {"cheats", "place_anywhere", "false", "Plants can go anywhere (water, roof, on top of others).",
     CFG_BOOL, NULL, C(42)},
    {"cheats", "seeds_no_recharge", "false", NULL, CFG_BOOL, NULL, C(2)},
    {"cheats", "plants_reload_instantly", "false", NULL, CFG_BOOL, NULL, C(3)},
    {"cheats", "mushrooms_awake", "false", "Mushrooms awake in daytime (no Coffee Bean needed).",
     CFG_BOOL, NULL, C(4)},
    {"cheats", "game_speed", "off", "off, 1.2, 1.5, 2, 2.5, 3, 5 or 10 (times normal speed).",
     CFG_CHOICE, "off,1.2,1.5,2,2.5,3,5,10", C(8)},
    {"cheats", "show_plant_health", "false", NULL, CFG_BOOL, NULL, C(21)},
    {"cheats", "show_zombie_health", "false", NULL, CFG_BOOL, NULL, C(23)},
    {"cheats", "show_armor_health", "false", "Health of helmets and shields.", CFG_BOOL, NULL, C(24)},
    {"cheats", "clear_fog", "false", NULL, CFG_BOOL, NULL, C(6)},
    {"cheats", "transparent_vases", "false", "See inside Vasebreaker vases.", CFG_BOOL, NULL, C(43)},
    {"cheats", "zombies_cannot_enter_house", "false", NULL, CFG_BOOL, NULL, C(45)},
    {"cheats", "no_lawn_mowers", "false", NULL, CFG_BOOL, NULL, C(88)},
    /* ---- the Switch side ---- */
    {"controls", "swap_a_b", "false",
     "Swap A and B (true: the bottom button selects, the right one goes back).", CFG_BOOL, NULL, N},
    {"controls", "scheme", "xbox",
     "xbox: the Xbox 360 edition's controls (the engine's own gamepad mode):\n"
     "# A plants and collects, hold B to dig, L / R pick a seed packet, ZL / ZR\n"
     "# (the triggers) switch the sun magnet (auto-collect; shown top left), Y is\n"
     "# fast forward (2x, the button right of the progress meter), - opens the\n"
     "# special item bar (hidden otherwise), + the pause menu (Help & Options\n"
     "# there: Cheats, Mod Menu).\n"
     "# tv: the Android TV remote scheme (A opens a seed picker).",
     CFG_CHOICE, "xbox,tv", N},
    {"controls", "pointer", "true",
     "Click the right stick for a pointer: the right stick moves it and ZR taps\n"
     "# with it, for the screens made for touch (the Zombatar editor, the VS\n"
     "# lobbies). Click again, or leave it a few seconds, to put it away.\n"
     "# Otherwise the right stick scrolls and moves through menus.",
     CFG_BOOL, NULL, N},
    {"controls", "rumble", "true",
     "Rumble for explosions, the whack-a-zombie mallet and the like (the mod's\n"
     "# phone vibration).",
     CFG_BOOL, NULL, N},
    {"touch", "second_finger_on_lawn", "true",
     "A second finger on the lawn works as its own cursor (the mod's two-finger\n"
     "# play: plant with one hand, collect or shovel with the other).",
     CFG_BOOL, NULL, N},
    {"display", "resolution", "720",
     "Rendering resolution: 720, 1080 or auto (1080 if docked when the game\n"
     "# starts). The Switch scales the picture to the screen either way; the game's\n"
     "# art is made for 720.",
     CFG_CHOICE, "720,1080,auto", N},
    {"performance", "boost_cpu_when_loading", "true",
     "CPU at 1785 MHz while the game starts (until its first picture) and\n"
     "# inside loading frames (those over 50 ms), normal otherwise.",
     CFG_BOOL, NULL, N},
    {"debug", "gl_selftest", "false", "Graphics self-test picture at start-up.", CFG_BOOL, NULL, N},
    {"debug", "boot_log_on_screen", "false",
     "Show the start-up log on screen at every launch. Off: the log appears only\n"
     "# while something is being set up (first launch, a new APK or NRO).",
     CFG_BOOL, NULL, N},
    {"debug", "log_java_calls", "false",
     "Write every Java method the game calls to debug.log (slow; for bug reports).", CFG_BOOL,
     NULL, N},
    {"debug", "profile_startup_seconds", "0",
     "Every thread sampled for this many seconds from the first picture, with a\n"
     "# report in debug.log every 10 s (where the loading time goes). Slows the\n"
     "# start a little. 0 turns it off.",
     CFG_INT, NULL, &g_cfg.profile_secs, 0, 3600, 0, 0},
    {"debug", "load_touch_mod", "true",
     "false: start the plain TV edition, without the Touch mod (libHomura), its\n"
     "# settings and its cheats. Only for telling a problem in the mod from one in\n"
     "# the game itself; leave it true to play.",
     CFG_BOOL, NULL, N},
    {"debug", "mod_from_apk", "false",
     "true: the mod exactly as the game APK has it, instead of this port's build of\n"
     "# the same mod with the Switch controls (the built-in mod menu, the Cheats\n"
     "# button, controller support in the Zombatar and VS screens).",
     CFG_BOOL, NULL, N},
    /* [config] version = 2: the engine's row, last (CfgTable.version) */
};

static const CfgSection k_sections[] = {
    {"cheats", "# The mod's cheat menu. They change the game as it plays; saves made with\n"
               "# them on keep what they got.\n"},
};

/* Defaults that changed. Version 1 files came with profile_startup_seconds =
 * 90 (a test build's default): turned off once, the rest of the file kept. */
static const CfgMigrate k_migrate[] = {
    {"debug", "profile_startup_seconds", "90", "0", 0},
};

static void apply(void) {
  g_cfg.ncheats = 0;
  for (unsigned i = 0; i < CFG_COUNT(k_opts); i++) {
    const CfgOpt *o = &k_opts[i];
    if (o->tag <= 0 || o->tag >= TAG_HS)
      continue; /* the HS_ rows went to their dst */
    int v = rt_config_int(o->section, o->key); /* a bool 1/0, a choice its index */
    if (v && g_cfg.ncheats < (int)(sizeof g_cfg.cheats / sizeof g_cfg.cheats[0]))
      g_cfg.cheats[g_cfg.ncheats++] = (CheatSetting){o->tag, v, o->kind == CFG_CHOICE};
  }
  g_cfg.english = rt_config_int("game", "language") == 0;
  g_cfg.touch_logo = rt_config_bool("game", "touch_logo");
  g_cfg.button_pictures = rt_config_bool("game", "button_pictures");
  g_cfg.special_pause = rt_config_bool("game", "advanced_pause");
  g_cfg.adventure_levels = rt_config_int("game", "adventure_difficulty");
  g_cfg.swap_ab = rt_config_bool("controls", "swap_a_b");
  g_cfg.rumble = rt_config_bool("controls", "rumble");
  g_cfg.pointer = rt_config_bool("controls", "pointer");
  g_cfg.mod_from_apk = rt_config_bool("debug", "mod_from_apk");
  g_cfg.xbox_scheme = rt_config_int("controls", "scheme") == 0;
  g_cfg.second_touch = rt_config_bool("touch", "second_finger_on_lawn");
  g_cfg.load_mod = rt_config_bool("debug", "load_touch_mod");
  const RtConfig *rt = rt_config();
  g_cfg.boost = rt->boost;
  g_cfg.gl_selftest = rt->gl_selftest;
  g_cfg.boot_log = rt->boot_log;
  g_cfg.log_jni = rt->log_jni;
  g_cfg.res_w = rt->res_w;
  g_cfg.res_h = rt->res_h;

  char on[512];
  int n = 0;
  for (unsigned i = 0; i < CFG_COUNT(k_opts) && n < (int)sizeof on - 48; i++)
    if (k_opts[i].tag >= TAG_HS && g_cfg.homura[k_opts[i].tag - TAG_HS])
      n += snprintf(on + n, sizeof on - n, " %s", k_opts[i].key);
  debugPrintf("[config] game settings on:%s\n", n ? on : " (none)");
  n = 0;
  on[0] = 0;
  for (int i = 0; i < g_cfg.ncheats && n < (int)sizeof on - 32; i++)
    n += snprintf(on + n, sizeof on - n, " %d=%d", g_cfg.cheats[i].id, g_cfg.cheats[i].value);
  const int docked = appletGetOperationMode() == AppletOperationMode_Console;
  debugPrintf("[config] %s; cheats:%s; %dx%d (%s, %s), A/B %s, second finger %s, CPU boost %s\n",
              g_cfg.english ? "English" : "Chinese", g_cfg.ncheats ? on : " none", g_cfg.res_w,
              g_cfg.res_h, rt_config_get("display", "resolution"), docked ? "docked" : "handheld",
              g_cfg.swap_ab ? "swapped" : "normal", g_cfg.second_touch ? "on" : "off",
              g_cfg.boost ? "on" : "off");
}

static const CfgTable k_table = {
    .opts = k_opts,
    .nopts = CFG_COUNT(k_opts),
    .sections = k_sections,
    .nsections = CFG_COUNT(k_sections),
    .migrate = k_migrate,
    .nmigrate = CFG_COUNT(k_migrate),
    .version = 2,
    .apply = apply,
};

void dcr_config_load(void) { rt_config_load(&k_table); }
