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

#ifndef PVZ_STL_BITS_STL_TREE_H
#define PVZ_STL_BITS_STL_TREE_H

/**
 * @file bits/stl_tree.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00425.html">stl_tree.h File Reference</a>
 */

#include "PvZ/STL/compare.h"

#include "PvZ/STL/bits/alloc_traits.h"
#include "PvZ/STL/bits/move.h"
#include "PvZ/STL/bits/node_handle.h"
#include "PvZ/STL/bits/stdexcept_throw.h"
#include "PvZ/STL/bits/stl_function.h"

#include "PvZ/STL/ext/aligned_buffer.h"

#include <algorithm>
#include <iterator>

namespace pvzstl::detail {

enum rb_tree_color { red = false, black = true };

struct rb_tree_node_base {
    using base_ptr = rb_tree_node_base *;

    static base_ptr minimum(base_ptr x) noexcept {
        while (x->m_left != nullptr) {
            x = x->m_left;
        }
        return x;
    }

    static base_ptr maximum(base_ptr x) noexcept {
        while (x->m_right != nullptr) {
            x = x->m_right;
        }
        return x;
    }

    // This is not const-correct, but it's only used in a const access path
    // by pvzstl::detail::rb_tree::_end() where the pointer is used to initialize a
    // const_iterator and so constness is restored.
    base_ptr get_base_ptr() const noexcept {
        return const_cast<rb_tree_node_base *>(this);
    }

    rb_tree_color m_color;
    base_ptr m_parent;
    base_ptr m_left;
    base_ptr m_right;
};

// Helper type offering value initialization guarantee on the compare functor.
template <typename KeyCompare>
struct rb_tree_key_compare {
    static_assert(std::is_copy_constructible_v<KeyCompare>, "Comparator must be copy-constructible");

    rb_tree_key_compare() noexcept(std::is_nothrow_default_constructible_v<KeyCompare>)
        : m_key_compare() {}

    rb_tree_key_compare(const rb_tree_key_compare &) = default;

    rb_tree_key_compare(rb_tree_key_compare &&other) noexcept(std::is_nothrow_copy_constructible_v<KeyCompare>)
        : m_key_compare(other.m_key_compare) {}

    rb_tree_key_compare(const KeyCompare &comp)
        : m_key_compare(comp) {}

    KeyCompare m_key_compare;
};

// Helper type to manage default initialization of node count and header.
struct rb_tree_header {
    rb_tree_header() noexcept {
        m_header.m_color = red;
        reset();
    }

    rb_tree_header(rb_tree_header &&other) noexcept {
        if (other.m_header.m_parent != nullptr) {
            move_data(other);
        } else {
            m_header.m_color = red;
            reset();
        }
    }

    void move_data(rb_tree_header &other) noexcept {
        m_header = other.m_header;
        m_header.m_parent->m_parent = &m_header;
        m_node_count = other.m_node_count;

        other.reset();
    }

    void reset() noexcept {
        m_header.m_parent = nullptr;
        m_header.m_left = m_header.m_right = &m_header;
        m_node_count = 0;
    }

    rb_tree_node_base m_header;
    std::size_t m_node_count; // Keeps track of size of tree.
};

template <typename Val>
struct rb_tree_node : rb_tree_node_base {
    Val *get_valptr() noexcept {
        return m_storage.ptr();
    }

    const Val *get_valptr() const noexcept {
        return m_storage.ptr();
    }

    rb_tree_node *get_node_ptr() noexcept {
        return this;
    }

    pvzstl_cxx::aligned_membuf<Val> m_storage;
};

[[gnu::pure]] rb_tree_node_base *rb_tree_increment(rb_tree_node_base *x) noexcept;
[[gnu::pure]] rb_tree_node_base *rb_tree_decrement(rb_tree_node_base *x) noexcept;

template <typename Tp>
struct rb_tree_iterator {
    using value_type = Tp;
    using pointer = Tp *;
    using reference = Tp &;

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;

    using base_ptr = rb_tree_node_base::base_ptr;
    using node_ptr = rb_tree_node<Tp> *;

    rb_tree_iterator() noexcept
        : m_node() {}

    explicit rb_tree_iterator(base_ptr p) noexcept
        : m_node{p} {}

    reference operator*() const noexcept {
        return *static_cast<node_ptr>(m_node)->get_valptr();
    }

    pointer operator->() const noexcept {
        return static_cast<node_ptr>(m_node)->get_valptr();
    }

    rb_tree_iterator &operator++() noexcept {
        m_node = rb_tree_increment(m_node);
        return *this;
    }

    rb_tree_iterator operator++(int) noexcept {
        rb_tree_iterator tmp = *this;
        m_node = rb_tree_increment(m_node);
        return tmp;
    }

    rb_tree_iterator &operator--() noexcept {
        m_node = rb_tree_decrement(m_node);
        return *this;
    }

    rb_tree_iterator operator--(int) noexcept {
        rb_tree_iterator tmp = *this;
        m_node = rb_tree_decrement(m_node);
        return tmp;
    }

    friend bool operator==(const rb_tree_iterator &lhs, const rb_tree_iterator &rhs) = default;

    base_ptr m_node;
};

template <typename Tp>
struct rb_tree_const_iterator {
    using value_type = Tp;
    using pointer = const Tp *;
    using reference = const Tp &;

    using iterator = rb_tree_iterator<Tp>;

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;

    using base_ptr = rb_tree_node_base::base_ptr;
    using node_ptr = const rb_tree_node<Tp> *;

    rb_tree_const_iterator() noexcept
        : m_node() {}

    explicit rb_tree_const_iterator(base_ptr p) noexcept
        : m_node{p} {}

    rb_tree_const_iterator(const iterator &it) noexcept
        : m_node{it.m_node} {}

    reference operator*() const noexcept {
        return *static_cast<node_ptr>(m_node)->get_valptr();
    }

    pointer operator->() const noexcept {
        return static_cast<node_ptr>(m_node)->get_valptr();
    }

    rb_tree_const_iterator &operator++() noexcept {
        m_node = rb_tree_increment(m_node);
        return *this;
    }

    rb_tree_const_iterator operator++(int) noexcept {
        rb_tree_const_iterator tmp = *this;
        m_node = rb_tree_increment(m_node);
        return tmp;
    }

    rb_tree_const_iterator &operator--() noexcept {
        m_node = rb_tree_decrement(m_node);
        return *this;
    }

