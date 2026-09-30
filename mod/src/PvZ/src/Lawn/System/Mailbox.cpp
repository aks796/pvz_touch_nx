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

#include "PvZ/Lawn/System/Mailbox.h"
#include "Homura/Logger.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/PlayerInfo.h"

int Mailbox::GetNumUnseenMessages() {
    if (!mApp || !mApp->mPlayerInfo) {
        return 0;
    }

    int unseenCount = 0;
    [[maybe_unused]] int visibleMessageCount = 0;
    [[maybe_unused]] int seenCount = 0;
    [[maybe_unused]] int hiddenByLevelCount = 0;
    [[maybe_unused]] int invalidCount = 0;

    LawnPlayerInfo *playerInfo = mApp->mPlayerInfo;
    int levelGate = playerInfo->mLevel;
    if (playerInfo->GetFlag(1)) {
        levelGate = 50;
    }

    const auto *seenBitsBase = reinterpret_cast<const unsigned char *>(playerInfo) + 1875;

    for (int block = 0; block < 32; ++block) {
        for (int i = 0; i < 8; ++i) {
            const int messageIndex = i + block * 8;
            const unsigned char mask = static_cast<unsigned char>(1u << i);
            const bool seen = (seenBitsBase[block] & mask) != 0;

            int *message = reinterpret_cast<int *>(GetMessageByIndex(messageIndex, false));
            const int requiredLevel = *reinterpret_cast<int *>(reinterpret_cast<char *>(message) + 64);
            const bool valid = *reinterpret_cast<unsigned char *>(reinterpret_cast<char *>(message) + 70) != 0;

            if (!valid) {
                ++invalidCount;
            } else if (levelGate < requiredLevel) {
                ++hiddenByLevelCount;
            } else if (seen) {
                ++seenCount;
            } else {
                ++unseenCount;
            }

            if (valid && levelGate >= requiredLevel) {
                ++visibleMessageCount;
            }
        }
    }

    return unseenCount;
}
