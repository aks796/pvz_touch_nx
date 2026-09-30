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

#ifndef PVZ_COMMON_SYNCOBJECT_H
#define PVZ_COMMON_SYNCOBJECT_H

#include "Homura/TypeUtils.h"

#include <vector>

struct SyncBlockInfo {
    void *mAddress;
    size_t mSize;
};

class SyncObject {
public:
    void **vTable;                                           // 0
    homura::Storage<std::vector<SyncBlockInfo>> mSyncBlocks; // 0x04，大小 12 字节

protected:
    SyncObject() = default;
    ~SyncObject() = default;
};

#endif // PVZ_COMMON_SYNCOBJECT_H
