/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PVZ_LAWN_WIDGET_MOD_MENU_DIALOG_H
#define PVZ_LAWN_WIDGET_MOD_MENU_DIALOG_H

#include "PvZ/Lawn/Widget/LawnDialog.h"
#include "PvZ/SexyAppFramework/Misc/KeyCodes.h"

// Switch port: the cheat menu (cheat::lang::en_US::featureList) as a game
// dialog, driven by the controller or touch. On Android the Java overlay
// shows the same list; here Help & Options opens this one (HelpOptionsDialog.cpp).
// It opens on its sections -- stone buttons, as Help & Options' -- and a
// section on its own rows.
class ModMenuDialog final : public LawnDialog {
public:
    static constexpr int DIALOG_ID = 200; // above the game's Dialogs

    // Shows it over the dialogs already up (Help & Options).
    static void Show(LawnApp *theApp);
    // Re-applies the settings chosen here in an earlier session (modmenu.txt in
    // the files directory); once, when the game is up. They come after, so win
    // over, config.ini's [cheats] (set through the Java entry point at start).
    static void LoadSettings();
    // An entry set from outside the menu (Preferences.Changes): shown as it is.
    static void Remember(int theFeat, int theValue, bool theBoolean);
    // Closes Help & Options and the pause menu under it: back to the lawn (or
    // the main menu), for a dialog that should have the screen to itself.
    static void LeaveMenus(LawnApp *theApp);

    explicit ModMenuDialog(LawnApp *theApp);
    ~ModMenuDialog();

    void Update();
    void Draw(Sexy::Graphics *g);
    bool KeyDown(Sexy::KeyCode theKey);
    bool KeyUp(Sexy::KeyCode theKey);
    void MouseDown(int x, int y, int theBtnNum, int theClickCount);
    void MouseUp(int x, int y, int theBtnNum, int theClickCount);
    void MouseDrag(int x, int y);

private:
    int mHeldDir = 0;   // the direction key held (1 up, 2 down, 3 left, 4 right), for repeats
    int mHeldTicks = 0; // updates since it went down
    int mFlashRow = -1; // the action row just run, shown as done for a moment
    int mFlashTicks = 0;
    int mTop = 0;       // the first row shown, an index into the visible rows
    int mDownX = 0, mDownY = 0, mDragY = 0;
    int mTick = 0;      // updates, for the selection's glow
    bool mTouching = false, mDragged = false;
    bool mClosing = false;

    // The menu is its sections, as buttons; one opens to its rows (B: back).
    void OpenSection(int theAt); // theAt: an index into the sections, wrapping
    void CloseSection();
    void Step(int theDir, bool theRepeat);
    void Activate(int theRow);
    void Change(int theRow, int theDelta, bool theRepeat);
    void MoveTo(int theRow);
    void Close(bool theAlsoHelpOptions);
    int RowAt(int x, int y) const;

    void _destructor();
    void _destructor2();
};

#endif // PVZ_LAWN_WIDGET_MOD_MENU_DIALOG_H
