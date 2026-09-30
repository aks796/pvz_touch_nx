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

#ifndef PVZ_STL_BITS_STL_LIST_H
#define PVZ_STL_BITS_STL_LIST_H

/**
 * @file bits/stl_list.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00449.html">stl_list.h File Reference</a>
 */

#include "PvZ/STL/compare.h"

#include "PvZ/STL/debug/assertions.h"

#include "PvZ/STL/bits/alloc_traits.h"
#include "PvZ/STL/bits/allocated_ptr.h"
#include "PvZ/STL/bits/ranges_base.h"
#include "PvZ/STL/bits/stdexcept_throw.h"
#include "PvZ/STL/bits/stl_iterator.h"
#include "PvZ/STL/bits/stl_iterator_base_types.h"

#include "PvZ/STL/ext/aligned_buffer.h"

#include <cstdlib>

#include <type_traits>

namespace pvzstl {

namespace detail {
    struct list_node_base {
        using base_ptr = list_node_base *;

        static void swap(list_node_base &x, list_node_base &y) noexcept;

        void transfer(list_node_base *first, list_node_base *last) noexcept;

        void reverse() noexcept;

        void hook(list_node_base *pos) noexcept;

        void unhook() noexcept;

        list_node_base *get_base() noexcept {
            return this;
        }

        const list_node_base *get_base() const noexcept {
            return this;
        }

        list_node_base *m_next;
        list_node_base *m_prev;
    };

    struct list_size {
#if 0
        // Store the size here so that std::list::size() is fast.
        std::size_t m_size;
#endif
    };

    struct list_node_header : list_node_base, list_size {
        list_node_header() noexcept {
            init();
        }

        list_node_header(list_node_header &&x) noexcept
            : list_node_base(x)
            , list_size(x) {
            if (x.get_base()->m_next == x.get_base()) {
                m_next = m_prev = this;
            } else {
                m_next->m_prev = m_prev->m_next = get_base();
                x.init();
            }
        }

        void move_nodes(list_node_header &&x) noexcept {
            list_node_base *const xnode = x.get_base();
            if (xnode->m_next == xnode)
                init();
            else {
                list_node_base *const node = this->get_base();
                node->m_next = xnode->m_next;
                node->m_prev = xnode->m_prev;
                node->m_next->m_prev = node->m_prev->m_next = node;
                list_size::operator=(x);
                x.init();
            }
        }

        void init() noexcept {
            this->m_next = this->m_prev = this;
            list_size::operator=(list_size());
        }
    };

    template <typename Tp>
    struct list_node;

    template <typename Tp>
    struct list_iterator;

    template <typename Tp>
    struct list_const_iterator;

    namespace _list {
        // Determine the node and iterator types used by std::list.
        template <typename Tp, typename Ptr>
        struct node_traits;

        // Specialization for the simple case where the allocator's pointer type
        // is the same type as value_type*.
        // For ABI compatibility we can't change the types used for this case.
        template <typename Tp>
        struct node_traits<Tp, Tp *> {
            using node_base = detail::list_node_base;
            using node_header = detail::list_node_header;
            using node = detail::list_node<Tp>;
            using iterator = detail::list_iterator<Tp>;
            using const_iterator = detail::list_const_iterator<Tp>;
        };

        // Always use the T* specialization.
        template <typename Tp, typename Ptr>
        struct node_traits : node_traits<Tp, Tp *> {};

        // Used by std::list::sort to hold nodes being sorted.
        template <typename NodeBaseT>
        struct scratch_list : NodeBaseT {
            using base = NodeBaseT;
            using base_ptr = typename base::base_ptr;

            scratch_list() {
                this->m_next = this->m_prev = this->get_base();
            }

            bool empty() const {
                return this->m_next == this->get_base();
            }

            void swap(base &other) {
                base::swap(*this, other);
            }

            template <typename Iter, typename Cmp>
            struct ptr_cmp {
                bool operator()(base_ptr lhs, base_ptr rhs) /* not const */ {
                    return m_cmp(*Iter(lhs), *Iter(rhs));
                }

                Cmp m_cmp;
            };

            template <typename Iter>
            struct ptr_cmp<Iter, void> {
                bool operator()(base_ptr lhs, base_ptr rhs) const {
                    return *Iter(lhs) < *Iter(rhs);
                }
            };

