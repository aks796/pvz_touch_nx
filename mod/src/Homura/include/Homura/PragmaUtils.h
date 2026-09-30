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

#ifndef HOMURA_PRAGMAUTILS_H
#define HOMURA_PRAGMAUTILS_H

#define HOMURA_PRAGMA(x) _Pragma(#x)

#if defined(__GNUC__) || defined(__clang__)
// https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
#define DISABLE_WARNING_BEGIN(diag_flag) _Pragma("GCC diagnostic push") HOMURA_PRAGMA(GCC diagnostic ignored diag_flag)
#define DISABLE_WARNING_END _Pragma("GCC diagnostic pop")
#else
#define DISABLE_WARNING_BEGIN(diag_flag)
#define DISABLE_WARNING_END
#endif

#endif // HOMURA_PRAGMAUTILS_H
