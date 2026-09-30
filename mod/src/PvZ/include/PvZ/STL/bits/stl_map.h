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

#ifndef PVZ_STL_BITS_STL_MAP_H
#define PVZ_STL_BITS_STL_MAP_H

/**
 * @file bits/stl_map.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00446.html">stl_map.h File Reference</a>
 */

#include "PvZ/STL/bits/ranges_base.h"
#include "PvZ/STL/bits/stl_iterator.h"
#include "PvZ/STL/bits/stl_iterator_base_types.h"
#include "PvZ/STL/bits/stl_pair.h"
#include "PvZ/STL/bits/stl_tree.h"

#include <functional>

namespace pvzstl {

/**
 * @brief A standard container made up of (key,value) pairs, which can be
 * retrieved based on a key, in logarithmic time.
 */
template <typename Key, typename Tp, typename Compare = std::less<Key>, typename Alloc = std::allocator<std::pair<const Key, Tp>>>
class map {
public:
    using key_type = Key;
    using mapped_type = Tp;
    using value_type = std::pair<const Key, Tp>;
    using key_compare = Compare;
    using allocator_type = Alloc;

    static_assert(std::is_invocable_r_v<bool, Compare, Key, Key>);
    static_assert(std::is_same_v<typename Alloc::value_type, value_type>, "std::map must have the same value_type as its allocator");

    class value_compare : public binary_function<value_type, value_type, bool> {
        friend class map<Key, Tp, Compare, Alloc>;

    public:
        bool operator()(const value_type &x, const value_type &y) const {
            return m_comp(x.first, y.first);
        }

    protected:
        value_compare(Compare c)
            : m_comp(c) {}

        Compare m_comp;
    };

private:
    using pair_alloc_type = std::allocator_traits<Alloc>::template rebind_alloc<value_type>;

    using rep_type = detail::rb_tree<key_type, value_type, detail::Select1st<value_type>, key_compare, pair_alloc_type>;

    using alloc_traits = std::allocator_traits<pair_alloc_type>;

public:
    // many of these are specified differently in ISO, but the following are
    // "functionally equivalent"
    using pointer = alloc_traits::pointer;
    using const_pointer = alloc_traits::const_pointer;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = rep_type::iterator;
    using const_iterator = rep_type::const_iterator;
    using size_type = rep_type::size_type;
    using difference_type = rep_type::difference_type;
    using reverse_iterator = rep_type::reverse_iterator;
    using const_reverse_iterator = rep_type::const_reverse_iterator;

    using node_type = typename rep_type::node_type;
    using insert_return_type = typename rep_type::insert_return_type;

    map() = default;

    map(const map &) = default;

    map(map &&) = default;

    map(const map &other, const std::type_identity_t<allocator_type> &a)
        : m_t(other.m_t, pair_alloc_type(a)) {}

    map(map &&other, const std::type_identity_t<allocator_type> &a) noexcept(std::is_nothrow_copy_constructible_v<Compare> && alloc_traits::is_always_equal::value)
        : m_t(std::move(other.m_t), pair_alloc_type(a)) {}

    explicit map(const Compare &comp, const allocator_type &a = allocator_type())
        : m_t(comp, pair_alloc_type(a)) {}

    explicit map(const allocator_type &a)
        : m_t(pair_alloc_type(a)) {}

    template <typename InputIterator>
    map(InputIterator first, InputIterator last, const Compare &comp, const allocator_type &a = allocator_type())
        : m_t(comp, pair_alloc_type(a)) {
        m_t.insert_range_unique(first, last);
    }

    template <typename InputIterator>
    map(InputIterator first, InputIterator last, const allocator_type &a)
        : m_t(pair_alloc_type(a)) {
        m_t.insert_range_unique(first, last);
    }

    template <typename InputIterator>
    map(InputIterator first, InputIterator last)
        : m_t() {
        m_t.insert_range_unique(first, last);
    }

    template <detail::container_compatible_range<value_type> Rg>
    map(std::from_range_t, Rg &&rg, const Compare &comp, const Alloc &a = Alloc())
        : m_t(comp, pair_alloc_type(a)) {
        insert_range(std::forward<Rg>(rg));
    }