            // Merge nodes from x into *this. Both lists must be sorted wrt Cmp.
            template <typename Cmp>
            void merge(base &x, Cmp comp) {
                base_ptr first1 = this->m_next;
                base_ptr const last1 = this->get_base();
                base_ptr first2 = x.m_next;
                base_ptr const last2 = x.get_base();

                while (first1 != last1 && first2 != last2) {
                    if (comp(first2, first1)) {
                        base_ptr next = first2->m_next;
                        first1->transfer(first2, next);
                        first2 = next;
                    } else {
                        first1 = first1->m_next;
                    }
                }
                if (first2 != last2) {
                    this->transfer(first2, last2);
                }
            }

            // Splice the node at i into *this.
            void take_one(base_ptr i) {
                this->transfer(i, i->m_next);
            }

            // Splice all nodes from *this after i.
            void put_all(base_ptr i) {
                if (!empty()) {
                    i->transfer(this->m_next, this->get_base());
                }
            }
        };
    } // namespace _list

    template <typename Tp>
    struct list_node : detail::list_node_base {
        using node_ptr = list_node *;

        Tp *get_valptr() noexcept {
            return m_storage.ptr();
        }

        const Tp *get_valptr() const noexcept {
            return m_storage.ptr();
        }

        node_ptr get_node_ptr() noexcept {
            return this;
        }

        pvzstl_cxx::aligned_membuf<Tp> m_storage;
    };

    template <typename Tp>
    struct list_iterator {
        using node = list_node<Tp>;

        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Tp;
        using pointer = Tp *;
        using reference = Tp &;

        list_iterator() noexcept
            : m_node() {}

        explicit list_iterator(detail::list_node_base *p) noexcept
            : m_node(p) {}

        list_iterator _const_cast() const noexcept {
            return *this;
        }

        // Must downcast from list_node_base to list_node to get to value.
        [[nodiscard]] reference operator*() const noexcept {
            return *static_cast<node *>(m_node)->get_valptr();
        }

        [[nodiscard]] pointer operator->() const noexcept {
            return static_cast<node *>(m_node)->get_valptr();
        }

        list_iterator &operator++() noexcept {
            m_node = m_node->m_next;
            return *this;
        }

        list_iterator operator++(int) noexcept {
            list_iterator tmp = *this;
            m_node = m_node->m_next;
            return tmp;
        }

        list_iterator &operator--() noexcept {
            m_node = m_node->m_prev;
            return *this;
        }

        list_iterator operator--(int) noexcept {
            list_iterator tmp = *this;
            m_node = m_node->m_prev;
            return tmp;
        }

        [[nodiscard]] friend bool operator==(const list_iterator &lhs, const list_iterator &rhs) noexcept = default;

        // The only member points to the %list element.
        detail::list_node_base *m_node;
    };

    template <typename Tp>
    struct list_const_iterator {
        using node = const list_node<Tp>;
        using iterator = list_iterator<Tp>;

        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Tp;
        using pointer = const Tp *;
        using reference = const Tp &;

        list_const_iterator() noexcept
            : m_node() {}

        explicit list_const_iterator(const detail::list_node_base *p) noexcept
            : m_node(p) {}

        list_const_iterator(const iterator &it) noexcept
            : m_node(it.m_node) {}

        iterator _const_cast() const noexcept {
            return iterator(const_cast<detail::list_node_base *>(m_node));
        }

        // Must downcast from list_node_base to list_node to get to value.
        [[nodiscard]] reference operator*() const noexcept {
            return *static_cast<node *>(m_node)->get_valptr();
        }

        [[nodiscard]] pointer operator->() const noexcept {
            return static_cast<node *>(m_node)->get_valptr();
        }

        list_const_iterator &operator++() noexcept {
            m_node = m_node->m_next;
            return *this;
        }

        list_const_iterator operator++(int) noexcept {
            list_const_iterator tmp = *this;
            m_node = m_node->m_next;
            return tmp;
        }

        list_const_iterator &operator--() noexcept {
            m_node = m_node->m_prev;
            return *this;
        }

        list_const_iterator operator--(int) noexcept {
            list_const_iterator tmp = *this;
            m_node = m_node->m_prev;
            return tmp;
        }

        [[nodiscard]] friend bool operator==(const list_const_iterator &lhs, const list_const_iterator &rhs) noexcept = default;

        // The only member points to the %list element.
        const detail::list_node_base *m_node;
    };

    template <typename Tp, typename Alloc>
    class list_base {
    protected:
        using tp_alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<Tp>;
        using tp_alloc_traits = std::allocator_traits<tp_alloc_type>;

        using node_traits = _list::node_traits<Tp, typename tp_alloc_traits::pointer>;
        using node_alloc_type = typename tp_alloc_traits::template rebind_alloc<typename node_traits::node>;
        using node_alloc_traits = std::allocator_traits<node_alloc_type>;

        using node_ptr = list_node<Tp> *;

    public:
        using allocator_type = Alloc;

