#pragma once
#include <functional>
#include <random>
#include <type_traits>

template <typename Left, typename Right, typename CompareLeft,
          typename CompareRight>
struct bimap;

namespace details {

template <typename left_it, typename right_it, typename left_it_tag,
          typename right_it_tag>
struct base_iterator;

template <typename T, typename Comparator, typename Tag>
struct tree;

template <typename T, typename Comparator>
struct tree_node;

uint32_t get_next_random_uint32_t();

struct node_base {

    node_base() = default;
    node_base(const node_base&) = delete;
    node_base(node_base&& other) = default;

    friend node_base* get_next(node_base*) noexcept;

    friend node_base* get_prev(node_base*) noexcept;

    friend node_base* get_leftmost(node_base* v) noexcept;

    friend node_base* get_rightmost(node_base* v) noexcept;

    template <typename T, typename Comparator, typename Tag>
    friend struct tree;

    template <typename T, typename Tag>
    friend struct tree_node;

    template <typename Left, typename Right, typename CompareLeft,
              typename CompareRight>
    friend struct ::bimap;

    friend void set_parent(node_base* node, node_base* parent) noexcept;
    friend bool is_right_son(node_base* node, node_base* son) noexcept;
    friend bool is_left_son(node_base* node, node_base* son) noexcept;
    friend void swap_sentinel(node_base&, node_base&) noexcept;
    friend void actualize_children_parents(node_base& x, node_base& y) noexcept;

  private:
    node_base* left{nullptr};
    node_base* right{nullptr};
    node_base* parent{nullptr};
    uint32_t priority{get_next_random_uint32_t()};
};

template <typename T, typename Tag>
struct tree_node : node_base {
    T value;

    tree_node() = default;
    tree_node(const T& value_) : value(value_) {}
    tree_node(T&& value_) noexcept : value(std::move(value_)) {}
    ~tree_node() = default;
};

template <typename T, typename Tag>
node_base& to_base(tree_node<T, Tag>& x) {
    return static_cast<node_base&>(x);
}

template <typename T, typename Tag>
tree_node<T, Tag>& base_to_tree_node(node_base& x) {
    return static_cast<tree_node<T, Tag>&>(x);
}

template <typename T, typename Tag>
T& get_value(node_base* x) {
    return base_to_tree_node<T, Tag>(*x).value;
}

template <typename T, typename Comparator,
          typename Tag>
struct tree : Comparator {

    using cmp_t = Comparator;
    using node_t = tree_node<T, Tag>;

    tree(cmp_t&& cmp_) noexcept : Comparator(std::move(cmp_)) {};
    tree(const cmp_t& cmp_) : Comparator(cmp_) {};

    tree(const tree&) = delete;
    tree& operator=(tree const&) = delete;
    tree(tree&& other) noexcept : Comparator(std::move(other.cmp())),
          sentinel(std::move(other.sentinel)) {
        if (sentinel.left != nullptr) {
            sentinel.left->parent = &sentinel;
        }
    }

    void swap(tree& other) {
        swap_sentinel(sentinel, other.sentinel);
        std::swap(cmp(), other.cmp());
    }

    node_base* root() const noexcept {
        return sentinel.left;
    }

    node_base* insert(node_t* new_node) noexcept {
        if (root() == nullptr) {
            set_root(&to_base<T, Tag>(*new_node));
            return new_node;
        }
        std::pair<node_base*, node_base*> splited =
            split(root(), get_value<T, Tag>(new_node), true);
        splited.second = merge(new_node, splited.second);
        set_root(merge(splited.first, splited.second));
        return new_node;
    }

    node_base* find(const T& value) const noexcept {
        return find(root(), value);
    }

    node_base* const begin() const noexcept {
        return get_leftmost(get_sentinel());
    }

    node_base* const end() const noexcept {
        return get_sentinel();
    }

    node_base* lower_bound(const T& value) const noexcept {
        node_base* found = search(root(), value, true);
        if (equal(get_value<T, Tag>(found), value)) {
            return found;
        } else if (less(get_value<T, Tag>(found), value)) {
            return get_next(found);
        } else {
            return found;
        }
    }

