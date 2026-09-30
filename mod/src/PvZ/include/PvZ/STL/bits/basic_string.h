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

#ifndef PVZ_STL_BITS_BASIC_STRING_H
#define PVZ_STL_BITS_BASIC_STRING_H

/**
 * @file bits/basic_string.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-4.9.4/libstdc++/api/a00745.html">basic_string.h File Reference</a>
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00689.html">cow_string.h File Reference</a>
 */

#ifdef PVZ_VERSION
#include "PvZ/MagicNumbers.h"
#endif

#include "PvZ/STL/debug/debug.h"

#include "PvZ/STL/bits/alloc_traits.h"
#include "PvZ/STL/bits/iterator_concepts.h"
#include "PvZ/STL/bits/ranges_base.h"
#include "PvZ/STL/bits/stl_iterator_base_types.h"

#include "PvZ/STL/ext/string_conversions.h"
#include "PvZ/STL/ext/type_traits.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

#include <atomic>
#include <string>

#ifdef PVZ_VERSION
extern uintptr_t gLibGameMainBaseAddr;
#endif

namespace pvzstl {

/**
 * @class basic_string basic_string.h <string>
 * @brief Managing sequences of characters and character-like objects.
 */
template <typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = std::allocator<CharT>>
class basic_string {
    using chart_alloc_traits = std::allocator_traits<Alloc>;

    // A helper type for avoiding boiler-plate.
    using sv_type = std::basic_string_view<CharT, Traits>;

    template <typename Tp>
    static constexpr bool if_sv = std::is_convertible_v<const Tp &, sv_type> && !std::is_convertible_v<const Tp *, const basic_string *> && !std::is_convertible_v<const Tp &, const CharT *>;

public:
    using traits_type = Traits;
    using value_type = CharT;
    using allocator_type = Alloc;
    using size_type = chart_alloc_traits::size_type;
    using difference_type = chart_alloc_traits::difference_type;
    using reference = CharT &;
    using const_reference = const CharT &;
    using pointer = chart_alloc_traits::pointer;
    using const_pointer = chart_alloc_traits::const_pointer;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<pointer>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

#ifdef PVZ_VERSION
    static_assert(std::is_same_v<CharT, char> || std::is_same_v<CharT, wchar_t> || std::is_same_v<CharT, char32_t>);
    static_assert(sizeof(size_type) == 4);
#endif

    static_assert(!std::is_array_v<value_type>, "Character type of basic_string must not be an array");
    static_assert(std::is_standard_layout_v<value_type>, "Character type of basic_string must be standard-layout");
    static_assert(std::is_trivially_default_constructible_v<value_type>, "Character type of basic_string must be trivially default constructible");
    static_assert(std::is_trivially_copyable_v<value_type>, "Character type of basic_string must be trivially copyable");
    static_assert(std::is_same_v<CharT, typename traits_type::char_type>, "traits_type::char_type must be the same type as CharT");
    static_assert(std::is_same_v<typename allocator_type::value_type, value_type>, "Allocator::value_type must be same type as value_type");

    // NB: This is an unsigned type, and thus represents the maximum
    // size that the allocator can hold.
    static constexpr size_type npos = static_cast<size_type>(-1);

    basic_string() noexcept(std::is_nothrow_default_constructible_v<Alloc>)
        requires std::is_default_constructible_v<Alloc>
        : m_dataplus(empty_rep().refdata(), Alloc()) {}

    basic_string(const basic_string &other)
        : m_dataplus(other.get_rep()->grab(Alloc(other.get_allocator()), other.get_allocator()), other.get_allocator()) {}

    basic_string(basic_string &&other) noexcept
        : m_dataplus(std::move(other.m_dataplus)) {
        // Make other use the shared empty string rep.
        other._data(empty_rep().refdata());
    }

    explicit basic_string(const Alloc &a)
        : m_dataplus(construct(size_type(), CharT(), a), a) {}

    basic_string(const basic_string &other, const Alloc &a)
        : m_dataplus(other.get_rep()->grab(a, other.get_allocator()), a) {}

    basic_string(basic_string &&other, const Alloc &a)
        : m_dataplus(other._data(), a) {
        if (a == other.get_allocator()) {
            other._data(empty_rep().refdata());
        } else {
            m_dataplus.m_p = construct(other.ibegin(), other.iend(), a);
        }
    }

    basic_string(const basic_string &str, size_type pos, size_type n, const Alloc &a = Alloc())
        : m_dataplus(construct(str._data() + str.check(pos, "basic_string::basic_string"), str._data() + str.limit(pos, n) + pos, a), a) {}

    basic_string(const basic_string &str, size_type pos, const Alloc &a = Alloc())
        : m_dataplus(construct(str._data() + str.check(pos, "basic_string::basic_string"), str._data() + str.limit(pos, npos) + pos, a), a) {}

    template <typename Tp>
        requires std::is_convertible_v<const Tp &, sv_type>
    basic_string(const Tp &t, size_type pos, size_type n, const Alloc &a = Alloc())
        : basic_string(to_string_view(t).substr(pos, n), a) {}

    template <typename Tp>
        requires if_sv<Tp>
    explicit basic_string(const Tp &t, const Alloc &a = Alloc())
        : basic_string(sv_wrapper(to_string_view(t)), a) {}

    basic_string(const CharT *s, size_type n, const Alloc &a = Alloc())
        : m_dataplus(construct(s, s + n, a), a) {}

    basic_string(const CharT *s, const Alloc &a = Alloc())
        requires detail::allocator_like<Alloc>
        : m_dataplus(construct(s, s + (s != nullptr ? traits_type::length(s) : npos), a), a) {}

    basic_string(std::nullptr_t) = delete;

    basic_string(size_type n, CharT c, const Alloc &a = Alloc())
        requires detail::allocator_like<Alloc>
        : m_dataplus(construct(n, c, a), a) {}

    template <detail::has_input_iter_cat InputIterator>
    basic_string(InputIterator first, InputIterator last, const Alloc &a = Alloc())
        : m_dataplus(construct(first, last, a), a) {}

    template <detail::container_compatible_range<CharT> Rg>
    basic_string(std::from_range_t, Rg &&rg, const Alloc &a = Alloc())
        : basic_string(a) {
        if constexpr (std::ranges::forward_range<Rg> || std::ranges::sized_range<Rg>) {
            const auto n = static_cast<size_type>(std::ranges::distance(rg));
            if (n == 0) {
                return;
            }

            reserve(n);
            pointer p = _data();
            if constexpr (requires {
                              requires std::ranges::contiguous_range<Rg>;
                              { std::ranges::data(std::forward<Rg>(rg)) } -> std::convertible_to<const CharT *>;
                          }) {
                _copy(p, std::ranges::data(std::forward<Rg>(rg)), n);
            } else {
                auto first = std::ranges::begin(rg);
                const auto last = std::ranges::end(rg);
                for (; first != last; ++first) {
                    traits_type::assign(*p++, static_cast<CharT>(*first));
                }
            }
            get_rep()->set_length(n);
        } else {
            auto first = std::ranges::begin(rg);
            const auto last = std::ranges::end(rg);
            for (; first != last; ++first) {
                push_back(*first);
            }
        }
    }

    basic_string(std::initializer_list<CharT> il, const Alloc &a = Alloc())
        : m_dataplus(construct(il.begin(), il.end(), a), a) {}

    ~basic_string() {
        get_rep()->dispose(get_allocator());
    }

    basic_string &operator=(const basic_string &other) {
        return assign(other);
    }

