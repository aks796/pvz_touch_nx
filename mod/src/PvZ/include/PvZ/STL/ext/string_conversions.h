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

#ifndef PVZ_STL_EXT_STRING_CONVERSIONS_H
#define PVZ_STL_EXT_STRING_CONVERSIONS_H

/**
 * @file ext/string_conversions.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a01277.html">string_conversions.h File Reference</a>
 */

#include "PvZ/STL/bits/stdexcept_throw.h"

#include <cerrno>
#include <cstdarg>
#include <cstddef>

#include <limits>

namespace pvzstl_cxx {

// Helper for all the sto* functions.
template <typename TRet, typename Ret = TRet, typename CharT, typename... Base>
Ret stoa(TRet (*convf)(const CharT *, CharT **, Base...), const char *name, const CharT *str, std::size_t *idx, Base... base) {
    Ret ret;

    CharT *endptr;

    struct save_errno {
        save_errno()
            : m_errno(errno) {
            errno = 0;
        }
        ~save_errno() {
            if (errno == 0)
                errno = m_errno;
        }
        int m_errno;
    } const save_errno;

    struct range_chk {
        static bool chk(TRet, std::false_type) {
            return false;
        }

        static bool chk(TRet val, std::true_type) // only called when Ret is int
        {
            return val < TRet(std::numeric_limits<int>::min()) || val > TRet(std::numeric_limits<int>::max());
        }
    };

    const TRet tmp = convf(str, &endptr, base...);

    if (endptr == str) {
        pvzstl::detail::throw_invalid_argument(name);
    } else if (errno == ERANGE || range_chk::chk(tmp, std::is_same<Ret, int>{})) {
        pvzstl::detail::throw_out_of_range(name);
    } else {
        ret = tmp;
    }

    if (idx != nullptr) {
        *idx = endptr - str;
    }

    return ret;
}

// Helper for the to_string / to_wstring functions.
template <typename String, std::size_t N, typename CharT = typename String::value_type>
String to_xstring(int (*convf)(CharT *, std::size_t, const CharT *, std::va_list), const CharT *fmt, ...) {
    // XXX Eventually the result should be constructed in-place in
    // the __cxx11 string, likely with the help of internal hooks.
    CharT buf[N];

    std::va_list args;
    va_start(args, fmt);

    const int len = convf(buf, N, fmt, args);

    va_end(args);

    return String(buf, len);
}

} // namespace pvzstl_cxx

#endif // PVZ_STL_EXT_STRING_CONVERSIONS_H
