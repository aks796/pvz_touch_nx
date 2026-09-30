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

#include "PvZ/Lawn/Widget/CreditScreen.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/SwitchPad.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <cstring>

using namespace Sexy;

namespace {
// Switch port: leaving the credits says "Exit" ([CREDITS_EXIT], the English
// layer's), else the game's "Close" -- not the main menu's "Exit Game"
pvzstl::string CreditsExitLabel() {
    return switchpad::Label("[CREDITS_EXIT]", "[CLOSE]");
}
} // namespace

int CreditScreen_LawnMessageBox(LawnApp *theApp, Dialogs theDialogId, const char *theHeaderName, const char *theLinesName, const char *theButton1Name, const char *theButton2Name, int theButtonMode) {
    if (theHeaderName != nullptr && theButton2Name != nullptr && std::strcmp(theHeaderName, "[CREDITS_PAUSE_HEADER]") == 0 && std::strcmp(theButton2Name, "[MAIN_MENU_BUTTON]") == 0) {
        const pvzstl::string exitLabel = CreditsExitLabel();
        return old_LawnApp_LawnMessageBox(theApp, theDialogId, theHeaderName, theLinesName, theButton1Name, exitLabel.c_str(), theButtonMode);
    }
    return old_LawnApp_LawnMessageBox(theApp, theDialogId, theHeaderName, theLinesName, theButton1Name, theButton2Name, theButtonMode);
}

void CreditScreen::_constructor(LawnApp *theApp, bool theBool) {
    old_CreditScreen__constructor(this, theApp, theBool);

    gCreditScreenBackButton = MakeButton(1000, this, this, CreditsExitLabel().c_str());
    gCreditScreenBackButton->Resize(725, 0, 170, 50);
}

void CreditScreen::_destructor() {
    delete gCreditScreenBackButton;
    gCreditScreenBackButton = nullptr;

    old_CreditScreen__destructor(this);
}

void CreditScreen::AddedToManager(WidgetManager *theWidgetManager) {
    old_CreditScreen_AddedToManager(this, theWidgetManager);

    AddWidget(gCreditScreenBackButton);
}

void CreditScreen::RemovedFromManager(WidgetManager *theWidgetManager) {
    mFocusedChildWidget = gCreditScreenBackButton; // 修复触摸CreditScreen后点击按钮退出就会闪退的BUG,虽然不知道为什么
    RemoveWidget(gCreditScreenBackButton);

    old_CreditScreen_RemovedFromManager(this, theWidgetManager);
}

void CreditScreen::ButtonDepress(int theId) {
    if (theId == 1000) {
        LawnApp *lawnApp = gLawnApp;
        lawnApp->mCreditScreen->PauseCredits();
    }
}