    basic_string &operator=(basic_string &&other) noexcept(std::allocator_traits<Alloc>::is_always_equal::value) {
        swap(other);
        return *this;
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &operator=(const Tp &t) {
        return assign(t);
    }

    basic_string &operator=(const CharT *s) {
        return assign(s);
    }

    basic_string &operator=(std::nullptr_t) = delete;

    basic_string &operator=(CharT c) {
        return assign(1, c);
    }

    basic_string &operator=(std::initializer_list<CharT> il) {
        return assign(il);
    }

    operator sv_type() const noexcept {
        return sv_type(data(), size());
    }

    basic_string &assign(const basic_string &str, size_type pos, size_type n = npos) {
        return assign(str._data() + str.check(pos, "basic_string::assign"), str.limit(pos, n));
    }

    basic_string &assign(const basic_string &str) {
        if (get_rep() != str.get_rep()) {
            // XXX MT
            const allocator_type a = get_allocator();
            CharT *tmp = str.get_rep()->grab(a, str.get_allocator());
            get_rep()->dispose(a);
            _data(tmp);
        }
        return *this;
    }

    basic_string &assign(basic_string &&str) noexcept(std::allocator_traits<Alloc>::is_always_equal::value) {
        swap(str);
        return *this;
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &assign(const Tp &t, size_type pos, size_type n = npos) {
        sv_type sv = t;
        return assign(sv.substr(pos, n));
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &assign(const Tp &t) {
        sv_type sv = t;
        return assign(sv.data(), sv.size());
    }

    basic_string &assign(const CharT *s, size_type n) {
        PVZSTL_CXX_REQUIRES_STRING_LEN(s, n);
        check_length(size(), n, "basic_string::assign");
        if (disjunct(s) || get_rep()->is_shared()) {
            return replace_safe(0, size(), s, n);
        }
        // Work in-place.
        const size_type pos = s - _data();
        if (pos >= n) {
            _copy(_data(), s, n);
        } else if (pos > 0) {
            _move(_data(), s, n);
        }
        get_rep()->set_length_and_sharable(n);
        return *this;
    }

    basic_string &assign(const CharT *s) {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return assign(s, traits_type::length(s));
    }

    basic_string &assign(size_type n, CharT c) {
        return replace_aux(size_type(0), size(), n, c);
    }

    template <detail::has_input_iter_cat InputIterator>
    basic_string &assign(InputIterator first, InputIterator last) {
        return replace(ibegin(), iend(), first, last);
    }

    basic_string &assign(std::initializer_list<CharT> il) {
        return assign(il.begin(), il.size());
    }

    template <detail::container_compatible_range<CharT> Rg>
    basic_string &assign_range(Rg &&rg) {
        basic_string str(std::from_range, std::forward<Rg>(rg), get_allocator());
        assign(std::move(str));
        return *this;
    }

    allocator_type get_allocator() const noexcept {
        return m_dataplus;
    }

    reference at(size_type pos) {
        if (pos >= size()) {
            detail::throw_out_of_range_fmt("basic_string::at: pos (which is %zu) >= this->size() (which is %zu)", pos, size());
        }
        leak();
        return _data()[pos];
    }

    const_reference at(size_type pos) const {
        if (pos >= size()) {
            detail::throw_out_of_range_fmt("basic_string::at: pos (which is %zu) >= this->size() (which is %zu)", pos, size());
        }
        return _data()[pos];
    }

    reference operator[](size_type pos) {
        assert(pos <= size());
        leak();
        return _data()[pos];
    }

    const_reference operator[](size_type pos) const noexcept {
        assert(pos <= size());
        return _data()[pos];
    }

    reference front() {
        assert(!empty());
        return operator[](0);
    }

    const_reference front() const noexcept {
        assert(!empty());
        return operator[](0);
    }

    reference back() {
        assert(!empty());
        return operator[](size() - 1);
    }

    const_reference back() const noexcept {
        assert(!empty());
        return operator[](size() - 1);
    }

    CharT *data() noexcept(false) {
        leak();
        return _data();
    }

    const CharT *data() const noexcept {
        return _data();
    }

    const CharT *c_str() const noexcept {
        return _data();
    }

    iterator begin() // FIXME C++11: should be noexcept.
    {
        leak();
        return iterator(_data());
    }

    const_iterator begin() const noexcept {
        return const_iterator(_data());
    }

    iterator end() // FIXME C++11: should be noexcept.
    {
        leak();
        return iterator(_data() + size());
    }

    const_iterator end() const noexcept {
        return const_iterator(_data() + size());
    }

    reverse_iterator rbegin() // FIXME C++11: should be noexcept.
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() // FIXME C++11: should be noexcept.
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    const_iterator cend() const noexcept {
        return end();
    }

    const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    const_reverse_iterator crend() const noexcept {
        return rend();
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    size_type size() const noexcept {
        if (empty_rep().m_length != 0) {
            std::unreachable();
        }
        return get_rep()->m_length;
    }

    size_type length() const noexcept {
        return size();
    }

    constexpr size_type max_size() const noexcept {
        return rep::max_size();
    }

    void reserve(size_type res) {
        const size_type cap = capacity();

        if (res <= cap) {
            if (!get_rep()->is_shared()) {
                return;
            }

            // unshare, but keep same capacity
            res = cap;
        }

        const allocator_type a = get_allocator();
        CharT *tmp = get_rep()->clone(a, res - size());
        get_rep()->dispose(a);
        _data(tmp);
    }

    size_type capacity() const noexcept {
        return get_rep()->m_capacity;
    }

    void shrink_to_fit() noexcept {
#if __cpp_exceptions
        if (length() >= capacity() && !get_rep()->is_shared()) {
            return;
        }
        try {
            const allocator_type a = get_allocator();
            CharT *tmp = get_rep()->clone(a);
            get_rep()->dispose(a);
            _data(tmp);
        }
        // catch (const __cxxabiv1::__forced_unwind &) {
        //     throw;
        // }
        catch (...) {
            /* swallow the exception */
        }
#endif
    }

    void clear() noexcept {
        if (get_rep()->is_shared()) {
            get_rep()->dispose(get_allocator());
            _data(empty_rep().refdata());
        } else {
            get_rep()->set_length_and_sharable(0);
        }
    }

    basic_string &insert(size_type pos1, const basic_string &str, size_type pos2, size_type n = npos) {
        return insert(pos1, str._data() + str.check(pos2, "basic_string::insert"), str.limit(pos2, n));
    }

    basic_string &insert(size_type pos, const basic_string &str) {
        return insert(pos, str._data(), str.size());
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &insert(size_type pos1, const Tp &t, size_type pos2, size_type n = npos) {
        sv_type sv = t;
        return insert(pos1, sv.substr(pos2, n));
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &insert(size_type pos, const Tp &t) {
        sv_type sv = t;
        return insert(pos, sv.data(), sv.size());
    }

    basic_string &insert(size_type pos, const CharT *s, size_type n) {
        PVZSTL_CXX_REQUIRES_STRING_LEN(s, n);
        check(pos, "basic_string::insert");
        check_length(size_type(0), n, "basic_string::insert");
        if (disjunct(s) || get_rep()->is_shared()) {
            return replace_safe(pos, size_type(0), s, n);
        }
        // Work in-place.
        const size_type off = s - _data();
        mutate(pos, 0, n);
        s = _data() + off;
        CharT *p = _data() + pos;
        if (s + n <= p) {
            _copy(p, s, n);
        } else if (s >= p) {
            _copy(p, s + n, n);
        } else {
            const size_type nleft = p - s;
            _copy(p, s, nleft);
            _copy(p + nleft, p + n, n - nleft);
        }
        return *this;
    }

    basic_string &insert(size_type pos, const CharT *s) {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return insert(pos, s, traits_type::length(s));
    }

    basic_string &insert(size_type pos, size_type n, CharT c) {
        return replace_aux(check(pos, "basic_string::insert"), size_type(0), n, c);
    }

    iterator insert(iterator position, CharT c) {
        assert(position >= ibegin() && position <= iend());
        const size_type pos = position - ibegin();
        replace_aux(pos, size_type(0), size_type(1), c);
        get_rep()->set_leaked();
        return iterator(_data() + pos);
    }

    void insert(iterator position, size_type n, CharT c) {
        replace(position, position, n, c);
    }

    template <detail::has_input_iter_cat InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last) {
        replace(position, position, first, last);
    }

    void insert(iterator position, std::initializer_list<CharT> il) {
        assert(position >= ibegin() && position <= iend());
        insert(position - ibegin(), il.begin(), il.size());
    }

    template <detail::container_compatible_range<CharT> Rg>
    iterator insert_range(const_iterator position, Rg &&rg) {
        auto pos = position - cbegin();

        if constexpr (std::ranges::forward_range<Rg>) {
            if (std::ranges::empty(rg)) {
                return begin() + pos;
            }
        }

        if (position == cend()) {
            append_range(std::forward<Rg>(rg));
        } else {
            basic_string str(std::from_range, std::forward<Rg>(rg), get_allocator());
            insert(pos, str);
        }
        return begin() + pos;
    }

    basic_string &erase(size_type pos = 0, size_type n = npos) {
        mutate(check(pos, "basic_string::erase"), limit(pos, n), size_type(0));
        return *this;
    }

    iterator erase(iterator position) {
        assert(position >= ibegin() && position < iend());
        const size_type pos = position - ibegin();
        mutate(pos, size_type(1), size_type(0));
        get_rep()->set_leaked();
        return iterator(_data() + pos);
    }

    iterator erase(iterator first, iterator last) {
        assert(first >= ibegin() && first <= last && last <= iend());

        // NB: This isn't just an optimization (bail out early when
        // there is nothing to do, really), it's also a correctness
        // issue vs MT, see libstdc++/40518.
        const size_type sz = last - first;
        if (sz > 0) {
            const size_type pos = first - ibegin();
            mutate(pos, sz, size_type(0));
            get_rep()->set_leaked();
            return iterator(_data() + pos);
        }
        return first;
    }

    void push_back(CharT c) {
        const size_type sz = size();
        const size_type len = sz + 1;
        reserve(len);
        _data()[sz] = c;
        get_rep()->set_length_and_sharable(len);
    }

    void pop_back() // FIXME C++11: should be noexcept.
    {
        assert(!empty());
        erase(size() - 1, 1);
    }

    basic_string &append(const basic_string &str, size_type pos, size_type n = npos) {
        str.check(pos, "basic_string::append");
        n = str.limit(pos, n);
        if (n > 0) {
            const size_type len = n + size();
            reserve(len);
            _copy(_data() + size(), str._data() + pos, n);
            get_rep()->set_length_and_sharable(len);
        }
        return *this;
    }

    basic_string &append(const basic_string &str) {
        const size_type sz = str.size();
        if (sz > 0) {
            const size_type len = sz + size();
            reserve(len);
            _copy(_data() + size(), str._data(), sz);
            get_rep()->set_length_and_sharable(len);
        }
        return *this;
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &append(const Tp &t, size_type pos, size_type n = npos) {
        sv_type sv = t;
        return append(sv.substr(pos, n));
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &append(const Tp &t) {
        sv_type sv = t;
        return append(sv.data(), sv.size());
    }

    basic_string &append(const CharT *s, size_type n) {
        PVZSTL_CXX_REQUIRES_STRING_LEN(s, n);
        if (n == 0) {
            return *this;
        }
        check_length(size_type(0), n, "basic_string::append");
        const size_type len = n + size();
        if (disjunct(s)) {
            reserve(len);
        } else {
            const size_type off = s - _data();
            reserve(len);
            s = _data() + off;
        }
        _copy(_data() + size(), s, n);
        get_rep()->set_length_and_sharable(len);
        return *this;
    }

    basic_string &append(const CharT *s) {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return append(s, traits_type::length(s));
    }

    basic_string &append(size_type n, CharT c) {
        if (n > 0) {
            check_length(size_type(0), n, "basic_string::append");
            const size_type len = n + size();
            reserve(len);
            _assign(_data() + size(), n, c);
            get_rep()->set_length_and_sharable(len);
        }
        return *this;
    }

    template <detail::has_input_iter_cat InputIterator>
    basic_string &append(InputIterator first, InputIterator last) {
        return replace(iend(), iend(), first, last);
    }

    basic_string &append(std::initializer_list<CharT> il) {
        return append(il.begin(), il.size());
    }

    template <detail::container_compatible_range<CharT> Rg>
    basic_string &append_range(Rg &&rg) {
        basic_string str(std::from_range, std::forward<Rg>(rg), get_allocator());
        append(str);
        return *this;
    }

    basic_string &operator+=(const basic_string &str) {
        return append(str);
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &operator+=(const Tp &t) {
        return append(t);
    }

    basic_string &operator+=(const CharT *s) {
        return append(s);
    }

    basic_string &operator+=(CharT c) {
        push_back(c);
        return *this;
    }

    basic_string &operator+=(std::initializer_list<CharT> il) {
        return append(il);
    }

    basic_string &replace(size_type pos1, size_type n1, const basic_string &str, size_type pos2, size_type n2 = npos) {
        return replace(pos1, n1, str._data() + str.check(pos2, "basic_string::replace"), str.limit(pos2, n2));
    }

    basic_string &replace(size_type pos, size_type n, const basic_string &str) {
        return replace(pos, n, str._data(), str.size());
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &replace(size_type pos1, size_type n1, const Tp &t, size_type pos2, size_type n2 = npos) {
        sv_type sv = t;
        return replace(pos1, n1, sv.substr(pos2, n2));
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &replace(size_type pos, size_type n, const Tp &t) {
        sv_type sv = t;
        return replace(pos, n, sv.data(), sv.size());
    }

    basic_string &replace(size_type pos, size_type n1, const CharT *s, size_type n2) {
        PVZSTL_CXX_REQUIRES_STRING_LEN(s, n2);
        check(pos, "basic_string::replace");
        n1 = limit(pos, n1);
        check_length(n1, n2, "basic_string::replace");
        bool left;
        if (disjunct(s) || get_rep()->is_shared()) {
            return replace_safe(pos, n1, s, n2);
        }
        if ((left = (s + n2 <= _data() + pos)) || (_data() + pos + n1 <= s)) {
            // Work in-place: non-overlapping case.
            size_type off = s - _data();
            if (!left) {
                off += n2 - n1;
            }
            mutate(pos, n1, n2);
            _copy(_data() + pos, _data() + off, n2);
            return *this;
        }
        // TODO: overlapping case.
        const basic_string tmp(s, n2);
        return replace_safe(pos, n1, tmp._data(), n2);
    }

    basic_string &replace(size_type pos, size_type n, const CharT *s) {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return replace(pos, n, s, traits_type::length(s));
    }

    basic_string &replace(size_type pos, size_type n1, size_type n2, CharT c) {
        return replace_aux(check(pos, "basic_string::replace"), limit(pos, n1), n2, c);
    }

    basic_string &replace(iterator first, iterator last, const basic_string &str) {
        return replace(first, last, str._data(), str.size());
    }

    template <typename Tp>
        requires if_sv<Tp>
    basic_string &replace(const_iterator first, const_iterator last, const Tp &t) {
        sv_type sv = t;
        return replace(first - ibegin(), last - first, sv);
    }

    basic_string &replace(iterator first, iterator last, const CharT *s, size_type n) {
        assert(ibegin() <= first && first <= last && last <= iend());
        return replace(first - ibegin(), last - first, s, n);
    }

    basic_string &replace(iterator first, iterator last, const CharT *s) {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return replace(first, last, s, traits_type::length(s));
    }

    basic_string &replace(iterator first, iterator last, size_type n, CharT c) {
        assert(ibegin() <= first && first <= last && last <= iend());
        return replace(first - ibegin(), last - first, n, c);
    }

    template <detail::has_input_iter_cat InputIterator>
    basic_string &replace(iterator first1, iterator last1, InputIterator first2, InputIterator last2) {
        assert(ibegin() <= first1 && first1 <= last1 && last1 <= iend());
        return replace_dispatch(first1, last1, first2, last2);
    }

    // Specializations for the common case of pointer and iterator:
    // useful to avoid the overhead of temporary buffering in replace.
    basic_string &replace(iterator first1, iterator last1, CharT *first2, CharT *last2) {
        assert(ibegin() <= first1 && first1 <= last1 && last1 <= iend());
        PVZSTL_CXX_REQUIRES_VALID_RANGE(first2, last2);
        return replace(first1 - ibegin(), last1 - first1, first2, last2 - first2);
    }

    basic_string &replace(iterator first1, iterator last1, const CharT *first2, const CharT *last2) {
        assert(ibegin() <= first1 && first1 <= last1 && last1 <= iend());
        PVZSTL_CXX_REQUIRES_VALID_RANGE(first2, last2);
        return replace(first1 - ibegin(), last1 - first1, first2, last2 - first2);
    }

    basic_string &replace(iterator first, iterator last, std::initializer_list<CharT> il) {
        return replace(first, last, il.begin(), il.end());
    }

    template <detail::container_compatible_range<CharT> Rg>
    basic_string &replace_with_range(const_iterator first, const_iterator last, Rg &&rg) {
        if (first == cend()) {
            append_range(std::forward<Rg>(rg));
        } else {
            basic_string str(std::from_range, std::forward<Rg>(rg), get_allocator());
            replace(first - cbegin(), last - first, str);
        }
        return *this;
    }

    size_type copy(CharT *dest, size_type n, size_type pos = 0) const {
        check(pos, "basic_string::copy");
        n = limit(pos, n);
        PVZSTL_CXX_REQUIRES_STRING_LEN(dest, n);
        if (n > 0) {
            _copy(dest, _data() + pos, n);
        }
        return n;
    }

    void resize(size_type n, CharT c) {
        const size_type sz = size();
        check_length(sz, n, "basic_string::resize");
        if (sz < n) {
            append(n - sz, c);
        } else if (n < sz) {
            erase(n);
        }
        // else nothing (in particular, avoid calling mutate() unnecessarily.)
    }

    void resize(size_type n) {
        resize(n, CharT());
    }

    template <typename Operation>
    void resize_and_overwrite(size_type n, Operation op) {
        reserve(n);
        CharT *p = _data();
        struct Terminator {
            ~Terminator() {
                _this->get_rep()->set_length_and_sharable(_r);
            }
            basic_string *_this;
            size_type _r;
        };
        Terminator term{this, 0};
        auto r = std::move(op)(p, n);
        static_assert(ranges::detail::is_integer_like<decltype(r)>);
        assert(r >= 0 && size_type(r) <= n);
        term._r = size_type(r);
        if (term._r > n) {
            std::unreachable();
        }
    }

    void swap(basic_string &other) noexcept(std::allocator_traits<Alloc>::is_always_equal::value) {
        if (get_rep()->is_leaked()) {
            get_rep()->set_sharable();
        }
        if (other.get_rep()->is_leaked()) {
            other.get_rep()->set_sharable();
        }
        if (get_allocator() == other.get_allocator()) {
            CharT *tmp = _data();
            _data(other._data());
            other._data(tmp);
        }
        // The code below can usually be optimized away.
        else {
            const basic_string tmp1(ibegin(), iend(), other.get_allocator());
            const basic_string tmp2(other.ibegin(), other.iend(), get_allocator());
            *this = tmp2;
            other = tmp1;
        }
    }

    size_type find(const basic_string &str, size_type pos = 0) const noexcept {
        return sv_type(*this).find(sv_type(str), pos);
    }

    template <typename Tp>
        requires if_sv<Tp>
    size_type find(const Tp &t, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const Tp &, sv_type>) {
        sv_type sv = t;
        return sv_type(*this).find(sv, pos);
    }

    size_type find(const CharT *s, size_type pos, size_type n) const {
        return sv_type(*this).find(s, pos, n);
    }

    size_type find(const CharT *s, size_type pos = 0) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).find(s, pos);
    }

    size_type find(CharT c, size_type pos = 0) const noexcept {
        return sv_type(*this).find(c, pos);
    }

    size_type rfind(const basic_string &str, size_type pos = npos) const noexcept {
        return sv_type(*this).rfind(sv_type(str), pos);
    }

    template <typename Tp>
        requires if_sv<Tp>
    size_type rfind(const Tp &t, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const Tp &, sv_type>) {
        sv_type sv = t;
        return sv_type(*this).rfind(sv, pos);
    }

    size_type rfind(const CharT *s, size_type pos, size_type n) const {
        return sv_type(*this).rfind(s, pos, n);
    }

    size_type rfind(const CharT *s, size_type pos = npos) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).rfind(s, pos);
    }

    size_type rfind(CharT c, size_type pos = npos) const noexcept {
        return sv_type(*this).rfind(c, pos);
    }

    size_type find_first_of(const basic_string &str, size_type pos = 0) const noexcept {
        return sv_type(*this).find_first_of(sv_type(str), pos);
    }

    template <typename Tp>
        requires if_sv<Tp>
    size_type find_first_of(const Tp &t, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const Tp &, sv_type>) {
        sv_type sv = t;
        return sv_type(*this).find_first_of(sv, pos);
    }

    size_type find_first_of(const CharT *s, size_type pos, size_type n) const {
        return sv_type(*this).find_first_of(s, pos, n);
    }

    size_type find_first_of(const CharT *s, size_type pos = 0) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).find_first_of(s, pos);
    }

    size_type find_first_of(CharT c, size_type pos = 0) const noexcept {
        return find(c, pos);
    }

    size_type find_first_not_of(const basic_string &str, size_type pos = 0) const noexcept {
        return sv_type(*this).find_first_not_of(sv_type(str), pos);
    }

    template <typename Tp>
        requires if_sv<Tp>
    size_type find_first_not_of(const Tp &t, size_type pos = 0) const noexcept(std::is_nothrow_convertible_v<const Tp &, sv_type>) {
        sv_type sv = t;
        return sv_type(*this).find_first_not_of(sv, pos);
    }

    size_type find_first_not_of(const CharT *s, size_type pos, size_type n) const {
        return sv_type(*this).find_first_not_of(s, pos, n);
    }

    size_type find_first_not_of(const CharT *s, size_type pos = 0) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).find_first_not_of(s, pos);
    }

    size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept {
        return sv_type(*this).find_first_not_of(c, pos);
    }

    size_type find_last_of(const basic_string &str, size_type pos = npos) const noexcept {
        return sv_type(*this).find_last_of(sv_type(str), pos);
    }

    template <typename Tp>
        requires if_sv<Tp>
    size_type find_last_of(const Tp &t, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const Tp &, sv_type>) {
        sv_type sv = t;
        return sv_type(*this).find_last_of(sv, pos);
    }

    size_type find_last_of(const CharT *s, size_type pos, size_type n) const {
        return sv_type(*this).find_last_of(s, pos, n);
    }

    size_type find_last_of(const CharT *s, size_type pos = npos) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).find_last_of(s, pos);
    }

