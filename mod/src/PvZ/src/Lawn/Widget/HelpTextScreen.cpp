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

#include "PvZ/Lawn/SwitchPad.h"
#include "PvZ/Lawn/Widget/HelpTextScreen.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

using namespace Sexy;

namespace {
constexpr int nextPageButtonX = 836;
constexpr int nextPageButtonY = 318;

constexpr int prevPageButtonX = 2540;
constexpr int prevPageButtonY = 318;

GameButton *gHelpTextScreenCloseButton;
} // namespace

void HelpTextScreen::_constructor(LawnApp *theApp, int thePage) {
    if (theApp->mBoard) {
        if (theApp->IsCoopMode()) {
            thePage = HelpTextPage::HELP_TEXT_PAGE_COOP;
        } else if (theApp->IsVSMode()) {
            thePage = HelpTextPage::HELP_TEXT_PAGE_VS;
        }
    }

    // 这个HelpTextScreen是全屏的，但触控事件并不会分发到此处，而是发给子控件。只有内容外侧的点击事件才能收到
    old_HelpTextScreen__constructor(this, theApp, thePage);

    gHelpTextScreenCloseButton = MakeButton(1000, this, this, switchpad::Label("[SWITCH_CLOSE]", "[CLOSE]")); // Switch port: B closes it too
    gHelpTextScreenCloseButton->Resize(650 - mX, 540 - mY, 170, 50);

    Resize(mX, mY, 4000, mHeight);
}

void HelpTextScreen::_destructor() {
    delete gHelpTextScreenCloseButton;
    gHelpTextScreenCloseButton = nullptr;

    old_HelpTextScreen__destructor(this);
}

void HelpTextScreen::AddedToManager(WidgetManager *theWidgetNanager) {
    old_HelpTextScreen_AddedToManager(this, theWidgetNanager);

    AddWidget(gHelpTextScreenCloseButton);
}

void HelpTextScreen::RemovedFromManager(WidgetManager *theWidgetManager) {
    RemoveWidget(gHelpTextScreenCloseButton);

    old_HelpTextScreen_RemovedFromManager(this, theWidgetManager);
}

void HelpTextScreen::Update() {
    gHelpTextScreenCloseButton->Move(650 - mX, 540 - mY);

    old_HelpTextScreen_Update(this);
}

void HelpTextScreen::Draw(Sexy::Graphics *g) {
    old_HelpTextScreen_Draw(this, g);
    g->DrawImage(Sexy::IMAGE_ZEN_NEXTGARDEN, nextPageButtonX, nextPageButtonY);
    g->DrawImageMirror(Sexy::IMAGE_ZEN_NEXTGARDEN, prevPageButtonX, prevPageButtonY, true);
}

void HelpTextScreen::MouseDown(int x, int y, int theClickCount) {
    int imageWidth = (Sexy::IMAGE_ZEN_NEXTGARDEN)->GetCelWidth();
    int imageHeight = (Sexy::IMAGE_ZEN_NEXTGARDEN)->GetHeight();

    Sexy::Rect nextPageRect = {nextPageButtonX, nextPageButtonY, imageWidth, imageHeight};
    if (nextPageRect.Contains(x, y)) {
        KeyDown(Sexy::KeyCode::KEYCODE_RIGHT);
        return;
    }

    Sexy::Rect prevPageRect = {prevPageButtonX, prevPageButtonY, imageWidth, imageHeight};
    if (prevPageRect.Contains(x, y)) {
        KeyDown(Sexy::KeyCode::KEYCODE_LEFT);
        return;
    }
}

void HelpTextScreen::ButtonDepress(int theId) {
    if (theId == 1000) {
        gLawnApp->KillHelpTextScreen();
        return;
    }
    old_HelpTextScreen_ButtonDepress(this, theId);
}

// Switch port: the Controls page (the English layer's menu/HelpMenu.menu.txt)
// is in the How To Play page's type, BrianneTod, at a size the game has no
// font for: its 32 drawn at 62% -- an ImageFont of its own, so the game's
// BrianneTod32 stays as it is. HelpTextWidget: its font at +0x104.
void HelpTextWidget_Draw(Sexy::Widget *theWidget, Sexy::Graphics *g) {
    Sexy::Font *&font = *reinterpret_cast<Sexy::Font **>(reinterpret_cast<char *>(theWidget) + 0x104);
    if (font != nullptr && font == Sexy::FONT_BRIANNETOD32) {
        static Sexy::Font *sControlsFont = [] {
            Sexy::Font *f = reinterpret_cast<Sexy::Font *(*)(Sexy::Font *)>(Sexy_ImageFont_DuplicateAddr)(Sexy::FONT_BRIANNETOD32);
            if (f != nullptr) {
                reinterpret_cast<void (*)(Sexy::Font *, double)>(Sexy_ImageFont_SetScaleAddr)(f, 0.62);
            }
            return f;
        }();
        if (sControlsFont != nullptr) {
            font = sControlsFont;
        }
    }
    old_HelpTextWidget_Draw(theWidget, g);
}
