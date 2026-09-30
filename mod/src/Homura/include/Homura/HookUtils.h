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

#ifndef HOMURA_HOOKUTILS_H
#define HOMURA_HOOKUTILS_H

#include "Homura/MemberUtils.h"

#include <cstddef>
#include <cstdint>

#include <string_view>
#include <type_traits>

namespace homura {

namespace detail {
    void HookFuncImpl(void *symbol, void *newFunc, void **oldFuncAddr);
    bool HookVirtualFuncImpl(void *vTableSymbol, std::size_t index, void *newFunc, void **oldFuncAddr);
    bool HookPltFuncImpl(std::string_view libName, std::uintptr_t offset, void *newFunc, void **oldFuncAddr);
} // namespace detail

/**
 * @brief 替换全局函数/静态成员函数.
 *
 * @param [in] symbol 通过 dlsym 函数获取的函数符号地址.
 * @param [in] newFunc 用于替换的新函数.
 * @param [out] oldFuncAddr 一个函数指针的地址, 用于保留旧函数. (可传入 nullptr 字面量, 代表不保留)
 */
template <typename T>
    requires std::is_function_v<T>
void HookFunc(void *symbol, T *newFunc, std::type_identity_t<T **> oldFuncAddr) {
    detail::HookFuncImpl(symbol, reinterpret_cast<void *>(newFunc), reinterpret_cast<void **>(oldFuncAddr));
}

/**
 * @brief 替换非静态成员函数.
 *
 * @param [in] symbol 通过 dlsym 函数获取的函数符号地址.
 * @param [in] newFunc 用于替换的新函数.
 * @param [out] oldFuncAddr 一个函数指针的地址, 用于保留旧函数. (可传入 nullptr 字面量, 代表不保留)
 */
template <typename T>
    requires(std::is_member_function_pointer_v<T> && std::is_same_v<T, std::remove_cv_t<T>>)
void HookFunc(void *symbol, T newFunc, ExtractMemFuncPtrType<T> *oldFuncAddr) {
    detail::HookFuncImpl(symbol, reinterpret_cast<void *>(ExtractMemFuncPtr(newFunc)), reinterpret_cast<void **>(oldFuncAddr));
}

/**
 * @brief 替换虚函数.
 *
 * @param [in] vTableSymbol 通过 dlsym 函数获取的虚函数表符号地址.
 * @param [in] index 目标函数在虚函数表中的索引.
 * @param [in] newFunc 用于替换的新函数.
 * @param [out] oldFuncAddr 一个函数指针的地址, 用于保留旧函数. (可传入 nullptr 字面量, 代表不保留)
 *
 * @return 是否成功替换.
 */
template <typename T>
    requires std::is_function_v<T>
bool HookVirtualFunc(void *vTableSymbol, std::size_t index, T *newFunc, std::type_identity_t<T **> oldFuncAddr) {
    return detail::HookVirtualFuncImpl(vTableSymbol, index, reinterpret_cast<void *>(newFunc), reinterpret_cast<void **>(oldFuncAddr));
}

/**
 * @brief 替换虚函数.
 *
 * @param [in] vTableSymbol 通过 dlsym 函数获取的虚函数表符号地址.
 * @param [in] index 目标函数在虚函数表中的索引.
 * @param [in] newFunc 用于替换的新函数.
 * @param [out] oldFuncAddr 一个函数指针的地址, 用于保留旧函数. (可传入 nullptr 字面量, 代表不保留)
 *
 * @return 是否成功替换.
 */
template <typename T>
    requires(std::is_member_function_pointer_v<T> && std::is_same_v<T, std::remove_cv_t<T>>)
bool HookVirtualFunc(void *vTableSymbol, std::size_t index, T newFunc, ExtractMemFuncPtrType<T> *oldFuncAddr) {
    return detail::HookVirtualFuncImpl(vTableSymbol, index, reinterpret_cast<void *>(ExtractMemFuncPtr(newFunc)), reinterpret_cast<void **>(oldFuncAddr));
}

/**
 * @brief 替换 PLT 段函数.
 *
 * @param [in] libName 目标函数所属模块的名称.
 * @param [in] offset 目标函数在所属模块中的偏移量.
 * @param [in] newFunc 用于替换的新函数.
 * @param [out] oldFuncAddr 一个函数指针的地址, 用于保留旧函数. (可传入 nullptr 字面量, 代表不保留)
 *
 * @return 是否成功替换.
 */
template <typename T>
    requires std::is_function_v<T>
bool HookPltFunc(std::string_view libName, std::uintptr_t offset, T *newFunc, std::type_identity_t<T **> oldFuncAddr) {
    return detail::HookPltFuncImpl(libName, offset, reinterpret_cast<void *>(newFunc), reinterpret_cast<void **>(oldFuncAddr));
}

} // namespace homura

#endif // HOMURA_HOOKUTILS_H
