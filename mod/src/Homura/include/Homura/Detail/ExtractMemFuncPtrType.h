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

#ifndef HOMURA_DETAIL_EXTRACTMEMFUNCPTRTYPE_H
#define HOMURA_DETAIL_EXTRACTMEMFUNCPTRTYPE_H

#include <type_traits>

namespace homura {

namespace detail {
    template <typename>
    struct ExtractMemFuncPtrTypeHelper;

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...)> {
        using Type = Ret (*)(T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const> {
        using Type = Ret (*)(const T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) volatile> {
        using Type = Ret (*)(volatile T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const volatile> {
        using Type = Ret (*)(const volatile T *, Args...);
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...)> {
        using Type = Ret (*)(T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const> {
        using Type = Ret (*)(const T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) volatile> {
        using Type = Ret (*)(volatile T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const volatile> {
        using Type = Ret (*)(const volatile T *, Args..., ...);
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) &> {
        using Type = Ret (*)(T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const &> {
        using Type = Ret (*)(const T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) volatile &> {
        using Type = Ret (*)(volatile T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const volatile &> {
        using Type = Ret (*)(const volatile T *, Args...);
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) &> {
        using Type = Ret (*)(T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const &> {
        using Type = Ret (*)(const T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) volatile &> {
        using Type = Ret (*)(volatile T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const volatile &> {
        using Type = Ret (*)(const volatile T *, Args..., ...);
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) &&> {
        using Type = Ret (*)(T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const &&> {
        using Type = Ret (*)(const T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) volatile &&> {
        using Type = Ret (*)(volatile T *, Args...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const volatile &&> {
        using Type = Ret (*)(const volatile T *, Args...);
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) &&> {
        using Type = Ret (*)(T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const &&> {
        using Type = Ret (*)(const T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) volatile &&> {
        using Type = Ret (*)(volatile T *, Args..., ...);
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const volatile &&> {
        using Type = Ret (*)(const volatile T *, Args..., ...);
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) noexcept> {
        using Type = Ret (*)(T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const noexcept> {
        using Type = Ret (*)(const T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) volatile noexcept> {
        using Type = Ret (*)(volatile T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const volatile noexcept> {
        using Type = Ret (*)(const volatile T *, Args...) noexcept;
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) noexcept> {
        using Type = Ret (*)(T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const noexcept> {
        using Type = Ret (*)(const T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) volatile noexcept> {
        using Type = Ret (*)(volatile T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const volatile noexcept> {
        using Type = Ret (*)(const volatile T *, Args..., ...) noexcept;
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) & noexcept> {
        using Type = Ret (*)(T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const & noexcept> {
        using Type = Ret (*)(const T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) volatile & noexcept> {
        using Type = Ret (*)(volatile T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const volatile & noexcept> {
        using Type = Ret (*)(const volatile T *, Args...) noexcept;
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) & noexcept> {
        using Type = Ret (*)(T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const & noexcept> {
        using Type = Ret (*)(const T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) volatile & noexcept> {
        using Type = Ret (*)(volatile T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const volatile & noexcept> {
        using Type = Ret (*)(const volatile T *, Args..., ...) noexcept;
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) && noexcept> {
        using Type = Ret (*)(T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const && noexcept> {
        using Type = Ret (*)(const T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) volatile && noexcept> {
        using Type = Ret (*)(volatile T *, Args...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args...) const volatile && noexcept> {
        using Type = Ret (*)(const volatile T *, Args...) noexcept;
    };

    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) && noexcept> {
        using Type = Ret (*)(T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const && noexcept> {
        using Type = Ret (*)(const T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) volatile && noexcept> {
        using Type = Ret (*)(volatile T *, Args..., ...) noexcept;
    };
    template <typename T, typename Ret, typename... Args>
    struct ExtractMemFuncPtrTypeHelper<Ret (T::*)(Args..., ...) const volatile && noexcept> {
        using Type = Ret (*)(const volatile T *, Args..., ...) noexcept;
    };

    template <typename T>
    struct ExtractMemFuncPtrTypeCvWrapper {
        using Type = typename detail::ExtractMemFuncPtrTypeHelper<T>::Type;
    };
    template <typename T>
    struct ExtractMemFuncPtrTypeCvWrapper<const T> {
        using Type = const typename detail::ExtractMemFuncPtrTypeHelper<T>::Type;
    };
    template <typename T>
    struct ExtractMemFuncPtrTypeCvWrapper<volatile T> {
        using Type = volatile typename detail::ExtractMemFuncPtrTypeHelper<T>::Type;
    };
    template <typename T>
    struct ExtractMemFuncPtrTypeCvWrapper<const volatile T> {
        using Type = const volatile typename detail::ExtractMemFuncPtrTypeHelper<T>::Type;
    };
} // namespace detail

template <typename T>
    requires std::is_member_function_pointer_v<T>
using ExtractMemFuncPtrType = typename detail::ExtractMemFuncPtrTypeCvWrapper<T>::Type;

} // namespace homura

#endif // HOMURA_DETAIL_EXTRACTMEMFUNCPTRTYPE_H