    rb_tree_const_iterator operator--(int) noexcept {
        rb_tree_const_iterator tmp = *this;
        m_node = rb_tree_decrement(m_node);
        return tmp;
    }

    friend bool operator==(const rb_tree_const_iterator &lhs, const rb_tree_const_iterator &rhs) = default;

    base_ptr m_node;
};

[[gnu::nonnull]] void rb_tree_insert_and_rebalance(bool insert_left, rb_tree_node_base *x, rb_tree_node_base *p, rb_tree_node_base &header) noexcept;
[[gnu::nonnull, gnu::returns_nonnull]] rb_tree_node_base *rb_tree_rebalance_for_erase(rb_tree_node_base *z, rb_tree_node_base &header) noexcept;

namespace _rb_tree {
    // Determine the node and iterator types used by pvzstl::detail::rb_tree.
    template <typename Val, typename Ptr>
    struct node_traits;

    // Specialization for the simple case where the allocator's pointer type
    // is the same type as value_type*.
    // For ABI compatibility we can't change the types used for this case.
    template <typename Val>
    struct node_traits<Val, Val *> {
        using node = rb_tree_node<Val>;
        using node_ptr = node *;
        using node_base = rb_tree_node_base;
        using base_ptr = node_base *;
        using header_t = rb_tree_header;
        using iterator = rb_tree_iterator<Val>;
        using const_iterator = rb_tree_const_iterator<Val>;

        [[gnu::nonnull]] static void insert_and_rebalance(bool insert_left, node_base *x, node_base *p, node_base &header) noexcept {
            rb_tree_insert_and_rebalance(insert_left, x, p, header);
        }

        [[gnu::nonnull]] static node_base *rebalance_for_erase(node_base *z, node_base &header) noexcept {
            return rb_tree_rebalance_for_erase(z, header);
        }
    };

    // Always use the T* specialization.
    template <typename Val, typename Ptr>
    struct node_traits : node_traits<Val, Val *> {};
} // namespace _rb_tree

template <typename Tree1, typename Cmp2>
struct rb_tree_merge_helper {};

[[gnu::pure]] unsigned int rb_tree_black_count(const rb_tree_node_base *node, const rb_tree_node_base *root) noexcept;

template <typename Key, typename Val, typename KeyOfValue, typename Compare, typename Alloc = std::allocator<Val>>
class rb_tree {
    using val_alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<Val>;

    using val_alloc_traits = std::allocator_traits<val_alloc_type>;
    using valptr = typename val_alloc_traits::pointer;
    using node_traits = _rb_tree::node_traits<Val, valptr>;

    using node_base = node_traits::node_base;
    using node = node_traits::node;

    using node_alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<node>;

    using node_alloc_traits = std::allocator_traits<node_alloc_type>;

protected:
    using base_ptr = node_traits::base_ptr;
    using node_ptr = node_traits::node_ptr;

    using header_t = node_traits::header_t;

public:
    using key_type = Key;
    using value_type = Val;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = Alloc;

    using iterator = node_traits::iterator;
    using const_iterator = node_traits::const_iterator;

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using node_type = detail::node_handle<Key, Val, node_alloc_type>;
    using insert_return_type = detail::node_insert_result<std::conditional_t<std::is_same_v<Key, Val>, const_iterator, iterator>, node_type>;

    template <typename Iter>
    static constexpr bool same_value_type = std::is_same_v<value_type, typename std::iterator_traits<Iter>::value_type>;

    rb_tree() = default;

    rb_tree(const Compare &comp, const allocator_type &a = allocator_type())
        : m_impl(comp, node_alloc_type(a)) {}

    rb_tree(const rb_tree &other)
        : m_impl(other.m_impl) {
        if (other.root()) {
            root() = copy(other);
        }
    }

    rb_tree(rb_tree &&) = default;

    rb_tree(const allocator_type &a)
        : m_impl(node_alloc_type(a)) {}

    rb_tree(const rb_tree &other, const allocator_type &a)
        : m_impl(other.m_impl.m_key_compare, node_alloc_type(a)) {
        if (other.root()) {
            root() = copy(other);
        }
    }

    rb_tree(rb_tree &&other, const allocator_type &a)
        : rb_tree{std::move(other), node_alloc_type(a)} {}

    rb_tree(rb_tree &&other, node_alloc_type &&a) noexcept(std::is_nothrow_constructible_v<rb_tree, rb_tree &&, node_alloc_type &&, typename node_alloc_traits::is_always_equal>)
        : rb_tree(std::move(other), std::move(a), typename node_alloc_traits::is_always_equal()) {}

    ~rb_tree() {
        _erase(begin_node());
    }

    rb_tree &operator=(const rb_tree &other) {
        if (this == std::addressof(other)) {
            return *this;
        }

        // Note that Key may be a constant type.
        if (node_alloc_traits::propagate_on_container_copy_assignment::value) {
            auto &this_alloc = get_node_allocator();
            auto &that_alloc = other.get_node_allocator();
            if (!node_alloc_traits::is_always_equal::value && this_alloc != that_alloc) {
                // Replacement allocator cannot free existing storage, we need
                // to erase nodes first.
                clear();
                alloc_on_copy(this_alloc, that_alloc);
            }
        }

        reuse_or_alloc_node roan(*this);
        m_impl.reset();
        m_impl.m_key_compare = other.m_impl.m_key_compare;
        if (other.root()) {
            root() = copy<as_lvalue>(other, roan);
        }

        return *this;
    }

    rb_tree &operator=(rb_tree &&other) noexcept((node_alloc_traits::propagate_on_container_move_assignment::value || node_alloc_traits::is_always_equal::value)
                                                 && std::is_nothrow_move_assignable_v<Compare>) {
        m_impl.m_key_compare = std::move(other.m_impl.m_key_compare);
        constexpr bool nothrow_move = node_alloc_traits::propagate_on_container_move_assignment::value || node_alloc_traits::is_always_equal::value;
        move_assign(other, std::bool_constant<nothrow_move>());
        return *this;
    }

    template <typename Iterator>
    void assign_unique(Iterator first, Iterator last) {
        reuse_or_alloc_node roan(*this);
        m_impl.reset();
        for (; first != last; ++first) {
            insert_unique(end(), *first, roan);
        }
    }

    template <typename Iterator>
    void assign_equal(Iterator first, Iterator last) {
        reuse_or_alloc_node roan(*this);
        m_impl.reset();
        for (; first != last; ++first) {
            insert_equal(end(), *first, roan);
        }
    }

    node_alloc_type &get_node_allocator() noexcept {
        return m_impl;
    }

