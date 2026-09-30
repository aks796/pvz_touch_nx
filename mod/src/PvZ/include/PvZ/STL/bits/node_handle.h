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

#ifndef PVZ_STL_BITS_NODE_HANDLE_H
#define PVZ_STL_BITS_NODE_HANDLE_H

/**
 * @file bits/node_handle.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00584.html">node_handle.h File Reference</a>
 */

#include "PvZ/STL/bits/alloc_traits.h"
#include "PvZ/STL/bits/ptr_traits.h"

#include <cassert>

namespace pvzstl::detail {

template <typename Val, typename NodeAlloc>
class node_handle_common {
    using _alloc_traits = std::allocator_traits<NodeAlloc>;

public:
    using allocator_type = alloc_rebind<NodeAlloc, Val>;

    allocator_type get_allocator() const noexcept {
        assert(!empty());
        return allocator_type(m_alloc.m_alloc);
    }

    explicit operator bool() const noexcept {
        return m_ptr != nullptr;
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_ptr == nullptr;
    }

protected:
    constexpr node_handle_common() noexcept
        : m_ptr() {}

    node_handle_common(node_handle_common &&other) noexcept
        : m_ptr(other.m_ptr) {
        if (m_ptr) {
            move(std::move(other));
        }
    }

    node_handle_common(typename _alloc_traits::pointer ptr, const NodeAlloc &alloc)
        : m_ptr(ptr)
        , m_alloc(alloc) {
        assert(ptr != nullptr);
    }

    ~node_handle_common() {
        if (!empty()) {
            reset();
        }
    }

