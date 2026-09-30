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

#ifndef PVZ_STL_BITS_ALLOCATED_PTR_H
#define PVZ_STL_BITS_ALLOCATED_PTR_H

/**
 * @file bits/allocated_ptr.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00728.html">allocated_ptr.h File Reference</a>
 */

#include <memory>

namespace pvzstl::detail {

template <typename Alloc>
struct allocated_ptr {
    using pointer = typename std::allocator_traits<Alloc>::pointer;
    using value_type = typename std::allocator_traits<Alloc>::value_type;

    allocated_ptr(Alloc &a, pointer ptr) noexcept
        : m_alloc(std::addressof(a))
        , m_ptr(ptr) {}

    template <typename Ptr>
        requires std::is_same_v<Ptr, value_type *>
    allocated_ptr(Alloc &a, Ptr ptr)
        : m_alloc(std::addressof(a))
        , m_ptr(std::pointer_traits<pointer>::pointer_to(*ptr)) {}

    allocated_ptr(allocated_ptr &&other) noexcept
        : m_alloc(other.m_alloc)
        , m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    ~allocated_ptr() {
        if (m_ptr != nullptr) {
            std::allocator_traits<Alloc>::deallocate(*m_alloc, m_ptr, 1);
        }
    }

    allocated_ptr &operator=(std::nullptr_t) noexcept {
        m_ptr = nullptr;
        return *this;
    }

    explicit operator bool() const noexcept {
        return bool(m_ptr);
    }

    value_type *get() const {
        return std::to_address(m_ptr);
    }

    pointer release() {
        pointer tmp = std::move(m_ptr);
        m_ptr = nullptr;
        return tmp;
    }

private:
    Alloc *m_alloc;
    pointer m_ptr;
};

template <typename Alloc>
allocated_ptr<Alloc> allocate_guarded(Alloc &a) {
    return {a, std::allocator_traits<Alloc>::allocate(a, 1)};
}

template <typename Alloc>
struct allocated_obj : allocated_ptr<Alloc> {
    using value_type = typename allocated_ptr<Alloc>::value_type;

    allocated_obj(allocated_obj &&) = default;

    // Default-initialize a value_type at *ptr
    allocated_obj(allocated_ptr<Alloc> &&ptr)
        : allocated_ptr<Alloc>(std::move(ptr)) {
        ::new ((void *)this->get()) value_type;
    }

    ~allocated_obj() {
        if (static_cast<bool>(*this)) {
            this->get()->~value_type();
        }
    }

    using allocated_ptr<Alloc>::operator=;

    value_type &operator*() const {
        return *this->get();
    }

    value_type *operator->() const {
        return this->get();
    }
};

template <typename Alloc>
allocated_obj<Alloc> allocate_guarded_obj(Alloc &a) {
    return {allocate_guarded(a)};
}

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_ALLOCATED_PTR_H
