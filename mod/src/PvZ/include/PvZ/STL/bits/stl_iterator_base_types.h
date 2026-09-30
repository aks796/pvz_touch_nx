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

#ifndef PVZ_STL_BITS_STL_ITERATOR_BASE_TYPES_H
#define PVZ_STL_BITS_STL_ITERATOR_BASE_TYPES_H

/**
 * @file bits/stl_iterator_base_types.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00779.html">stl_iterator_base_types.h File Reference</a>
 */

#include <iterator>

namespace pvzstl::detail {

template <typename Iter>
[[gnu::always_inline]] constexpr auto iterator_category(const Iter &) -> typename std::iterator_traits<Iter>::iterator_category {
    return typename std::iterator_traits<Iter>::iterator_category();
}

template <typename Iter>
using iter_category_t = typename std::iterator_traits<Iter>::iterator_category;

template <typename InIter>
concept has_input_iter_cat = std::is_convertible_v<iter_category_t<InIter>, std::input_iterator_tag>;

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_STL_ITERATOR_BASE_TYPES_H