    size_type find_last_of(CharT c, size_type pos = npos) const noexcept {
        return rfind(c, pos);
    }

    size_type find_last_not_of(const basic_string &str, size_type pos = npos) const noexcept {
        return sv_type(*this).find_last_not_of(sv_type(str), pos);
    }

    template <typename Tp>
        requires if_sv<Tp>
    size_type find_last_not_of(const Tp &t, size_type pos = npos) const noexcept(std::is_nothrow_convertible_v<const Tp &, sv_type>) {
        sv_type sv = t;
        return sv_type(*this).find_last_not_of(sv, pos);
    }

    size_type find_last_not_of(const CharT *s, size_type pos, size_type n) const {
        return sv_type(*this).find_last_not_of(s, pos, n);
    }

    size_type find_last_not_of(const CharT *s, size_type pos = npos) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).find_last_not_of(s, pos);
    }

    size_type find_last_not_of(CharT c, size_type pos = npos) const noexcept {
        return sv_type(*this).find_last_not_of(c, pos);
    }

    int compare(size_type pos1, size_type n1, const basic_string &str, size_type pos2, size_type n2 = npos) const {
        return sv_type(*this).compare(pos1, n1, sv_type(str), pos2, n2);
    }

    int compare(size_type pos, size_type n, const basic_string &str) const {
        return sv_type(*this).compare(pos, n, sv_type(str));
    }

    int compare(const basic_string &str) const noexcept {
        return sv_type(*this).compare(sv_type(str));
    }

    template <typename Tp>
        requires if_sv<Tp>
    int compare(size_type pos1, size_type n1, const Tp &t, size_type pos2, size_type n2 = npos) const {
        sv_type sv = t;
        return sv_type(*this).compare(pos1, n1, sv, pos2, n2);
    }

    template <typename Tp>
        requires if_sv<Tp>
    int compare(size_type pos, size_type n, const Tp &t) const {
        sv_type sv = t;
        return sv_type(*this).compare(pos, n, sv);
    }

    template <typename Tp>
        requires if_sv<Tp>
    int compare(const Tp &t) const noexcept(std::is_nothrow_convertible_v<const Tp &, sv_type>) {
        sv_type sv = t;
        return sv_type(*this).compare(sv);
    }

    int compare(size_type pos1, size_type n1, const CharT *s, size_type n2) const {
        PVZSTL_CXX_REQUIRES_STRING_LEN(s, n2);
        return sv_type(*this).compare(pos1, n1, s, n2);
    }

    int compare(size_type pos, size_type n, const CharT *s) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).compare(pos, n, s);
    }

    int compare(const CharT *s) const {
        PVZSTL_CXX_REQUIRES_STRING(s);
        return sv_type(*this).compare(s);
    }

    bool starts_with(sv_type sv) const noexcept {
        return sv_type(*this).starts_with(sv);
    }

    [[gnu::nonnull]] bool starts_with(const CharT *s) const {
        return sv_type(*this).starts_with(s);
    }

    bool starts_with(CharT c) const noexcept {
        return sv_type(*this).starts_with(c);
    }

    bool ends_with(sv_type sv) const noexcept {
        return sv_type(*this).ends_with(sv);
    }

    [[gnu::nonnull]] bool ends_with(const CharT *s) const {
        return sv_type(*this).ends_with(s);
    }

    bool ends_with(CharT c) const noexcept {
        return sv_type(*this).ends_with(c);
    }

    bool contains(sv_type sv) const noexcept {
        return sv_type(*this).contains(sv);
    }

    [[gnu::nonnull]] bool contains(const CharT *s) const {
        return sv_type(*this).contains(s);
    }

    bool contains(CharT c) const noexcept {
        return sv_type(*this).contains(c);
    }

    basic_string substr(size_type pos = 0, size_type n = npos) const {
        return basic_string(*this, check(pos, "basic_string::substr"), n);
    }

