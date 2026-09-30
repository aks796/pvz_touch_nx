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

#ifndef PVZ_SEXYAPPFRAMEWORK_MISC_MT_RAND_H
#define PVZ_SEXYAPPFRAMEWORK_MISC_MT_RAND_H

#include "PvZ/Symbols.h"

namespace Sexy {

constexpr int MTRAND_N = 624;

class MTRand {
public:
    unsigned long mt[MTRAND_N]; /* the array for the state vector  */
    int mti;

    MTRand(unsigned long seed) {
        _costructor(seed);
    }

    int Next(unsigned long range) {
        return reinterpret_cast<int (*)(MTRand *, unsigned long)>(Sexy_MTRand_NextUlongAddr)(this, range);
    }

protected:
    void _costructor(unsigned long seed) {
        reinterpret_cast<void (*)(MTRand *, unsigned long)>(Sexy_MTRand_MTRandUlongAddr)(this, seed);
    }
};

} // namespace Sexy

#endif // PVZ_SEXYAPPFRAMEWORK_MISC_MT_RAND_H
