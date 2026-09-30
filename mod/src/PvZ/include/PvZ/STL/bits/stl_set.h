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

#ifndef PVZ_STL_BITS_STL_SET_H
#define PVZ_STL_BITS_STL_SET_H

/**
 * @file bits/stl_set.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00434.html">stl_set.h File Reference</a>
 */

#include "PvZ/STL/bits/ranges_base.h"
#include "PvZ/STL/bits/stl_iterator_base_types.h"
#include "PvZ/STL/bits/stl_tree.h"

#include <functional>

namespace pvzstl {

/**
 * @brief A standard container made up of unique keys, which can be
 * retrieved in logarithmic time.
 */
template <typename Key, typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
class set {
    static_assert(std::is_invocable_r_v<bool, Compare, Key, Key>);
    static_assert(std::is_same_v<typename std::remove_cv_t<Key>, Key>, "std::set must have a non-const, non-volatile value_type");
    static_assert(std::is_same_v<typename Alloc::value_type, Key>, "std::set must have the same value_type as its allocator");

public:
    using key_type = Key;
    using value_type = Key;
    using key_compare = Compare;
    using value_compare = Compare;
    using allocator_type = Alloc;

private:
    using key_alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<Key>;

    using rep_type = detail::rb_tree<key_type, value_type, detail::Identity<value_type>, key_compare, key_alloc_type>;

    using alloc_traits = std::allocator_traits<key_alloc_type>;

public:
    using pointer = typename alloc_traits::pointer;
    using const_pointer = typename alloc_traits::const_pointer;
    using reference = value_type &;
    using const_reference = const value_type &;
    // DR 103. set::iterator is required to be modifiable,
    // but this allows modification of keys.
    using iterator = typename rep_type::const_iterator;
    using const_iterator = typename rep_type::const_iterator;
    using reverse_iterator = typename rep_type::const_reverse_iterator;
    using const_reverse_iterator = typename rep_type::const_reverse_iterator;
    using size_type = typename rep_type::size_type;
    using difference_type = typename rep_type::difference_type;

    using node_type = typename rep_type::node_type;
    using insert_return_type = typename rep_type::insert_return_type;

    set() = default;

    set(const set &) = default;

    set(set &&) = default;

    set(const set &other, const std::type_identity_t<allocator_type> &a)
        : m_t(other.m_t, key_alloc_type(a)) {}

    set(set &&other, const std::type_identity_t<allocator_type> &a) noexcept(std::is_nothrow_copy_constructible_v<Compare> && alloc_traits::is_always_equal::value)
        : m_t(std::move(other.m_t), key_alloc_type(a)) {}

    explicit set(const Compare &comp, const allocator_type &a = allocator_type())
        : m_t(comp, key_alloc_type(a)) {}

    explicit set(const allocator_type &a)
        : m_t(key_alloc_type(a)) {}

    template <typename InputIterator>
    set(InputIterator first, InputIterator last, const Compare &comp, const allocator_type &a = allocator_type())
        : m_t(comp, key_alloc_type(a)) {
        m_t.insert_range_unique(first, last);
    }

    template <typename InputIterator>
    set(InputIterator first, InputIterator last, const allocator_type &a)
        : m_t(key_alloc_type(a)) {
        m_t.insert_range_unique(first, last);
    }

    template <typename InputIterator>
    set(InputIterator first, InputIterator last)
        : m_t() {
        m_t.insert_range_unique(first, last);
    }

    template <detail::container_compatible_range<value_type> Rg>
    set(std::from_range_t, Rg &&rg, const Compare &comp, const Alloc &a = Alloc())
        : m_t(comp, key_alloc_type(a)) {
        insert_range(std::forward<Rg>(rg));
    }

    template <detail::container_compatible_range<value_type> Rg>
    set(std::from_range_t, Rg &&rg, const Alloc &a = Alloc())
        : m_t(key_alloc_type(a)) {
        insert_range(std::forward<Rg>(rg));
    }

    set(std::initializer_list<value_type> il, const Compare &comp = Compare(), const allocator_type &a = allocator_type())
        : m_t(comp, key_alloc_type(a)) {
        m_t.insert_range_unique(il.begin(), il.end());
    }

    set(std::initializer_list<value_type> il, const allocator_type &a)
        : m_t(key_alloc_type(a)) {
        m_t.insert_range_unique(il.begin(), il.end());
    }

    ~set() = default;

    set &operator=(const set &) = default;

    set &operator=(set &&) = default;

    set &operator=(std::initializer_list<value_type> il) {
        m_t.assign_unique(il.begin(), il.end());
        return *this;
    }

