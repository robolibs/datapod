#pragma once

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/sequential/vector.hpp"

namespace datapod {

    template <typename T, typename Container = Vector<T>> struct Stack {
        using value_type = T;
        using size_type = std::size_t;
        using container_type = Container;

        Stack() = default;

        bool empty() const noexcept { return c_.empty(); }
        size_type size() const noexcept { return c_.size(); }

        value_type &top() {
            if (empty()) {
                throw std::out_of_range{"Stack::top: empty"};
            }
            return c_.back();
        }

        value_type const &top() const {
            if (empty()) {
                throw std::out_of_range{"Stack::top: empty"};
            }
            return c_.back();
        }

        void push(value_type const &v) { c_.push_back(v); }
        void push(value_type &&v) { c_.push_back(std::move(v)); }

        template <typename... Args> value_type &emplace(Args &&...args) {
            c_.emplace_back(std::forward<Args>(args)...);
            return c_.back();
        }

        void pop() {
            if (empty()) {
                throw std::out_of_range{"Stack::pop: empty"};
            }
            c_.pop_back();
        }

        void clear() noexcept { c_.clear(); }

        auto members() noexcept { return std::tie(c_); }
        auto members() const noexcept { return std::tie(c_); }

      private:
        container_type c_{};
    };

    template <typename T, typename Container = Vector<T>> using Lifo = Stack<T, Container>;

} // namespace datapod
