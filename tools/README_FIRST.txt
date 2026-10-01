Plants vs. Zombies: Touch for Nintendo Switch (32-bit wrapper)
=============================================================
by aks796 (the Switch port) and ZombieYetis (the Touch mod)

WHAT YOU NEED ON THE SD CARD
  switch/pvz_touch_nx/pvz_touch_nx.nro   the launcher (this folder)
  switch/pvz_touch_nx/<any name>.apk     YOUR copy of the game:
      PvZ TV Touch 1.1.5 (com.trans.pvztv), e.g.
      PvZ-TV-v1.1.5-260925-release.apk (or the 260924 one).
      Keep its name: the game finds it by what is in it.
  The English pictures, fonts and text come with the NRO: the first
  start copies them into the folder as "PvZ Touch English.apk" (leave it
  there). An English APK of your own (PvZTouch 4.0.5 or RedStr1x, any
  name) in the folder is used instead.
  Atmosphere and sphaira.
Everything else is made on the console.

COMING FROM AN OLDER RELEASE (switch/pvztouch, PvZTouch.nro)
  The folder is now switch/pvz_touch_nx. Either:
    - copy pvz_touch_nx.nro into your old switch/pvztouch folder and launch
      your existing icon: it updates itself, then moves the folder over; or
    - set up the new folder and a new forwarder as below.
  Either way the first start moves everything from switch/pvztouch (your
  APKs, saves, settings) into switch/pvz_touch_nx and leaves only
  PvZTouch.nro behind; the move is written in debug.log. Then delete
  switch/pvztouch and, if you made a new one, the old forwarder.

SET UP (once)
  1. In sphaira: Homebrew -> Plants vs. Zombies: Touch -> Install Forwarder
     (already installed? Install it again for the new name and icon).
  2. Launch the new "Plants vs. Zombies: Touch" icon on the HOME menu. The launcher
     installs the 32-bit game program for that icon
     (atmosphere/contents/<its title id>/exefs.nsp) and restarts it.
  3. The first start unpacks the game's three libraries from the game APK,
     copies the English files out of the NRO and makes the English layer
     (up to half a minute, shown on screen; again only when an APK changes).

UPDATING
  Copy the new pvz_touch_nx.nro over the old one and launch the icon: the
  game installs the newer build itself and restarts.

COPYING ON A MAC
  Your card already has "atmosphere" and "switch" folders. Dragging these
  on top and choosing REPLACE deletes what is there. Hold Option while
  dragging and choose "Merge", or copy the individual files.

UNDO
  Delete atmosphere/contents/<id>/exefs.nsp (the icon then opens the
  launcher again) and uninstall the forwarder like any title.

AFTER A RUN, SEND BACK
  switch/pvz_touch_nx/debug.log
  switch/pvz_touch_nx/crash.log                 (if present)
  atmosphere/crash_reports/ newest report   (if it crashed)

SETTINGS: switch/pvz_touch_nx/config.ini
  Written on the first start with every option explained. Language
  (english / chinese), the mod's own settings, its cheat menu, A/B swap, rumble, the second
  touch finger, resolution. Changes apply at the next start.

CONTROLS
  As the Xbox 360 edition: A select/plant/collect, B back (hold B on the
  lawn: dig), L/R seed packets, ZL/ZR held: the sun vacuum (every sun and
  coin flies to the cursor; the cursor also picks up what it passes), + pause
  menu, sticks move the cursor. Y: fast forward (2x; single player and local
  co-op). Fast forward and Menu are also stone buttons in the top right. -: the special item bar (power-ups, free; hidden until then). A on a text field brings up the keyboard. Clicking the right
  stick: a pointer (ZR taps). Any button skips the intro video.
  In menus a D-pad direction held keeps going; lists (the VS modes, the
  challenges) also take the left stick, held or pushed.
  The touchscreen works as on a phone; a second finger on the lawn is a
  second cursor. Two controllers: 2 players.

CHEATS / MOD MENU
  Pause menu > Help & Options: Cheats (cheat codes, in a level) and Mod Menu
  (the mod's cheat menu: its sections as buttons; A opens one, then A
  toggles, left/right change, L/R the other sections, B back / closes;
  kept in data/files/modmenu.txt).

SAVES
  switch/pvz_touch_nx/data/files/userdata/
