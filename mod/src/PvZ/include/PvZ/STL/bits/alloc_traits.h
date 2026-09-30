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

#ifndef PVZ_STL_BITS_ALLOC_TRAITS_H
#define PVZ_STL_BITS_ALLOC_TRAITS_H

/**
 * @file bits/alloc_traits.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a23486.html">alloc_traits.h File Reference</a>
 */

#include <memory>

namespace pvzstl::detail {

template <typename Alloc, typename Up>
using alloc_rebind = typename std::allocator_traits<Alloc>::template rebind_alloc<Up>;

template <typename Alloc>
[[gnu::always_inline]] constexpr void alloc_on_copy(Alloc &one, const Alloc &two) {
    using traits = std::allocator_traits<Alloc>;
    using pocca = typename traits::propagate_on_container_copy_assignment::type;
    if constexpr (pocca::value) {
        one = two;
    }
}

template <typename Alloc>
[[gnu::always_inline]] constexpr Alloc alloc_on_copy(const Alloc &a) {
    using traits = std::allocator_traits<Alloc>;
    return traits::select_on_container_copy_construction(a);
}

template <typename Alloc>
[[gnu::always_inline]] constexpr void alloc_on_move(Alloc &one, Alloc &two) {
    using traits = std::allocator_traits<Alloc>;
    using pocma = typename traits::propagate_on_container_move_assignment::type;
    if constexpr (pocma::value) {
        one = std::move(two);
    }
}

template <typename Alloc>
[[gnu::always_inline]] constexpr void alloc_on_swap(Alloc &one, Alloc &two) {
    using traits = std::allocator_traits<Alloc>;
    using pocs = typename traits::propagate_on_container_swap::type;
    if constexpr (pocs::value) {
        using std::swap;
        swap(one, two);
    }
}

template <typename Alloc>
concept allocator_like = requires(Alloc &a) {
    typename Alloc::value_type;
    a.deallocate(a.allocate(1u), 1u);
};

template <typename Alloc>
concept not_allocator_like = !allocator_like<Alloc>;

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_ALLOC_TRAITS_H
