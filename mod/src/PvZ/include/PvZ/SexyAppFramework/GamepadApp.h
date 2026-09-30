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

#ifndef PVZ_SEXYAPPFRAMEWORK_GAMEPAD_APP_H
#define PVZ_SEXYAPPFRAMEWORK_GAMEPAD_APP_H

#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/SexyAppFramework/Graphics/SharedImageRef.h"
#include "PvZ/SexyAppFramework/Widget/InputDeviceListener.h"
#include "PvZ/Symbols.h"

#include "SexyCommonApp.h"

namespace Sexy {

class GamepadApp : public SexyCommonApp, public Sexy::InputDeviceListener {
public:
    Sexy::Gamepad *mGamepads[4];
    bool mSimulateGamepadWithKeyboard; // 0x838, LAWN_SIMULATE_GAMEPAD
    bool mGamePad1IsOn;                // 0x839

    homura::Storage<CritSect> mDeviceEventCritSect;         // 0x83C
    homura::Storage<pvzstl::list<void *>> mDeviceEventList; // 0x840 ~ 0x847

    struct AtlasParserStorage {
        char unk[0x30];
    } mAtlasParser;                           // 0x848 ~ 0x877
    homura::Storage<CritSect> mAtlasCritSect; // 0x878

    // std::map<std::string, Sexy::SharedImageRef>
    homura::Storage<pvzstl::map<pvzstl::string, Sexy::SharedImageRef>> mAtlasImageMap; // 0x87C ~ 0x893

    bool mHasGamepad;   // 0x894, CheckGamepad() result
    bool mGamePad2IsOn; // 0x895, LAWN_GAMEPAD_MODE
public:
    void UpdateFrames() {
        reinterpret_cast<void (*)(GamepadApp *)>(Sexy_GamepadApp_UpdateFramesAddr)(this);
    }
    bool HasGamepad() {
        return reinterpret_cast<bool (*)(GamepadApp *)>(Sexy_GamepadApp_HasGamepadAddr)(this);
    }
    void SwapGamepadId(int a2, int a3) {
        reinterpret_cast<void (*)(GamepadApp *, int, int)>(Sexy_GamepadApp_SwapGamepadIdAddr)(this, a2, a3);
    }

protected:
    GamepadApp() = default;
    ~GamepadApp() = default;
};
} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_GAMEPAD_APP_H
