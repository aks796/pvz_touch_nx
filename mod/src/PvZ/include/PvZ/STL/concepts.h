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

#ifndef PVZ_STL_CONCEPTS_H
#define PVZ_STL_CONCEPTS_H

#include <concepts>

namespace pvzstl::detail {

// [concept.booleantestable], Boolean testability
template <typename Tp>
concept boolean_testable_impl = std::convertible_to<Tp, bool>;

template <typename Tp>
concept boolean_testable = boolean_testable_impl<Tp> && requires(Tp &&t) {
    { !static_cast<Tp &&>(t) } -> boolean_testable_impl;
};

} // namespace pvzstl::detail

#endif // PVZ_STL_CONCEPTS_H
