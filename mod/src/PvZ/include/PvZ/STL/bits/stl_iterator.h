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

#ifndef PVZ_STL_BITS_STL_ITERATOR_H
#define PVZ_STL_BITS_STL_ITERATOR_H

/**
 * @file bits/stl_iterator.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a23552.html">stl_iterator.h File Reference</a>
 */

#include "PvZ/STL/bits/move.h"

#include <iterator>

namespace pvzstl::detail {

template <typename Iterator, typename ReturnType = std::conditional_t<move_if_noexcept_cond<typename std::iterator_traits<Iterator>::value_type>, Iterator, std::move_iterator<Iterator>>>
[[nodiscard]] constexpr ReturnType make_move_if_noexcept_iterator(Iterator it) {
    return ReturnType(it);
}

template <typename Tp, typename ReturnType = std::conditional_t<move_if_noexcept_cond<Tp>, const Tp *, std::move_iterator<Tp *>>>
[[nodiscard]] constexpr ReturnType make_move_if_noexcept_iterator(Tp *ptr) {
    return ReturnType(ptr);
}

template <typename InputIterator>
using iter_key_t = std::tuple_element_t<0, typename std::iterator_traits<InputIterator>::value_type>;

template <typename InputIterator>
using iter_val_t = std::tuple_element_t<1, typename std::iterator_traits<InputIterator>::value_type>;

template <typename InputIterator>
using iter_to_alloc_t = std::pair<const iter_key_t<InputIterator>, iter_val_t<InputIterator>>;

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_STL_ITERATOR_H
