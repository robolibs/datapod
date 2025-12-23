#pragma once

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/adapters/optional.hpp"
#include "datapod/sequential/array.hpp"

namespace datapod {

    template <typename T, std::size_t N, bool OverwriteOnFull = false> struct FixedQueue {
        static_assert(N > 0, "FixedQueue capacity must be > 0");

        using value_type = T;
        using size_type = std::size_t;

        FixedQueue() = default;

        static constexpr size_type capacity() noexcept { return N; }
        size_type size() const noexcept { return size_; }
        bool empty() const noexcept { return size_ == 0; }
        bool full() const noexcept { return size_ == N; }

        void clear() noexcept {
            for (auto &slot : data_) {
                slot.reset();
            }
            head_ = 0;
            size_ = 0;
        }

        value_type &front() {
            if (empty()) {
                throw std::out_of_range{"FixedQueue::front: empty"};
            }
            return data_[head_].value();
        }

        value_type const &front() const {
            if (empty()) {
                throw std::out_of_range{"FixedQueue::front: empty"};
            }
            return data_[head_].value();
        }

        value_type &back() {
            if (empty()) {
                throw std::out_of_range{"FixedQueue::back: empty"};
            }
            return data_[back_index()].value();
        }

        value_type const &back() const {
            if (empty()) {
                throw std::out_of_range{"FixedQueue::back: empty"};
            }
            return data_[back_index()].value();
        }

        bool try_push(value_type const &v) {
            if (full()) {
                if constexpr (!OverwriteOnFull) {
                    return false;
                }
                overwrite_front(v);
                return true;
            }
            data_[tail_index()] = v;
            ++size_;
            return true;
        }

        bool try_push(value_type &&v) {
            if (full()) {
                if constexpr (!OverwriteOnFull) {
                    return false;
                }
                overwrite_front(std::move(v));
                return true;
            }
            data_[tail_index()] = std::move(v);
            ++size_;
            return true;
        }

        void push(value_type const &v) {
            if (!try_push(v)) {
                throw std::out_of_range{"FixedQueue::push: full"};
            }
        }

        void push(value_type &&v) {
            if (!try_push(std::move(v))) {
                throw std::out_of_range{"FixedQueue::push: full"};
            }
        }

        template <typename... Args> bool try_emplace(Args &&...args) {
            if (full()) {
                if constexpr (!OverwriteOnFull) {
                    return false;
                }
                data_[head_].emplace(std::forward<Args>(args)...);
                head_ = inc(head_);
                return true;
            }
            data_[tail_index()].emplace(std::forward<Args>(args)...);
            ++size_;
            return true;
        }

        template <typename... Args> value_type &emplace(Args &&...args) {
            if (!try_emplace(std::forward<Args>(args)...)) {
                throw std::out_of_range{"FixedQueue::emplace: full"};
            }
            return back();
        }

        void pop() {
            if (empty()) {
                throw std::out_of_range{"FixedQueue::pop: empty"};
            }
            data_[head_].reset();
            head_ = inc(head_);
            --size_;
        }

        auto members() noexcept { return std::tie(data_, head_, size_); }
        auto members() const noexcept { return std::tie(data_, head_, size_); }

      private:
        static constexpr size_type inc(size_type i) noexcept { return (i + 1) % N; }

        size_type tail_index() const noexcept { return (head_ + size_) % N; }
        size_type back_index() const noexcept { return (head_ + size_ - 1) % N; }

        template <typename V> void overwrite_front(V &&v) {
            data_[head_] = std::forward<V>(v);
            head_ = inc(head_);
        }

        Array<Optional<value_type>, N> data_{};
        size_type head_{0};
        size_type size_{0};
    };

    template <typename T, std::size_t N> using FixedFifo = FixedQueue<T, N, false>;
    template <typename T, std::size_t N> using OverwritingFifo = FixedQueue<T, N, true>;

} // namespace datapod
