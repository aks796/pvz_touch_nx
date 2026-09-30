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

#ifndef PVZ_STL_MAP_H
#define PVZ_STL_MAP_H

#include "PvZ/STL/bits/erase_if.h"
#include "PvZ/STL/bits/stl_map.h"

namespace pvzstl {

template <typename Key, typename Tp, typename Compare, typename Alloc, typename Predicate>
typename map<Key, Tp, Compare, Alloc>::size_type erase_if(map<Key, Tp, Compare, Alloc> &cont, Predicate pred) {
    return detail::erase_nodes_if(cont, cont, pred);
}

} // namespace pvzstl

#endif // PVZ_STL_MAP_H
