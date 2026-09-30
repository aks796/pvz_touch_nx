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

#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Graphics/Image.h"
#include "PvZ/Symbols.h"
#include "PvZ/SexyAppFramework/Misc/Gamepad.h"
#include "PvZ/SexyAppFramework/Misc/GamepadButtons.h"
#include "PvZ/TodLib/Common/TodStringFile.h"

#include <algorithm>
#include <cmath>

namespace switchpad {

namespace {
constexpr int kRepeatDelay = 35; // updates (100 a second) before a held direction repeats
constexpr int kRepeatEvery = 9;
} // namespace

Sexy::KeyCode AsArrowKey(Sexy::KeyCode theKey) {
    switch (theKey) {
        case Sexy::KEYCODE_GAMEPAD_UP:
        case Sexy::KEYCODE_GAMEPAD_DPAD_UP:
            return Sexy::KEYCODE_UP;
        case Sexy::KEYCODE_GAMEPAD_DOWN:
        case Sexy::KEYCODE_GAMEPAD_DPAD_DOWN:
            return Sexy::KEYCODE_DOWN;
        case Sexy::KEYCODE_GAMEPAD_LEFT:
        case Sexy::KEYCODE_GAMEPAD_DPAD_LEFT:
            return Sexy::KEYCODE_LEFT;
        case Sexy::KEYCODE_GAMEPAD_RIGHT:
        case Sexy::KEYCODE_GAMEPAD_DPAD_RIGHT:
            return Sexy::KEYCODE_RIGHT;
        default:
            return theKey;
    }
}

int StickHeldVertical(LawnApp *theApp) {
    for (Sexy::Gamepad *pad : theApp->mGamepads) {
        if (pad == nullptr) {
            continue;
        }
        if (pad->IsButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_UP)) {
            return -1;
        }
        if (pad->IsButtonDown(Sexy::GamepadButton::GAMEPAD_BUTTON_DOWN)) {
            return 1;
        }
    }
    return 0;
}

pvzstl::string Label(const char *theSwitchKey, const char *theFallbackKey) {
    pvzstl::string text = TodStringTranslate(theSwitchKey);
    if (text.empty() || text[0] == '<' || text[0] == '[') { // not in this language's table
        text = TodStringTranslate(theFallbackKey);
    }
    return text;
}

namespace {
// a rounded rectangle's outline (corner radius theRadius), a pixel wide
void RoundedOutline(Sexy::Graphics *g, const Sexy::Rect &r, int theRadius) {
    const int rad = std::min(theRadius, std::min(r.mWidth, r.mHeight) / 2);
    g->FillRect(Sexy::Rect(r.mX + rad, r.mY, r.mWidth - 2 * rad, 1));
    g->FillRect(Sexy::Rect(r.mX + rad, r.mY + r.mHeight - 1, r.mWidth - 2 * rad, 1));
    g->FillRect(Sexy::Rect(r.mX, r.mY + rad, 1, r.mHeight - 2 * rad));
    g->FillRect(Sexy::Rect(r.mX + r.mWidth - 1, r.mY + rad, 1, r.mHeight - 2 * rad));
    for (int i = 0; i < rad; ++i) { // the corners, a quarter circle each
        const int j = rad - int(std::sqrt(float(rad * rad - (rad - i) * (rad - i))) + 0.5f);
        g->FillRect(Sexy::Rect(r.mX + i, r.mY + j, 1, 1));
        g->FillRect(Sexy::Rect(r.mX + r.mWidth - 1 - i, r.mY + j, 1, 1));
        g->FillRect(Sexy::Rect(r.mX + i, r.mY + r.mHeight - 1 - j, 1, 1));
        g->FillRect(Sexy::Rect(r.mX + r.mWidth - 1 - i, r.mY + r.mHeight - 1 - j, 1, 1));
        g->FillRect(Sexy::Rect(r.mX + j, r.mY + i, 1, 1));
        g->FillRect(Sexy::Rect(r.mX + r.mWidth - 1 - j, r.mY + i, 1, 1));
        g->FillRect(Sexy::Rect(r.mX + j, r.mY + r.mHeight - 1 - i, 1, 1));
        g->FillRect(Sexy::Rect(r.mX + r.mWidth - 1 - j, r.mY + r.mHeight - 1 - i, 1, 1));
    }
}

float Breath(int theTick, int thePeriod) {
    return 0.5f - 0.5f * std::cos(float(theTick % thePeriod) * 6.2831853f / thePeriod);
}
} // namespace