        list_base() = default;

        list_base(list_base &&) = default;

        list_base(const node_alloc_type &a)
            : m_impl(a) {}

        // Used when allocator !is_always_equal.
        list_base(node_alloc_type &&a)
            : m_impl(std::move(a)) {}

        // Used when allocator is_always_equal.
        list_base(node_alloc_type &&a, list_base &&other)
            : m_impl(std::move(a), std::move(other.m_impl)) {}

        // This is what actually destroys the list.
        ~list_base() {
            _clear();
        }

        node_alloc_type &get_node_allocator() noexcept {
            return m_impl;
        }

        const node_alloc_type &get_node_allocator() const noexcept {
            return m_impl;
        }

        void move_nodes(list_base &&other) {
            m_impl.m_node.move_nodes(std::move(other.m_impl.m_node));
        }

        void _clear() noexcept {
            using Node = typename node_traits::node;
            using NodeBase = typename node_traits::node_base;
            typename NodeBase::base_ptr cur = m_impl.m_node.m_next;
            while (cur != m_impl.m_node.get_base()) {
                Node &tmp = static_cast<Node &>(*cur);
                cur = tmp.m_next;
                destroy_node(tmp.get_node_ptr());
            }
        }

        void init() noexcept {
            m_impl.m_node.init();
        }

    protected:
        struct list_impl : node_alloc_type {
            list_impl() noexcept(std::is_nothrow_default_constructible_v<node_alloc_type>)
                : node_alloc_type() {}

            list_impl(list_impl &&) = default;

            list_impl(const node_alloc_type &a) noexcept
                : node_alloc_type(a) {}

            list_impl(node_alloc_type &&a) noexcept
                : node_alloc_type(std::move(a)) {}

            list_impl(node_alloc_type &&a, list_impl &&other)
                : node_alloc_type(std::move(a))
                , m_node(std::move(other.m_node)) {}

            typename node_traits::node_header m_node;
        };

#if 1
        // dummy implementations used when the size is not stored
        std::size_t get_size() const noexcept {
            return 0;
        }
        void set_size(std::size_t) noexcept {}
        void inc_size(std::size_t) noexcept {}
        void dec_size(std::size_t) noexcept {}
#endif

        typename node_alloc_traits::pointer get_node() {
            return node_alloc_traits::allocate(m_impl, 1);
        }

        void put_node(node_ptr p) noexcept {
            using alloc_pointer = typename node_alloc_traits::pointer;
            if constexpr (std::is_same_v<node_ptr, alloc_pointer>) {
                node_alloc_traits::deallocate(m_impl, p, 1);
            } else {
                // When not using the allocator's pointer type internally we must
                // convert p to alloc_pointer so it can be deallocated.
                auto ap = std::pointer_traits<alloc_pointer>::pointer_to(*p);
                node_alloc_traits::deallocate(m_impl, ap, 1);
            }
        }

        void destroy_node(node_ptr p) {
            // Destroy the element
            node_alloc_traits::destroy(m_impl, p->get_valptr());
            // Only destroy the node if the pointers require it.
            using Node = typename node_traits::node;
            using BasePtr = typename node_traits::node_base::base_ptr;
            if constexpr (!std::is_trivially_destructible_v<BasePtr>) {
                p->~Node();
            }
            put_node(p);
        }

        list_impl m_impl;
    };
} // namespace detail

/**
 * @brief A standard container with linear time access to elements,
 * and fixed time insertion/deletion at any point in the sequence.
 */
template <typename Tp, typename Alloc = std::allocator<Tp>>
class list : protected detail::list_base<Tp, Alloc> {
    static_assert(std::is_same_v<std::remove_cv_t<Tp>, Tp>, "std::list must have a non-const, non-volatile value_type");
    static_assert(std::is_same_v<typename Alloc::value_type, Tp>, "std::list must have the same value_type as its allocator");

    using base = detail::list_base<Tp, Alloc>;
    using tp_alloc_type = typename base::tp_alloc_type;
    using tp_alloc_traits = typename base::tp_alloc_traits;
    using node_alloc_type = typename base::node_alloc_type;
    using node_alloc_traits = typename base::node_alloc_traits;
    using node_traits = typename base::node_traits;

protected:
    // Note that pointers-to-node's can be ctor-converted to
    // iterator types.
    using node_ptr = typename node_alloc_traits::pointer;

    using base::get_node;
    using base::get_node_allocator;
    using base::m_impl;
    using base::put_node;

public:
    using value_type = Tp;
    using pointer = typename tp_alloc_traits::pointer;
    using const_pointer = typename tp_alloc_traits::const_pointer;
    using reference = Tp &;
    using const_reference = const Tp &;
    using iterator = typename node_traits::iterator;
    using const_iterator = typename node_traits::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = Alloc;

