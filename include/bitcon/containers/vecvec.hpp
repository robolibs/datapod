#pragma once

#include <cassert>
#include <iterator>
#include <type_traits>

#include "bitcon/containers/vector.hpp"
#include "bitcon/core/char_traits.hpp"
#include "bitcon/core/const_iterator.hpp"
#include "bitcon/core/verify.hpp"

namespace bitcon {

    template <typename Key, typename DataVec, typename IndexVec> struct BasicVecvec {
        using key = Key;
        using data_value_type = typename DataVec::value_type;
        using index_value_type = typename IndexVec::value_type;

        struct Bucket final {
            using value_type = data_value_type;
            using iterator = typename DataVec::iterator;
            using const_iterator = typename DataVec::const_iterator;

            using iterator_category = std::random_access_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using pointer = std::add_pointer_t<value_type>;
            using reference = Bucket;

            Bucket(BasicVecvec *map, index_value_type const i) : map_{map}, i_{to_idx(i)} {}

            friend data_value_type *data(Bucket b) { return b.data(); }
            friend index_value_type size(Bucket b) { return b.size(); }

            data_value_type *data() { return empty() ? nullptr : &(*this)[0]; }

            data_value_type const *data() const { return empty() ? nullptr : &(*this)[0]; }

            template <typename T = std::decay_t<data_value_type>,
                      typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
            std::basic_string_view<T, CharTraits<T>> view() const {
                return {data(), size()};
            }

            value_type &front() {
                assert(!empty());
                return operator[](0);
            }

            value_type &back() {
                assert(!empty());
                return operator[](size() - 1U);
            }

            value_type const &front() const {
                assert(!empty());
                return operator[](0);
            }

            value_type const &back() const {
                assert(!empty());
                return operator[](size() - 1U);
            }

            bool empty() const { return begin() == end(); }

            value_type &operator[](std::size_t const i) {
                assert(is_inside_bucket(i));
                return map_->data_[to_idx(map_->bucket_starts_[i_] + i)];
            }

            value_type const &operator[](std::size_t const i) const {
                assert(is_inside_bucket(i));
                return map_->data_[to_idx(map_->bucket_starts_[i_] + i)];
            }

            value_type const &at(std::size_t const i) const {
                verify(i < size(), "Bucket::at: index out of range");
                return *(begin() + i);
            }

            value_type &at(std::size_t const i) {
                verify(i < size(), "Bucket::at: index out of range");
                return *(begin() + i);
            }

            std::size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }
            iterator begin() { return map_->data_.begin() + bucket_begin_idx(); }
            iterator end() { return map_->data_.begin() + bucket_end_idx(); }
            const_iterator begin() const { return map_->data_.begin() + bucket_begin_idx(); }
            const_iterator end() const { return map_->data_.begin() + bucket_end_idx(); }
            friend iterator begin(Bucket const &b) { return const_cast<Bucket &>(b).begin(); }
            friend iterator end(Bucket const &b) { return const_cast<Bucket &>(b).end(); }
            friend iterator begin(Bucket &b) { return b.begin(); }
            friend iterator end(Bucket &b) { return b.end(); }

            friend bool operator==(Bucket const &a, Bucket const &b) {
                assert(a.map_ == b.map_);
                return a.i_ == b.i_;
            }
            friend bool operator!=(Bucket const &a, Bucket const &b) {
                assert(a.map_ == b.map_);
                return a.i_ != b.i_;
            }
            Bucket &operator++() {
                ++i_;
                return *this;
            }
            Bucket &operator--() {
                --i_;
                return *this;
            }
            Bucket operator*() const { return *this; }
            Bucket &operator+=(difference_type const n) {
                i_ += n;
                return *this;
            }
            Bucket &operator-=(difference_type const n) {
                i_ -= n;
                return *this;
            }
            Bucket operator+(difference_type const n) const {
                auto tmp = *this;
                tmp += n;
                return tmp;
            }
            Bucket operator-(difference_type const n) const {
                auto tmp = *this;
                tmp -= n;
                return tmp;
            }
            friend difference_type operator-(Bucket const &a, Bucket const &b) {
                assert(a.map_ == b.map_);
                return a.i_ - b.i_;
            }

          private:
            index_value_type bucket_begin_idx() const {
                return map_->empty() ? index_value_type{} : to_idx(map_->bucket_starts_[i_]);
            }
            index_value_type bucket_end_idx() const {
                return map_->empty() ? index_value_type{} : to_idx(map_->bucket_starts_[i_ + 1U]);
            }
            bool is_inside_bucket(std::size_t const i) const { return bucket_begin_idx() + i < bucket_end_idx(); }

            BasicVecvec *map_;
            index_value_type i_;
        };

