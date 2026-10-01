/* pvz_res.S -- the port's own English lines (resources/english/, see
 * pvz_english.c), as NUL-terminated strings. */
	.section .rodata.pvz_res, "a"

	.global pvz_res_addon_en
	.type pvz_res_addon_en, %object
pvz_res_addon_en:
	.incbin "resources/english/AddonStrings_en.txt"
	.byte 0

	.global pvz_res_lawn_en
	.type pvz_res_lawn_en, %object
pvz_res_lawn_en:
	.incbin "resources/english/LawnStrings_en.txt"
	.byte 0

	.global pvz_res_lawn_fix
	.type pvz_res_lawn_fix, %object
pvz_res_lawn_fix:
	.incbin "resources/english/LawnStrings_fix.txt"
	.byte 0

/* The port's build of the mod (mod/src, built by mod/build_mod.sh): the
 * 1.1.5-260925 mod's own source (nightly e91bc9a) with the Switch changes. */
	.balign 4
	.global pvz_res_homura
	.type pvz_res_homura, %object
pvz_res_homura:
	.incbin "mod/out/libHomura.so"
pvz_res_homura_end:
	.global pvz_res_homura_size
	.balign 4
pvz_res_homura_size:
	.word pvz_res_homura_end - pvz_res_homura

/* The Plants vs. Zombies: Touch logo (logo.png, sized by tools/make_logo.py):
 * the main menu's and the title screen's (dcr_setup.c ensure_logo). */
	.balign 4
	.global pvz_res_logo_menu
	.type pvz_res_logo_menu, %object
pvz_res_logo_menu:
	.incbin "resources/logo/menu.png"
pvz_res_logo_menu_end:
	.balign 4
	.global pvz_res_logo_menu_size
pvz_res_logo_menu_size:
	.word pvz_res_logo_menu_end - pvz_res_logo_menu

	.balign 4
	.global pvz_res_logo_title
	.type pvz_res_logo_title, %object
pvz_res_logo_title:
	.incbin "resources/logo/title.png"
pvz_res_logo_title_end:
	.balign 4
	.global pvz_res_logo_title_size
pvz_res_logo_title_size:
	.word pvz_res_logo_title_end - pvz_res_logo_title

/* The Switch's buttons (tools/make_button_icons.py): every one in a row, for
 * the fonts' button layer, and the help bar's sheets (pvz_english.c). */
	.balign 4
	.global pvz_res_button_icons
pvz_res_button_icons:
	.incbin "resources/buttons/icons.png"
pvz_res_button_icons_end:
	.balign 4
	.global pvz_res_button_icons_size
pvz_res_button_icons_size:
	.word pvz_res_button_icons_end - pvz_res_button_icons

	.balign 4
	.global pvz_res_help_buttons
pvz_res_help_buttons:
	.incbin "resources/buttons/help_buttons.png"
pvz_res_help_buttons_end:
	.balign 4
	.global pvz_res_help_buttons_size
pvz_res_help_buttons_size:
	.word pvz_res_help_buttons_end - pvz_res_help_buttons

	.balign 4
	.global pvz_res_help_buttons_small
pvz_res_help_buttons_small:
	.incbin "resources/buttons/help_buttons_small.png"
pvz_res_help_buttons_small_end:
	.balign 4
	.global pvz_res_help_buttons_small_size
pvz_res_help_buttons_small_size:
	.word pvz_res_help_buttons_small_end - pvz_res_help_buttons_small

/* The versus side picker's controllers, players 1 and 2: the Switch Pro
 * Controller (tools/make_controller_icons.py; pvz_english.c puts them in the
 * layer with the held ones recoloured). */
	.balign 4
	.global pvz_res_gamepad0
pvz_res_gamepad0:
	.incbin "resources/controllers/gamepad0.png"
pvz_res_gamepad0_end:
	.balign 4
	.global pvz_res_gamepad0_size
pvz_res_gamepad0_size:
	.word pvz_res_gamepad0_end - pvz_res_gamepad0

	.balign 4
	.global pvz_res_gamepad1
pvz_res_gamepad1:
	.incbin "resources/controllers/gamepad1.png"
pvz_res_gamepad1_end:
	.balign 4
	.global pvz_res_gamepad1_size
pvz_res_gamepad1_size:
	.word pvz_res_gamepad1_end - pvz_res_gamepad1