    template <detail::container_compatible_range<value_type> Rg>
    map(std::from_range_t, Rg &&rg, const Alloc &a = Alloc())
        : m_t(pair_alloc_type(a)) {
        insert_range(std::forward<Rg>(rg));
    }

    map(std::initializer_list<value_type> il, const Compare &comp = Compare(), const allocator_type &a = allocator_type())
        : m_t(comp, pair_alloc_type(a)) {
        m_t.insert_range_unique(il.begin(), il.end());
    }

    map(std::initializer_list<value_type> il, const allocator_type &a)
        : m_t(pair_alloc_type(a)) {
        m_t.insert_range_unique(il.begin(), il.end());
    }

    ~map() = default;

    map &operator=(const map &) = default;

    map &operator=(map &&) = default;

    map &operator=(std::initializer_list<value_type> il) {
        m_t.assign_unique(il.begin(), il.end());
        return *this;
    }

    allocator_type get_allocator() const noexcept {
        return allocator_type(m_t.get_allocator());
    }

    mapped_type &at(const key_type &k) {
        iterator it = lower_bound(k);
        if (it == end() || key_comp()(k, (*it).first)) {
            detail::throw_out_of_range("map::at");
        }
        return (*it).second;
    }

    const mapped_type &at(const key_type &k) const {
        const_iterator it = lower_bound(k);
        if (it == end() || key_comp()(k, (*it).first)) {
            detail::throw_out_of_range("map::at");
        }
        return (*it).second;
    }

    mapped_type &operator[](const key_type &k) {
        static_assert(std::is_default_constructible_v<mapped_type>);

        iterator it = lower_bound(k);
        // i->first is greater than or equivalent to k.
        if (it == end() || key_comp()(k, (*it).first)) {
            it = m_t.emplace_hint_unique(it, std::piecewise_construct, std::forward_as_tuple(k), std::tuple<>());
        }
        return (*it).second;
    }