private:
    static_assert(sizeof(int) == sizeof(std::atomic_int) && alignof(int) >= alignof(std::atomic_int));

    struct rep_base {
        size_type m_length;
        size_type m_capacity;
        int m_refcount;
    };

    struct rep : rep_base {
        using raw_bytes_alloc = std::allocator_traits<Alloc>::template rebind_alloc<unsigned char>;

        // 为了 constexpr, 将 `max_size` 实现为静态成员函数.
        [[nodiscard]] static consteval size_type max_size() noexcept {
            // npos = (m + 1) * sizeof(CharT) + sizeof(rep)
            constexpr size_type m = ((npos - sizeof(rep_base)) / sizeof(CharT)) - 1;
            return m / 4;
        }

        static rep &empty_rep() noexcept {
#ifdef PVZ_VERSION
            constexpr uintptr_t offset = std::is_same_v<CharT, char> ? /* string */ PVZSTL_STRING_EMPTY_REP : /* basic_string<int> */ PVZSTL_WSTRING_EMPTY_REP;
            return *reinterpret_cast<rep *>(::gLibGameMainBaseAddr + offset);
#else
            alignas(rep) static unsigned char empty_rep_storage[sizeof(rep_base) + sizeof(CharT)] = {};
            return reinterpret_cast<rep &>(empty_rep_storage);
#endif
        }

        static rep *create(size_type cap, size_type old_cap, const Alloc &alloc) {
            if (cap > max_size()) {
                detail::throw_length_error("basic_string::create");
            }

            // pagesize need not match the actual VM page size for good
            // results in practice, thus we pick a common value on the low
            // side.  malloc_header_size is an estimate of the amount of
            // overhead per memory allocation (in practice seen N * sizeof
            // (void*) where N is 0, 2 or 4).  According to folklore,
            // picking this value on the high side is better than
            // low-balling it (especially when this algorithm is used with
            // malloc implementations that allocate memory blocks rounded up
            constexpr size_type pagesize = 4096;
            constexpr size_type malloc_header_size = 4 * sizeof(void *);

            // The below implements an exponential growth policy, necessary to
            // meet amortized linear time requirements of the library: see
            // http://gcc.gnu.org/ml/libstdc++/2001-07/msg00085.html.
            // It's active for allocations requiring an amount of memory above
            // system pagesize. This is consistent with the requirements of the
            // standard: http://gcc.gnu.org/ml/libstdc++/2001-07/msg00130.html
            if ((cap > old_cap) && (cap < 2 * old_cap)) {
                cap = 2 * old_cap;
            }

            // NB: Need an array of char_type[capacity], plus a terminating
            // null char_type() element, plus enough for the _Rep data structure.
            // Whew. Seemingly so needy, yet so elemental.
            size_type size = (cap + 1) * sizeof(CharT) + sizeof(rep);

            const size_type adj_size = size + malloc_header_size;
            if (adj_size > pagesize && cap > old_cap) {
                const size_type extra = pagesize - adj_size % pagesize;
                cap += extra / sizeof(CharT);
                // Never allocate a string bigger than max_size.
                if (cap > max_size()) {
                    cap = max_size();
                }
                size = (cap + 1) * sizeof(CharT) + sizeof(rep);
            }

            // NB: Might throw, but no worries about a leak, mate: rep()
            // does not throw.
            void *place = raw_bytes_alloc(alloc).allocate(size);
            rep *p = ::new (place) rep;
            p->m_capacity = cap;
            p->set_sharable();
            return p;
        }

        CharT *refdata() noexcept {
            return reinterpret_cast<CharT *>(this + 1);
        }

        CharT *refcopy() noexcept {
            if (this != &empty_rep()) [[unlikely]] {
                reinterpret_cast<std::atomic_int &>(this->m_refcount).fetch_add(1, std::memory_order_acq_rel);
            }
            return refdata();
        } // XXX MT

        CharT *clone(const Alloc &alloc, size_type res = 0) {
            // Requested capacity of the clone.
            const size_type requested_cap = this->m_length + res;
            rep *r = create(requested_cap, this->m_capacity, alloc);
            if (this->m_length > 0) {
                _copy(r->refdata(), refdata(), this->m_length);
            }

            r->set_length(this->m_length);
            return r->refdata();
        }

        CharT *grab(const Alloc &alloc1, const Alloc &alloc2) {
            return (!is_leaked() && alloc1 == alloc2) ? refcopy() : clone(alloc1);
        }

        void destroy(const Alloc &a) noexcept {
            const size_type size = sizeof(rep_base) + (this->m_capacity + 1) * sizeof(CharT);
            raw_bytes_alloc(a).deallocate(reinterpret_cast<unsigned char *>(this), size);
        }

        void dispose(const Alloc &a) noexcept {
            if (this != &empty_rep()) [[unlikely]] {
                // Decrement of m_refcount is acq_rel, because:
                // - all but last decrements need to release to synchronize with
                //   the last decrement that will delete the object.
                // - the last decrement needs to acquire to synchronize with
                //   all the previous decrements.
                // - last but one decrement needs to release to synchronize with
                //   the acquire load in is_shared that will conclude that
                //   the object is not shared anymore.
                if (reinterpret_cast<std::atomic_int &>(this->m_refcount).fetch_sub(1, std::memory_order_acq_rel) <= 0) {
                    destroy(a);
                }
            }
        } // XXX MT

        bool is_leaked() const noexcept {
            // m_refcount is mutated concurrently by refcopy/dispose,
            // so we need to use an atomic load. However, is_leaked
            // predicate does not change concurrently (i.e. the string is either
            // leaked or not), so a relaxed load is enough.
            return reinterpret_cast<const std::atomic_int &>(this->m_refcount).load(std::memory_order_relaxed) < 0;
        }

        bool is_shared() const noexcept {
            // m_refcount is mutated concurrently by refcopy/dispose,
            // so we need to use an atomic load. Another thread can drop last
            // but one reference concurrently with this check, so we need this
            // load to be acquire to synchronize with release fetch_and_add in
            // dispose.
            return reinterpret_cast<const std::atomic_int &>(this->m_refcount).load(std::memory_order_acquire) > 0;
        }

        void set_leaked() noexcept {
            this->m_refcount = -1;
        }

        void set_sharable() noexcept {
            this->m_refcount = 0;
        }

        void set_length(size_type n) noexcept {
            this->m_length = n;
            traits_type::assign(refdata()[n], CharT());
        }

        void set_length_and_sharable(size_type n) noexcept {
            if (this != &empty_rep()) [[unlikely]] {
                set_sharable();
                set_length(n);
            }
        }
    };

    // Use empty-base optimization: http://www.cantrip.org/emptyopt.html
    struct alloc_hider : Alloc {
        alloc_hider(CharT *dat, const Alloc &a) noexcept
            : Alloc(a)
            , m_p(dat) {}

        CharT *m_p; // The actual data.
    };

    // Wraps a string_view by explicit conversion and thus
    // allows to add an internal constructor that does not
    // participate in overload resolution when a string_view
    // is provided.
    struct sv_wrapper {
        explicit sv_wrapper(sv_type sv) noexcept
            : m_sv(sv) {}

        sv_type m_sv;
    };

    // Allows an implicit conversion to sv_type.
    static sv_type to_string_view(sv_type sv) noexcept {
        return sv;
    }

    template <typename InputIterator>
    static CharT *construct(InputIterator first, InputIterator last, const Alloc &a) {
        using Tag = typename std::iterator_traits<InputIterator>::iterator_category;
        return construct(first, last, a, Tag());
    }

    // For Input Iterators, used in istreambuf_iterators, etc.
    template <typename InputIterator>
    static CharT *construct(InputIterator first, InputIterator last, const Alloc &a, std::input_iterator_tag) {
        if (first == last && a == Alloc()) {
            return empty_rep().refdata();
        }
        // Avoid reallocation for common case.
        CharT buf[128];
        size_type len = 0;
        while (first != last && len < sizeof(buf) / sizeof(CharT)) {
            buf[len++] = *first;
            ++first;
        }
        rep *r = rep::create(len, size_type(0), a);
        _copy(r->refdata(), buf, len);
        PVZSTL_TRY {
            while (first != last) {
                if (len == r->m_capacity) {
                    // Allocate more space.
                    rep *another = rep::create(len + 1, len, a);
                    _copy(another->refdata(), r->refdata(), len);
                    r->destroy(a);
                    r = another;
                }
                r->refdata()[len++] = *first;
                ++first;
            }
        }
        PVZSTL_CATCH(...) {
            r->destroy(a);
            PVZSTL_THROW_EXCEPTION_AGAIN;
        }
        r->set_length(len);
        return r->refdata();
    }

    // For forward_iterators up to random_access_iterators, used for
    // string::iterator, CharT*, etc.
    template <typename InputIterator>
    static CharT *construct(InputIterator first, InputIterator last, const Alloc &a, std::forward_iterator_tag) {
        if (first == last && a == Alloc()) {
            return empty_rep().refdata();
        }
        // NB: Not required, but considered best practice.
        if (pvzstl_cxx::is_null_pointer(first) && first != last) {
            detail::throw_logic_error("basic_string::construct null not valid");
        }

        const size_type dnew = static_cast<size_type>(std::distance(first, last));
        // Check for out_of_range and length_error exceptions.
        rep *r = rep::create(dnew, size_type(0), a);
        PVZSTL_TRY {
            copy_chars(r->refdata(), first, last);
        }
        PVZSTL_CATCH(...) {
            r->destroy(a);
            PVZSTL_THROW_EXCEPTION_AGAIN;
        }
        r->set_length(dnew);
        return r->refdata();
    }

    static CharT *construct(size_type n, CharT c, const Alloc &a) {
        if (n == 0 && a == Alloc()) {
            return empty_rep().refdata();
        }
        // Check for out_of_range and length_error exceptions.
        rep *r = rep::create(n, size_type(0), a);
        if (n > 0) {
            _assign(r->refdata(), n, c);
        }

        r->set_length(n);
        return r->refdata();
    }

    // When n = 1 way faster than the general multichar
    // traits_type::copy/move/assign.
    static void _copy(CharT *dest, const CharT *src, size_type n) noexcept {
        if (n == 1) {
            traits_type::assign(*dest, *src);
        } else if (n > 1) {
            traits_type::copy(dest, src, n);
        }
    }

    static void _move(CharT *dest, const CharT *src, size_type n) noexcept {
        if (n == 1) {
            traits_type::assign(*dest, *src);
        } else if (n > 1) {
            traits_type::move(dest, src, n);
        }
    }

    static void _assign(CharT *dest, size_type n, CharT c) noexcept {
        if (n == 1) {
            traits_type::assign(*dest, c);
        } else if (n > 1) {
            traits_type::assign(dest, n, c);
        }
    }

    // copy_chars is a separate template to permit specialization
    // to optimize for the common case of pointers as iterators.
    template <typename Iterator>
    static void copy_chars(CharT *p, Iterator first, Iterator last) {
        for (; first != last; ++first, (void)++p) {
            traits_type::assign(*p, static_cast<CharT>(*first));
        }
    }

    static void copy_chars(CharT *p, CharT *first, CharT *last) {
        _copy(p, first, last - first);
    }

    static void copy_chars(CharT *p, const CharT *first, const CharT *last) {
        _copy(p, first, last - first);
    }

    static rep &empty_rep() noexcept {
        return rep::empty_rep();
    }

    explicit basic_string(sv_wrapper svw, const Alloc &a)
        : basic_string(svw.m_sv.data(), svw.m_sv.size(), a) {}

    CharT *_data() const noexcept {
        return m_dataplus.m_p;
    }

    CharT *_data(CharT *p) noexcept {
        return (m_dataplus.m_p = p);
    }

    rep *get_rep() const noexcept {
        return reinterpret_cast<rep *>(_data()) - 1;
    }

    // For the internal use we have functions similar to `begin'/`end'
    // but they do not call leak.
    iterator ibegin() const noexcept {
        return iterator(_data());
    }

    iterator iend() const noexcept {
        return iterator(_data() + size());
    }

    size_type check(size_type pos, const char *msg) const {
        if (pos > size()) {
            detail::throw_out_of_range_fmt("%s: pos (which is %zu) > this->size() (which is %zu)", msg, pos, size());
        }
        return pos;
    }

    void check_length(size_type n1, size_type n2, const char *msg) const {
        if (max_size() - (size() - n1) < n2) {
            detail::throw_length_error(msg);
        }
    }

    // NB: limit doesn't check for a bad pos value.
    size_type limit(size_type pos, size_type off) const noexcept {
        const bool testoff = off < size() - pos;
        return testoff ? off : size() - pos;
    }

    bool disjunct(const CharT *s) const noexcept {
        return (s < _data()) || (_data() + size() < s);
    }

    // for use in begin() & non-const op[]
    void leak() {
        if (get_rep()->is_leaked()) {
            return;
        }

        // No need to create a new copy of an empty string when a non-const
        // reference/pointer/iterator into it is obtained. Modifying the
        // trailing null character is undefined, so the ref/pointer/iterator
        // is effectively const anyway.
        if (empty()) {
            return;
        }

        if (get_rep()->is_shared()) {
            mutate(0, 0, 0);
        }
        get_rep()->set_leaked();
    }

    // 清空范围 [ `begin() + pos`, `begin() + pos + len1` ) 中的字符,
    // 并在原位置预留大小为 `len2` 的空间.
    void mutate(size_type pos, size_type len1, size_type len2) {
        const size_type cap = capacity();
        const size_type old_size = size();
        const size_type new_size = old_size + len2 - len1;
        const size_type how_much = old_size - pos - len1;

        if (new_size > cap || get_rep()->is_shared()) {
            // Must reallocate.
            const allocator_type a = get_allocator();
            rep *r = rep::create(new_size, cap, a);
            if (pos > 0) {
                _copy(r->refdata(), _data(), pos);
            }
            if (how_much > 0) {
                _copy((r->refdata() + pos + len2), (_data() + pos + len1), how_much);
            }
            get_rep()->dispose(a);
            _data(r->refdata());
        } else if ((how_much > 0) && (len1 != len2)) {
            // Work in-place.
            _move((_data() + pos + len2), (_data() + pos + len1), how_much);
        }
        get_rep()->set_length_and_sharable(new_size);
    }

    template <typename InputIterator>
    basic_string &replace_dispatch(iterator first1, iterator last1, InputIterator first2, InputIterator last2) {
        PVZSTL_CXX_REQUIRES_VALID_RANGE(first2, last2);
        const basic_string str(first2, last2);
        const size_type n1 = last1 - first1;
        check_length(n1, str.size(), "basic_string::replace_dispatch");
        return replace_safe(first1 - ibegin(), n1, str._data(), str.size());
    }

    basic_string &replace_aux(size_type pos, size_type n1, size_type n2, CharT c) {
        check_length(n1, n2, "basic_string::replace_aux");
        mutate(pos, n1, n2);
        if (n2 > 0) {
            _assign(_data() + pos, n2, c);
        }
        return *this;
    }

    basic_string &replace_safe(size_type pos, size_type n1, const CharT *s, size_type n2) {
        mutate(pos, n1, n2);
        if (n2 > 0) {
            _copy(_data() + pos, s, n2);
        }
        return *this;
    }

    mutable alloc_hider m_dataplus;
};

