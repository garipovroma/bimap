#pragma once

#include "cartesian_tree.h"
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>

struct left_tag {};
struct right_tag {};

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {

    template <typename left_it, typename right_it, typename left_it_tag,
              typename right_it_tag>
    struct base_iterator;

    using left_t = Left;
    using right_t = Right;
    using cmp_left_t = CompareLeft;
    using cmp_right_t = CompareRight;
    using left_iterator = base_iterator<left_t, right_t, left_tag, right_tag>;
    using right_iterator = base_iterator<right_t, left_t, right_tag, left_tag>;

    struct node : details::tree_node<Left, left_tag>, details::tree_node<Right, right_tag> {
        template <typename LeftT, typename RightT>
        node(LeftT&& left, RightT&& right)
            : details::tree_node<Left, left_tag>(std::forward<LeftT>(left)),
              details::tree_node<Right, right_tag>(std::forward<RightT>(right)) {}

        template <typename X, typename Tag>
        details::tree_node<X, Tag>& to_tree_node() {
            return static_cast<details::tree_node<X, Tag>&>(*this);
        }
    };

    using node_t = node;

    template <typename X, typename Tag>
    static node_t& to_bimap_node(details::tree_node<X, Tag>& v) {
        return static_cast<node_t&>(v);
    }

    template <typename LeftT, typename RightT, typename LeftTag,
              typename RightTag>
    struct base_iterator {

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = LeftT;
        using reference = LeftT&;
        using pointer = LeftT*;
        using difference_type = std::ptrdiff_t;

        base_iterator(details::node_base* ptr_) : ptr(ptr_){};

        base_iterator() = default;

        base_iterator(const base_iterator& other) = default;

        LeftT const& operator*() const {
            return details::get_value<LeftT, LeftTag>(ptr);
        }

        LeftT const* operator->() const {
            return &**this;
        }

        base_iterator& operator++() {
            ptr = get_next(ptr);
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator copy = *this;
            ++*this;
            return copy;
        }

        base_iterator& operator--() {
            ptr = get_prev(ptr);
            return *this;
        }
        base_iterator operator--(int) {
            base_iterator copy = *this;
            --*this;
            return copy;
        }

        bool operator==(const base_iterator& other) const noexcept {
            return (ptr == other.ptr);
        }

        bool operator!=(const base_iterator& other) const noexcept {
            return (ptr != other.ptr);
        }

        using another_iterator =
            base_iterator<RightT, LeftT, RightTag, LeftTag>;

        another_iterator flip() const {
            if (ptr->parent == nullptr) {
                return another_iterator(ptr->right);
            } else {
                details::tree_node<LeftT, LeftTag>* cartesian_node =
                    &details::base_to_tree_node<LeftT, LeftTag>(*ptr);

                node_t* bimap_node =
                    &to_bimap_node<LeftT, LeftTag>(*cartesian_node);

                details::tree_node<RightT, RightTag>* casted_cartesian_node =
                    &bimap_node->template to_tree_node<RightT, RightTag>();

                details::node_base* right_ptr =
                    &details::to_base<RightT, RightTag>(*casted_cartesian_node);

                return another_iterator(right_ptr);
            }
        }

        friend struct bimap;

      private:
        details::node_base* ptr{nullptr};
    };

    void share_pointers() noexcept {
        left_tree.set_another_tree_pointer(&right_tree.sentinel);
        right_tree.set_another_tree_pointer(&left_tree.sentinel);
    }

    bimap(cmp_left_t compare_left = CompareLeft(),
          cmp_right_t compare_right = CompareRight())
        : left_tree(std::move(compare_left)),
          right_tree(std::move(compare_right)) {
        share_pointers();
    }

    bimap(bimap const& other)
        : left_tree(other.left_tree.cmp()),
          right_tree(other.right_tree.cmp()) {
        share_pointers();
        auto left_it = other.begin_left();
        while (left_it != other.end_left()) {
            insert(*left_it, *left_it.flip());
            left_it++;
        }
    }

    bimap(bimap&& other) noexcept :
          left_tree(std::move(other.left_tree)),
          right_tree(std::move(other.right_tree)) {
        share_pointers();
    }

    bimap& operator=(bimap const& other) {
        if (&other == this) {
            return *this;
        }
        bimap(other).swap(*this);
        return *this;
    }

    bimap& operator=(bimap&& other) noexcept {
        if (&other == this) {
            return *this;
        }
        bimap(std::move(other)).swap(*this);
        return *this;
    }

    void swap(bimap& other) noexcept {
        left_tree.swap(other.left_tree);
        right_tree.swap(other.right_tree);
        std::swap(sz, other.sz);
        share_pointers();
        other.share_pointers();
    }

    ~bimap() {
        left_iterator it = begin_left();
        while (it != end_left()) {
            erase_left(it++);
        }
    }

    template <typename X = left_t, typename Y = right_t>
    left_iterator insert(X&& left, Y&& right) noexcept {
        if (left_tree.find(left) != left_tree.end()) {
            return end_left();
        }
        if (right_tree.find(right) != right_tree.end()) {
            return end_left();
        }
        node_t* new_node =
            new node_t(std::forward<X>(left), std::forward<Y>(right));
        details::node_base* result = left_tree.insert(
            &new_node->template to_tree_node<left_t, left_tag>());
        right_tree.insert(
            &new_node->template to_tree_node<right_t, right_tag>());
        sz++;
        return left_iterator(result);
    }

    left_iterator erase_left(left_iterator it) noexcept {
        if (it == end_left()) {
            return left_iterator(nullptr);
        }
        right_iterator right_it = it.flip();
        left_iterator res = it;
        res++;
        left_tree.erase_helper(it.ptr);
        right_tree.erase_helper(right_it.ptr);
        node_t* casted_e = &to_bimap_node<left_t, left_tag>(
            details::base_to_tree_node<left_t, left_tag>(*it.ptr));
        delete casted_e;
        sz--;
        return res;
    }

    bool erase_left(left_t const& left) noexcept {
        details::node_base* found = left_tree.find(left);
        return erase_left(left_iterator(found)) != left_iterator(nullptr);
    }

    right_iterator erase_right(right_iterator it) noexcept {
        if (it == end_right()) {
            return right_iterator(nullptr);
        }
        return erase_left(it.flip()).flip();
    }

    bool erase_right(right_t const& right) noexcept {
        details::node_base* found = right_tree.find(right);
        return erase_right(right_iterator(found)) != right_iterator(nullptr);
    }

    left_iterator erase_left(left_iterator first, left_iterator last) noexcept {
        while (first != last) {
            erase_left(first++);
        }
        return last;
    }

    right_iterator erase_right(right_iterator first,
                               right_iterator last) noexcept {
        while (first != last) {
            erase_right(first++);
        }
        return last;
    }

    left_iterator find_left(left_t const& left) const noexcept {
        return left_iterator(left_tree.find(left));
    }
    right_iterator find_right(right_t const& right) const noexcept {
        return right_iterator(right_tree.find(right));
    }

    right_t const& at_left(left_t const& key) const {
        details::node_base* found_node = left_tree.find(key);
        if (found_node == left_tree.end()) {
            throw std::out_of_range("there is no such value in bimap");
        }
        node_t* bimap_node = &to_bimap_node<left_t, left_tag>(
            details::base_to_tree_node<left_t, left_tag>(*found_node));
        return bimap_node->template to_tree_node<right_t, right_tag>().value;
    }

    left_t const& at_right(right_t const& key) const {
        details::node_base* found_node = right_tree.find(key);
        if (found_node == right_tree.end()) {
            throw std::out_of_range("there is no such value in bimap");
        }
        node_t* bimap_node = &to_bimap_node<right_t, right_tag>(
            details::base_to_tree_node<right_t, right_tag>(*found_node));
        return bimap_node->template to_tree_node<left_t, left_tag>().value;
    }

    template <
        typename = std::enable_if<std::is_default_constructible_v<right_t>>>
    right_t const& at_left_or_default(left_t const& key) {
        if (left_tree.find(key) == left_tree.end()) {
            right_t default_value{};
            details::node_base* found = right_tree.find(default_value);
            if (found != right_tree.end()) {
                left_iterator it = right_iterator(found).flip();
                erase_left(it);
                return *insert(key, std::move(default_value)).flip();
            }
            return *insert(key, std::move(default_value)).flip();
        } else {
            return at_left(key);
        }
    }
    template <
        typename = std::enable_if<std::is_default_constructible_v<left_t>>>
    left_t const& at_right_or_default(right_t const& key) {
        if (right_tree.find(key) == right_tree.end()) {
            left_t default_value{};
            details::node_base* found = left_tree.find(default_value);
            if (found != right_tree.end()) {
                right_iterator it = left_iterator(found).flip();
                erase_right(it);
                return *insert(std::move(default_value), key);
            }
            return *insert(std::move(default_value), key);
        } else {
            return at_right(key);
        }
    }

    left_iterator lower_bound_left(const left_t& left) const noexcept {
        details::node_base* found = left_tree.lower_bound(left);
        return left_iterator(found);
    }

    left_iterator upper_bound_left(const left_t& left) const noexcept {
        details::node_base* found = left_tree.upper_bound(left);
        return left_iterator(found);
    }

    right_iterator lower_bound_right(const right_t& right) const noexcept {
        details::node_base* found = right_tree.lower_bound(right);
        return right_iterator(found);
    }

    right_iterator upper_bound_right(const right_t& right) const noexcept {
        details::node_base* found = right_tree.upper_bound(right);
        return right_iterator(found);
    }

    left_iterator begin_left() const noexcept {
        return left_iterator(left_tree.begin());
    }
    left_iterator end_left() const noexcept {
        return left_iterator(left_tree.end());
    }

    right_iterator begin_right() const noexcept {
        return right_iterator(right_tree.begin());
    }
    right_iterator end_right() const noexcept {
        return right_iterator(right_tree.end());
    }

    bool empty() const noexcept {
        return (size() == 0);
    }

    std::size_t size() const noexcept {
        return sz;
    }

    bool operator==(bimap const& b) const noexcept {
        if (this == &b) {
            return true;
        }
        if (size() != b.size()) {
            return false;
        }
        left_iterator a_it = begin_left();
        left_iterator b_it = b.begin_left();
        while (a_it != end_left()) {
            if (!left_tree.equal(*a_it, *b_it)) {
                return false;
            }
            if (!right_tree.equal(*a_it.flip(), *b_it.flip())) {
                return false;
            }
            a_it++;
            b_it++;
        }
        return true;
    }

    friend bool operator!=(bimap const& a, bimap const& b) {
        return !(a == b);
    }

  private:
    details::tree<left_t, cmp_left_t, left_tag> left_tree;
    details::tree<right_t, cmp_right_t, right_tag> right_tree;
    size_t sz{0};
};
