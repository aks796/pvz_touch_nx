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

#ifndef PVZ_LAWN_BOARD_SHOVEL_REDIRECT_WIDGET_H
#define PVZ_LAWN_BOARD_SHOVEL_REDIRECT_WIDGET_H

#include "PvZ/SexyAppFramework/Widget/Widget.h"

class Board;

class ShovelRedirectWidget : public Sexy::Widget {
public:
    Board *mBoard;

    explicit ShovelRedirectWidget(Board *board);
    ~ShovelRedirectWidget();

    void _destructor();
    void _destructor2();

    void Draw(Sexy::Graphics *g);
    void MouseDown(int x, int y, int theClickCount);
    void MouseUp(int x, int y, int theClickCount);
    void MouseDrag(int x, int y);
};

#endif // PVZ_LAWN_BOARD_SHOVEL_REDIRECT_WIDGET_H
