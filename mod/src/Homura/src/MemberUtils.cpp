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

#include "Homura/MemberUtils.h"

#include <stdexcept>

// https://itanium-cxx-abi.github.io/cxx-abi/abi.html#member-function-pointers
struct homura::detail::CppMemFuncPtr {
    void *ptr;
    std::ptrdiff_t adj;
};

void homura::detail::CheckPmfBeforeExtract(const CppMemFuncPtr *pmfPtr) {
#if defined(__GNUC__) || defined(__clang__)

#if defined(__arm__) || defined(__aarch64__)
    // Arm-Thumb: https://github.com/ARM-software/abi-aa/blob/main/cppabi32/cppabi32.rst#representation-of-pointer-to-member-function
    // AArch64: https://github.com/ARM-software/abi-aa/blob/main/cppabi64/cppabi64.rst#representation-of-pointer-to-member-function
    const bool isVirtual = pmfPtr->adj & 0x1;
    const bool isAdjusted = (pmfPtr->adj >> 1) != 0;
#else
    const bool isVirtual = std::uintptr_t(pmfPtr->ptr) & 0x1;
    const bool isAdjusted = pmfPtr->adj != 0;
#endif

    if (isVirtual) [[unlikely]] {
        throw std::logic_error{"Cannot use a virtual function to hook"};
    }
    if (isAdjusted) [[unlikely]] {
        throw std::logic_error{"Cannot use a adjusted PMF to hook"};
    }

#endif
}
