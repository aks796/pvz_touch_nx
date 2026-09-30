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

#include "PvZ/Lawn/Widget/HelpOptionsDialog.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/GameButton.h"
#include "PvZ/Lawn/Widget/ModMenuDialog.h"
#include "PvZ/ModFeature.h"

#include <vector>

using namespace Sexy;

namespace {
// Switch port: two more buttons. In a level "Cheats" opens the game's cheat
// code dialog (the mod's entry 41) over the lawn; on both screens "Mod Menu" opens the
// mod's cheat menu (ModMenuDialog), which Android shows as a Java overlay.
constexpr int kCheatsButtonId = 10;
constexpr int kModMenuButtonId = 11;
HelpOptionsDialog *gExtraOwner;
GameButton *gCheatsButton;
GameButton *gModMenuButton;

// The buttons in a column from How to Play down, then Back where it is; the
// controller moves along them (round). In a level Switch User is gone and
// Credits is not added (the game adds it on the main menu only).
void LayoutButtons(HelpOptionsDialog *a) {
    if (!isMainMenu) {
        GameButton *switchUserButton = a->mSwitchUserButton;
        switchUserButton->mDisabled = true;
        switchUserButton->mVisible = false;
        switchUserButton->Resize(0, 0, 0, 0);
    }
    std::vector<GameButton *> column{a->mHowToPlayButton};
    if (isMainMenu) {
        column.push_back(a->mSwitchUserButton);
    }
    column.push_back(a->mSettingsButton);
    if (gExtraOwner == a && gCheatsButton != nullptr && !isMainMenu) {
        column.push_back(gCheatsButton);
    }
    if (a->mCreditsButton->mParent != nullptr) {
        column.push_back(a->mCreditsButton);
    }
    if (gExtraOwner == a && gModMenuButton != nullptr) {
        column.push_back(gModMenuButton);
    }
    GameButton *first = a->mHowToPlayButton;
    GameButton *back = a->mBackButton;
    const int n = static_cast<int>(column.size());
    for (int i = 0; i < n; ++i) {
        column[i]->Resize(first->mX, first->mY + i * first->mHeight, first->mWidth, first->mHeight);
        column[i]->mFocusLinks[0] = i == 0 ? static_cast<Widget *>(back) : column[i - 1];
        column[i]->mFocusLinks[1] = i + 1 < n ? static_cast<Widget *>(column[i + 1]) : back;
        column[i]->mFocusLinks[2] = nullptr;
        column[i]->mFocusLinks[3] = nullptr;
    }
    back->mFocusLinks[0] = column[n - 1];
    back->mFocusLinks[1] = column[0];
    if (gCheatsButton != nullptr && gExtraOwner == a) {
        gCheatsButton->mVisible = !isMainMenu;
        gCheatsButton->mDisabled = isMainMenu;
    }
}
} // namespace

void HelpOptionsDialog_HelpOptionsDialog(HelpOptionsDialog *a, LawnApp *a2) {
    old_HelpOptionsDialog_HelpOptionsDialog(a, a2);
    // The previous dialog's buttons went with it (RemovedFromManager).
    gExtraOwner = a;
    gCheatsButton = MakeButton(kCheatsButtonId, a, a, "Cheats");
    gModMenuButton = MakeButton(kModMenuButtonId, a, a, "Mod Menu");
    LayoutButtons(a);
}

void HelpOptionsDialog_AddedToManager(HelpOptionsDialog *a, WidgetManager *a2) {
    old_HelpOptionsDialog_AddedToManager(a, a2);
    if (gExtraOwner == a) {
        a->AddWidget(gCheatsButton);
        a->AddWidget(gModMenuButton);
        LayoutButtons(a);
    }
}

void HelpOptionsDialog_RemovedFromManager(HelpOptionsDialog *a, WidgetManager *a2) {
    if (gExtraOwner == a) {
        a->RemoveWidget(gCheatsButton);
        a->RemoveWidget(gModMenuButton);
        // after the click that closed the dialog has returned
        a->mApp->SafeDeleteWidget(gCheatsButton);
        a->mApp->SafeDeleteWidget(gModMenuButton);
        gCheatsButton = nullptr;
        gModMenuButton = nullptr;
        gExtraOwner = nullptr;
    }
    old_HelpOptionsDialog_RemovedFromManager(a, a2);
}

void HelpOptionsDialog_ButtonDepress(HelpOptionsDialog *a, int a2) {
    if (a2 == kCheatsButtonId) {
        // back to the lawn, then the cheat code dialog (LawnApp::UpdateApp):
        // opened over the menus it went under Help & Options
        ModMenuDialog::LeaveMenus(a->mApp);
        ApplyModFeature(41, 0, true, {});
        return;
    }
    if (a2 == kModMenuButtonId) {
        ModMenuDialog::Show(a->mApp);
        return;
    }
    // 修复在游戏战斗中打开新版暂停菜单时可以切换用户
    if (a2 == 1) {
        if (isMainMenu)
            a->mApp->DoUserDialog();
        else
            a->mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "[DIALOG_WARNING]", "[CHANGE_USER_FORBID]", "[DIALOG_BUTTON_OK]", "", 3);
        return;
    }
    // if( thePlayerIndex == 0){
    // int * lawnApp = (int*)a[HELPOPTIONS_LAWNAPP_OFFSET];
    // GameMode::GameMode mGameMode = (GameMode::GameMode) *(lawnApp + LAWNAPP_GAMEMODE_OFFSET);
    // if (mGameMode >= GameMode::GAMEMODE_MP_VS && mGameMode < GameMode::GAMEMODE_TWO_PLAYER_COOP_ENDLESS) {
    // LawnApp_ShowHelpTextScreen(lawnApp, 2);
    // } else {
    // LawnApp_ShowHelpTextScreen(lawnApp, 0);
    // }
    // return;
    // }
    old_HelpOptionsDialog_ButtonDepress(a, a2);
}

void HelpOptionsDialog_Resize(HelpOptionsDialog *a, int a2, int a3, int a4, int a5) {
    // 在战斗界面去除“切换用户”按钮 (LayoutButtons)
    old_HelpOptionsDialog_Resize(a, a2, a3, a4, a5);
    LayoutButtons(a);
}
