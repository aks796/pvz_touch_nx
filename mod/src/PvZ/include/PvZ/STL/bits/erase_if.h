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

#ifndef PVZ_STL_BITS_ERASE_IF_H
#define PVZ_STL_BITS_ERASE_IF_H

/**
 * @file bits/erase_if.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00680.html">erase_if.h File Reference</a>
 */

#include <algorithm>

namespace pvzstl::detail {

template <typename Container, typename UnsafeContainer, typename Predicate>
constexpr typename Container::size_type erase_if(Container &cont, UnsafeContainer &ucont, Predicate pred) {
    const auto osz = ucont.size();
    const auto end = ucont.end();
    auto removed = std::remove_if(ucont.begin(), end, std::move(pred));
    if (removed != end) {
        cont.erase(removed, cont.end());
        return osz - ucont.size();
    }

    return 0;
}

template <typename Container, typename UnsafeContainer, typename Predicate>
typename Container::size_type erase_nodes_if(Container &cont, UnsafeContainer &ucont, Predicate pred) {
    typename Container::size_type num = 0;
    for (auto it = ucont.begin(), last = ucont.end(); it != last;) {
        if (pred(*it)) {
            it = cont.erase(it);
            ++num;
        } else {
            ++it;
        }
    }
    return num;
}

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_ERASE_IF_H
