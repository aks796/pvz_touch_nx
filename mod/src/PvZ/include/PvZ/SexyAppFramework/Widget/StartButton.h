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

#ifndef PVZ_SEXYAPPFRAMEWORK_WIDGET_STARTBUTTON_H
#define PVZ_SEXYAPPFRAMEWORK_WIDGET_STARTBUTTON_H

#include "ButtonWidget.h"

class TitleScreen;


class StartButton : public Sexy::ButtonWidget {
public:
    Sexy::Color mColor;                     // 816
    Sexy::Color mOverColor;                 // 832
    TitleScreen *mParent;                   // 212 * 4
    Sexy::Font *mFont;                      // 852
    homura::Storage<pvzstl::string> mLabel; // 856
    int mGamepadIndex;                      // 215
    int mTextDrawMode;                      // 216
    float mTextAlpha;                       // 217
    int mUnderlineOffset;

public:
    void SetFont(Sexy::Font *theFont) {
        reinterpret_cast<void (*)(StartButton *, Sexy::Font *)>(StartButton_SetFontAddr)(this, theFont);
    }
}; // 大小 218 个 int


#endif // PVZ_SEXYAPPFRAMEWORK_WIDGET_STARTBUTTON_H
