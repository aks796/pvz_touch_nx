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

#ifndef PVZ_STL_DEBUG_HELPER_FUNCTIONS_H
#define PVZ_STL_DEBUG_HELPER_FUNCTIONS_H

/**
 * @file debug/helper_functions.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a01064.html">helper_functions.h File Reference</a>
 */

#include "PvZ/STL/bits/stl_iterator_base_types.h"

namespace pvzstl_debug {

// An arbitrary iterator pointer is not singular.
template <typename Iterator>
constexpr bool check_singular(Iterator const &) {
    return false;
}

/** Non-NULL pointers are nonsingular. */
template <typename Tp>
constexpr bool check_singular(Tp *const &ptr) {
    return ptr == nullptr;
}

template <typename InputIterator>
constexpr bool valid_range_aux(InputIterator first, InputIterator last, std::input_iterator_tag) {
    // FIXME: The checks for singular iterators fail during constant eval
    // due to PR c++/85944. e.g. PR libstdc++/109517 and PR libstdc++/109976.
    if (std::is_constant_evaluated()) {
        return true;
    }

    return first == last || (!check_singular(first) && !check_singular(last));
}

template <typename InputIterator>
constexpr bool valid_range_aux(InputIterator first, InputIterator last, std::random_access_iterator_tag) {
    return valid_range_aux(first, last, std::input_iterator_tag()) && first <= last;
}

template <typename InputIterator>
constexpr bool valid_range(InputIterator first, InputIterator last) {
    return valid_range_aux(first, last, pvzstl::detail::iterator_category(first));
}

} // namespace pvzstl_debug

#endif // PVZ_STL_DEBUG_HELPER_FUNCTIONS_H
