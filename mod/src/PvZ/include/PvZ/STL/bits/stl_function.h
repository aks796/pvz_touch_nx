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

#ifndef PVZ_STL_BITS_STL_FUNCTION_H
#define PVZ_STL_BITS_STL_FUNCTION_H

/**
 * @file bits/stl_function.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00788.html">stl_function.h File Reference</a>
 */

#include <type_traits>

namespace pvzstl {

/**
 *  Helper for defining adaptable unary function objects.
 *  @deprecated Deprecated in C++11, no longer in the standard since C++17.
 */
template <typename Arg, typename Result>
struct unary_function {
    using argument_type = Arg;
    using result_type = Result;
};

/**
 *  Helper for defining adaptable binary function objects.
 *  @deprecated Deprecated in C++11, no longer in the standard since C++17.
 */
template <typename Arg1, typename Arg2, typename Result>
struct binary_function {
    using first_argument_type = Arg1;
    using second_argument_type = Arg2;
    using result_type = Result;
};

namespace detail {

    template <typename Tp>
    struct Identity : public unary_function<Tp, Tp> {
        Tp &operator()(Tp &x) const {
            return x;
        }

        const Tp &operator()(const Tp &x) const {
            return x;
        }
    };

    // Partial specialization, avoids confusing errors in e.g. std::set<const T>.
    template <typename _Tp>
    struct Identity<const _Tp> : Identity<_Tp> {};

    template <typename Pair>
    struct Select1st : unary_function<Pair, typename Pair::first_type> {
        typename Pair::first_type &operator()(Pair &x) const {
            return x.first;
        }

        const typename Pair::first_type &operator()(const Pair &x) const {
            return x.first;
        }

        template <typename Pair2>
        typename Pair2::first_type &operator()(Pair2 &x) const {
            return x.first;
        }

        template <typename Pair2>
        const typename Pair2::first_type &operator()(const Pair2 &x) const {
            return x.first;
        }
    };

    template <typename Func>
    concept transparent_comparator = requires { typename Func::is_transparent; };

    template <typename Kt, typename Container>
    concept not_container_iterator = !std::is_convertible_v<Kt &&, typename Container::iterator> && !std::is_convertible_v<Kt &&, typename Container::const_iterator>;

    template <typename Kt, typename Container>
    concept heterogeneous_key = !std::is_same_v<typename Container::key_type, std::remove_cvref_t<Kt>> && not_container_iterator<Kt, Container>;

} // namespace detail

} // namespace pvzstl

#endif // PVZ_STL_BITS_STL_FUNCTION_H
