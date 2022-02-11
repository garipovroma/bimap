#include "cartesian_tree.h"

static std::mt19937 rnd;

uint32_t details::get_next_random_uint32_t() {
    return static_cast<uint32_t>(rnd());
}

void details::actualize_children_parents(details::node_base& x, details::node_base& y) noexcept {
    std::swap(x.left->parent, y.left->parent);
    std::swap(x.right->parent, y.right->parent);
}

void details::swap_sentinel(details::node_base& x, details::node_base& y) noexcept {
    actualize_children_parents(x, y);
    std::swap(x.left, y.left);
    std::swap(x.right, y.right);
    std::swap(x.parent, y.parent);
    std::swap(x.priority, y.priority);
}

void details::set_parent(details::node_base* node, details::node_base* parent) noexcept {
    if (node != nullptr) {
        node->parent = parent;
    }
}

bool details::is_right_son(details::node_base* node, details::node_base* son) noexcept {
    if (node == nullptr || son == nullptr) {
        return false;
    }
    return (node->right == son);
}

bool details::is_left_son(details::node_base* node, details::node_base* son) noexcept {
    if (node == nullptr || son == nullptr) {
        return false;
    }
    return (node->left == son);
}

details::node_base* details::get_next(details::node_base* v) noexcept {
    if (v == nullptr) {
        return nullptr;
    }
    if (v->right != nullptr) {
        return get_leftmost(v->right);
    } else {
        while (is_right_son(v->parent, v)) {
            v = v->parent;
        }
        return v->parent;
    }
}

details::node_base* details::get_prev(details::node_base* v) noexcept {
    if (v == nullptr) {
        return nullptr;
    }
    if (v->left != nullptr) {
        return get_rightmost(v->left);
    } else {
        while (is_left_son(v->parent, v)) {
            v = v->parent;
        }
        return v->parent;
    }
}

details::node_base* details::get_leftmost(details::node_base* v) noexcept {
    if (v == nullptr) {
        return nullptr;
    }
    while (v->left != nullptr) {
        v = v->left;
    }
    return v;
}

details::node_base* details::get_rightmost(details::node_base* v) noexcept {
    if (v == nullptr) {
        return nullptr;
    }
    while (v->right != nullptr) {
        v = v->right;
    }
    return v;
}