    list() = default;

    list(const list &other)
        : base(node_alloc_traits::select_on_container_copy_construction(other.get_node_allocator())) {
        initialize_dispatch(other.begin(), other.end());
    }

    list(list &&) = default;

    explicit list(const allocator_type &a) noexcept
        : base(node_alloc_type(a)) {}

    list(const list &other, const std::type_identity_t<allocator_type> &a)
        : base(node_alloc_type(a)) {
        initialize_dispatch(other.begin(), other.end());
    }

    list(list &&other, const std::type_identity_t<allocator_type> &a) noexcept(node_alloc_traits::is_always_equal::value)
        : list(std::move(other), a, typename node_alloc_traits::is_always_equal{}) {}

    list(size_type n, const value_type &value, const allocator_type &a = allocator_type())
        : base(node_alloc_type(a)) {
        fill_initialize(n, value);
    }

    explicit list(size_type n, const allocator_type &a = allocator_type())
        : base(node_alloc_type(a)) {
        default_initialize(n);
    }

    template <detail::has_input_iter_cat InputIterator>
    list(InputIterator first, InputIterator last, const allocator_type &a = allocator_type())
        : base(node_alloc_type(a)) {
        initialize_dispatch(first, last);
    }

    template <detail::container_compatible_range<Tp> Rg>
    list(std::from_range_t, Rg &&rg, const Alloc &a = Alloc())
        : base(node_alloc_type(a)) {
        auto first = std::ranges::begin(rg);
        const auto last = std::ranges::end(rg);
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

    list(std::initializer_list<value_type> il, const allocator_type &a = allocator_type())
        : base(node_alloc_type(a)) {
        initialize_dispatch(il.begin(), il.end());
    }

    ~list() = default;

    list &operator=(const list &other) {
        if (this == std::addressof(other)) {
            return *this;
        }
        if (node_alloc_traits::propagate_on_container_copy_assignment::value) {
            auto &this_alloc = get_node_allocator();
            auto &that_alloc = other.get_node_allocator();
            if (!node_alloc_traits::is_always_equal::value && this_alloc != that_alloc) {
                // replacement allocator cannot free existing storage
                clear();
            }
            detail::alloc_on_copy(this_alloc, that_alloc);
        }
        assign_dispatch(other.begin(), other.end());
        return *this;
    }

    list &operator=(list &&other) noexcept(node_alloc_traits::propagate_on_container_move_assignment::value || node_alloc_traits::is_always_equal::value) {
        constexpr bool move_storage = node_alloc_traits::propagate_on_container_move_assignment::value || node_alloc_traits::is_always_equal::value;
        if constexpr (!move_storage) {
            if (other.get_node_allocator() != get_node_allocator()) {
                // The rvalue's allocator cannot be moved, or is not equal,
                // so we need to individually move each element.
                assign_dispatch(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
                return *this;
            }
        }

        clear();
        this->move_nodes(std::move(other));

        detail::alloc_on_move(get_node_allocator(), other.get_node_allocator());

        return *this;
    }

    list &operator=(std::initializer_list<value_type> il) {
        assign(il.begin(), il.end());
        return *this;
    }

    void assign(size_type n, const value_type &val) {
        fill_assign(n, val);
    }

    template <detail::has_input_iter_cat InputIterator>
    void assign(InputIterator first, InputIterator last) {
        assign_dispatch(first, last);
    }

    void assign(std::initializer_list<value_type> il) {
        assign_dispatch(il.begin(), il.end());
    }

    template <detail::container_compatible_range<Tp> Rg>
    void assign_range(Rg &&rg) {
        static_assert(std::assignable_from<Tp &, std::ranges::range_reference_t<Rg>>);

        iterator first1 = begin();
        const iterator last1 = end();
        auto first2 = std::ranges::begin(rg);
        const auto last2 = std::ranges::end(rg);
        for (; first1 != last1 && first2 != last2; ++first1, (void)++first2) {
            *first1 = *first2;
        }
        if (first2 == last2) {
            erase(first1, last1);
        } else {
            insert_range(last1, std::ranges::subrange(std::move(first2), last2));
        }
    }

    allocator_type get_allocator() const noexcept {
        return allocator_type(base::get_node_allocator());
    }

    [[nodiscard]] reference front() noexcept {
        PVZSTL_CXX_REQUIRES_NOEMPTY();
        return *begin();
    }

    [[nodiscard]] const_reference front() const noexcept {
        PVZSTL_CXX_REQUIRES_NOEMPTY();
        return *begin();
    }

    [[nodiscard]] reference back() noexcept {
        PVZSTL_CXX_REQUIRES_NOEMPTY();
        iterator tmp = end();
        --tmp;
        return *tmp;
    }

    [[nodiscard]] const_reference back() const noexcept {
        PVZSTL_CXX_REQUIRES_NOEMPTY();
        const_iterator tmp = end();
        --tmp;
        return *tmp;
    }

    [[nodiscard]] iterator begin() noexcept {
        return iterator(m_impl.m_node.m_next);
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return const_iterator(m_impl.m_node.m_next);
    }

    [[nodiscard]] iterator end() noexcept {
        return iterator(m_impl.m_node.get_base());
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return const_iterator(m_impl.m_node.get_base());
    }

    [[nodiscard]] reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    [[nodiscard]] reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }

    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }

    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return rend();
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_impl.m_node.m_next == m_impl.m_node.get_base();
    }