        struct ConstBucket final {
            using value_type = data_value_type;
            using iterator = const_iterator_t<DataVec>;
            using const_iterator = iterator;

            using iterator_category = std::random_access_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using pointer = std::add_pointer_t<value_type>;
            using reference = std::add_lvalue_reference<value_type>;

            ConstBucket(BasicVecvec const *map, index_value_type const i) : map_{map}, i_{to_idx(i)} {}

            friend data_value_type const *data(ConstBucket b) { return b.data(); }
            friend index_value_type size(ConstBucket b) { return b.size(); }

            data_value_type const *data() const { return empty() ? nullptr : &front(); }

            template <typename T = std::decay_t<data_value_type>,
                      typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
            std::basic_string_view<T, CharTraits<T>> view() const {
                return {begin(), size()};
            }

            value_type const &front() const {
                assert(!empty());
                return operator[](0);
            }

            value_type const &back() const {
                assert(!empty());
                return operator[](size() - 1U);
            }

            bool empty() const { return begin() == end(); }

            value_type const &at(std::size_t const i) const {
                verify(i < size(), "Bucket::at: index out of range");
                return *(begin() + i);
            }

            value_type const &operator[](std::size_t const i) const {
                assert(is_inside_bucket(i));
                return map_->data_[map_->bucket_starts_[i_] + i];
            }

            index_value_type size() const { return bucket_end_idx() - bucket_begin_idx(); }
            const_iterator begin() const { return map_->data_.begin() + bucket_begin_idx(); }
            const_iterator end() const { return map_->data_.begin() + bucket_end_idx(); }
            friend const_iterator begin(ConstBucket const &b) { return b.begin(); }
            friend const_iterator end(ConstBucket const &b) { return b.end(); }

            std::reverse_iterator<const_iterator> rbegin() const { return std::reverse_iterator{begin() + size()}; }
            std::reverse_iterator<const_iterator> rend() const { return std::reverse_iterator{begin()}; }

            friend bool operator==(ConstBucket const &a, ConstBucket const &b) {
                assert(a.map_ == b.map_);
                return a.i_ == b.i_;
            }
            friend bool operator!=(ConstBucket const &a, ConstBucket const &b) {
                assert(a.map_ == b.map_);
                return a.i_ != b.i_;
            }
            ConstBucket &operator++() {
                ++i_;
                return *this;
            }
            ConstBucket &operator--() {
                --i_;
                return *this;
            }
            ConstBucket operator*() const { return *this; }
            ConstBucket &operator+=(difference_type const n) {
                i_ += n;
                return *this;
            }
            ConstBucket &operator-=(difference_type const n) {
                i_ -= n;
                return *this;
            }
            ConstBucket operator+(difference_type const n) const {
                auto tmp = *this;
                tmp += n;
                return tmp;
            }
            ConstBucket operator-(difference_type const n) const {
                auto tmp = *this;
                tmp -= n;
                return tmp;
            }
            friend difference_type operator-(ConstBucket const &a, ConstBucket const &b) {
                assert(a.map_ == b.map_);
                return a.i_ - b.i_;
            }

          private:
            std::size_t bucket_begin_idx() const { return to_idx(map_->bucket_starts_[i_]); }
            std::size_t bucket_end_idx() const { return to_idx(map_->bucket_starts_[i_ + 1]); }
            bool is_inside_bucket(std::size_t const i) const { return bucket_begin_idx() + i < bucket_end_idx(); }

            std::size_t i_;
            BasicVecvec const *map_;
        };

