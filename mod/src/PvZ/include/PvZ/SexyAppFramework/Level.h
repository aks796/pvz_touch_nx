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

#ifndef PVZ_SEXYAPPFRAMEWORK_LEVEL_H
#define PVZ_SEXYAPPFRAMEWORK_LEVEL_H

#include "Homura/TypeUtils.h"
#include "PvZ/STL/map.h"
#include "PvZ/STL/string.h"

#include <vector>

struct LevelPlantInfo {
    int mX;       // +0x00
    int mY;       // +0x04
    int mPlantId; // +0x08
};
static_assert(sizeof(LevelPlantInfo) == 0x0C);

namespace Sexy {

class _Wave {};

class Level {
public:
    int unk_00; // +0x00 = -1
    int unk_04; // +0x04 = -1
    int unk_08; // +0x08 = -1

    std::vector<void *> unkVector_0C; // +0x0C，元素类型未知

    int unk_18; // +0x18 = 0

    std::vector<LevelPlantInfo> mPlants; // +0x1C

    std::vector<int> mForbiddenSeeds; // +0x28
    // 同时被 isCardNotAllowedToPick 和 isSeedForbidden 使用

    std::vector<void *> unkVector_34; // +0x34，元素类型未知

    int mSeedBankLimit; // +0x40

    std::vector<int> mSeedConfig; // +0x44，

    int unk_50;         // +0x50 = 0
    int mFlagWaveCount; // +0x54 = 0

    bool unk_58;         // +0x58 = true
    bool pad_59[3];      // +0x59
    unsigned int unk_5C; // +0x5C，构造函数未初始化

    std::vector<void *> unkVector_60; // +0x60，元素类型未知

    pvzstl::map<int, Sexy::_Wave> mWaves; // +0x6C

    pvzstl::string mStringStorage; // +0x84，旧版32位 std::string
};

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_LEVEL_H
