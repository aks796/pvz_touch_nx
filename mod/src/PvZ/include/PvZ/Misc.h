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

#ifndef PVZ_MISC_H
#define PVZ_MISC_H

enum VibrationEffect {
    VIBRATION_THUMP,
    VIBRATION_EXPLOSION,
    VIBRATION_BOWLING,
    VIBRATION_SLOT_MACHINE,
    VIBRATION_WHACK_HIT,
    VIBRATION_WHACK_MISS,
    VIBRATION_ICE_TRAP,
    VIBRATION_JUMP,
    VIBRATION_ZOMBIE_RISE_FROM_GRAVE,
    VIBRATION_ZOMBIE_RISE_FROM_POOL,
    VIBRATION_BUNGEE_LANDING,
    VIBRATION_BUNGEE_RISING,
    VIBRATION_BOSS_HIT
};

void TriggerVibration(VibrationEffect theVibrationEffect);

#endif // PVZ_MISC_H
