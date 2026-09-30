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

#ifndef PVZ_STL_COMPARE_H
#define PVZ_STL_COMPARE_H

#include "PvZ/STL/concepts.h"

#include <compare>

namespace pvzstl::detail {

// [expos.only.func] synth-three-way
inline constexpr struct Synth3way {
    template <typename Tp, typename Up>
    static constexpr bool _noexcept(const Tp *t = nullptr, const Up *u = nullptr) {
        if constexpr (std::three_way_comparable_with<Tp, Up>) {
            return noexcept(*t <=> *u);
        } else {
            return noexcept(*t < *u) && noexcept(*u < *t);
        }
    }

    template <typename Tp, typename Up>
    [[nodiscard]] constexpr auto operator()(const Tp &t, const Up &u) const noexcept(_noexcept<Tp, Up>())
        requires requires {
            { t < u } -> boolean_testable;
            { u < t } -> boolean_testable;
        }
    {
        if constexpr (std::three_way_comparable_with<Tp, Up>) {
            return t <=> u;
        } else {
            if (t < u) {
                return std::weak_ordering::less;
            } else if (u < t) {
                return std::weak_ordering::greater;
            } else {
                return std::weak_ordering::equivalent;
            }
        }
    }
} synth3way{};

// [expos.only.func] synth-three-way-result
template <typename Tp, typename Up = Tp>
using synth3way_t = decltype(synth3way(std::declval<Tp &>(), std::declval<Up &>()));

} // namespace pvzstl::detail

#endif // PVZ_STL_COMPARE_H
