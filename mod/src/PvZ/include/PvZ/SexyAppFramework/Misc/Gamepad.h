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

#ifndef PVZ_SEXYAPPFRAMEWORK_MISC_GAMEPAD_H
#define PVZ_SEXYAPPFRAMEWORK_MISC_GAMEPAD_H

#include "PvZ/Symbols.h"

namespace Sexy {

class Gamepad {
public:
    bool IsButtonDown(int button) {
        return reinterpret_cast<bool (*)(Gamepad *, int)>(Sexy_Gamepad_IsButtonDownAddr)(this, button);
    }
    bool IsConnected() {
        return reinterpret_cast<bool (*)(Gamepad *)>(Sexy_Gamepad_IsConnectedAddr)(this);
    }
    float GetAxisXPosition() { // the left stick, -1 (left) .. 1
        return reinterpret_cast<float (*)(Gamepad *)>(Sexy_Gamepad_GetAxisXPositionAddr)(this);
    }
    float GetAxisYPosition() { // the left stick, -1 .. 1
        return reinterpret_cast<float (*)(Gamepad *)>(Sexy_Gamepad_GetAxisYPositionAddr)(this);
    }
    // Gamepad::mStatus (+0x19c, Gamepad::SetStatus): 0 gone, 1 there but never
    // used, 3 its sticks moved lately, 2 idle 20 s since (Gamepad::UpdateStates)
    int GetStatus() const {
        return *reinterpret_cast<const int *>(reinterpret_cast<const char *>(this) + 0x19c);
    }
};

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_MISC_GAMEPAD_H