    const node_alloc_type &get_node_allocator() const noexcept {
        return m_impl;
    }

    allocator_type get_allocator() const noexcept {
        return allocator_type(get_node_allocator());
    }

    Compare key_comp() const {
        return m_impl.m_key_compare;
    }

    iterator begin() noexcept {
        return iterator(m_impl.m_header.m_left);
    }

    const_iterator begin() const noexcept {
        return const_iterator(m_impl.m_header.m_left);
    }

    iterator end() noexcept {
        return iterator(_end());
    }

    const_iterator end() const noexcept {
        return const_iterator(_end());
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_impl.m_node_count;
    }

    size_type size() const noexcept {
        return m_impl.m_node_count;
    }

    size_type max_size() const noexcept {
        return node_alloc_traits::max_size(get_node_allocator());
    }

    void swap(rb_tree &other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        if (!root()) {
            if (other.root()) {
                m_impl.move_data(other.m_impl);
            }
        } else if (!other.root()) {
            other.m_impl.move_data(m_impl);
        } else {
            std::swap(root(), other.root());
            std::swap(leftmost(), other.leftmost());
            std::swap(rightmost(), other.rightmost());

            root()->m_parent = _end();
            other.root()->m_parent = other._end();
            std::swap(m_impl.m_node_count, other.m_impl.m_node_count);
        }
        // No need to swap header's color as it does not change.

        using std::swap;
        swap(m_impl.m_key_compare, other.m_impl.m_key_compare);

        detail::alloc_on_swap(get_node_allocator(), other.get_node_allocator());
    }

    std::pair<base_ptr, base_ptr> get_insert_unique_pos(const key_type &k) {
        using Res = std::pair<base_ptr, base_ptr>;
        base_ptr x = _begin();
        base_ptr y = _end();
        bool comp = true;
        while (x) {
            y = x;
            comp = key_compare(k, key(x));
            x = comp ? left(x) : right(x);
        }
        iterator it = iterator(y);
        if (comp) {
            if (it == begin()) {
                return Res(x, y);
            } else {
                --it;
            }
        }
        if (key_compare(key(it.m_node), k)) {
            return Res(x, y);
        }
        return Res(it.m_node, base_ptr());
    }

    template <typename Kt>
    std::pair<base_ptr, base_ptr> get_insert_unique_pos_tr(Kt &&k) {
        if (size() == 0) {
            return {_end(), _end()}; // Insert as root.
        }
        base_ptr x = _begin();
        base_ptr y = x;
        bool k_le_y = false;
        do {
            y = x;
            k_le_y = !key_compare(key(y), k);
            x = k_le_y ? left(x) : right(x);
        } while (x);
        // If !k_le_y, k > *y;
        //   If y is rightmost, put at _M_right under *y.
        //   else if k < *(y+1), put at _M_right under *y.
        //   else k == *(y+1), do not insert, report (y+1).
        // else, k_le_y, k <= *y;
        //   If k < *Y, put at _M_left under *y.
        //   else k == *y, do not insert, report y.
        auto it = iterator(y);
        if (!k_le_y) { // k > *y
            if (y == rightmost()) {
                return {{}, y}; // Place to right under y.
            }
            ++it;
        }
        if (key_compare(k, key(it.m_node))) {
            if (k_le_y) {
                return {y, y}; // Place to left under y.
            } else {
                return {{}, y}; // Place to right under y.
            }
        }
        return {it.m_node, {}}; // No insert.
    }

    std::pair<base_ptr, base_ptr> get_insert_equal_pos(const key_type &k) {
        using Res = std::pair<base_ptr, base_ptr>;
        base_ptr x = _begin();
        base_ptr y = _end();
        while (x) {
            y = x;
            x = key_compare(k, key(x)) ? left(x) : right(x);
        }
        return Res(x, y);
    }

    std::pair<base_ptr, base_ptr> get_insert_hint_unique_pos(const_iterator pos, const key_type &k) {
        using Res = std::pair<base_ptr, base_ptr>;

        // end()
        if (pos.m_node == _end()) {
            if (size() > 0 && key_compare(key(rightmost()), k)) {
                return Res(base_ptr(), rightmost());
            } else {
                return get_insert_unique_pos(k);
            }
        } else if (key_compare(k, key(pos.m_node))) {
            // First, try before...
            iterator before(pos.m_node);
            if (pos.m_node == leftmost()) { // begin()
                return Res(leftmost(), leftmost());
            } else if (key_compare(key((--before).m_node), k)) {
                if (!right(before.m_node)) {
                    return Res(base_ptr(), before.m_node);
                } else {
                    return Res(pos.m_node, pos.m_node);
                }
            } else {
                return get_insert_unique_pos(k);
            }
        } else if (key_compare(key(pos.m_node), k)) {
            // ... then try after.
            iterator after(pos.m_node);
            if (pos.m_node == rightmost()) {
                return Res(base_ptr(), rightmost());
            } else if (key_compare(k, key((++after).m_node))) {
                if (!right(pos.m_node)) {
                    return Res(base_ptr(), pos.m_node);
                } else {
                    return Res(after.m_node, after.m_node);
                }
            } else {
                return get_insert_unique_pos(k);
            }
        } else {
            // Equivalent keys.
            return Res(pos.m_node, base_ptr());
        }
    }

    std::pair<base_ptr, base_ptr> get_insert_hint_equal_pos(const_iterator pos, const key_type &k) {
        using Res = std::pair<base_ptr, base_ptr>;

        // end()
        if (pos.m_node == _end()) {
            if (size() > 0 && !key_compare(k, key(rightmost()))) {
                return Res(base_ptr(), rightmost());
            } else {
                return get_insert_equal_pos(k);
            }
        } else if (!key_compare(key(pos.m_node), k)) {
            // First, try before...
            iterator before(pos.m_node);
            if (pos.m_node == leftmost()) { // begin()
                return Res(leftmost(), leftmost());
            } else if (!key_compare(k, key((--before).m_node))) {
                if (!right(before.m_node)) {
                    return Res(base_ptr(), before.m_node);
                } else {
                    return Res(pos.m_node, pos.m_node);
                }
            } else {
                return get_insert_equal_pos(k);
            }
        } else {
            // ... then try after.
            iterator after(pos.m_node);
            if (pos.m_node == rightmost()) {
                return Res(base_ptr(), rightmost());
            } else if (!key_compare(key((++after).m_node), k)) {
                if (!right(pos.m_node)) {
                    return Res(base_ptr(), pos.m_node);
                } else {
                    return Res(after.m_node, after.m_node);
                }
            } else {
                return Res(base_ptr(), base_ptr());
            }
        }
    }

