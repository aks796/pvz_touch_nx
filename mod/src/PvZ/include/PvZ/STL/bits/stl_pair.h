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

#ifndef PVZ_STL_BITS_STL_PAIR_H
#define PVZ_STL_BITS_STL_PAIR_H

/**
 * @file bits/stl_pair.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00770.html">stl_pair.h File Reference</a>
 */

#include <utility>

namespace pvzstl::detail {

template <typename Tp>
constexpr bool is_pair = false;

template <typename Tp, typename Up>
constexpr bool is_pair<std::pair<Tp, Up>> = true;

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_STL_PAIR_H
