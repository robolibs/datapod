#pragma once
#include <datapod/types/types.hpp>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cinttypes>
#include <iosfwd>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "datapod/core/atomic.hpp"
#include "datapod/core/bit_counting.hpp"
#include "datapod/core/strong.hpp"
#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    template <typename Vec, typename Key = typename Vec::size_type> struct BasicBitvec {
        using block_t = typename Vec::value_type;
        using size_type = typename Vec::size_type;
        static constexpr auto const bits_per_block = static_cast<size_type>(sizeof(block_t) * 8);

        constexpr BasicBitvec() noexcept {}
        BasicBitvec(std::string_view s) { set(s); }
        BasicBitvec(size_type const size) { resize(size); }
        constexpr BasicBitvec(Vec &&v) : size_{v.size() * bits_per_block}, blocks_{std::move(v)} {}
        constexpr BasicBitvec(Vec &&v, size_type const size) : size_{size}, blocks_{std::move(v)} {}

        static constexpr BasicBitvec max(datapod::usize const size) {
            BasicBitvec ret;
            ret.resize(size);
            for (auto &b : ret.blocks_) {
                b = std::numeric_limits<block_t>::max();
            }
            return ret;
        }

        auto members() noexcept { return std::tie(size_, blocks_); }

        static constexpr size_type num_blocks(size_type num_bits) {
            return static_cast<size_type>(num_bits / bits_per_block + (num_bits % bits_per_block == 0 ? 0 : 1));
        }

        void zero_out() {
            for (auto &b : blocks_) {
                b = 0U;
            }
        }

        void one_out() {
            for (auto &b : blocks_) {
                b = ~block_t{0};
            }
        }

        void resize(size_type const new_size) {
            if (new_size == size_) {
                return;
            }

            if (!empty() && (size_ % bits_per_block) != 0U) {
                blocks_[blocks_.size() - 1] &= ~((~block_t{0}) << (size_ % bits_per_block));
            }
            blocks_.resize(num_blocks(new_size));
            size_ = new_size;
        }

        void set(std::string_view s) {
            assert(std::all_of(begin(s), end(s), [](char const c) { return c == '0' || c == '1'; }));
            resize(s.size());
            auto const max_size = std::min(static_cast<datapod::usize>(size_), static_cast<datapod::usize>(s.size()));
            for (auto i = datapod::usize{0U}; i != max_size; ++i) {
                set(i, s[s.size() - i - 1] != '0');
            }
        }

        constexpr void set(Key const i, bool const val = true) noexcept {
            assert(i < size_);
            assert((to_idx(i) / bits_per_block) < blocks_.size());
            auto &block = blocks_[static_cast<size_type>(to_idx(i)) / bits_per_block];
            auto const bit = to_idx(i) % bits_per_block;
            if (val) {
                block |= (block_t{1U} << bit);
            } else {
                block &= (~block_t{0U} ^ (block_t{1U} << bit));
            }
        }

        template <bool IsAtomic = false> void set_atomic(Key const i, bool const val = true) noexcept {
            assert(i < size_);
            assert((to_idx(i) / bits_per_block) < blocks_.size());

            auto const bit = to_idx(i) % bits_per_block;
            auto &block = blocks_[static_cast<size_type>(to_idx(i)) / bits_per_block];
            if constexpr (IsAtomic) {
                if (val) {
                    fetch_or(block, block_t{1U} << bit);
                } else {
                    fetch_and(block, (~block_t{0U} ^ (block_t{1U} << bit)));
                }
            } else {
                if (val) {
                    block |= (block_t{1U} << bit);
                } else {
                    block &= (~block_t{0U} ^ (block_t{1U} << bit));
                }
            }
        }

        void reset() noexcept {
            size_ = 0;
            blocks_ = {};
        }

        bool operator[](Key const i) const noexcept { return test(i); }

        datapod::usize count() const noexcept {
            if (empty()) {
                return 0;
            }
            auto sum = datapod::usize{0U};
            for (auto i = size_type{0U}; i != blocks_.size() - 1; ++i) {
                sum += popcount(blocks_[i]);
            }
            return sum + popcount(sanitized_last_block());
        }

        constexpr bool test(Key const i) const noexcept {
            if (i >= size_) {
                return false;
            }
            assert((i / bits_per_block) < blocks_.size());
            auto const block = blocks_[static_cast<size_type>(to_idx(i)) / bits_per_block];
            auto const bit = (to_idx(i) % bits_per_block);
            return (block & (block_t{1U} << bit)) != 0U;
        }

        template <typename Fn> void for_each_set_bit(Fn &&f) const {
            if (empty()) {
                return;
            }
            auto const check_block = [&](size_type const i, block_t const block) {
                if (block != 0U) {
                    for (auto bit = size_type{0U}; bit != bits_per_block; ++bit) {
                        if ((block & (block_t{1U} << bit)) != 0U) {
                            f(Key{i * bits_per_block + bit});
                        }
                    }
                }
            };
            for (auto i = size_type{0U}; i != blocks_.size() - 1; ++i) {
                check_block(i, blocks_[i]);
            }
            check_block(blocks_.size() - 1, sanitized_last_block());
        }

        std::optional<Key> next_set_bit(size_type const i) const {
            if (i >= size()) {
                return std::nullopt;
            }

            auto const first_block_idx = i / bits_per_block;
            auto const first_block =
                first_block_idx == blocks_.size() - 1 ? sanitized_last_block() : blocks_[first_block_idx];
            if (first_block != 0U) {
                auto const first_bit = i % bits_per_block;
                auto const n = std::min(size(), bits_per_block);
                for (auto bit = first_bit; bit != n; ++bit) {
                    if ((first_block & (block_t{1U} << bit)) != 0U) {
                        return Key{first_block_idx * bits_per_block + bit};
                    }
                }
            }

            if (first_block_idx + 1U == blocks_.size()) {
                return std::nullopt;
            }

            auto const check_block = [&](size_type const block_idx, block_t const block) -> std::optional<Key> {
                if (block != 0U) {
                    for (auto bit = size_type{0U}; bit != bits_per_block; ++bit) {
                        if ((block & (block_t{1U} << bit)) != 0U) {
                            return Key{block_idx * bits_per_block + bit};
                        }
                    }
                }
                return std::nullopt;
            };

            for (auto block_idx = first_block_idx + 1U; block_idx != blocks_.size() - 1; ++block_idx) {
                if (auto const set_bit_idx = check_block(block_idx, blocks_[block_idx]); set_bit_idx.has_value()) {
                    return set_bit_idx;
                }
            }

            if (auto const set_bit_idx = check_block(blocks_.size() - 1, sanitized_last_block());
                set_bit_idx.has_value()) {
                return set_bit_idx;
            }

            return std::nullopt;
        }

        std::optional<Key> get_next(std::atomic_size_t &next) const {
            while (true) {
                auto expected = next.load();
                auto idx = next_set_bit(Key{static_cast<base_t<Key>>(expected)});
                if (!idx.has_value()) {
                    return std::nullopt;
                }
                if (next.compare_exchange_weak(expected, *idx + 1U)) {
                    return idx;
                }
            }
        }

        size_type size() const noexcept { return size_; }
        bool empty() const noexcept { return size() == 0U; }

        bool any() const noexcept {
            if (empty()) {
                return false;
            }
            for (auto i = size_type{0U}; i != blocks_.size() - 1; ++i) {
                if (blocks_[i] != 0U) {
                    return true;
                }
            }
            return sanitized_last_block() != 0U;
        }

        bool none() const noexcept { return !any(); }

        // Modifiers
        void push_back(bool value) {
            resize(size_ + 1);
            set(size_ - 1, value);
        }

        void pop_back() {
            if (!empty()) {
                resize(size_ - 1);
            }
        }

        void reserve(size_type new_capacity) { blocks_.reserve(num_blocks(new_capacity)); }

        size_type capacity() const noexcept { return blocks_.capacity() * bits_per_block; }

        void clear() noexcept {
            size_ = 0;
            blocks_.clear();
        }

        void flip(Key const i) noexcept {
            assert(i < size_);
            auto &block = blocks_[static_cast<size_type>(to_idx(i)) / bits_per_block];
            auto const bit = to_idx(i) % bits_per_block;
            block ^= (block_t{1U} << bit);
        }

        void flip() noexcept {
            for (auto &b : blocks_) {
                b = ~b;
            }
        }

        block_t sanitized_last_block() const noexcept {
            if ((size_ % bits_per_block) != 0) {
                return blocks_[blocks_.size() - 1] & ~((~block_t{0}) << (size_ % bits_per_block));
            } else {
                return blocks_[blocks_.size() - 1];
            }
        }

        std::string str() const {
            auto s = std::string{};
            s.resize(size_);
            for (auto i = 0U; i != size_; ++i) {
                s[i] = test(size_ - i - 1) ? '1' : '0';
            }
            return s;
        }

        friend bool operator==(BasicBitvec const &a, BasicBitvec const &b) noexcept {
            if (a.size() != b.size()) {
                return false;
            }

            if (a.empty() && b.empty()) {
                return true;
            }

            for (auto i = size_type{0U}; i != a.blocks_.size() - 1; ++i) {
                if (a.blocks_[i] != b.blocks_[i]) {
                    return false;
                }
            }
            return a.sanitized_last_block() == b.sanitized_last_block();
        }

        friend bool operator!=(BasicBitvec const &a, BasicBitvec const &b) noexcept { return !(a == b); }

        BasicBitvec &operator&=(BasicBitvec const &o) noexcept {
            assert(size() == o.size());
            for (auto i = 0U; i < blocks_.size(); ++i) {
                blocks_[i] &= o.blocks_[i];
            }
            return *this;
        }

        BasicBitvec &operator|=(BasicBitvec const &o) noexcept {
            assert(size() == o.size());
            for (auto i = 0U; i < blocks_.size(); ++i) {
                blocks_[i] |= o.blocks_[i];
            }
            return *this;
        }

        BasicBitvec &operator^=(BasicBitvec const &o) noexcept {
            assert(size() == o.size());
            for (auto i = 0U; i < blocks_.size(); ++i) {
                blocks_[i] ^= o.blocks_[i];
            }
            return *this;
        }

        BasicBitvec operator~() const noexcept {
            auto copy = *this;
            for (auto &b : copy.blocks_) {
                b = ~b;
            }
            return copy;
        }

      private:
        template <typename T> static constexpr auto to_idx(T const &t) {
            if constexpr (is_strong_v<T>) {
                return t.v_;
            } else {
                return t;
            }
        }

        size_type size_{0U};
        Vec blocks_{};
    };

    using Bitvec = BasicBitvec<Vector<datapod::u64>>;

    namespace bitvec {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace bitvec

} // namespace datapod