    template <typename Arg>
    std::pair<iterator, bool> insert_unique(Arg &&arg) {
        using Res = std::pair<iterator, bool>;
        std::pair<base_ptr, base_ptr> res = get_insert_unique_pos(KeyOfValue()(arg));

        if (res.second) {
            alloc_node an(*this);
            return Res(insert(res.first, res.second, std::forward<Arg>(arg), an), true);
        }

        return Res(iterator(res.first), false);
    }

    template <typename Arg, typename NodeGen>
    iterator insert_unique(const_iterator pos, Arg &&arg, NodeGen &node_gen) {
        std::pair<base_ptr, base_ptr> res = get_insert_hint_unique_pos(pos, KeyOfValue()(arg));

        if (res.second) {
            return insert(res.first, res.second, std::forward<Arg>(arg), node_gen);
        }
        return iterator(res.first);
    }

    template <typename Arg>
    iterator insert_unique(const_iterator pos, Arg &&arg) {
        alloc_node an(*this);
        return insert_unique(pos, std::forward<Arg>(arg), an);
    }

    template <typename Arg>
    iterator insert_equal(Arg &&arg);

    template <typename Arg, typename NodeGen>
    iterator insert_equal(const_iterator pos, Arg &&arg, NodeGen &node_gen) {
        std::pair<base_ptr, base_ptr> res = get_insert_hint_equal_pos(pos, KeyOfValue()(arg));

        if (res.second) {
            return insert(res.first, res.second, std::forward<Arg>(arg), node_gen);
        }
        return insert_equal_lower(res.first);
    }

