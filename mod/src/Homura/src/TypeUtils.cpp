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

#include "Homura/TypeUtils.h"

// just like 'BOOST_CORE_HAS_CXXABI_H' defined in <boost/core/demangle.hpp>
#if defined(__has_include) && (defined(__clang__) || !defined(__GNUC__) || (__GNUC__ + 0) >= 5)
#if __has_include(<cxxabi.h>)
#define HOMURA_HAS_CXXABI_H
#endif
#elif defined(__GLIBCXX__) || defined(__GLIBCPP__)
#define HOMURA_HAS_CXXABI_H
#endif

#ifdef HOMURA_HAS_CXXABI_H
#include <cxxabi.h>
#endif

#include <cassert>
#include <cstdlib>

#include <memory>

std::string homura::Demangle(const char *name) {
    assert(name != nullptr);
#ifdef HOMURA_HAS_CXXABI_H
    // https://itanium-cxx-abi.github.io/cxx-abi/abi.html#demangler
    std::unique_ptr<char, decltype(&std::free)> res{abi::__cxa_demangle(name, nullptr, nullptr, nullptr), std::free};
    return res ? res.get() : name;
#else
    return name;
#endif
}