    allocator_type get_allocator() const noexcept {
        return allocator_type(m_t.get_allocator());
    }

    iterator begin() noexcept {
        return m_t.begin();
    }

    const_iterator begin() const noexcept {
        return m_t.begin();
    }

    iterator end() noexcept {
        return m_t.end();
    }

    const_iterator end() const noexcept {
        return m_t.end();
    }

    reverse_iterator rbegin() noexcept {
        return m_t.rbegin();
    }

    const_reverse_iterator rbegin() const noexcept {
        return m_t.rbegin();
    }

    reverse_iterator rend() noexcept {
        return m_t.rend();
    }

    const_reverse_iterator rend() const noexcept {
        return m_t.rend();
    }

    iterator cbegin() const noexcept {
        return m_t.begin();
    }

    const_iterator cend() const noexcept {
        return m_t.end();
    }

    const_reverse_iterator crbegin() const noexcept {
        return m_t.rbegin();
    }

    const_reverse_iterator crend() const noexcept {
        return m_t.rend();
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_t.empty();
    }

    size_type size() const noexcept {
        return m_t.size();
    }

    size_type max_size() const noexcept {
        return m_t.max_size();
    }

    void clear() noexcept {
        m_t.clear();
    }

    std::pair<iterator, bool> insert(const value_type &v) {
        return m_t.insert_unique(v);
    }

    std::pair<iterator, bool> insert(value_type &&v) {
        return m_t.insert_unique(std::move(v));
    }

    iterator insert(const_iterator pos, const value_type &v) {
        return m_t.insert_unique(pos, v);
    }

    iterator insert(const_iterator pos, value_type &&v) {
        return m_t.insert_unique(pos, std::move(v));
    }

    template <typename Pair>
        requires std::is_constructible_v<value_type, Pair &&>
    iterator insert(const_iterator pos, Pair &&p) {
        return m_t.emplace_hint_unique(pos, std::forward<Pair>(p));
    }

    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last) {
        m_t.insert_range_unique(first, last);
    }

    void insert(std::initializer_list<value_type> il) {
        insert(il.begin(), il.end());
    }

    insert_return_type insert(node_type &&nh) {
        return m_t.reinsert_node_unique(std::move(nh));
    }

    iterator insert(const_iterator pos, node_type &&nh) {
        return m_t.reinsert_node_hint_unique(pos, std::move(nh));
    }