template <detail::has_input_iter_cat InputIterator, typename CharT = std::iterator_traits<InputIterator>::value_type, detail::allocator_like Allocator = std::allocator<CharT>>
basic_string(InputIterator, InputIterator, Allocator = Allocator()) -> basic_string<CharT, std::char_traits<CharT>, Allocator>;

template <typename CharT, typename Traits, detail::allocator_like Allocator = std::allocator<CharT>>
basic_string(std::basic_string_view<CharT, Traits>, const Allocator & = Allocator()) -> basic_string<CharT, Traits, Allocator>;

template <typename CharT, typename Traits, detail::allocator_like Allocator = std::allocator<CharT>>
basic_string(std::basic_string_view<CharT, Traits>,
             typename basic_string<CharT, Traits, Allocator>::size_type,
             typename basic_string<CharT, Traits, Allocator>::size_type,
             const Allocator & = Allocator()) -> basic_string<CharT, Traits, Allocator>;

template <std::ranges::input_range Rg, typename Allocator = std::allocator<std::ranges::range_value_t<Rg>>>
basic_string(std::from_range_t, Rg &&, Allocator = Allocator()) -> basic_string<std::ranges::range_value_t<Rg>, std::char_traits<std::ranges::range_value_t<Rg>>, Allocator>;