    node_handle_common &operator=(node_handle_common &&other) noexcept {
        if (empty()) {
            if (!other.empty()) {
                move(std::move(other));
            }
        } else if (other.empty()) {
            reset();
        } else {
            // Free the current node before replacing the allocator.
            _alloc_traits::destroy(*m_alloc, m_ptr->get_valptr());
            _alloc_traits::deallocate(*m_alloc, m_ptr, 1);

            m_alloc = other.m_alloc.release(); // assigns if POCMA
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    void _swap(node_handle_common &other) noexcept {
        if (empty()) {
            if (!other.empty()) {
                move(std::move(other));
            }
        } else if (other.empty()) {
            other.move(std::move(*this));
        } else {
            using std::swap;
            swap(m_ptr, other.m_ptr);
            m_alloc.swap(other.m_alloc); // swaps if POCS
        }
    }

    typename _alloc_traits::pointer m_ptr;

private:
    // A simplified, non-copyable std::optional<NodeAlloc>.
    // Call release() before destruction iff the allocator member is active.
    union optional_alloc {
        optional_alloc() {}

        optional_alloc(const NodeAlloc &alloc) noexcept
            : m_alloc(alloc) {}

        optional_alloc(optional_alloc &&) = delete;

        ~optional_alloc() {}

        optional_alloc &operator=(optional_alloc &&) = delete;

        // Precondition: m_alloc is the active member of the union.
        void operator=(NodeAlloc &&alloc) noexcept {
            using ATr = _alloc_traits;
            if constexpr (ATr::propagate_on_container_move_assignment::value) {
                m_alloc = std::move(alloc);
            } else if constexpr (!ATr::is_always_equal::value) {
                assert(m_alloc == alloc);
            }
        }

        // Precondition: m_alloc is the active member of both unions.
        void swap(optional_alloc &other) noexcept {
            using std::swap;
            if constexpr (_alloc_traits::propagate_on_container_swap::value) {
                swap(m_alloc, other.m_alloc);
            } else if constexpr (!_alloc_traits::is_always_equal::value) {
                assert(m_alloc == other.m_alloc);
            }
        }

        // Precondition: m_alloc is the active member of the union.
        NodeAlloc &operator*() noexcept {
            return m_alloc;
        }

        // Precondition: m_alloc is the active member of the union.
        NodeAlloc release() noexcept {
            NodeAlloc tmp = std::move(m_alloc);
            m_alloc.~NodeAlloc();
            return tmp;
        }

        [[no_unique_address]] NodeAlloc m_alloc;
    };

    // Moves the pointer and allocator from other to *this.
    // Precondition: empty() && !other.empty()
    // Postcondition: !empty() && other.empty()
    void move(node_handle_common &&other) noexcept {
        ::new (std::addressof(m_alloc)) NodeAlloc(other.m_alloc.release());
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
    }

    // Deallocates the node, destroys the allocator.
    // Precondition: !empty()
    // Postcondition: empty()
    void reset() noexcept {
        NodeAlloc alloc = m_alloc.release();
        _alloc_traits::destroy(alloc, m_ptr->get_valptr());
        _alloc_traits::deallocate(alloc, m_ptr, 1);
        m_ptr = nullptr;
    }

    // Destroys the allocator. Does not deallocate or destroy the node.
    // Precondition: !empty()
    // Postcondition: empty()
    void release() noexcept {
        m_alloc.release();
        m_ptr = nullptr;
    }

    template <typename Key2, typename Value2, typename KeyOfValue, typename Compare, typename ValueAlloc>
    friend class rb_tree;

    [[no_unique_address]] optional_alloc m_alloc;
};

/// Node handle type for maps.
template <typename Key, typename Value, typename NodeAlloc>
class node_handle : public node_handle_common<Value, NodeAlloc> {
public:
    using key_type = Key;
    using mapped_type = typename Value::second_type;

    constexpr node_handle() noexcept = default;
    node_handle(node_handle &&) noexcept = default;
    ~node_handle() = default;

    node_handle &operator=(node_handle &&) noexcept = default;

    key_type &key() const noexcept {
        assert(!this->empty());
        return *m_pkey;
    }

    mapped_type &mapped() const noexcept {
        assert(!this->empty());
        return *m_pmapped;
    }

    void swap(node_handle &other) noexcept {
        this->_swap(other);
        using std::swap;
        swap(m_pkey, other.m_pkey);
        swap(m_pmapped, other.m_pmapped);
    }

    friend void swap(node_handle &lhs, node_handle &rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

private:
    using alloc_traits = std::allocator_traits<NodeAlloc>;

    template <typename Tp>
    using pointer = ptr_rebind<typename alloc_traits::pointer, std::remove_reference_t<Tp>>;

    node_handle(typename alloc_traits::pointer ptr, const NodeAlloc &alloc)
        : node_handle_common<Value, NodeAlloc>(ptr, alloc) {
        if (ptr) {
            auto &key = const_cast<Key &>(ptr->get_valptr()->first);
            m_pkey = pointer_to(key);
            m_pmapped = pointer_to(ptr->get_valptr()->second);
        } else {
            m_pkey = nullptr;
            m_pmapped = nullptr;
        }
    }

    template <typename Tp>
    pointer<Tp> pointer_to(Tp &obj) {
        return std::pointer_traits<pointer<Tp>>::pointer_to(obj);
    }

    const key_type &_key() const noexcept {
        return key();
    }

    template <typename Key2, typename Value2, typename KeyOfValue, typename Compare, typename ValueAlloc>
    friend class rb_tree;

    pointer<Key> m_pkey = nullptr;
    pointer<typename Value::second_type> m_pmapped = nullptr;
};

/// Node handle type for sets.
template <typename Value, typename NodeAlloc>
class node_handle<Value, Value, NodeAlloc> : public node_handle_common<Value, NodeAlloc> {
public:
    using value_type = Value;

    constexpr node_handle() noexcept = default;
    node_handle(node_handle &&) noexcept = default;
    ~node_handle() = default;

    node_handle &operator=(node_handle &&) noexcept = default;

    value_type &value() const noexcept {
        assert(!this->empty());
        return *this->m_ptr->get_valptr();
    }

    void swap(node_handle &other) noexcept {
        this->_swap(other);
    }

    friend void swap(node_handle &lhs, node_handle &rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

private:
    using alloc_traits = std::allocator_traits<NodeAlloc>;

    node_handle(typename alloc_traits::pointer ptr, const NodeAlloc &alloc)
        : node_handle_common<Value, NodeAlloc>(ptr, alloc) {}

    const value_type &_key() const noexcept {
        return value();
    }

    template <typename Key2, typename Value2, typename KeyOfValue, typename Compare, typename ValueAlloc>
    friend class rb_tree;
};

/// Return type of insert(node_handle&&) on unique maps/sets.
template <typename Iterator, typename NodeHandle>
struct node_insert_result {
    Iterator position = Iterator();
    bool inserted = false;
    NodeHandle node;
};

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_NODE_HANDLE_H