    template <detail::container_compatible_range<value_type> Rg>
    void insert_range(Rg &&rg) {
        auto first = std::ranges::begin(rg);
        const auto last = std::ranges::end(rg);
        using Rv = std::remove_cvref_t<std::ranges::range_reference_t<Rg>>;
        for (; first != last; ++first) {
            if constexpr (std::is_same_v<Rv, value_type>) {
                m_t.insert_unique(*first);
            } else {
                m_t.emplace_unique(*first);
            }
        }
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        return m_t.emplace_unique(std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args &&...args) {
        return m_t.emplace_hint_unique(hint, std::forward<Args>(args)...);
    }

    iterator erase(iterator pos) {
        return m_t.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last) {
        return m_t.erase(first, last);
    }

    size_type erase(const key_type &k) {
        return m_t.erase_unique(k);
    }

    template <detail::heterogeneous_tree_key<set> Kt>
    size_type erase(Kt &&k) {
        return m_t.erase_tr(k);
    }

    void swap(set &other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        m_t.swap(other.m_t);
    }

    node_type extract(const_iterator pos) {
        assert(pos != end());
        return m_t.extract(pos);
    }

    node_type extract(const key_type &k) {
        return m_t.extract(k);
    }

    template <detail::heterogeneous_tree_key<set> Kt>
    node_type extract(Kt &&k) {
        return m_t.extract_tr(k);
    }

    template <typename, typename>
    friend struct detail::rb_tree_merge_helper;

    template <typename Compare2>
    void merge(set<Key, Compare2, Alloc> &src) {
        using merge_helper = detail::rb_tree_merge_helper<set, Compare2>;
        m_t.merge_unique(merge_helper::get_tree(src));
    }

    template <typename Compare2>
    void merge(set<Key, Compare2, Alloc> &&src) {
        merge(src);
    }

    size_type count(const key_type &k) const {
        return m_t.find(k) == m_t.end() ? 0 : 1;
    }

    template <typename Kt>
    size_type count(const Kt &k) const {
        return m_t.count_tr(k);
    }

    iterator find(const key_type &k) {
        return m_t.find(k);
    }

    const_iterator find(const key_type &k) const {
        return m_t.find(k);
    }

    template <typename Kt>
    iterator find(const Kt &k) {
        return m_t.find_tr(k);
    }

    template <typename Kt>
    const_iterator find(const Kt &k) const {
        return m_t.find_tr(k);
    }

    bool contains(const key_type &k) const {
        return m_t.find(k) != m_t.end();
    }

    template <typename Kt>
    bool contains(const Kt &k) const {
        return m_t.find_tr(k) != m_t.end();
    }

    std::pair<iterator, iterator> equal_range(const key_type &k) {
        return m_t.equal_range(k);
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type &k) const {
        return m_t.equal_range(k);
    }

    template <typename Kt>
    std::pair<iterator, iterator> equal_range(const Kt &k) {
        return m_t.equal_range_tr(k);
    }

    template <typename Kt>
    std::pair<const_iterator, const_iterator> equal_range(const Kt &k) const {
        return m_t.equal_range_tr(k);
    }

    iterator lower_bound(const key_type &k) {
        return m_t.lower_bound(k);
    }

    const_iterator lower_bound(const key_type &k) const {
        return m_t.lower_bound(k);
    }

    template <typename Kt>
    iterator lower_bound(const Kt &k) {
        return m_t.lower_bound_tr(k);
    }

    template <typename Kt>
    const_iterator lower_bound(const Kt &k) const {
        return m_t.lower_bound_tr(k);
    }

    iterator upper_bound(const key_type &k) {
        return m_t.upper_bound(k);
    }

    const_iterator upper_bound(const key_type &k) const {
        return m_t.upper_bound(k);
    }

    template <typename Kt>
    iterator upper_bound(const Kt &k) {
        return m_t.upper_bound_tr(k);
    }

    template <typename Kt>
    const_iterator upper_bound(const Kt &k) const {
        return m_t.upper_bound_tr(k);
    }

    friend bool operator==(const set &lhs, const set &rhs) {
        return lhs.m_t == rhs.m_t;
    }

    friend auto operator<=>(const set &lhs, const set &rhs) {
        return lhs.m_t <=> rhs.m_t;
    }

private:
    rep_type m_t; // Red-black tree representing set.
};

template <detail::has_input_iter_cat InputIterator,
          detail::not_allocator_like Compare = std::less<typename std::iterator_traits<InputIterator>::value_type>,
          detail::allocator_like Allocator = std::allocator<typename std::iterator_traits<InputIterator>::value_type>>
set(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator()) -> set<typename std::iterator_traits<InputIterator>::value_type, Compare, Allocator>;

template <typename Key, detail::not_allocator_like Compare = std::less<Key>, detail::allocator_like Allocator = std::allocator<Key>>
set(std::initializer_list<Key>, Compare = Compare(), Allocator = Allocator()) -> set<Key, Compare, Allocator>;

template <detail::has_input_iter_cat InputIterator, detail::allocator_like Allocator>
set(InputIterator, InputIterator, Allocator) -> set<typename std::iterator_traits<InputIterator>::value_type, std::less<typename std::iterator_traits<InputIterator>::value_type>, Allocator>;

template <typename Key, detail::allocator_like Allocator>
set(std::initializer_list<Key>, Allocator) -> set<Key, std::less<Key>, Allocator>;

template <std::ranges::input_range Rg,
          detail::not_allocator_like Compare = std::less<std::ranges::range_value_t<Rg>>,
          detail::allocator_like Allocator = std::allocator<std::ranges::range_value_t<Rg>>>
set(std::from_range_t, Rg &&, Compare = Compare(), Allocator = Allocator()) -> set<std::ranges::range_value_t<Rg>, Compare, Allocator>;

template <std::ranges::input_range Rg, detail::allocator_like Allocator>
set(std::from_range_t, Rg &&, Allocator) -> set<std::ranges::range_value_t<Rg>, std::less<std::ranges::range_value_t<Rg>>, Allocator>;

template <typename Key, typename Compare, typename Alloc>
void swap(set<Key, Compare, Alloc> &lhs, set<Key, Compare, Alloc> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

// Allow pvzstl::set access to internals of compatible sets.
template <typename Val, typename Cmp1, typename Alloc, typename Cmp2>
struct detail::rb_tree_merge_helper<set<Val, Cmp1, Alloc>, Cmp2> {
private:
    friend class set<Val, Cmp1, Alloc>;

    static auto &get_tree(set<Val, Cmp2, Alloc> &set) noexcept {
        return set.m_t;
    }
};

} // namespace pvzstl

#endif // PVZ_STL_BITS_STL_SET_H