void DrawPurpleGlow(Sexy::Graphics *g, const Sexy::Rect &theRect, int theTick) {
    const float pulse = Breath(theTick, 90);
    const Sexy::Rect &r = theRect;
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
    for (int k = 1; k <= 3; ++k) { // a close, soft halo
        g->SetColor(Sexy::Color(200, 100, 255, int((60.0f + 60.0f * pulse) * float(4 - k) / 3.0f)));
        RoundedOutline(g, Sexy::Rect(r.mX - k, r.mY - k, r.mWidth + 2 * k, r.mHeight + 2 * k), 8 + k);
    }
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
    g->SetColor(Sexy::Color(232, 164, 255, int(190.0f + 65.0f * pulse))); // the rim, hugging the button
    RoundedOutline(g, r, 8);
    RoundedOutline(g, Sexy::Rect(r.mX + 1, r.mY + 1, r.mWidth - 2, r.mHeight - 2), 7);
}

void DrawWarmGlow(Sexy::Graphics *g, const Sexy::Rect &theRect, int theTick) {
    const float pulse = Breath(theTick, 80);
    const Sexy::Rect &r = theRect;
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
    for (int k = 1; k <= 6; ++k) { // a soft halo, fading outwards
        g->SetColor(Sexy::Color(255, 226, 120, int((14.0f + 26.0f * pulse) * float(7 - k) / 6.0f)));
        g->DrawRect(Sexy::Rect(r.mX - k, r.mY - k, r.mWidth + 2 * k - 1, r.mHeight + 2 * k - 1));
    }
    g->SetColor(Sexy::Color(255, 240, 190, int(24.0f + 56.0f * pulse))); // the thing itself lit
    g->FillRect(r);
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
}

namespace {
struct StonePieces {
    Sexy::Image *left, *middle, *right;
    int x, y, count; // where the left end goes; how many middle pieces
};

bool StoneLayout(int x, int y, int theWidth, bool theDown, StonePieces &out) {
    out.left = theDown ? Sexy::IMAGE_BUTTON_DOWN_LEFT : Sexy::IMAGE_BUTTON_LEFT;
    out.middle = theDown ? Sexy::IMAGE_BUTTON_DOWN_MIDDLE : Sexy::IMAGE_BUTTON_MIDDLE;
    out.right = theDown ? Sexy::IMAGE_BUTTON_DOWN_RIGHT : Sexy::IMAGE_BUTTON_RIGHT;
    if (out.left == nullptr || out.middle == nullptr || out.right == nullptr || out.middle->mWidth <= 0) {
        return false;
    }
    // the engine's own sum: the middle pieces that fit best, rounded
    const float n = float(theWidth - out.left->mWidth - out.right->mWidth) / float(out.middle->mWidth);
    out.count = std::max(0, int(n > 0.0f ? n + 0.5f : n - 0.5f));
    out.x = x + (theDown ? 1 : 0);
    out.y = y + (theDown ? 1 : 0);
    return true;
}

void DrawStonePieces(Sexy::Graphics *g, const StonePieces &p, int ox, int oy) {
    int x = p.x + ox;
    g->DrawImage(p.left, x, p.y + oy);
    x += p.left->mWidth;
    for (int i = 0; i < p.count; ++i, x += p.middle->mWidth) {
        g->DrawImage(p.middle, x, p.y + oy);
    }
    g->DrawImage(p.right, x, p.y + oy);
}
} // namespace

Sexy::Rect StoneRect(int x, int y, int theWidth, bool theDown) {
    StonePieces p;
    if (!StoneLayout(x, y, theWidth, theDown, p)) {
        return Sexy::Rect(x, y, theWidth, 40);
    }
    return Sexy::Rect(p.x, p.y, p.left->mWidth + p.count * p.middle->mWidth + p.right->mWidth, p.left->mHeight);
}

void DrawStoneGlow(Sexy::Graphics *g, int x, int y, int theWidth, bool theDown, int theTick) {
    StonePieces p;
    if (!StoneLayout(x, y, theWidth, theDown, p)) {
        return;
    }
    const float pulse = Breath(theTick, 90);
    static const int kDirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    g->SetColorizeImages(true);
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
    for (int ring = 6; ring >= 2; ring -= 2) { // the outer rings fainter: a glow, then a rim
        const int alpha = int((ring == 6 ? 45.0f : ring == 4 ? 85.0f : 150.0f) * (0.7f + 0.3f * pulse));
        g->SetColor(Sexy::Color(214, 110, 255, alpha));
        for (const auto &d : kDirs) {
            const int step = d[0] != 0 && d[1] != 0 ? ring * 7 / 10 : ring;
            DrawStonePieces(g, p, d[0] * step, d[1] * step);
        }
    }
    g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
    g->SetColorizeImages(false);
}

int Repeater::Step(int theHeld) {
    if (theHeld != mDir) {
        mDir = theHeld;
        mTicks = 0;
        return 0;
    }
    if (mDir == 0) {
        return 0;
    }
    ++mTicks;
    return mTicks >= kRepeatDelay && (mTicks - kRepeatDelay) % kRepeatEvery == 0 ? mDir : 0;
}

} // namespace switchpad