    [[nodiscard]] size_type size() const noexcept {
        return std::distance(begin(), end()); // count the number of nodes
    }

    [[nodiscard]] size_type max_size() const noexcept {
        return node_alloc_traits::max_size(get_node_allocator());
    }

    void clear() noexcept {
        base::_clear();
        base::init();
    }

    iterator insert(const_iterator pos, const value_type &v) {
        node_ptr tmp = create_node(v);
        tmp->hook(pos._const_cast().m_node);
        this->inc_size(1);
        return iterator(tmp->get_base());
    }


    iterator insert(const_iterator pos, value_type &&v) {
        return emplace(pos, std::move(v));
    }

    iterator insert(const_iterator pos, size_type n, const value_type &v) {
        if (n > 0) {
            list tmp(n, v, get_allocator());
            iterator it = tmp.begin();
            splice(pos, tmp);
            return it;
        }
        return pos._const_cast();
    }

    template <detail::has_input_iter_cat InputIterator>
    iterator insert(const_iterator pos, InputIterator first, InputIterator last) {
        list tmp(first, last, get_allocator());
        if (!tmp.empty()) {
            iterator it = tmp.begin();
            splice(pos, tmp);
            return it;
        }
        return pos._const_cast();
    }

    iterator insert(const_iterator pos, std::initializer_list<value_type> il) {
        return insert(pos, il.begin(), il.end());
    }

