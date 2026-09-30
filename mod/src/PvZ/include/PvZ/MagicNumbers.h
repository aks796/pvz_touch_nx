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

#ifndef PVZ_MAGICNUMBERS_H
#define PVZ_MAGICNUMBERS_H

/**
 * @file 此文件主要记录 v1.1.1 和 v1.1.5 中不同的内容.
 */

#include <cstdint>

#if PVZ_VERSION == 111
inline constexpr uintptr_t BOARD_UPDATE_ADDR_RELATIVE = 0x1669E4;
inline constexpr uintptr_t ZOMBIE_ISTANGLEKELPTARGET_ADDR_RELATIVE = 0x1AA288;
inline constexpr uintptr_t ZOMBIE_ISTANGLEKELPTARGET2_ADDR_RELATIVE = 0x1ACCE8;
inline constexpr uintptr_t J_AUDIOWRITE_ADDR_RELATIVE = 0x524E28;
inline constexpr uintptr_t PVZSTL_STRING_EMPTY_REP = 0x7185BC;
inline constexpr uintptr_t PVZSTL_WSTRING_EMPTY_REP = 0x69AEBC;
#elif PVZ_VERSION == 115
inline constexpr uintptr_t BOARD_UPDATE_ADDR_RELATIVE = 0x164A88;
inline constexpr uintptr_t ZOMBIE_ISTANGLEKELPTARGET_ADDR_RELATIVE = 0x1AAD98;  // 符号重复，无法dlsym获取地址，只好常量封装
inline constexpr uintptr_t ZOMBIE_ISTANGLEKELPTARGET2_ADDR_RELATIVE = 0x1A8338; // 符号重复，无法dlsym获取地址，只好常量封装
inline constexpr uintptr_t J_AUDIOWRITE_ADDR_RELATIVE = 0x12E6C4;
inline constexpr uintptr_t PVZSTL_STRING_EMPTY_REP = 0x71BB54;
inline constexpr uintptr_t PVZSTL_WSTRING_EMPTY_REP = 0x69E45C;
#endif // PVZ_VERSION

#endif // PVZ_MAGICNUMBERS_H
