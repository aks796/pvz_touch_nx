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

#ifndef PVZ_STL_BITS_ITERATOR_CONCEPTS_H
#define PVZ_STL_BITS_ITERATOR_CONCEPTS_H

/**
 * @file bits/iterator_concepts.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00872.html">iterator_concepts.h File Reference</a>
 */

#include <concepts>

namespace pvzstl::ranges::detail {

template <typename Tp>
concept cv_bool = std::same_as<const volatile Tp, const volatile bool>;

template <typename Tp>
concept integral_nonbool = std::integral<Tp> && !cv_bool<Tp>;

template <typename Tp>
concept is_integer_like = integral_nonbool<Tp>;

} // namespace pvzstl::ranges::detail

#endif // PVZ_STL_BITS_ITERATOR_CONCEPTS_H
