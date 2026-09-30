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

#ifndef PVZ_LAWN_WIDGET_REPLAY_CONTROLS_WIDGET_H
#define PVZ_LAWN_WIDGET_REPLAY_CONTROLS_WIDGET_H

#include "PvZ/SexyAppFramework/Widget/Widget.h"

class Board;

class ReplayControlsWidget : public Sexy::Widget {
public:
    static constexpr int kX = 0;
    static constexpr int kInitialY = -40;
    static constexpr int kWidth = 800;
    static constexpr int kHeight = 40;

    int mStartLevelTick = -2;

    explicit ReplayControlsWidget(Board *board);
    ~ReplayControlsWidget();

    void Draw(Sexy::Graphics *g);
    void MouseDown(int x, int y, int theClickCount);
    void MouseDrag(int x, int y);
    void MouseUp(int x, int y, int theClickCount);

protected:
    static bool Contains(const Sexy::Rect &rect, int x, int y);
    static void DrawButton(Sexy::Graphics *g, const Sexy::Rect &rect, const char *label);

    static Board *GetCurrentBoard();
    int GetStartLevelTick();
    static bool ShowSkipSetupButton();
    static Sexy::Rect PauseButtonRect();
    static Sexy::Rect SpeedButtonRect();
    static Sexy::Rect ForwardButtonRect();
    static Sexy::Rect RestartButtonRect();
    static Sexy::Rect SkipSetupButtonRect();

    void _destructor();
    void _destructor2();
};

#endif // PVZ_LAWN_WIDGET_REPLAY_CONTROLS_WIDGET_H