template <typename CharT, typename Traits, typename Alloc>
bool operator==(const basic_string<CharT, Traits, Alloc> &lhs, const basic_string<CharT, Traits, Alloc> &rhs) noexcept {
    return std::basic_string_view<CharT, Traits>(lhs) == std::basic_string_view<CharT, Traits>(rhs);
}

template <typename CharT, typename Traits, typename Alloc>
bool operator==(const basic_string<CharT, Traits, Alloc> &lhs, const CharT *rhs) {
    return std::basic_string_view<CharT, Traits>(lhs) == rhs;
}

template <typename CharT, typename Traits, typename Alloc>
auto operator<=>(const basic_string<CharT, Traits, Alloc> &lhs, const basic_string<CharT, Traits, Alloc> &rhs) noexcept {
    return std::basic_string_view<CharT, Traits>(lhs) <=> std::basic_string_view<CharT, Traits>(rhs);
}

template <typename CharT, typename Traits, typename Alloc>
auto operator<=>(const basic_string<CharT, Traits, Alloc> &lhs, const CharT *rhs) {
    return std::basic_string_view<CharT, Traits>(lhs) <=> rhs;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc> &lhs, const basic_string<CharT, Traits, Alloc> &rhs) {
    basic_string<CharT, Traits, Alloc> r = lhs;
    r.append(rhs);
    return r;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(const CharT *lhs, const basic_string<CharT, Traits, Alloc> &rhs) {
    assert(lhs != nullptr);
    const auto len = basic_string<CharT, Traits, Alloc>::traits_type::length(lhs);
    basic_string<CharT, Traits, Alloc> r;
    r.reserve(len + rhs.size());
    r.append(lhs, len);
    r.append(rhs);
    return r;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(CharT lhs, const basic_string<CharT, Traits, Alloc> &rhs) {
    basic_string<CharT, Traits, Alloc> r;
    r.reserve(1 + rhs.size());
    r.push_back(lhs);
    r.append(rhs);
    return r;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc> &lhs, const CharT *rhs) {
    basic_string<CharT, Traits, Alloc> r = lhs;
    r.append(rhs);
    return r;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc> &lhs, CharT rhs) {
    basic_string<CharT, Traits, Alloc> r = lhs;
    r.push_back(rhs);
    return r;
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc> &&lhs, const basic_string<CharT, Traits, Alloc> &rhs) {
    return std::move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(const basic_string<CharT, Traits, Alloc> &lhs, basic_string<CharT, Traits, Alloc> &&rhs) {
    return std::move(rhs.insert(0, lhs));
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc> &&lhs, basic_string<CharT, Traits, Alloc> &&rhs) {
    const auto size = lhs.size() + rhs.size();
    const bool cond = size > lhs.capacity() && size <= rhs.capacity();
    return cond ? std::move(rhs.insert(0, lhs)) : std::move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(const CharT *lhs, basic_string<CharT, Traits, Alloc> &&rhs) {
    return std::move(rhs.insert(0, lhs));
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(CharT lhs, basic_string<CharT, Traits, Alloc> &&rhs) {
    return std::move(rhs.insert(0, 1, lhs));
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc> &&lhs, const CharT *rhs) {
    return std::move(lhs.append(rhs));
}

template <typename CharT, typename Traits, typename Alloc>
basic_string<CharT, Traits, Alloc> operator+(basic_string<CharT, Traits, Alloc> &&lhs, CharT rhs) {
    lhs.push_back(rhs);
    return std::move(lhs);
}

template <typename CharT, typename Traits, typename Alloc>
void swap(basic_string<CharT, Traits, Alloc> &lhs, basic_string<CharT, Traits, Alloc> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;    // `basic_string<int>` in PvZ
using u32string = basic_string<char32_t>; // `basic_string<int>` in PvZ
#ifndef PVZ_VERSION
using u8string = basic_string<char8_t>;
using u16string = basic_string<char16_t>;
#endif

// 21.4 Numeric Conversions [string.conversions].
inline int stoi(const string &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa<long, int>(&std::strtol, "stoi", str.c_str(), idx, base);
}

inline long stol(const string &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::strtol, "stol", str.c_str(), idx, base);
}

inline unsigned long stoul(const string &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::strtoul, "stoul", str.c_str(), idx, base);
}

inline long long stoll(const string &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::strtoll, "stoll", str.c_str(), idx, base);
}

inline unsigned long long stoull(const string &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::strtoull, "stoull", str.c_str(), idx, base);
}

// NB: strtof vs strtod.
inline float stof(const string &str, std::size_t *idx = nullptr) {
    return pvzstl_cxx::stoa(&std::strtof, "stof", str.c_str(), idx);
}

inline double stod(const string &str, std::size_t *idx = nullptr) {
    return pvzstl_cxx::stoa(&std::strtod, "stod", str.c_str(), idx);
}

inline long double stold(const string &str, std::size_t *idx = nullptr) {
    return pvzstl_cxx::stoa(&std::strtold, "stold", str.c_str(), idx);
}

[[nodiscard]] inline string to_string(int val) {
    return pvzstl_cxx::to_xstring<string, 4 * sizeof(int)>(&std::vsnprintf, "%d", val);
}

[[nodiscard]] inline string to_string(unsigned val) {
    return pvzstl_cxx::to_xstring<string, 4 * sizeof(unsigned)>(&std::vsnprintf, "%u", val);
}

[[nodiscard]] inline string to_string(long val) {
    return pvzstl_cxx::to_xstring<string, 4 * sizeof(long)>(&std::vsnprintf, "%ld", val);
}

[[nodiscard]] inline string to_string(unsigned long val) {
    return pvzstl_cxx::to_xstring<string, 4 * sizeof(unsigned long)>(&std::vsnprintf, "%lu", val);
}

[[nodiscard]] inline string to_string(long long val) {
    return pvzstl_cxx::to_xstring<string, 4 * sizeof(long long)>(&std::vsnprintf, "%lld", val);
}

[[nodiscard]] inline string to_string(unsigned long long val) {
    return pvzstl_cxx::to_xstring<string, 4 * sizeof(unsigned long long)>(&std::vsnprintf, "%llu", val);
}

[[nodiscard]] inline string to_string(float val) {
    return pvzstl_cxx::to_xstring<string, std::numeric_limits<float>::max_exponent10 + 20>(&std::vsnprintf, "%f", val);
}

[[nodiscard]] inline string to_string(double val) {
    return pvzstl_cxx::to_xstring<string, std::numeric_limits<double>::max_exponent10 + 20>(&std::vsnprintf, "%f", val);
}

[[nodiscard]] inline string to_string(long double val) {
    return pvzstl_cxx::to_xstring<string, std::numeric_limits<long double>::max_exponent10 + 20>(&std::vsnprintf, "%Lf", val);
}

inline int stoi(const wstring &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa<long, int>(&std::wcstol, "stoi", str.c_str(), idx, base);
}

inline long stol(const wstring &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::wcstol, "stol", str.c_str(), idx, base);
}

inline unsigned long stoul(const wstring &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::wcstoul, "stoul", str.c_str(), idx, base);
}