    template <detail::container_compatible_range<Tp> Rg>
    iterator insert_range(const_iterator pos, Rg &&rg) {
        list tmp(std::from_range, std::forward<Rg>(rg), get_allocator());
        if (!tmp.empty()) {
            iterator it = tmp.begin();
            splice(pos, tmp);
            return it;
        }
        return pos._const_cast();
    }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args &&...args) {
        node_ptr tmp = create_node(std::forward<Args>(args)...);
        tmp->hook(pos._const_cast().m_node);
        this->inc_size(1);
        return iterator(tmp->get_base());
    }

    iterator erase(const_iterator pos) {
        iterator ret = iterator(pos.m_node->m_next);
        _erase(pos._const_cast());
        return ret;
    }

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            first = erase(first);
        }
        return last._const_cast();
    }

    void push_back(const value_type &v) {
        _insert(end(), v);
    }

    void push_back(value_type &&v) {
        _insert(end(), std::move(v));
    }

    template <typename... Args>
    reference emplace_back(Args &&...args) {
        _insert(end(), std::forward<Args>(args)...);
        return back();
    }

    template <detail::container_compatible_range<Tp> Rg>
    void append_range(Rg &&rg) {
        list tmp(std::from_range, std::forward<Rg>(rg), get_allocator());
        if (!tmp.empty()) {
            splice(end(), tmp);
        }
    }

    void pop_back() noexcept {
        PVZSTL_CXX_REQUIRES_NOEMPTY();
        _erase(iterator(m_impl.m_node.m_prev));
    }

    void push_front(const value_type &v) {
        _insert(begin(), v);
    }

    void push_front(value_type &&v) {
        _insert(begin(), std::move(v));
    }

    template <typename... Args>
    reference emplace_front(Args &&...args) {
        _insert(begin(), std::forward<Args>(args)...);
        return front();
    }

    template <detail::container_compatible_range<Tp> Rg>
    void prepend_range(Rg &&rg) {
        list tmp(std::from_range, std::forward<Rg>(rg), get_allocator());
        if (!tmp.empty()) {
            splice(begin(), tmp);
        }
    }

    void pop_front() noexcept {
        PVZSTL_CXX_REQUIRES_NOEMPTY();
        _erase(begin());
    }

    void resize(size_type new_size) {
        const_iterator it = resize_pos(new_size);
        if (new_size > 0) {
            default_append(new_size);
        } else {
            erase(it, end());
        }
    }

    void resize(size_type new_size, const value_type &v) {
        const_iterator it = resize_pos(new_size);
        if (new_size > 0) {
            insert(end(), new_size, v);
        } else {
            erase(it, end());
        }
    }

    void swap(list &other) noexcept {
        node_traits::node_base::swap(m_impl.m_node, other.m_impl.m_node);

        std::size_t xsize = other.get_size();
        other.set_size(this->get_size());
        this->set_size(xsize);

        detail::alloc_on_swap(get_node_allocator(), other.get_node_allocator());
    }

    void merge(list &other) {
        merge(std::move(other));
    }

    void merge(list &&other) {
        if (this == std::addressof(other)) {
            return;
        }
        check_equal_allocators(other);

        iterator first1 = begin();
        iterator last1 = end();
        iterator first2 = other.begin();
        iterator last2 = other.end();

        const finalize_merge fin(*this, other, first2);

        while (first1 != last1 && first2 != last2) {
            if (*first2 < *first1) {
                iterator next = first2;
                transfer(first1, first2, ++next);
                first2 = next;
            } else {
                ++first1;
            }
        }
        if (first2 != last2) {
            transfer(last1, first2, last2);
            first2 = last2;
        }
    }

    template <typename StrictWeakOrdering>
    void merge(list &other, StrictWeakOrdering comp) {
        merge(std::move(other), comp);
    }

    template <typename StrictWeakOrdering>
    void merge(list &&other, StrictWeakOrdering comp) {
        if (this == std::addressof(other)) {
            return;
        }
        check_equal_allocators(other);

        iterator first1 = begin();
        iterator last1 = end();
        iterator first2 = other.begin();
        iterator last2 = other.end();

        const finalize_merge fin(*this, other, first2);

        while (first1 != last1 && first2 != last2) {
            if (comp(*first2, *first1)) {
                iterator next = first2;
                transfer(first1, first2, ++next);
                first2 = next;
            } else {
                ++first1;
            }
        }
        if (first2 != last2) {
            transfer(last1, first2, last2);
            first2 = last2;
        }
    }

    void splice(const_iterator pos, list &other) noexcept {
        splice(pos, std::move(other));
    }

    void splice(const_iterator pos, list &&other) noexcept {
        if (!other.empty()) {
            check_equal_allocators(other);

            transfer(pos._const_cast(), other.begin(), other.end());

            this->inc_size(other.get_size());
            other.set_size(0);
        }
    }

    void splice(const_iterator pos, list &other, const_iterator it) noexcept {
        splice(pos, std::move(other), it);
    }

    void splice(const_iterator pos, list &&other, const_iterator it) noexcept {
        iterator j = it._const_cast();
        ++j;
        if (pos == it || pos == j) {
            return;
        }

        if (this != std::addressof(other)) {
            check_equal_allocators(other);
        }

        transfer(pos._const_cast(), it._const_cast(), j);

        this->inc_size(1);
        other.dec_size(1);
    }

    void splice(const_iterator pos, list &other, const_iterator first, const_iterator last) noexcept {
        splice(pos, std::move(other), first, last);
    }

    void splice(const_iterator pos, list &&other, const_iterator first, const_iterator last) noexcept {
        if (first == last) {
            return;
        }
        if (this != std::addressof(other)) {
            check_equal_allocators(other);
        }

        transfer(pos._const_cast(), first._const_cast(), last._const_cast());
    }

    size_type remove(const value_type &value) {
        size_type removed = 0;
        list to_destroy(get_allocator());
        iterator first = begin();
        iterator last = end();
        while (first != last) {
            iterator next = first;
            ++next;
            if (*first == value) {
                to_destroy.splice(to_destroy.end(), *this, first);
                ++removed;
            }
            first = next;
        }

        return removed;
    }

    template <typename Predicate>
    size_type remove_if(Predicate pred) {
        size_type removed = 0;
        list to_destroy(get_allocator());
        iterator first = begin();
        iterator last = end();
        while (first != last) {
            iterator next = first;
            ++next;
            if (pred(*first)) {
                to_destroy.splice(to_destroy.end(), *this, first);
                ++removed;
            }
            first = next;
        }

        return removed;
    }

    void reverse() noexcept {
        m_impl.m_node.reverse();
    }

    size_type unique() {
        iterator first = begin();
        iterator last = end();
        if (first == last) {
            return 0;
        }
        size_type removed = 0;
        list to_destroy(get_allocator());
        iterator next = first;
        while (++next != last) {
            if (*first == *next) {
                to_destroy.splice(to_destroy.end(), *this, next);
                ++removed;
            } else {
                first = next;
            }
            next = first;
        }

        return removed;
    }

    template <typename BinaryPredicate>
    size_type unique(BinaryPredicate binary_pred) {
        iterator first = begin();
        iterator last = end();
        if (first == last) {
            return 0;
        }
        size_type removed = 0;
        list to_destroy(get_allocator());
        iterator next = first;
        while (++next != last) {
            if (binary_pred(*first, *next)) {
                to_destroy.splice(to_destroy.end(), *this, next);
                ++removed;
            } else {
                first = next;
            }
            next = first;
        }

        return removed;
    }

    void sort() {
        // Do nothing if the list has length 0 or 1.
        if (empty() || ++begin() == end()) {
            return;
        }

        using scratch_list = detail::_list::scratch_list<typename node_traits::node_base>;

        // The algorithm used here is largely unchanged from the SGI STL
        // and is described in The C++ Standard Template Library by Plauger,
        // Stepanov, Lee, Musser.
        // Each element of *this is spliced out and merged into one of the
        // sorted lists in tmp, then all the lists in tmp are merged
        // together and then swapped back into *this.
        // Because all nodes end up back in *this we do not need to update
        // this->size() while nodes are temporarily moved out.
        scratch_list carry;
        scratch_list tmp[64];
        scratch_list *fill = tmp;
        scratch_list *counter;

        typename scratch_list::template ptr_cmp<iterator, void> ptr_cmp;

        PVZSTL_TRY {
            do {
                carry.take_one(begin().m_node);

                for (counter = tmp; counter != fill && !counter->empty(); ++counter) {
                    counter->merge(carry, ptr_cmp);
                    carry.swap(*counter);
                }
                carry.swap(*counter);
                if (counter == fill) {
                    ++fill;
                }
            } while (!empty());

            for (counter = tmp + 1; counter != fill; ++counter) {
                counter->merge(counter[-1], ptr_cmp);
            }
            fill[-1].swap(m_impl.m_node);
        }
        PVZSTL_CATCH(...) {
            // Move all nodes back into *this.
            carry.put_all(end().m_node);
            for (int i = 0; i < sizeof(tmp) / sizeof(tmp[0]); ++i) {
                tmp[i].put_all(end().m_node);
            }
            PVZSTL_THROW_EXCEPTION_AGAIN;
        }
    }

    template <typename StrictWeakOrdering>
    void sort(StrictWeakOrdering comp) {
        // Do nothing if the list has length 0 or 1.
        if (empty() || ++begin() == end()) {
            return;
        }

        using scratch_list = detail::_list::scratch_list<typename node_traits::node_base>;

        scratch_list carry;
        scratch_list tmp[64];
        scratch_list *fill = tmp;
        scratch_list *counter;

        typename scratch_list::template ptr_cmp<iterator, StrictWeakOrdering> ptr_cmp{comp};

        PVZSTL_TRY {
            do {
                carry.take_one(begin().m_node);

                for (counter = tmp; counter != fill && !counter->empty(); ++counter) {
                    counter->merge(carry, ptr_cmp);
                    carry.swap(*counter);
                }
                carry.swap(*counter);
                if (counter == fill) {
                    ++fill;
                }
            } while (!empty());

            for (counter = tmp + 1; counter != fill; ++counter) {
                counter->merge(counter[-1], ptr_cmp);
            }
            fill[-1].swap(m_impl.m_node);
        }
        PVZSTL_CATCH(...) {
            // Move all nodes back into *this.
            carry.put_all(end().m_node);
            for (int i = 0; i < sizeof(tmp) / sizeof(tmp[0]); ++i) {
                tmp[i].put_all(end().m_node);
            }
            PVZSTL_THROW_EXCEPTION_AGAIN;
        }
    }