    node_base* upper_bound(const T& value) const noexcept {
        node_base* found = search(root(), value, false);
        if (equal(get_value<T, Tag>(found), value)) {
            return get_next(found);
        } else if (less(get_value<T, Tag>(found), value)) {
            return get_next(found);
        } else {
            return found;
        }
    }

    template <typename Left, typename Right, typename CompareLeft,
              typename CompareRight>
    friend struct ::bimap;

  private:
    node_base sentinel;

    cmp_t& cmp() noexcept {
        return static_cast<cmp_t&>(*this);
    }

    cmp_t const& cmp() const noexcept {
        return static_cast<cmp_t const&>(*this);
    }

    bool greater_or_equal(const T& x, const T& y) const noexcept {
        return !cmp()(x, y);
    }

    bool less_or_equal(const T& x, const T& y) const noexcept {
        return greater_or_equal(y, x);
    }

    bool less(const T& x, const T& y) const noexcept {
        return cmp()(x, y);
    }

    bool greater(const T& x, const T& y) const noexcept {
        return less(y, x);
    }

    bool equal(const T& x, const T& y) const noexcept {
        return less_or_equal(x, y) && less_or_equal(y, x);
    }

    std::pair<node_base*, node_base*> split(node_base* v, const T& value,
                                            bool inclusive) noexcept {
        if (v == nullptr) {
            return {nullptr, nullptr};
        }
        bool go_left = (inclusive ? less_or_equal(value, get_value<T, Tag>(v))
                                  : less(value, get_value<T, Tag>(v)));
        if (go_left) {
            std::pair<node_base*, node_base*> splited =
                split(v->left, value, inclusive);

            v->left = splited.second;
            set_parent(splited.second, v);
            return {splited.first, v};
        } else {
            std::pair<node_base*, node_base*> splited =
                split(v->right, value, inclusive);

            v->right = splited.first;
            set_parent(splited.first, v);
            return {v, splited.second};
        }
    }

    node_base* merge(node_base* left, node_base* right) noexcept {
        if (left == nullptr && right == nullptr) {
            return nullptr;
        }
        if (left == nullptr) {
            return right;
        }
        if (right == nullptr) {
            return left;
        }
        if (left->priority >= right->priority) {
            node_base* merged = merge(left->right, right);
            left->right = merged;
            set_parent(merged, left);
            return left;
        } else {
            node_base* merged = merge(left, right->left);
            right->left = merged;
            set_parent(merged, right);
            return right;
        }
    }

    node_base* find(node_base* v, const T& value) const noexcept {
        if (v == nullptr) {
            return end();
        }
        if (less(value, get_value<T, Tag>(v))) {
            return find(v->left, value);
        } else if (greater(value, get_value<T, Tag>(v))) {
            return find(v->right, value);
        } else {
            return v;
        }
    }

    node_base* search(node_base* v, const T& value,
                      bool inclusive) const noexcept {
        if (v == nullptr) {
            return nullptr;
        } else if (equal(value, get_value<T, Tag>(v))) {
            return v;
        } else if ((inclusive && less_or_equal(value, get_value<T, Tag>(v))) ||
                   ((!inclusive && less(value, get_value<T, Tag>(v))))) {
            node_base* found = search(v->left, value, inclusive);
            if (found == nullptr) {
                return v;
            }
            return found;
        } else {
            node_base* found = search(v->right, value, inclusive);
            if (found == nullptr) {
                return v;
            }
            return found;
        }
    }

    void erase_helper(node_base* v) noexcept {
        node_base* left = v->left;
        node_base* right = v->right;
        node_base* new_son = merge(left, right);
        if (new_son != nullptr) {
            new_son->parent = v->parent;
        }
        if (is_left_son(v->parent, v)) {
            v->parent->left = new_son;
        } else if (is_right_son(v->parent, v)) {
            v->parent->right = new_son;
        }
    }

    void set_another_tree_pointer(node_base* other_tree_sentinel) noexcept {
        sentinel.right = other_tree_sentinel;
    }

    node_base* get_sentinel() const noexcept {
        return const_cast<node_base*>(&sentinel);
    }

    void set_root(node_base* new_root) noexcept {
        sentinel.left = new_root;
        set_parent(new_root, &sentinel);
    }
};
}