    template <typename Arg>
    iterator insert_equal(const_iterator pos, Arg &&arg) {
        alloc_node an(*this);
        return insert_equal(pos, std::forward<Arg>(arg), an);
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace_unique(Args &&...args) {
        auto_node z(*this, std::forward<Args>(args)...);
        auto res = get_insert_unique_pos(z.key());
        if (res.second) {
            return {z.insert(res), true};
        }
        return {iterator(res.first), false};
    }

    template <typename... Args>
    iterator emplace_equal(Args &&...args) {
        auto_node z(*this, std::forward<Args>(args)...);
        auto res = get_insert_unique_pos(z.key());
        return z.insert(res);
    }

    template <typename... Args>
    iterator emplace_hint_unique(const_iterator pos, Args &&...args) {
        auto_node z(*this, std::forward<Args>(args)...);
        auto res = get_insert_hint_unique_pos(pos, z.key());
        if (res.second) {
            return z.insert(res);
        }
        return iterator(res.first);
    }

    template <typename... Args>
    iterator emplace_hint_equal(const_iterator pos, Args &&...args) {
        auto_node z(*this, std::forward<Args>(args)...);
        auto res = get_insert_hint_equal_pos(pos, z.key());
        if (res.second) {
            return z.insert(res);
        }
        return z.insert_equal_lower();
    }

    template <typename InputIterator>
        requires same_value_type<InputIterator>
    void insert_range_unique(InputIterator first, InputIterator last) {
        alloc_node an(*this);
        for (; first != last; ++first)
            insert_unique(end(), *first, an);
    }

    template <typename InputIterator>
        requires(!same_value_type<InputIterator>)
    void insert_range_unique(InputIterator first, InputIterator last) {
        for (; first != last; ++first)
            emplace_unique(*first);
    }

    template <typename InputIterator>
        requires same_value_type<InputIterator>
    void insert_range_equal(InputIterator first, InputIterator last) {
        alloc_node an(*this);
        for (; first != last; ++first)
            insert_equal(end(), *first, an);
    }

    template <typename InputIterator>
        requires(!same_value_type<InputIterator>)
    void insert_range_equal(InputIterator first, InputIterator last) {
        for (; first != last; ++first)
            emplace_equal(*first);
    }

    iterator erase(const_iterator pos) {
        assert(pos != end());
        const_iterator result = pos;
        ++result;
        erase_aux(pos);
        return iterator(result.m_node);
    }

    iterator erase(iterator pos) {
        assert(pos != end());
        iterator result = pos;
        ++result;
        erase_aux(pos);
        return iterator(result.m_node);
    }

    size_type erase(const key_type &k) {
        std::pair<iterator, iterator> p = equal_range(k);
        const size_type old_size = size();
        erase_aux(p.first, p.second);
        return old_size - size();
    }

    template <typename Kt>
    size_type erase_tr(const Kt &k) {
        std::pair<iterator, iterator> p = equal_range_tr(k);
        const size_type old_size = size();
        erase_aux(p.first, p.second);
        return old_size - size();
    }

    size_type erase_unique(const key_type &k) {
        iterator it = find(k);
        if (it == end()) {
            return 0;
        }
        erase_aux(it);
        return 1;
    }

    iterator erase(const_iterator first, const_iterator last) {
        erase_aux(first, last);
        return iterator(last.m_node);
    }

    void clear() noexcept {
        _erase(begin_node());
        m_impl.reset();
    }

    insert_return_type reinsert_node_unique(node_type &&nh) {
        insert_return_type ret;
        if (nh.empty()) {
            ret.position = end();
        } else {
            assert(get_node_allocator() == *nh.m_alloc);
            auto res = get_insert_unique_pos(nh.key());
            if (res.second) {
                ret.position = insert_node(res.first, res.second, adapt(nh.m_ptr));
                nh.release();
                ret.inserted = true;
            } else {
                ret.node = std::move(nh);
                ret.position = iterator(res.first);
                ret.inserted = false;
            }
        }
        return ret;
    }

    insert_return_type reinsert_node_equal(node_type &&nh) {
        insert_return_type ret;
        if (nh.empty()) {
            ret.position = end();
        } else {
            assert(get_node_allocator() == *nh.m_alloc);
            auto res = get_insert_equal_pos(nh.key());
            if (res.second) {
                ret = insert_node(res.first, res.second, adapt(nh.m_ptr));
            } else {
                ret = insert_equal_lower_node(adapt(nh.m_ptr));
            }
            nh.release();
        }
        return ret;
    }

    iterator reinsert_node_hint_unique(const_iterator pos, node_type &&nh) {
        iterator ret;
        if (nh.empty()) {
            ret = end();
        } else {
            assert(get_node_allocator() == *nh.m_alloc);
            auto res = get_insert_hint_unique_pos(pos, nh.key());
            if (res.second) {
                ret = insert_node(res.first, res.second, adapt(nh.m_ptr));
                nh.release();
            } else {
                ret = iterator(res.first);
            }
        }
        return ret;
    }

    iterator reinsert_node_hint_equal(const_iterator pos, node_type &&nh) {
        iterator ret;
        if (nh.empty()) {
            ret = end();
        } else {
            assert(get_node_allocator() == *nh.m_alloc);
            auto res = get_insert_hint_equal_pos(pos, nh.key());
            if (res.second) {
                ret = insert_node(res.first, res.second, adapt(nh.m_ptr));
                nh.release();
            } else {
                ret = insert_equal_lower_node(adapt(nh.m_ptr));
            }
        }
        return ret;
    }

    node_type extract(const_iterator pos) {
        auto ptr = node_traits::rebalance_for_erase(pos.m_node, m_impl.m_header);
        --m_impl.m_node_count;
        auto node_p = static_cast<node &>(*ptr).get_node_ptr();
        using alloc_ptr = typename node_alloc_traits::pointer;
        if constexpr (std::is_same_v<node_ptr, alloc_ptr>) {
            return {node_p, get_node_allocator()};
        } else {
            auto ap = std::pointer_traits<alloc_ptr>::pointer_to(*node_p);
            return {ap, get_node_allocator()};
        }
    }

    node_type extract(const key_type &k) {
        node_type nh;
        auto pos = find(k);
        if (pos != end()) {
            nh = extract(const_iterator(pos));
        }
        return nh;
    }

    template <typename Kt>
    node_type extract_tr(const Kt &k) {
        node_type nh;
        auto pos = find_tr(k);
        if (pos != end()) {
            nh = extract(const_iterator(pos));
        }
        return nh;
    }

    template <typename Compare2>
    using compatible_tree = rb_tree<Key, Val, KeyOfValue, Compare2, Alloc>;

    template <typename, typename>
    friend struct rb_tree_merge_helper;

    template <typename Compare2>
    void merge_unique(compatible_tree<Compare2> &src) noexcept {
        using MergeHelper = rb_tree_merge_helper<rb_tree, Compare2>;
        for (auto it = src.begin(), end = src.end(); it != end;) {
            auto pos = it++;
            auto res = get_insert_unique_pos(KeyOfValue()(*pos));
            if (res.second) {
                auto &src_impl = MergeHelper::get_impl(src);
                auto ptr = node_traits::rebalance_for_erase(pos.m_node, src_impl.m_header);
                --src_impl.m_node_count;
                auto node_p = static_cast<node &>(*ptr).get_node_ptr();
                insert_node(res.first, res.second, node_p);
            }
        }
    }

    template <typename Compare2>
    void merge_equal(compatible_tree<Compare2> &src) noexcept {
        using MergeHelper = rb_tree_merge_helper<rb_tree, Compare2>;
        for (auto it = src.begin(), end = src.end(); it != end;) {
            auto pos = it++;
            auto res = get_insert_equal_pos(KeyOfValue()(*pos));
            if (res.second) {
                auto &src_impl = MergeHelper::get_impl(src);
                auto ptr = node_traits::rebalance_for_erase(pos.m_node, src_impl.m_header);
                --src_impl.m_node_count;
                auto node_p = static_cast<node &>(*ptr).get_node_ptr();
                insert_node(res.first, res.second, node_p);
            }
        }
    }

    // Set operations.
    iterator find(const key_type &k) {
        iterator it(_lower_bound(_begin(), _end(), k));
        return (it == end() || key_compare(k, key(it.m_node))) ? end() : it;
    }

    const_iterator find(const key_type &k) const {
        const_iterator it(_lower_bound(_begin(), _end(), k));
        return (it == end() || key_compare(k, key(it.m_node))) ? end() : it;
    }

    size_type count(const key_type &k) const {
        std::pair<const_iterator, const_iterator> p = equal_range(k);
        const size_type n = std::distance(p.first, p.second);
        return n;
    }

    iterator lower_bound(const key_type &k) {
        return iterator(_lower_bound(_begin(), _end(), k));
    }

    const_iterator lower_bound(const key_type &k) const {
        return const_iterator(_lower_bound(_begin(), _end(), k));
    }

    iterator upper_bound(const key_type &k) {
        return iterator(_upper_bound(_begin(), _end(), k));
    }

    const_iterator upper_bound(const key_type &k) const {
        return const_iterator(_upper_bound(_begin(), _end(), k));
    }

    std::pair<iterator, iterator> equal_range(const key_type &k) {
        using Ret = std::pair<iterator, iterator>;

        base_ptr x = _begin();
        base_ptr y = _end();
        while (x) {
            if (key_compare(key(x), k)) {
                x = right(x);
            } else if (key_compare(k, key(x))) {
                y = x;
                x = left(x);
            } else {
                base_ptr xu(x);
                base_ptr yu(y);
                y = x;
                x = left(x);
                xu = right(xu);
                return Ret(iterator(_lower_bound(x, y, k)), iterator(_upper_bound(xu, yu, k)));
            }
        }
        return Ret(iterator(y), iterator(y));
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type &k) const {
        using Ret = std::pair<const_iterator, const_iterator>;

        base_ptr x = _begin();
        base_ptr y = _end();
        while (x) {
            if (key_compare(key(x), k)) {
                x = right(x);
            } else if (key_compare(k, key(x))) {
                y = x;
                x = left(x);
            } else {
                base_ptr xu(x);
                base_ptr yu(y);
                y = x;
                x = left(x);
                xu = right(xu);
                return Ret(const_iterator(_lower_bound(x, y, k)), const_iterator(_upper_bound(xu, yu, k)));
            }
        }
        return Ret(const_iterator(y), const_iterator(y));
    }

    template <typename Kt>
        requires transparent_comparator<Compare>
    iterator find_tr(const Kt &k) {
        const rb_tree *const_this = this;
        return iterator(const_this->find_tr(k).m_node);
    }

    template <typename Kt>
        requires transparent_comparator<Compare>
    const_iterator find_tr(const Kt &k) const {
        const_iterator it(lower_bound_tr(k));
        if (it != end() && key_compare(k, key(it.m_node))) {
            it = end();
        }
        return it;
    }

    template <typename Kt>
        requires transparent_comparator<Compare>
    size_type count_tr(const Kt &k) const {
        auto p = equal_range_tr(k);
        return std::distance(p.first, p.second);
    }

    template <typename Kt>
        requires transparent_comparator<Compare>
    base_ptr lower_bound_tr(const Kt &k) const {
        auto x = _begin();
        auto y = _end();
        while (x) {
            if (!key_compare(key(x), k)) {
                y = x;
                x = left(x);
            } else {
                x = right(x);
            }
        }
        return y;
    }

    template <typename Kt>
        requires transparent_comparator<Compare>
    base_ptr upper_bound_tr(const Kt &k) const {
        auto x = _begin();
        auto y = _end();
        while (x) {
            if (key_compare(k, key(x))) {
                y = x;
                x = left(x);
            } else {
                x = right(x);
            }
        }
        return y;
    }

    template <typename Kt>
        requires transparent_comparator<Compare>
    std::pair<iterator, iterator> equal_range_tr(const Kt &k) {
        const rb_tree *const_this = this;
        auto ret = const_this->equal_range_tr(k);
        return {iterator(ret.first.m_node), iterator(ret.second.m_node)};
    }

    template <typename Kt>
        requires transparent_comparator<Compare>
    std::pair<const_iterator, const_iterator> equal_range_tr(const Kt &k) const {
        const_iterator low(lower_bound_tr(k));
        auto high = low;
        auto &cmp = m_impl.m_key_compare;
        while (high != end() && !cmp(k, key(high.m_node))) {
            ++high;
        }
        return {low, high};
    }

    bool rb_verify() const {
        if (m_impl.m_node_count == 0 || begin() == end()) {
            return m_impl.m_node_count == 0 && begin() == end() && m_impl.m_header.m_left == _end() && m_impl.m_header.m_right == _end();
        }

        unsigned int len = rb_tree_black_count(leftmost(), root());
        for (const_iterator it = begin(); it != end(); ++it) {
            base_ptr x = it.m_node;
            base_ptr L = left(x);
            base_ptr R = right(x);

            if (x->m_color == red) {
                if ((L && L->m_color == red) || (R && R->m_color == red)) {
                    return false;
                }

                if (L && key_compare(key(x), key(L))) {
                    return false;
                }
                if (R && key_compare(key(R), key(x))) {
                    return false;
                }

                if (!L && !R && rb_tree_black_count(x, root()) != len) {
                    return false;
                }
            }
        }

        if (leftmost() != node_base::minimum(root())) {
            return false;
        }
        if (right() != node_base::maximum(root())) {
            return false;
        }
        return true;
    }

protected:
    template <typename KeyCompare>
    struct rb_tree_impl : node_alloc_type, rb_tree_key_compare<KeyCompare>, header_t {
        using base_key_compare = rb_tree_key_compare<KeyCompare>;

        rb_tree_impl() noexcept(std::is_nothrow_default_constructible_v<node_alloc_type> && std::is_nothrow_default_constructible_v<base_key_compare>)
            : node_alloc_type() {}

        rb_tree_impl(const rb_tree_impl &other)
            : node_alloc_type(node_alloc_traits::select_on_container_copy_construction(other))
            , base_key_compare(other.m_key_compare)
            , header_t() {}

        rb_tree_impl(rb_tree_impl &&) = default;

        explicit rb_tree_impl(node_alloc_type &&a)
            : node_alloc_type(std::move(a)) {}

        rb_tree_impl(const KeyCompare &comp, node_alloc_type &&a)
            : node_alloc_type(std::move(a))
            , base_key_compare(comp) {}

        rb_tree_impl(rb_tree_impl &&other, node_alloc_type &&a)
            : node_alloc_type(std::move(a))
            , base_key_compare(std::move(other))
            , header_t(std::move(other)) {}
    };

    static const Key &key(const node &n) {
        return KeyOfValue()(*n.get_valptr());
    }

    static const Key &key(base_ptr p) {
        return key(static_cast<const node &>(*p));
    }

    static const Key &key(node_ptr p) {
        return key(*p);
    }

    static base_ptr left(base_ptr p) noexcept {
        return p->m_left;
    }

    static node_ptr left(node_ptr p) noexcept {
        return p->m_left ? static_cast<node &>(*p->m_left).get_node_ptr() : node_ptr();
    }

    static base_ptr right(base_ptr p) noexcept {
        return p->m_right;
    }

    static node_ptr right(node_ptr p) noexcept {
        return p->m_right ? static_cast<node &>(*p->m_right).get_node_ptr() : node_ptr();
    }

    node_ptr get_node() {
        using alloc_pointer = typename node_alloc_traits::pointer;
        if constexpr (std::is_same_v<node_ptr, alloc_pointer>) {
            return node_alloc_traits::allocate(get_node_allocator(), 1);
        } else {
            auto ptr = node_alloc_traits::allocate(get_node_allocator(), 1);
            return std::to_address(ptr);
        }
    }

    void put_node(node_ptr p) noexcept {
        using alloc_pointer = typename node_alloc_traits::pointer;
        if constexpr (std::is_same_v<node_ptr, alloc_pointer>) {
            node_alloc_traits::deallocate(get_node_allocator(), p, 1);
        } else {
            auto ap = std::pointer_traits<alloc_pointer>::pointer_to(*p);
            node_alloc_traits::deallocate(get_node_allocator(), ap, 1);
        }
    }

    template <typename... Args>
    void construct_node(node_ptr p, Args &&...args) {
        PVZSTL_TRY {
            ::new (std::addressof(*p)) node;
            node_alloc_traits::construct(get_node_allocator(), p->get_valptr(), std::forward<Args>(args)...);
        }
        PVZSTL_CATCH(...) {
            p->~node();
            put_node(p);
            PVZSTL_THROW_EXCEPTION_AGAIN;
        }
    }

    template <typename... Args>
    node_ptr create_node(Args &&...args) {
        node_ptr p = get_node();
        construct_node(p, std::forward<Args>(args)...);
        return p;
    }

    void destroy_node(node_ptr p) noexcept {
        node_alloc_traits::destroy(get_node_allocator(), p->get_valptr());
        p->~node();
    }

    void drop_node(node_ptr p) noexcept {
        destroy_node(p);
        put_node(p);
    }

    template <bool MoveValue, typename NodeGen>
    node_ptr clone_node(node_ptr x, NodeGen &node_gen) {
        using Vp = std::conditional_t<MoveValue, value_type &&, const value_type &>;
        node_ptr tmp = node_gen(std::forward<Vp>(*x->get_valptr()));
        tmp->m_color = x->m_color;
        tmp->m_left = tmp->m_right = base_ptr();
        return tmp;
    }

    base_ptr &root() noexcept {
        return m_impl.m_header.m_parent;
    }

    base_ptr root() const noexcept {
        return m_impl.m_header.m_parent;
    }

    base_ptr &leftmost() noexcept {
        return m_impl.m_header.m_left;
    }

    base_ptr leftmost() const noexcept {
        return m_impl.m_header.m_left;
    }

    base_ptr &rightmost() noexcept {
        return m_impl.m_header.m_right;
    }

    base_ptr rightmost() const noexcept {
        return m_impl.m_header.m_right;
    }

    base_ptr _begin() const noexcept {
        return m_impl.m_header.m_parent;
    }

    node_ptr begin_node() const noexcept {
        base_ptr beg = m_impl.m_header.m_parent;
        return beg ? static_cast<node *>(beg)->get_node_ptr() : node_ptr();
    }

    base_ptr _end() const noexcept {
        return m_impl.m_header.get_base_ptr();
    }

    template <typename Key1, typename Key2>
    bool key_compare(const Key1 &k1, const Key2 &k2) const {
        static_assert(std::is_invocable_r_v<bool, const Compare &, const Key &, const Key &>, "comparison object must be invocable with arguments of key_type");
        return m_impl.m_key_compare(k1, k2);
    }

    rb_tree_impl<Compare> m_impl;

private:
    enum { as_lvalue, as_rvalue };

    // Functor recycling a pool of nodes and using allocation once the pool
    // is empty.
    struct reuse_or_alloc_node {
        reuse_or_alloc_node(rb_tree &t)
            : m_root(t.root())
            , m_nodes(t.rightmost())
            , m_t(t) {
            if (m_root) {
                m_root->m_parent = base_ptr();

                if (m_nodes->m_left) {
                    m_nodes = m_nodes->m_left;
                }
            } else {
                m_nodes = base_ptr();
            }
        }

        reuse_or_alloc_node(const reuse_or_alloc_node &) = delete;

        ~reuse_or_alloc_node() {
            if (m_root) {
                m_t._erase(static_cast<node &>(*m_root).get_node_ptr());
            }
        }

        template <typename Arg>
        node_ptr operator()(Arg &&arg) {
            base_ptr base = extract();
            if (base) {
                node_ptr p = static_cast<node &>(*base).get_node_ptr();
                m_t.destroy_node(p);
                m_t.construct_node(p, std::forward<Arg>(arg));
                return p;
            }

            return m_t.create_node(std::forward<Arg>(arg));
        }

    private:
        base_ptr extract() {
            if (!m_nodes) {
                return m_nodes;
            }

            base_ptr p = m_nodes;
            m_nodes = m_nodes->m_parent;
            if (m_nodes) {
                if (m_nodes->m_right == p) {
                    m_nodes->m_right = base_ptr();

                    if (m_nodes->m_left) {
                        m_nodes = m_nodes->m_left;

                        while (m_nodes->m_right) {
                            m_nodes = m_nodes->m_right;
                        }
                        if (m_nodes->m_left) {
                            m_nodes = m_nodes->m_left;
                        }
                    }
                } else {
                    // node is on the left.
                    m_nodes->m_left = base_ptr();
                }
            } else {
                m_root = base_ptr();
            }

            return p;
        }

        base_ptr m_root;
        base_ptr m_nodes;
        rb_tree &m_t;
    };

    struct alloc_node {
        alloc_node(rb_tree &t) noexcept
            : m_t(t) {}

        template <typename Arg>
        node_ptr operator()(Arg &&arg) const {
            return m_t.create_node(std::forward<Arg>(arg));
        }

    private:
        rb_tree &m_t;
    };

    // An RAII Node handle
    struct auto_node {
        template <typename... Args>
        auto_node(rb_tree &t, Args &&...args)
            : m_t(t)
            , m_node(t.create_node(std::forward<Args>(args)...)) {}

        auto_node(auto_node &&other) noexcept
            : m_t(other.m_t)
            , m_node(other.m_node) {
            other.m_node = nullptr;
        }

        ~auto_node() {
            if (m_node) {
                m_t.drop_node(m_node);
            }
        }

        const Key &key() const {
            return rb_tree::key(m_node);
        }

        iterator insert(std::pair<base_ptr, base_ptr> p) {
            auto it = m_t.insert_node(p.first, p.second, m_node);
            m_node = nullptr;
            return it;
        }

        iterator insert_equal_lower() {
            auto it = m_t.insert_equal_lower_node(m_node);
            m_node = nullptr;
            return it;
        }

        rb_tree &m_t;
        node_ptr m_node;
    };

    static node_ptr adapt(typename node_alloc_traits::pointer ptr) {
        using alloc_ptr = typename node_alloc_traits::pointer;
        if constexpr (std::is_same_v<node_ptr, alloc_ptr>) {
            return ptr;
        } else {
            return std::to_address(ptr);
        }
    }

    rb_tree(rb_tree &&other, node_alloc_type &&a, std::true_type) noexcept(std::is_nothrow_default_constructible_v<Compare>)
        : m_impl(std::move(other.m_impl), std::move(a)) {}

    rb_tree(rb_tree &&other, node_alloc_type &&a, std::false_type)
        : m_impl(other.m_impl.m_key_compare, std::move(a)) {
        if (other.root()) {
            move_data(other, std::false_type());
        }
    }

    // Move elements from container with equal allocator.
    void move_data(rb_tree &other, std::true_type) noexcept {
        m_impl.move_data(other.m_impl);
    }

    // Move elements from container with possibly non-equal allocator,
    // which might result in a copy not a move.
    void move_data(rb_tree &other, std::false_type) {
        if (get_node_allocator() == other.get_node_allocator()) {
            move_data(other, std::true_type());
        } else {
            constexpr bool move = !move_if_noexcept_cond<value_type>;
            alloc_node an(*this);
            root() = copy<move>(other, an);
            if constexpr (move) {
                other.clear();
            }
        }
    }

    // Move assignment from container with equal allocator.
    void move_assign(rb_tree &other, std::true_type) noexcept {
        clear();
        if (other.root()) {
            move_data(other, std::true_type());
        }
        alloc_on_move(get_node_allocator(), other.get_node_allocator());
    }

    // Move assignment from container with possibly non-equal allocator,
    // which might result in a copy not a move.
    void move_assign(rb_tree &other, std::false_type) {
        if (get_node_allocator() == other.get_node_allocator()) {
            move_assign(other, std::true_type());
            return;
        }

        // Try to move each node reusing existing nodes and copying x nodes
        // structure.
        reuse_or_alloc_node roan(*this);
        m_impl.reset();
        if (other.root()) {
            root() = copy<as_rvalue>(other, roan);
            other.clear();
        }
    }

    template <typename Arg, typename NodeGen>
    iterator insert(base_ptr x, base_ptr p, Arg &&arg, NodeGen &node_gen) {
        bool insert_left = (x || p == _end() || key_compare(KeyOfValue()(arg), key(p)));

        base_ptr z = node_gen(std::forward<Arg>(arg))->get_base_ptr();
        node_traits::insert_and_rebalance(insert_left, z, p, m_impl.m_header);
        ++m_impl.m_node_count;
        return iterator(z);
    }

    template <typename Arg>
    iterator insert_lower(base_ptr p, Arg &&arg) {
        bool insert_left = (p == _end() || !key_compare(key(p), KeyOfValue()(arg)));

        base_ptr z = create_node(std::forward<Arg>(arg))->get_base_ptr();
        node_traits::insert_and_rebalance(insert_left, z, p, m_impl.m_header);
        ++m_impl.m_node_count;
        return iterator(z);
    }

    template <typename Arg>
    iterator insert_equal_lower(Arg &&arg) {
        base_ptr x = _begin();
        base_ptr y = _end();
        while (x) {
            y = x;
            x = !key_compare(key(x), KeyOfValue()(arg)) ? left(x) : right(x);
        }
        return insert_lower(y, std::forward<Arg>(arg));
    }

    iterator insert_node(base_ptr x, base_ptr p, node_ptr z) {
        bool insert_left = (x || p == _end() || key_compare(key(z), key(p)));

        base_ptr base_z = z->get_base_ptr();
        node_traits::insert_and_rebalance(insert_left, base_z, p, m_impl.m_header);
        ++m_impl.m_node_count;
        return iterator(base_z);
    }

    iterator insert_lower_node(base_ptr p, node_ptr z) {
        bool insert_left = (p == _end() || !key_compare(key(p), key(z)));

        base_ptr base_z = z->get_base_ptr();
        node_traits::insert_and_rebalance(insert_left, base_z, p, m_impl.m_header);
        ++m_impl.m_node_count;
        return iterator(base_z);
    }

    iterator insert_equal_lower_node(node_ptr z) {
        base_ptr x = _begin();
        base_ptr y = _end();
        while (x) {
            y = x;
            x = !key_compare(key(x), key(z)) ? left(x) : right(x);
        }
        return insert_lower_node(y, z);
    }

    template <bool MoveValue, typename NodeGen>
    base_ptr copy(node_ptr x, base_ptr p, NodeGen &node_gen) {
        // Structural copy. x and p must be non-null.
        node_ptr top = clone_node<MoveValue>(x, node_gen);
        base_ptr top_base = top->get_base_ptr();
        top->m_parent = p;

        PVZSTL_TRY {
            if (x->m_right) {
                top->m_right = copy<MoveValue>(right(x), top_base, node_gen);
            }
            p = top_base;
            x = left(x);

            while (x) {
                base_ptr y = clone_node<MoveValue>(x, node_gen)->get_base_ptr();
                p->m_left = y;
                y->m_parent = p;
                if (x->m_right) {
                    y->m_right = copy<MoveValue>(right(x), y, node_gen);
                }
                p = y;
                x = left(x);
            }
        }
        PVZSTL_CATCH(...) {
            _erase(top);
            PVZSTL_THROW_EXCEPTION_AGAIN;
        }
        return top_base;
    }

    template <bool MoveValue, typename NodeGen>
    base_ptr copy(const rb_tree &other, NodeGen &gen) {
        base_ptr root = copy<MoveValue>(other.begin_node(), _end(), gen);
        leftmost() = node_base::minimum(root);
        rightmost() = node_base::maximum(root);
        m_impl.m_node_count = other.m_impl.m_node_count;
        return root;
    }

    base_ptr copy(const rb_tree &other) {
        alloc_node an(*this);
        return copy<as_lvalue>(other, an);
    }

    void _erase(node_ptr x) {
        // Erase without rebalancing.
        while (x) {
            _erase(right(x));
            node_ptr y = left(x);
            drop_node(x);
            x = y;
        }
    }

    base_ptr _lower_bound(base_ptr x, base_ptr y, const Key &k) const {
        while (x) {
            if (!key_compare(key(x), k)) {
                y = x;
                x = left(x);
            } else {
                x = right(x);
            }
        }
        return y;
    }

    base_ptr _upper_bound(base_ptr x, base_ptr y, const Key &k) const {
        while (x) {
            if (key_compare(k, key(x))) {
                y = x;
                x = left(x);
            } else {
                x = right(x);
            }
        }
        return y;
    }

    void erase_aux(const_iterator pos) {
        base_ptr y = node_traits::rebalance_for_erase(pos.m_node, m_impl.m_header);
        drop_node(static_cast<node &>(*y).get_node_ptr());
        ++m_impl.m_node_count;
    }

    void erase_aux(const_iterator first, const_iterator last) {
        if (first == begin() && last == end()) {
            clear();
        } else {
            while (first != last) {
                erase_aux(first++);
            }
        }
    }

    friend bool operator==(const rb_tree &lhs, const rb_tree &rhs) {
        return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    friend auto operator<=>(const rb_tree &lhs, const rb_tree &rhs) {
        static_assert(requires { typename synth3way_t<Val>; });
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), synth3way);
    }
};

template <typename Key, typename Val, typename KeyOfValue, typename Compare, typename Alloc>
void swap(rb_tree<Key, Val, KeyOfValue, Compare, Alloc> &lhs, rb_tree<Key, Val, KeyOfValue, Compare, Alloc> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

// Allow access to internals of compatible rb_tree specializations.
template <typename Key, typename Val, typename Sel, typename Cmp1, typename Alloc, typename Cmp2>
struct rb_tree_merge_helper<rb_tree<Key, Val, Sel, Cmp1, Alloc>, Cmp2> {
private:
    friend class rb_tree<Key, Val, Sel, Cmp1, Alloc>;

    static auto &get_impl(rb_tree<Key, Val, Sel, Cmp2, Alloc> &tree) noexcept {
        return tree.m_impl;
    }
};

template <typename Kt, typename Container>
concept heterogeneous_tree_key = transparent_comparator<typename Container::key_compare> && heterogeneous_key<Kt, Container>;

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_STL_TREE_H
