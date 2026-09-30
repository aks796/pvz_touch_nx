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

#ifndef PVZ_SEXYAPPFRAMEWORK_WIDGET_EDIT_WIDGET_H
#define PVZ_SEXYAPPFRAMEWORK_WIDGET_EDIT_WIDGET_H

#include "PvZ/STL/string.h"

#include "Widget.h"

namespace Sexy {

class Font;
class EditListener;

class EditWidget : public Widget {
public:
    int SetEditText(const pvzstl::string &theText) {
        return reinterpret_cast<int (*)(EditWidget *, const pvzstl::string &)>(Sexy_EditWidget_SetEditTextAddr)(this, theText);
    }

    int ProcessKey(KeyCode theKey, int theChar);
};

} // namespace Sexy

inline int (*old_Sexy_EditWidget_ProcessKey)(Sexy::EditWidget *self, Sexy::KeyCode theKey, int theChar);

#endif // PVZ_SEXYAPPFRAMEWORK_WIDGET_EDIT_WIDGET_H