protected:
    struct finalize_merge {
        explicit finalize_merge(list &, list &, const iterator &) {}
    };

    list(list &&other, const allocator_type &a, std::true_type) noexcept
        : base(node_alloc_type(a), std::move(other)) {}

    list(list &&other, const allocator_type &a, std::false_type)
        : base(node_alloc_type(a)) {
        if (other.get_node_allocator() == get_node_allocator()) {
            this->move_nodes(std::move(other));
        } else {
            insert(begin(), detail::make_move_if_noexcept_iterator(other.begin()), detail::make_move_if_noexcept_iterator(other.end()));
        }
    }

    template <typename... Args>
    node_ptr create_node(Args &&...args) {
        auto &alloc = get_node_allocator();
        auto guard = detail::allocate_guarded_obj(alloc);
        node_alloc_traits::construct(alloc, guard->get_valptr(), std::forward<Args>(args)...);
        return guard.release();
    }

    // Called by the range constructor to implement [23.1.1]/9
    template <typename InputIterator>
    void initialize_dispatch(InputIterator first, InputIterator last) {
        bool notempty = first != last;
        for (; first != last; ++first) {
            emplace_back(*first);
        }
        if (!notempty) {
            if (begin() == end()) {
                std::unreachable();
            }
        }
    }

    // Called by list(n,v,a), and the range constructor when it turns out
    // to be the same thing.
    void fill_initialize(size_type n, const value_type &v) {
        for (; n > 0; --n) {
            push_back(v);
        }
    }

    // Called by list(n).
    void default_initialize(size_type n) {
        for (; n > 0; --n) {
            emplace_back();
        }
    }

    // Called by resize(sz).
    void default_append(size_type n) {
        size_type i = 0;
        PVZSTL_TRY {
            for (; i < n; ++i) {
                emplace_back();
            }
        }
        PVZSTL_CATCH(...) {
            for (; i > 0; --i) {
                pop_back();
            }
            PVZSTL_THROW_EXCEPTION_AGAIN;
        }
    }

    // Called by the range assign to implement [23.1.1]/9
    template <typename InputIterator>
    void assign_dispatch(InputIterator first, InputIterator last) {
        iterator first1 = begin();
        iterator last1 = end();
        for (; first1 != last1 && first != last; ++first1, (void)++first) {
            *first1 = *first;
        }
        if (first == last) {
            erase(first1, last1);
        } else {
            insert(last1, first, last);
        }
    }

    // Called by assign(n,t), and the range assign when it turns out
    // to be the same thing.
    void fill_assign(size_type n, const value_type &val) {
        iterator it = begin();
        for (; it != end() && n > 0; ++it, --n) {
            *it = val;
        }
        if (n > 0) {
            insert(end(), n, val);
        } else {
            erase(it, end());
        }
    }

    // Moves the elements from [first,last) before position.
    void transfer(iterator pos, iterator first, iterator last) {
        pos.m_node->transfer(first.m_node, last.m_node);
    }

    // Inserts new element at position given and with value given.
    template <typename... Args>
    void _insert(iterator pos, Args &&...args) {
        node_ptr tmp = create_node(std::forward<Args>(args)...);
        tmp->hook(pos.m_node);
        this->inc_size(1);
    }

    // Erases element at position given.
    void _erase(iterator pos) noexcept {
        using node = typename node_traits::node;
        this->dec_size(1);
        pos.m_node->unhook();
        node &n = static_cast<node &>(*pos.m_node);
        this->destroy_node(n.get_node_ptr());
    }

    // To implement the splice (and merge) bits of N1599.
    void check_equal_allocators(const list &other) noexcept {
        if (get_node_allocator() != other.get_node_allocator()) {
            std::abort();
        }
    }

    const_iterator resize_pos(size_type &new_size) const {
        const_iterator it;
        size_type len = 0;
        for (it = begin(); it != end() && len < new_size; ++it, ++len) {}
        new_size -= len;
        return it;
    }
};

