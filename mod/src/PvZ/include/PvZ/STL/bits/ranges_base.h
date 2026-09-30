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

#ifndef PVZ_STL_BITS_STL_RANGES_BASE_H
#define PVZ_STL_BITS_STL_RANGES_BASE_H

/**
 * @file bits/ranges_base.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00827.html">ranges_base.h File Reference</a>
 */

#include <ranges>

namespace pvzstl::detail {

template <typename Rg, typename Tp>
concept container_compatible_range = std::ranges::input_range<Rg> && std::convertible_to<std::ranges::range_reference_t<Rg>, Tp>;

template <std::ranges::input_range Rg>
using range_key_type = std::remove_cvref_t<std::tuple_element_t<0, std::ranges::range_value_t<Rg>>>;

template <std::ranges::input_range Rg>
using range_mapped_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<Rg>>>;

// The allocator's value_type for map-like containers.
template <std::ranges::input_range Rg>
using range_to_alloc_type = std::pair<const range_key_type<Rg>, range_mapped_type<Rg>>;

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_STL_RANGES_BASE_H
