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

#include "PvZ/STL/bits/stl_tree.h"

static pvzstl::detail::rb_tree_node_base *local_rb_tree_increment(pvzstl::detail::rb_tree_node_base *x) noexcept {
    using namespace pvzstl::detail;
    if (x->m_right != nullptr) {
        x = x->m_right;
        while (x->m_left != nullptr) {
            x = x->m_left;
        }
    } else {
        rb_tree_node_base *y = x->m_parent;
        while (x == y->m_right) {
            x = y;
            y = y->m_parent;
        }
        if (x->m_right != y)
            x = y;
    }
    return x;
}

auto pvzstl::detail::rb_tree_increment(pvzstl::detail::rb_tree_node_base *x) noexcept -> rb_tree_node_base * {
    return local_rb_tree_increment(x);
}

static pvzstl::detail::rb_tree_node_base *local_rb_tree_decrement(pvzstl::detail::rb_tree_node_base *x) noexcept {
    using namespace pvzstl::detail;
    if (x->m_color == rb_tree_color::red && x->m_parent->m_parent == x) {
        x = x->m_right;
    } else if (x->m_left != nullptr) {
        rb_tree_node_base *y = x->m_left;
        while (y->m_right != nullptr) {
            y = y->m_right;
        }
        x = y;
    } else {
        rb_tree_node_base *y = x->m_parent;
        while (x == y->m_left) {
            x = y;
            y = y->m_parent;
        }
        x = y;
    }
    return x;
}

auto pvzstl::detail::rb_tree_decrement(rb_tree_node_base *x) noexcept -> rb_tree_node_base * {
    return local_rb_tree_decrement(x);
}

static void local_rb_tree_rotate_left(pvzstl::detail::rb_tree_node_base *x, pvzstl::detail::rb_tree_node_base *&root) noexcept {
    using namespace pvzstl::detail;
    rb_tree_node_base *const y = x->m_right;

    x->m_right = y->m_left;
    if (y->m_left != nullptr) {
        y->m_left->m_parent = x;
    }
    y->m_parent = x->m_parent;

    if (x == root) {
        root = y;
    } else if (x == x->m_parent->m_left) {
        x->m_parent->m_left = y;
    } else {
        x->m_parent->m_right = y;
    }
    y->m_left = x;
    x->m_parent = y;
}

static void local_rb_tree_rotate_right(pvzstl::detail::rb_tree_node_base *x, pvzstl::detail::rb_tree_node_base *&root) noexcept {
    using namespace pvzstl::detail;
    rb_tree_node_base *const y = x->m_left;

    x->m_left = y->m_right;
    if (y->m_right != nullptr) {
        y->m_right->m_parent = x;
    }
    y->m_parent = x->m_parent;

    if (x == root) {
        root = y;
    } else if (x == x->m_parent->m_right) {
        x->m_parent->m_right = y;
    } else {
        x->m_parent->m_left = y;
    }
    y->m_right = x;
    x->m_parent = y;
}

void pvzstl::detail::rb_tree_insert_and_rebalance(bool insert_left, rb_tree_node_base *x, rb_tree_node_base *p, rb_tree_node_base &header) noexcept {
    using namespace pvzstl::detail;
    rb_tree_node_base *&root = header.m_parent;

    // Initialize fields in new node to insert.
    x->m_parent = p;
    x->m_left = nullptr;
    x->m_right = nullptr;
    x->m_color = rb_tree_color::red;

    // Insert.
    // Make new node child of parent and maintain root, leftmost and
    // rightmost nodes.
    // N.B. First node is always inserted left.
    if (insert_left) {
        p->m_left = x; // also makes leftmost = x when p == &header

        if (p == &header) {
            header.m_parent = x;
            header.m_right = x;
        } else if (p == header.m_left) {
            header.m_left = x; // maintain leftmost pointing to min node
        }
    } else {
        p->m_right = x;

        if (p == header.m_right) {
            header.m_right = x; // maintain rightmost pointing to max node
        }
    }

    // Rebalance.
    while (x != root && x->m_parent->m_color == rb_tree_color::red) {
        rb_tree_node_base *const xpp = x->m_parent->m_parent;

        if (x->m_parent == xpp->m_left) {
            rb_tree_node_base *const y = xpp->m_right;
            if (y && y->m_color == rb_tree_color::red) {
                x->m_parent->m_color = rb_tree_color::black;
                y->m_color = rb_tree_color::black;
                xpp->m_color = rb_tree_color::red;
                x = xpp;
            } else {
                if (x == x->m_parent->m_right) {
                    x = x->m_parent;
                    local_rb_tree_rotate_left(x, root);
                }
                x->m_parent->m_color = rb_tree_color::black;
                xpp->m_color = rb_tree_color::red;
                local_rb_tree_rotate_right(xpp, root);
            }
        } else {
            rb_tree_node_base *const y = xpp->m_left;
            if (y && y->m_color == rb_tree_color::red) {
                x->m_parent->m_color = rb_tree_color::black;
                y->m_color = rb_tree_color::black;
                xpp->m_color = rb_tree_color::red;
                x = xpp;
            } else {
                if (x == x->m_parent->m_left) {
                    x = x->m_parent;
                    local_rb_tree_rotate_right(x, root);
                }
                x->m_parent->m_color = rb_tree_color::black;
                xpp->m_color = rb_tree_color::red;
                local_rb_tree_rotate_left(xpp, root);
            }
        }
    }
    root->m_color = rb_tree_color::black;
}

