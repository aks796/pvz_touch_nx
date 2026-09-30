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

#ifndef PVZ_STL_LIST_H
#define PVZ_STL_LIST_H

#include "PvZ/STL/bits/stl_list.h"

namespace pvzstl {

template <typename Tp, typename Alloc, typename Predicate>
typename list<Tp, Alloc>::size_type erase_if(list<Tp, Alloc> &cont, Predicate pred) {
    return cont.remove_if(pred);
}

template <typename Tp, typename Alloc, typename Up>
typename list<Tp, Alloc>::size_type erase(list<Tp, Alloc> &cont, const Up &value) {
    return erase_if(cont, [&](const auto &elem) { return elem == value; });
}

} // namespace pvzstl

#endif // PVZ_STL_LIST_H
