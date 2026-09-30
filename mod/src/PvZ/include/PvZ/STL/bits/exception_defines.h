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

#ifndef PVZ_STL_BITS_EXCEPTION_DEFINES_H
#define PVZ_STL_BITS_EXCEPTION_DEFINES_H

/**
 * @file bits/exception_defines.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00365.html">exception_defines.h File Reference</a>
 */

#if !__cpp_exceptions
// Iff -fno-exceptions, transform error handling code to work without it.
#define PVZSTL_TRY if (true)
#define PVZSTL_CATCH(x) if (false)
#define PVZSTL_THROW_EXCEPTION_AGAIN
#else
// Else proceed normally.
#define PVZSTL_TRY try
#define PVZSTL_CATCH(x) catch (x)
#define PVZSTL_THROW_EXCEPTION_AGAIN throw
#endif

#endif // PVZ_STL_BITS_EXCEPTION_DEFINES_H
