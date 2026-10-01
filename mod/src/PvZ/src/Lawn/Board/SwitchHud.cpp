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

#include "PvZ/Lawn/Board/SwitchHud.h"

#include "PvZ/GlobalVariable.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/SwitchPad.h"
#include "PvZ/NetPlay.h"
#include "PvZ/SexyAppFramework/Graphics/Font.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Misc/SexyMatrix.h"
#include "PvZ/Symbols.h"
#include "PvZ/SexyAppFramework/Misc/Gamepad.h"
#include "PvZ/TodLib/Common/TodCommon.h"

#include <algorithm>
#include <cmath>

namespace switchhud {
namespace {

bool gFastForward;  // this session's choice; it stays from level to level
bool gTouchTaken;   // the touch going on began on the fast forward button
int gVacuum[2];     // updates each player's sun vacuum still runs (ZL / ZR held)

// The buttons in the screen's top right corner, as Plants vs. Zombies 2 has
// its pause button: the game's own stone buttons (DrawStoneButton, the Menu
// button's), fast forward then Menu, right to left. The lawn (the board's
// 0, 0) is 240 right of the screen's left and 60 below its top; the screen
// is 1280 wide.
constexpr int kRight = 1280 - 240 - 12, kTop = -60 + 10;
constexpr int kMenuWidth = 120, kFastWidth = 96, kGap = 8;
constexpr int kHelpY = 3, kHelpPlus = 4; // the help bar sheet's cels (the Switch's buttons in the English layer)

int MenuX() {
    return kRight - kMenuWidth;
}

Sexy::Rect FastForwardStone(bool theOn) {
    const Sexy::Rect menu = switchpad::StoneRect(MenuX(), kTop, kMenuWidth, false);
    return switchpad::StoneRect(menu.mX - kGap - kFastWidth, kTop, kFastWidth, theOn);
}

bool OnTheLawn(LawnApp *theApp) {
    // not once the level is won: the award on the lawn, the fade to white
    return theApp->mBoard != nullptr && theApp->mGameScene == GameScenes::SCENE_PLAYING && theApp->mBoard->mBoardFadeOutCounter <= 0
        && !theApp->mBoard->HasLevelAwardDropped() && !gIsReplayMode && !gIsServerModeSpectator
        && theApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && theApp->mGameMode != GameMode::GAMEMODE_TREE_OF_WISDOM;
}

// The Zen Garden and the Tree of Wisdom keep their own "Main Menu" button.
bool HudTakesMenuButton(LawnApp *theApp) {
    return theApp->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN && theApp->mGameMode != GameMode::GAMEMODE_TREE_OF_WISDOM;
}

void DrawHelp(Sexy::Graphics *g, LawnApp *theApp, const Sexy::Rect &theStone, int theCel) {
    if (!theApp->GamepadMode() || Sexy::IMAGE_HELP_BUTTONS_SMALL == nullptr) {
        return;
    }
    const int cel = Sexy::IMAGE_HELP_BUTTONS_SMALL->GetCelWidth();
    g->DrawImageCel(Sexy::IMAGE_HELP_BUTTONS_SMALL, theStone.mX + theStone.mWidth - cel + 2, theStone.mY + theStone.mHeight - cel + 4, theCel);
}

using DrawStoneFn = void (*)(Sexy::Graphics *, int, int, int, int, bool, bool, const pvzstl::string &, bool);

// Fast forward on: the game's pressed stone pieces sunk a pixel deeper than a
// held button and in shadow, and its label lit where the label is when off:
// the letters in the light green of a lit button, haloed in green, over a
// faint touch of the game's own lawn-stone glow (IMAGE_BUTTON_GLOW,
// LawnStone_Button_Glow) -- the letters glow, the stone stays stone. The label
// placed as DrawStoneButton places an unpressed one, the same sums and the
// same TodDrawStringMatrix: FONT_DWARVENTODCRAFT18, left edge one pixel right
// of centred, baseline half the left piece down plus half of 7/6 of the ascent
// less one, each truncated; scaled down only when too wide (36 px spare).
void DrawFastForwardOn(Sexy::Graphics *g, DrawStoneFn theDrawStone, const Sexy::Rect &theStone, int theTick) {
    constexpr int kSink = 1; // below the engine's own pressed pixel
    g->SetColorizeImages(true);
    g->SetColor(Sexy::Color(172, 172, 184));
    theDrawStone(g, theStone.mX, theStone.mY + kSink, kFastWidth, theStone.mHeight, true, false, pvzstl::string(), false);
    g->SetColorizeImages(false);

    Sexy::Font *font = Sexy::FONT_DWARVENTODCRAFT18;
    if (font == nullptr) {
        return;
    }
    const pvzstl::string label("2x");
    const int textWidth = font->GetVTable()->StringWidth(font, label);
    const int ascent = font->GetVTable()->GetAscent(font);
    const int left = Sexy::IMAGE_BUTTON_LEFT != nullptr ? Sexy::IMAGE_BUTTON_LEFT->mHeight : theStone.mHeight;
    const float scale = textWidth > 0 ? std::min(1.0f, float(theStone.mWidth - 36) / float(textWidth)) : 1.0f;
    const int textLeft = int(float(theStone.mX) + 1.0f + 0.5f * (float(theStone.mWidth) - float(textWidth) * scale));
    const int baseline = int(float(theStone.mY) + float(left / 2) + 0.5f * scale * float(ascent + ascent / 6 - 1));
    auto drawLabel = [&](int dx, int dy, const Sexy::Color &theColor) {
        Sexy::SexyMatrix3 matrix{};
        TodScaleTransformMatrix(matrix, float(textLeft + dx) + g->mTransX, float(baseline + dy) + g->mTransY, scale * g->mScaleX, scale * g->mScaleY);
        TodDrawStringMatrix(g, font, matrix, label, theColor);
    };
    const float pulse = 0.8f + 0.2f * (0.5f - 0.5f * std::cos(float(theTick % 120) * 6.2831853f / 120.0f));

    g->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
    if (Sexy::Image *glow = Sexy::IMAGE_BUTTON_GLOW) { // behind the letters' middle
        const int w = std::min(theStone.mWidth - 16, int(float(textWidth) * scale) + 16), h = 22;
        const int cx = textLeft + int(float(textWidth) * scale) / 2, cy = baseline - int(float(ascent) * scale) / 3;
        g->SetColorizeImages(true);
        g->SetColor(Sexy::Color(150, 255, 90, int(110.0f * pulse)));
        g->DrawImage(glow, Sexy::Rect(cx - w / 2, cy - h / 2, w, h), Sexy::Rect(0, 0, glow->mWidth, glow->mHeight));
        g->SetColorizeImages(false);
    }
    static const int kDirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    static const int kRings[3][2] = {{3, 34}, {2, 60}, {1, 105}}; // radius, alpha: fainter outwards
    for (const auto &ring : kRings) { // the letters' own halo
        const Sexy::Color halo(70, 255, 40, int(float(ring[1]) * pulse));
        for (const auto &d : kDirs) {
            drawLabel(d[0] * ring[0], d[1] * ring[0], halo);
        }
    }
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
    drawLabel(0, 0, Sexy::Color(200, 255, 140));
}

} // namespace

bool FastForwardAllowed(LawnApp *theApp) {
    return OnTheLawn(theApp) && !theApp->IsVSMode() && !IsOnlineModeActive();
}

bool FastForwardOn(LawnApp *theApp) {
    return gFastForward && FastForwardAllowed(theApp);
}

void ToggleFastForward(LawnApp *theApp) {
    gFastForward = !gFastForward;
    theApp->PlaySample(gFastForward ? Sexy::SOUND_BUTTONCLICK : Sexy::SOUND_TAP);
}

bool XboxSunCollecting(LawnApp *theApp) {
    return theApp->GamepadMode() && !IsOnlineModeActive() && !gIsReplayMode && !gIsServerModeSpectator;
}

void SunTrigger(GamepadControls *theControls) {
    LawnApp *app = theControls->mApp;
    if (XboxSunCollecting(app) && OnTheLawn(app)) {
        gVacuum[theControls->mPlayerIndex & 1] = 15; // VacuumUpdate keeps it going while held
        return;
    }
    // online as the game has it: every sun on the lawn at once (for a zombie
    // player, the brains) -- the engine's L3
    old_GamepadControls_OnButtonDown(theControls, Sexy::GamepadButton::GAMEPAD_BUTTON_THUMBL, theControls->mGamepadIndex, 0);
}

void VacuumUpdate(GamepadControls *theControls) {
    int &left = gVacuum[theControls->mPlayerIndex & 1];
    if (left <= 0) {
        return;
    }
    LawnApp *app = theControls->mApp;
    Board *board = theControls->mBoard;
    const int pad = theControls->mGamepadIndex;
    Sexy::Gamepad *gamepad = pad >= 0 && pad < 4 ? app->mGamepads[pad] : nullptr;
    const bool held = gamepad != nullptr
        && (gamepad->IsButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_TL2) || gamepad->IsButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_TR2));
    left = held ? 15 : left - 1; // a press pulls for a moment at least
    if (board == nullptr || !OnTheLawn(app) || !XboxSunCollecting(app)) {
        left = 0;
        return;
    }
    // every sun and coin on the lawn drawn to the cursor; in VS a side's own: the plants'
    // sun, the zombies' brains (as the engine's cursor picks them up). Each goes the way a
    // sun near the cursor goes anyway -- the game's own pull (COIN_MOTION_FROM_NEAR_CURSOR,
    // Coin::UpdateFallForAward), from rest, faster and faster, collected at the cursor --
    // only harder while the vacuum runs (VacuumPulling). One motion all the way in: it used to
    // be flung most of the way, then handed to that pull from rest near the cursor, which
    // showed as a jump and a slow crawl (hardware, 2026-10-01).
    const bool vs = app->IsVSMode();
    const int player = theControls->mPlayerIndex;
    Coin *coin = nullptr;
    while (board->IterateCoins(coin)) {
        if (coin->mDead || coin->mIsBeingCollected || coin->IsLevelAward()) {
            continue;
        }
        const bool pulled = vs ? (theControls->mIsZombie ? coin->IsDeath() : coin->IsSun()) : (coin->IsSun() || coin->IsMoney());
        if (!pulled || (coin->mCoinMotion == CoinMotion::COIN_MOTION_FROM_NEAR_CURSOR && coin->mPlayerIndex == player)) {
            continue;
        }
        // as Coin::GamepadCursorOver takes one: not co-op's double sun, nor a sun still growing
        // out of its sunflower (it comes once grown)
        if (coin->mType == CoinType::COIN_COOP_DOUBLE_SUN || (coin->IsSun() && coin->mScale < coin->GetSunScale())) {
            continue;
        }
        coin->mCoinMotion = CoinMotion::COIN_MOTION_FROM_NEAR_CURSOR;
        coin->mPlayerIndex = player;
        coin->unk2 = 0.0f; // its speed toward the cursor
        if (coin->IsSun()) {
            coin->mScale = coin->GetSunScale();
        }
    }
}

