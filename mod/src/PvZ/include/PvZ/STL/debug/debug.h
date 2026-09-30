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

#ifndef PVZ_STL_DEBUG_DEBUG_H
#define PVZ_STL_DEBUG_DEBUG_H

/**
 * @file debug/debug.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a01073.html">debug.h File Reference</a>
 */

#include "PvZ/STL/debug/assertions.h"
#include "PvZ/STL/debug/helper_functions.h"

#define PVZSTL_CXX_REQUIRES_VALID_RANGE(first, last) assert(pvzstl_debug::valid_range(first, last))
#define PVZSTL_CXX_REQUIRES_STRING(string) assert(string != nullptr)
#define PVZSTL_CXX_REQUIRES_STRING_LEN(string, len) assert(string != nullptr || len == 0)

#endif // PVZ_STL_DEBUG_DEBUG_H
