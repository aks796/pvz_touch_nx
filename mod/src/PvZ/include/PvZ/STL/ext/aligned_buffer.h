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

#ifndef PVZ_STL_EXT_ALIGNED_BUFFER_H
#define PVZ_STL_EXT_ALIGNED_BUFFER_H

/**
 * @file ext/aligned_buffer.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a01343.html">aligned_buffer.h File Reference</a>
 */

#include <cstddef>

namespace pvzstl_cxx {

// A utility type containing a POD object that can hold an object of type
// _Tp initialized via placement new or allocator_traits::construct.
// Intended for use as a data member subobject, use aligned_buffer for
// complete objects.
template <typename Tp>
struct aligned_membuf {
    aligned_membuf() = default;

    // Can be used to avoid value-initialization zeroing _M_storage.
    aligned_membuf(std::nullptr_t) noexcept {}

    void *addr() noexcept {
        return m_storage;
    }

    const void *addr() const noexcept {
        return m_storage;
    }

    Tp *ptr() noexcept {
        return static_cast<Tp *>(addr());
    }

    const Tp *ptr() const noexcept {
        return static_cast<const Tp *>(addr());
    }

    alignas(Tp) unsigned char m_storage[sizeof(Tp)];
};

// Similar to aligned_membuf but aligned for complete objects, not members.
// This type is used in <forward_list>, <future>, <bits/shared_ptr_base.h>
// and <bits/hashtable_policy.h>, but ideally they would use aligned_membuf
// instead, as it has smaller size for some types on some targets.
// This type is still used to avoid an ABI change.
template <typename Tp>
struct aligned_buffer {
    aligned_buffer() = default;

    // Can be used to avoid value-initialization
    aligned_buffer(std::nullptr_t) noexcept {}

    void *addr() noexcept {
        return m_storage;
    }

    const void *addr() const noexcept {
        return m_storage;
    }

    Tp *ptr() noexcept {
        return static_cast<Tp *>(addr());
    }

    const Tp *ptr() const noexcept {
        return static_cast<const Tp *>(addr());
    }

    // Using __alignof__ gives the alignment for a complete object.
    alignas(__alignof__(Tp)) unsigned char m_storage[sizeof(Tp)];
};

} // namespace pvzstl_cxx

#endif // PVZ_STL_EXT_ALIGNED_BUFFER_H