bool VacuumPulling(int thePlayerIndex) {
    return gVacuum[thePlayerIndex & 1] > 0;
}

void PlaceMenuButton(Board *theBoard) {
    GameButton *menu = theBoard->mBoardMenuButton;
    if (menu == nullptr || !HudTakesMenuButton(theBoard->mApp)) {
        return;
    }
    menu->Resize(MenuX(), kTop, kMenuWidth, 60); // the game's Menu button, in the corner
}

void Draw(Board *theBoard, Sexy::Graphics *g) {
    LawnApp *app = theBoard->mApp;
    if (FastForwardAllowed(app)) {
        const Sexy::Rect stone = FastForwardStone(false);
        auto drawStone = reinterpret_cast<DrawStoneFn>(DrawStoneButtonAddr);
        if (drawStone != nullptr) {
            if (FastForwardOn(app)) {
                DrawFastForwardOn(g, drawStone, stone, theBoard->mMainCounter);
            } else {
                drawStone(g, stone.mX, stone.mY, kFastWidth, stone.mHeight, false, false, pvzstl::string("2x"), false);
            }
        }
        DrawHelp(g, app, stone, kHelpY);
    }
}

void DrawOnButton(GameButton *theButton, Sexy::Graphics *g) {
    LawnApp *app = gLawnApp;
    Board *board = app != nullptr ? app->mBoard : nullptr;
    if (board == nullptr || theButton != board->mBoardMenuButton || !HudTakesMenuButton(app) || theButton->mBtnNoDraw || !theButton->mVisible
        || theButton->mX != MenuX() || theButton->mY != kTop) {
        return;
    }
    // the + at the stone's foot, over the button as the Y is over fast
    // forward (the button draws after the lawn: a child widget); it reaches
    // past the button's edge, so not clipped to it
    Sexy::Rect stone = switchpad::StoneRect(MenuX(), kTop, kMenuWidth, false);
    stone.mX -= theButton->mX;
    stone.mY -= theButton->mY;
    g->PushState();
    g->ClearClipRect();
    g->SetColorizeImages(false);
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
    DrawHelp(g, app, stone, kHelpPlus);
    g->PopState();
}

void GameButton_Draw(GameButton *theButton, Sexy::Graphics *g) {
    old_GameButton_Draw(theButton, g);
    DrawOnButton(theButton, g);
}

bool MouseDown(Board *theBoard, int x, int y) {
    LawnApp *app = theBoard->mApp;
    if (!FastForwardAllowed(app) || !FastForwardStone(false).Contains(x, y)) {
        return false;
    }
    ToggleFastForward(app);
    gTouchTaken = true;
    return true;
}

bool TouchTaken(bool theEnds) {
    const bool taken = gTouchTaken;
    if (theEnds) {
        gTouchTaken = false;
    }
    return taken;
}

} // namespace switchhud