inline long long stoll(const wstring &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::wcstoll, "stoll", str.c_str(), idx, base);
}

inline unsigned long long stoull(const wstring &str, std::size_t *idx = nullptr, int base = 10) {
    return pvzstl_cxx::stoa(&std::wcstoull, "stoull", str.c_str(), idx, base);
}

// NB: wcstof vs wcstod.
inline float stof(const wstring &str, std::size_t *idx = nullptr) {
    return pvzstl_cxx::stoa(&std::wcstof, "stof", str.c_str(), idx);
}

inline double stod(const wstring &str, std::size_t *idx = nullptr) {
    return pvzstl_cxx::stoa(&std::wcstod, "stod", str.c_str(), idx);
}

inline long double stold(const wstring &str, std::size_t *idx = nullptr) {
    return pvzstl_cxx::stoa(&std::wcstold, "stold", str.c_str(), idx);
}

[[nodiscard]] inline wstring to_wstring(int val) {
    return pvzstl_cxx::to_xstring<wstring, 4 * sizeof(int)>(&std::vswprintf, L"%d", val);
}

[[nodiscard]] inline wstring to_wstring(unsigned val) {
    return pvzstl_cxx::to_xstring<wstring, 4 * sizeof(unsigned)>(&std::vswprintf, L"%u", val);
}

