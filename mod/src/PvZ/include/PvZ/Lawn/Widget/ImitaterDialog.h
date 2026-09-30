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

#ifndef PVZ_LAWN_WIDGET_IMITATER_DIALOG_H
#define PVZ_LAWN_WIDGET_IMITATER_DIALOG_H

#include "PvZ/Lawn/Common/ConstEnums.h"

#include "LawnDialog.h"

class ImitaterDialog : public LawnDialog {
public:
    ImitaterDialog(int thePlayerIndex) {
        _constructor(thePlayerIndex);
    }

    SeedType SeedHitTest(int x, int y) {
        return reinterpret_cast<SeedType (*)(ImitaterDialog *, int, int)>(ImitaterDialog_SeedHitTestAddr)(this, x, y);
    }

    void ShowToolTip();
    bool KeyDown(Sexy::KeyCode theKey);
    void MouseDown(int x, int y, int theCount);

protected:
    friend void InitHookFunction();

    void _constructor(int thePlayerIndex);
};


inline void (*old_ImitaterDialog_ImitaterDialog)(ImitaterDialog *, int);

inline void (*old_ImitaterDialog_ShowToolTip)(ImitaterDialog *);

inline bool (*old_ImitaterDialog_KeyDown)(ImitaterDialog *, Sexy::KeyCode);

inline void (*old_ImitaterDialog_MouseDown)(ImitaterDialog *, int, int, int);

#endif // PVZ_LAWN_WIDGET_IMITATER_DIALOG_H