template <detail::has_input_iter_cat InputIterator, typename ValT = typename std::iterator_traits<InputIterator>::value_type, detail::allocator_like Allocator = std::allocator<ValT>>
list(InputIterator, InputIterator, Allocator = Allocator()) -> list<ValT, Allocator>;

template <std::ranges::input_range Rg, typename Allocator = std::allocator<std::ranges::range_value_t<Rg>>>
list(std::from_range_t, Rg &&, Allocator = Allocator()) -> list<std::ranges::range_value_t<Rg>, Allocator>;

template <typename Tp, typename Alloc>
[[nodiscard]] bool operator==(const list<Tp, Alloc> &lhs, const list<Tp, Alloc> &rhs) {
    using const_iterator = typename list<Tp, Alloc>::const_iterator;
    const_iterator end1 = lhs.end();
    const_iterator end2 = rhs.end();

    const_iterator it1 = lhs.begin();
    const_iterator it2 = rhs.begin();
    while (it1 != end1 && it2 != end2 && *it1 == *it2) {
        ++it1;
        ++it2;
    }
    return it1 == end1 && it2 == end2;
}

template <typename Tp, typename Alloc>
[[nodiscard]] auto operator<=>(const list<Tp, Alloc> &lhs, const list<Tp, Alloc> &rhs) {
    return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), detail::synth3way);
}

template <typename Tp, typename Alloc>
void swap(list<Tp, Alloc> &lhs, list<Tp, Alloc> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

} // namespace pvzstl

#endif // PVZ_STL_BITS_STL_LIST_H
