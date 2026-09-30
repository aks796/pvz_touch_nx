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

#ifndef HOMURA_ASSERT_H
#define HOMURA_ASSERT_H

#include <format>
#include <source_location>

namespace homura {

namespace detail {
    [[noreturn]] void AssertionFailedImpl(std::source_location location, const char *expression);
    [[noreturn]] void AssertionFailedImpl(std::source_location location, const char *expression, const char *message);
} // namespace detail

[[noreturn]] inline void AssertionFailed(std::source_location location, const char *expression) {
    detail::AssertionFailedImpl(location, expression);
}

template <typename... Args>
[[noreturn]] void AssertionFailed(std::source_location location, const char *expression, std::format_string<Args...> format, Args &&...args) {
    detail::AssertionFailedImpl(location, expression, std::vformat(format.get(), std::make_format_args(args...)).c_str());
}

} // namespace homura

#ifndef NDEBUG
#define HOMURA_ASSERT(expr, ...) (bool(expr) ? static_cast<void>(0) : homura::AssertionFailed(std::source_location::current(), "" #expr __VA_OPT__(, ) __VA_ARGS__))
#else
#define HOMURA_ASSERT(expr, ...) (static_cast<void>(0))
#endif

#endif // HOMURA_ASSERT_H
