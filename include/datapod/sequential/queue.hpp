#pragma once

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/sequential/vector.hpp"

namespace datapod {

    template <typename T, typename Container = Vector<T>> class Queue {
      public:
        using value_type = T;
        using size_type = std::size_t;
        using container_type = Container;

        Queue() = default;

        bool empty() const noexcept { return in_.empty() && out_.empty(); }
        size_type size() const noexcept { return in_.size() + out_.size(); }

        void clear() noexcept {
            in_.clear();
            out_.clear();
        }

        void push(value_type const &v) { in_.push_back(v); }
        void push(value_type &&v) { in_.push_back(std::move(v)); }

        template <typename... Args> value_type &emplace(Args &&...args) {
            in_.emplace_back(std::forward<Args>(args)...);
            return in_.back();
        }

        value_type &front() {
            ensure_out_for_front();
            return out_.back();
        }

        value_type const &front() const {
            ensure_out_for_front();
            return out_.back();
        }

        value_type &back() {
            if (!in_.empty()) {
                return in_.back();
            }
            if (out_.empty()) {
                throw std::out_of_range{"Queue::back: empty"};
            }
            return out_.front();
        }

        value_type const &back() const {
            if (!in_.empty()) {
                return in_.back();
            }
            if (out_.empty()) {
                throw std::out_of_range{"Queue::back: empty"};
            }
            return out_.front();
        }

        void pop() {
            ensure_out_for_front();
            out_.pop_back();
        }

        void reserve(size_type n) { in_.reserve(n); }

        auto members() noexcept { return std::tie(in_, out_); }
        auto members() const noexcept { return std::tie(in_, out_); }

      private:
        void transfer_in_to_out() {
            while (!in_.empty()) {
                out_.push_back(std::move(in_.back()));
                in_.pop_back();
            }
        }

        void ensure_out_for_front() {
            if (out_.empty()) {
                transfer_in_to_out();
            }
            if (out_.empty()) {
                throw std::out_of_range{"Queue::front: empty"};
            }
        }

        void ensure_out_for_front() const { const_cast<Queue *>(this)->ensure_out_for_front(); }

        container_type in_{};
        container_type out_{};
    };

    template <typename T, typename Container = Vector<T>> using Fifo = Queue<T, Container>;

} // namespace datapod