[[nodiscard]] inline wstring to_wstring(long val) {
    return pvzstl_cxx::to_xstring<wstring, 4 * sizeof(long)>(&std::vswprintf, L"%ld", val);
}

[[nodiscard]] inline wstring to_wstring(unsigned long val) {
    return pvzstl_cxx::to_xstring<wstring, 4 * sizeof(unsigned long)>(&std::vswprintf, L"%lu", val);
}

[[nodiscard]] inline wstring to_wstring(long long val) {
    return pvzstl_cxx::to_xstring<wstring, 4 * sizeof(long long)>(&std::vswprintf, L"%lld", val);
}

[[nodiscard]] inline wstring to_wstring(unsigned long long val) {
    return pvzstl_cxx::to_xstring<wstring, 4 * sizeof(unsigned long long)>(&std::vswprintf, L"%llu", val);
}

[[nodiscard]] inline wstring to_wstring(float val) {
    return pvzstl_cxx::to_xstring<wstring, std::numeric_limits<float>::max_exponent10 + 20>(&std::vswprintf, L"%f", val);
}

[[nodiscard]] inline wstring to_wstring(double val) {
    return pvzstl_cxx::to_xstring<wstring, std::numeric_limits<double>::max_exponent10 + 20>(&std::vswprintf, L"%f", val);
}

[[nodiscard]] inline wstring to_wstring(long double val) {
    return pvzstl_cxx::to_xstring<wstring, std::numeric_limits<long double>::max_exponent10 + 20>(&std::vswprintf, L"%Lf", val);
}

} // namespace pvzstl

template <typename CharT, typename Traits, typename Alloc>
struct std::hash<pvzstl::basic_string<CharT, Traits, Alloc>> {
    [[nodiscard]] size_t operator()(const pvzstl::basic_string<CharT, Traits, Alloc> &val) const noexcept {
        using StringView = basic_string_view<CharT, Traits>;
        return hash<StringView>()(StringView(val));
    }
};

namespace pvzstl::inline literals::inline string_literals {

inline string operator""_s(const char *str, std::size_t len) {
    return string{str, len};
}

inline wstring operator""_s(const wchar_t *str, std::size_t len) {
    return wstring{str, len};
}

#ifndef PVZ_VERSION
inline u8string operator""_s(const char8_t *str, std::size_t len) {
    return u8string{str, len};
}

inline u16string operator""_s(const char16_t *str, std::size_t len) {
    return u16string{str, len};
}
#endif // PVZ_VERSION

inline u32string operator""_s(const char32_t *str, std::size_t len) {
    return u32string{str, len};
}

} // namespace pvzstl::inline literals::inline string_literals

#endif // PVZ_STL_BITS_BASIC_STRING_H
