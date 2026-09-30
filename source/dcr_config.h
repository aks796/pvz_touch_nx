/* dcr_config.h -- the user's settings, from <game folder>/config.ini (dcr_config.c). */
#ifndef DCR_USER_CONFIG_H
#define DCR_USER_CONFIG_H

/* The mod's own settings: EnhanceActivity.loadGameSettings calls one Homura
 * native per option that is on (names are the SharedPreferences keys). */
enum {
  HS_XBOX_MUSIC,      /* useXboxMusics          -> nativeUseXboxMusics */
  HS_MANUAL_COLLECT,  /* enableManualCollect    -> nativeEnableManualCollect */
  HS_DISABLE_SHOP,    /* disableShop            -> nativeDisableShop */
  HS_XBOX_PAUSE,      /* enableNewOptionsDialog -> nativeEnableNewOptionsDialog */
  HS_HIDE_COVER,      /* hideCoverLayer         -> nativeHideCoverLayer */
  HS_SHOW_COOLDOWN,   /* showCoolDown           -> nativeShowCoolDown */
  HS_NORMAL_LEVEL,    /* normalLevel            -> nativeEnableNormalLevelMode */
  HS_CLASSIC_SHOVEL,  /* useNewShovel           -> nativeEnableNewShovel */
  HS_IMITATER_GREY,   /* imitater               -> nativeEnableImitater */
  HS_NO_TRASH_BIN,    /* disableTrashBin        -> nativeDisableTrashBinZombie */
  HS_SHOW_HOUSE,      /* showHouse              -> nativeShowHouse */
  HS_NEW_COB_CANNON,  /* useNewCobCannon        -> nativeUseNewCobCannon */
  HS_AUTO_FIX_POS,    /* positionAutoFix        -> nativeAutoFixPosition */
  HS_PIN_SEED_BANK,   /* seedBankPin            -> nativeSeedBankPin */
  HS_DYNAMIC_PREVIEW, /* dynamicPreview         -> nativeDynamicPreview */
  HS_SKIP_LOGO,       /* jumpLogo               -> nativeJumpLogo */
  HS_PLAY_VIDEO,      /* playVideo              -> nativePlayVideo */
  HS_COUNT
};

/* [game] adventure_difficulty: the APK's assets/levels/ file copied to the
 * user files dir as properties/levels.xml, as the mod's settings screen does. */
enum { DCR_LEVELS_TV_TOUCH, DCR_LEVELS_TRANSMENSION, DCR_LEVELS_TOUMAI, DCR_LEVELS_CUSTOM };

/* Cheats: Homura's Preferences.Changes(featNum, value, boolean), sent once at
 * start-up. */
typedef struct {
  int id;       /* featNum */
  int value;    /* spinner value (0 = off) or boolean */
  int is_value; /* 1: pass as value, 0: pass as boolean */
} CheatSetting;

typedef struct {
  int english;           /* [game] language: the English layer (pvz_english.c) */
  int touch_logo;        /* [game] touch_logo: the port's logo (dcr_setup.c ensure_logo) */
  int button_pictures;   /* [game] button_pictures: the Switch's buttons drawn in the text */
  int homura[HS_COUNT];  /* [game] */
  int special_pause;     /* [game] advanced_pause: the pause button freezes the lawn
                            but keeps the cursor (useSpecialPause) */
  int adventure_levels;  /* [game] adventure_difficulty: DCR_LEVELS_* (dcr_setup.c installs
                            the file when homura[HS_NORMAL_LEVEL] is off) */
  CheatSetting cheats[24];
  int ncheats;
  int second_touch;      /* [touch] second_finger_on_lawn */
  int swap_ab;           /* [controls] swap_a_b */
  int rumble;            /* [controls] rumble */
  int pointer;           /* [controls] pointer: right stick + ZR as a touch */
  int xbox_scheme;       /* [controls] scheme: xbox (LAWN_GAMEPAD_MODE) or tv */
  int mod_from_apk;      /* [debug] mod_from_apk: game.apk's libHomura, not the port's build */
  int res_w, res_h;      /* [display] resolution */
  int boost;             /* [performance] boost_cpu_when_loading */
  int gl_selftest;       /* [debug] gl_selftest */
  int boot_log;          /* [debug] boot_log_on_screen */
  int log_jni;           /* [debug] log_java_calls */
  int load_mod;          /* [debug] load_touch_mod: false runs without libHomura */
  int profile_secs;      /* [debug] profile_startup_seconds: pvz_prof.c, 0 = off */
} DcrConfig;

/* Read config.ini (writing it with the defaults, or adding missing options,
 * first). Early in main(); the defaults hold until then. */
void dcr_config_load(void);
const DcrConfig *dcr_config(void);

#endif