    mapped_type &operator[](key_type &&k) {
        static_assert(std::is_default_constructible_v<mapped_type>);

        iterator it = lower_bound(k);
        // i->first is greater than or equivalent to k.
        if (it == end() || key_comp()(k, (*it).first)) {
            it = m_t.emplace_hint_unique(it, std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::tuple<>());
        }
        return (*it).second;
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

    const_iterator cbegin() const noexcept {
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

    template <typename Pair>
        requires std::is_constructible_v<value_type, Pair &&>
    std::pair<iterator, bool> insert(Pair &&p) {
        using P2 = std::remove_reference_t<Pair>;
        if constexpr (detail::is_pair<std::remove_const_t<P2>>) {
            if constexpr (std::is_same_v<allocator_type, std::allocator<value_type>>) {
                if constexpr (usable_key<typename P2::first_type>) {
                    const key_type &k = p.first;
                    iterator it = lower_bound(k);
                    if (it == end() || key_comp()(k, (*it).first)) {
                        it = emplace_hint(it, std::forward<Pair>(p));
                        return {it, true};
                    }
                    return {it, false};
                }
            }
        }
        return m_t.emplace_unique(std::forward<Pair>(p));
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
        for (; first != last; ++first)
            insert(*first);
    }

    template <typename Obj>
    std::pair<iterator, bool> insert_or_assign(const key_type &k, Obj &&obj) {
        iterator it = lower_bound(k);
        if (it == end() || key_comp()(k, (*it).first)) {
            it = emplace_hint(it, std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(std::forward<Obj>(obj)));
            return {it, true};
        }
        (*it).second = std::forward<Obj>(obj);
        return {it, false};
    }

    template <typename Obj>
    std::pair<iterator, bool> insert_or_assign(key_type &&k, Obj &&obj) {
        iterator it = lower_bound(k);
        if (it == end() || key_comp()(k, (*it).first)) {
            it = emplace_hint(it, std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<Obj>(obj)));
            return {it, true};
        }
        (*it).second = std::forward<Obj>(obj);
        return {it, false};
    }

    template <typename Obj>
    iterator insert_or_assign(const_iterator hint, const key_type &k, Obj &&obj) {
        iterator it;
        auto true_hint = m_t.get_insert_hint_unique_pos(hint, k);
        if (true_hint.second) {
            it = emplace_hint(iterator(true_hint.second), std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(std::forward<Obj>(obj)));
        } else {
            it = iterator(true_hint.first);
            (*it).second = std::forward<Obj>(obj);
        }
        return it;
    }

    template <typename Obj>
    iterator insert_or_assign(const_iterator hint, key_type &&k, Obj &&obj) {
        iterator it;
        auto true_hint = m_t.get_insert_hint_unique_pos(hint, k);
        if (true_hint.second) {
            it = emplace_hint(iterator(true_hint.second), std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<Obj>(obj)));
        } else {
            it = iterator(true_hint.first);
            (*it).second = std::forward<Obj>(obj);
        }
        return it;
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        if constexpr (sizeof...(Args) == 2) {
            if constexpr (std::is_same_v<allocator_type, std::allocator<value_type>>) {
                auto &&[a, v] = std::pair<Args &...>(args...);
                if constexpr (usable_key<decltype(a)>) {
                    const key_type &k = a;
                    iterator it = lower_bound(k);
                    if (it == end() || key_comp()(k, (*it).first)) {
                        it = emplace_hint(it, std::forward<Args>(args)...);
                        return {it, true};
                    }
                    return {it, false};
                }
            }
        }
        return m_t.emplace_unique(std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args &&...args) {
        return m_t.emplace_hint_unique(hint, std::forward<Args>(args)...);
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const key_type &k, Args &&...args) {
        iterator it = lower_bound(k);
        if (it == end() || key_comp()(k, (*it).first)) {
            it = emplace_hint(it, std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(std::forward<Args>(args)...));
            return {it, true};
        }
        return {it, false};
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(key_type &&k, Args &&...args) {
        iterator it = lower_bound(k);
        if (it == end() || key_comp()(k, (*it).first)) {
            it = emplace_hint(it, std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<Args>(args)...));
            return {it, true};
        }
        return {it, false};
    }

    template <typename... Args>
    iterator try_emplace(const_iterator hint, const key_type &k, Args &&...args) {
        iterator it;
        auto true_hint = m_t.get_insert_hint_unique_pos_tr(hint, k);
        if (true_hint.second) {
            it = emplace_hint(iterator(true_hint.second), std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(std::forward<Args>(args)...));
        } else {
            it = iterator(true_hint.first);
        }
        return it;
    }

    template <typename... Args>
    iterator try_emplace(const_iterator hint, key_type &&k, Args &&...args) {
        iterator it;
        auto true_hint = m_t.get_insert_hint_unique_pos_tr(hint, k);
        if (true_hint.second) {
            it = emplace_hint(iterator(true_hint.second), std::piecewise_construct, std::forward_as_tuple(std::move(k)), std::forward_as_tuple(std::forward<Args>(args)...));
        } else {
            it = iterator(true_hint.first);
        }
        return it;
    }

    iterator erase(iterator pos) {
        return m_t.erase(pos);
    }

    iterator erase(const_iterator pos) {
        return m_t.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last) {
        return m_t.erase(first, last);
    }

    size_type erase(const key_type &k) {
        return m_t.erase_unique(k);
    }

    template <detail::heterogeneous_tree_key<map> Kt>
    size_type erase(Kt &&k) {
        return m_t.erase_tr(k);
    }

    void swap(map &other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        m_t.swap(other.m_t);
    }

    node_type extract(const_iterator pos) {
        assert(pos != end());
        return m_t.extract(pos);
    }

    node_type extract(const key_type &k) {
        return m_t.extract(k);
    }

    template <detail::heterogeneous_tree_key<map> Kt>
    node_type extract(Kt &&k) {
        return m_t.extract_tr(k);
    }

    template <typename, typename>
    friend struct detail::rb_tree_merge_helper;

    template <typename Compare2>
    void merge(map<Key, Tp, Compare2, Alloc> &src) {
        using merge_helper = detail::rb_tree_merge_helper<map, Compare2>;
        m_t.merge_unique(merge_helper::get_tree(src));
    }

    template <typename Compare2>
    void merge(map<Key, Tp, Compare2, Alloc> &&src) {
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
        return iterator(m_t.lower_bound_tr(k));
    }

    template <typename Kt>
    const_iterator lower_bound(const Kt &k) const {
        return const_iterator(m_t.lower_bound_tr(k));
    }

    iterator upper_bound(const key_type &k) {
        return m_t.upper_bound(k);
    }

    const_iterator upper_bound(const key_type &k) const {
        return m_t.upper_bound(k);
    }

    template <typename Kt>
    iterator upper_bound(const Kt &k) {
        return iterator(m_t.upper_bound_tr(k));
    }

    template <typename Kt>
    const_iterator upper_bound(const Kt &k) const {
        return const_iterator(m_t.upper_bound_tr(k));
    }

    key_compare key_comp() const {
        return m_t.key_comp();
    }

    value_compare value_comp() const {
        return value_compare(m_t.key_comp());
    }

    friend bool operator==(const map &lhs, const map &rhs) {
        return lhs.m_t == rhs.m_t;
    }

    friend auto operator<=>(const map &lhs, const map &rhs) {
        return lhs.m_t <=> rhs.m_t;
    }

private:
    template <typename Up, typename Vp = std::remove_reference_t<Up>>
    static constexpr bool usable_key = std::is_same_v<const Vp, const Key> || (std::is_scalar_v<Vp> && std::is_scalar_v<Key>);

    // The actual tree structure.
    rep_type m_t;
};

template <detail::has_input_iter_cat InputIterator,
          detail::not_allocator_like Compare = std::less<detail::iter_key_t<InputIterator>>,
          detail::allocator_like Allocator = std::allocator<detail::iter_to_alloc_t<InputIterator>>>
map(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator()) -> map<detail::iter_key_t<InputIterator>, detail::iter_val_t<InputIterator>, Compare, Allocator>;

template <typename Key, typename Tp, detail::not_allocator_like Compare = std::less<Key>, detail::allocator_like Allocator = std::allocator<std::pair<const Key, Tp>>>
map(std::initializer_list<std::pair<Key, Tp>>, Compare = Compare(), Allocator = Allocator()) -> map<Key, Tp, Compare, Allocator>;

template <detail::has_input_iter_cat InputIterator, detail::allocator_like Allocator>
map(InputIterator, InputIterator, Allocator) -> map<detail::iter_key_t<InputIterator>, detail::iter_val_t<InputIterator>, std::less<detail::iter_key_t<InputIterator>>, Allocator>;

template <typename Key, typename Tp, detail::allocator_like Allocator>
map(std::initializer_list<std::pair<Key, Tp>>, Allocator) -> map<Key, Tp, std::less<Key>, Allocator>;

template <std::ranges::input_range Rg, detail::not_allocator_like Compare = std::less<detail::range_key_type<Rg>>, detail::allocator_like Allocator = std::allocator<detail::range_to_alloc_type<Rg>>>
map(std::from_range_t, Rg &&, Compare = Compare(), Allocator = Allocator()) -> map<detail::range_key_type<Rg>, detail::range_mapped_type<Rg>, Compare, Allocator>;

template <std::ranges::input_range Rg, detail::allocator_like Allocator>
map(std::from_range_t, Rg &&, Allocator) -> map<detail::range_key_type<Rg>, detail::range_mapped_type<Rg>, std::less<detail::range_key_type<Rg>>, Allocator>;

template <typename Key, typename Tp, typename Compare, typename Alloc>
void swap(map<Key, Tp, Compare, Alloc> &lhs, map<Key, Tp, Compare, Alloc> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

// Allow pvzstl::map access to internals of compatible maps.
template <typename Key, typename Val, typename Cmp1, typename Alloc, typename Cmp2>
struct detail::rb_tree_merge_helper<map<Key, Val, Cmp1, Alloc>, Cmp2> {
private:
    friend class map<Key, Val, Cmp1, Alloc>;

    static auto &get_tree(map<Key, Val, Cmp2, Alloc> &map) noexcept {
        return map.m_t;
    }
};

} // namespace pvzstl

#endif // PVZ_STL_BITS_STL_MAP_H