auto pvzstl::detail::rb_tree_rebalance_for_erase(rb_tree_node_base *z, rb_tree_node_base &header) noexcept -> rb_tree_node_base * {
    using namespace pvzstl::detail;
    rb_tree_node_base *&root = header.m_parent;
    rb_tree_node_base *&leftmost = header.m_left;
    rb_tree_node_base *&rightmost = header.m_right;
    rb_tree_node_base *y = z;
    rb_tree_node_base *x = nullptr;
    rb_tree_node_base *x_parent = nullptr;

    if (y->m_left == nullptr) {         // z has at most one non-null child. y == z.
        x = y->m_right;                 // x might be null.
    } else if (y->m_right == nullptr) { // z has exactly one non-null child. y == z.
        x = y->m_left;                  // y has a single child
    } else {
        // z has two non-null children.  Set y to
        y = y->m_right; //   z's successor.  x might be null.
        while (y->m_left != nullptr) {
            y = y->m_left;
        }
        x = y->m_right;
    }
    if (y != z) {
        // relink y in place of z.  y is z's successor
        z->m_left->m_parent = y;
        y->m_left = z->m_left;
        if (y != z->m_right) {
            x_parent = y->m_parent;
            if (x) {
                x->m_parent = y->m_parent;
            }
            y->m_parent->m_left = x; // y must be a child of m_left
            y->m_right = z->m_right;
            z->m_right->m_parent = y;
        } else {
            x_parent = y;
        }
        if (root == z) {
            root = y;
        } else if (z->m_parent->m_left == z) {
            z->m_parent->m_left = y;
        } else {
            z->m_parent->m_right = y;
        }
        y->m_parent = z->m_parent;
        std::swap(y->m_color, z->m_color);
        y = z;
        // y now points to node to be actually deleted
    } else { // y == z
        x_parent = y->m_parent;
        if (x) {
            x->m_parent = y->m_parent;
        }
        if (root == z) {
            root = x;
        } else if (z->m_parent->m_left == z) {
            z->m_parent->m_left = x;
        } else {
            z->m_parent->m_right = x;
        }
        if (leftmost == z) {
            if (z->m_right == nullptr) { // z->m_left must be null also
                leftmost = z->m_parent;
            }
            // makes leftmost == &m_header if z == root
            else {
                leftmost = rb_tree_node_base::minimum(x);
            }
        }
        if (rightmost == z) {
            if (z->m_left == nullptr) { // z->m_right must be null also
                rightmost = z->m_parent;
            }
            // makes rightmost == &m_header if z == root
            else { // x == z->_M_left
                rightmost = rb_tree_node_base::maximum(x);
            }
        }
    }
    if (y->m_color != rb_tree_color::red) {
        while (x != root && (x == nullptr || x->m_color == rb_tree_color::black)) {
            if (x == x_parent->m_left) {
                rb_tree_node_base *w = x_parent->m_right;
                if (w->m_color == rb_tree_color::red) {
                    w->m_color = rb_tree_color::black;
                    x_parent->m_color = rb_tree_color::red;
                    local_rb_tree_rotate_left(x_parent, root);
                    w = x_parent->m_right;
                }
                if ((w->m_left == nullptr || w->m_left->m_color == rb_tree_color::black) && (w->m_right == nullptr || w->m_right->m_color == rb_tree_color::black)) {
                    w->m_color = rb_tree_color::red;
                    x = x_parent;
                    x_parent = x_parent->m_parent;
                } else {
                    if (w->m_right == nullptr || w->m_right->m_color == rb_tree_color::black) {
                        w->m_left->m_color = rb_tree_color::black;
                        w->m_color = rb_tree_color::red;
                        local_rb_tree_rotate_right(w, root);
                        w = x_parent->m_right;
                    }
                    w->m_color = x_parent->m_color;
                    x_parent->m_color = rb_tree_color::black;
                    if (w->m_right) {
                        w->m_right->m_color = rb_tree_color::black;
                    }
                    local_rb_tree_rotate_left(x_parent, root);
                    break;
                }
            } else {
                // same as above, with m_right <-> m_left.
                rb_tree_node_base *w = x_parent->m_left;
                if (w->m_color == rb_tree_color::red) {
                    w->m_color = rb_tree_color::black;
                    x_parent->m_color = rb_tree_color::red;
                    local_rb_tree_rotate_right(x_parent, root);
                    w = x_parent->m_left;
                }
                if ((w->m_right == nullptr || w->m_right->m_color == rb_tree_color::black) && (w->m_left == nullptr || w->m_left->m_color == rb_tree_color::black)) {
                    w->m_color = rb_tree_color::red;
                    x = x_parent;
                    x_parent = x_parent->m_parent;
                } else {
                    if (w->m_left == nullptr || w->m_left->m_color == rb_tree_color::black) {
                        w->m_right->m_color = rb_tree_color::black;
                        w->m_color = rb_tree_color::red;
                        local_rb_tree_rotate_left(w, root);
                        w = x_parent->m_left;
                    }
                    w->m_color = x_parent->m_color;
                    x_parent->m_color = rb_tree_color::black;
                    if (w->m_left) {
                        w->m_left->m_color = rb_tree_color::black;
                    }
                    local_rb_tree_rotate_right(x_parent, root);
                    break;
                }
            }
        }
        if (x) {
            x->m_color = rb_tree_color::black;
        }
    }
    return y;
}

unsigned int pvzstl::detail::rb_tree_black_count(const rb_tree_node_base *node, const rb_tree_node_base *root) noexcept {
    if (node == nullptr) {
        return 0;
    }
    unsigned int sum = 0;
    do {
        if (node->m_color == rb_tree_color::black) {
            ++sum;
        }
        if (node == root) {
            break;
        }
        node = node->m_parent;
    } while (true);
    return sum;
}