        using value_type = Bucket;
        using iterator = Bucket;
        using const_iterator = ConstBucket;

        Bucket operator[](Key const i) { return Bucket{this, to_idx(i)}; }
        ConstBucket operator[](Key const i) const { return ConstBucket{this, to_idx(i)}; }

        ConstBucket at(Key const i) const {
            verify(to_idx(i) < bucket_starts_.size() - 1, "BasicVecvec::at: index out of range");
            return {this, to_idx(i)};
        }

        Bucket at(Key const i) {
            verify(to_idx(i) < bucket_starts_.size() - 1, "BasicVecvec::at: index out of range");
            return {this, to_idx(i)};
        }

        Bucket front() { return at(Key{0}); }
        Bucket back() { return at(Key{size() - 1}); }

        ConstBucket front() const { return at(Key{0}); }
        ConstBucket back() const { return at(Key{size() - 1}); }

        index_value_type size() const { return empty() ? 0U : bucket_starts_.size() - 1; }
        bool empty() const { return bucket_starts_.empty(); }

        void clear() {
            bucket_starts_.clear();
            data_.clear();
        }

        template <typename Container, typename = std::enable_if_t<std::is_convertible_v<
                                          decltype(*std::declval<Container>().begin()), data_value_type>>>
        void emplace_back(Container &&bucket) {
            if (bucket_starts_.empty()) {
                bucket_starts_.push_back(index_value_type{0U});
            }
            for (auto &&elem : bucket) {
                data_.push_back(std::forward<decltype(elem)>(elem));
            }
            bucket_starts_.push_back(static_cast<index_value_type>(data_.size()));
        }

        Bucket add_back_sized(std::size_t const bucket_size) {
            if (bucket_starts_.empty()) {
                bucket_starts_.push_back(index_value_type{0U});
            }
            data_.resize(data_.size() + bucket_size);
            bucket_starts_.push_back(static_cast<index_value_type>(data_.size()));
            return at(Key{size() - 1U});
        }

        template <typename X>
        std::enable_if_t<std::is_convertible_v<std::decay_t<X>, data_value_type>>
        emplace_back(std::initializer_list<X> &&x) {
            if (bucket_starts_.empty()) {
                bucket_starts_.push_back(index_value_type{0U});
            }
            for (auto &&elem : x) {
                data_.push_back(std::forward<decltype(elem)>(elem));
            }
            bucket_starts_.push_back(static_cast<index_value_type>(data_.size()));
        }

        template <typename T = data_value_type, typename = std::enable_if_t<std::is_convertible_v<T, char const>>>
        void emplace_back(char const *s) {
            return emplace_back(std::string_view{s});
        }

        void resize(std::size_t const new_size) {
            auto const old_size = size();
            if (new_size < old_size) {
                // Shrink: remove buckets from the end
                bucket_starts_.resize(new_size + 1);
                data_.resize(bucket_starts_.back());
            } else if (new_size > old_size) {
                // Grow: add empty buckets
                if (bucket_starts_.empty()) {
                    bucket_starts_.push_back(index_value_type{0U});
                }
                auto const current_data_size = data_.size();
                for (std::size_t i = old_size; i < new_size; ++i) {
                    bucket_starts_.push_back(static_cast<index_value_type>(current_data_size));
                }
            }
        }

        Bucket begin() { return Bucket{this, 0U}; }
        Bucket end() { return Bucket{this, size()}; }
        ConstBucket begin() const { return ConstBucket{this, 0U}; }
        ConstBucket end() const { return ConstBucket{this, size()}; }

        friend Bucket begin(BasicVecvec &m) { return m.begin(); }
        friend Bucket end(BasicVecvec &m) { return m.end(); }
        friend ConstBucket begin(BasicVecvec const &m) { return m.begin(); }
        friend ConstBucket end(BasicVecvec const &m) { return m.end(); }

        DataVec data_;
        IndexVec bucket_starts_;
    };

    template <typename K, typename V, typename SizeType = std::size_t>
    using Vecvec = BasicVecvec<K, Vector<V>, Vector<SizeType>>;

} // namespace bitcon
