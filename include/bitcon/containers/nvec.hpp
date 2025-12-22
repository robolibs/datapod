#pragma once

#include <cinttypes>
#include <string_view>
#include <utility>

#include "bitcon/containers/array.hpp"
#include "bitcon/containers/vector.hpp"
#include "bitcon/core/strong.hpp"
#include "bitcon/core/verify.hpp"

namespace bitcon {

    template <typename DataVec, typename IndexVec, typename SizeType> struct ConstBucketNvec final {
        using size_type = SizeType;
        using index_value_type = typename IndexVec::value_type;
        using data_value_type = typename DataVec::value_type;

        using value_type = data_value_type;
        using iterator = typename DataVec::const_iterator;
        using const_iterator = typename DataVec::const_iterator;
        using reference = typename DataVec::reference;
        using const_reference = typename DataVec::const_reference;

        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = std::add_pointer_t<value_type>;

        ConstBucketNvec() = default;

        ConstBucketNvec(DataVec const *data, IndexVec const *index, index_value_type const i)
            : data_{data}, index_{index}, i_{static_cast<size_type>(to_idx(i))} {}

        friend data_value_type *data(ConstBucketNvec b) { return &b[0]; }
        friend index_value_type size(ConstBucketNvec b) { return b.size(); }

        template <typename T = std::decay_t<data_value_type>, typename = std::enable_if_t<std::is_same_v<T, char>>>
        std::string_view view() const {
            return std::string_view{begin(), size()};
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
            verify(i < size(), "bucket::at: index out of range");
            return *(begin() + i);
        }

        value_type const &operator[](std::size_t const i) const {
            assert(is_inside_bucket(i));
            return data()[index()[i_] + i];
        }

        ConstBucketNvec operator*() const { return *this; }

        std::size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }

        const_iterator begin() const { return data().begin() + bucket_begin_idx(); }
        const_iterator end() const { return data().begin() + bucket_end_idx(); }

        friend const_iterator begin(ConstBucketNvec const &b) { return b.begin(); }
        friend const_iterator end(ConstBucketNvec const &b) { return b.end(); }

        friend bool operator==(ConstBucketNvec const &a, ConstBucketNvec const &b) {
            assert(a.data_ == b.data_);
            assert(a.index_ == b.index_);
            return a.i_ == b.i_;
        }
        friend bool operator!=(ConstBucketNvec const &a, ConstBucketNvec const &b) {
            assert(a.data_ == b.data_);
            assert(a.index_ == b.index_);
            return a.i_ != b.i_;
        }
        ConstBucketNvec &operator++() {
            ++i_;
            return *this;
        }
        ConstBucketNvec operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        ConstBucketNvec &operator--() {
            --i_;
            return *this;
        }
        ConstBucketNvec operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }
        ConstBucketNvec &operator+=(difference_type const n) {
            i_ += n;
            return *this;
        }
        ConstBucketNvec &operator-=(difference_type const n) {
            i_ -= n;
            return *this;
        }
        ConstBucketNvec operator+(difference_type const n) const {
            auto tmp = *this;
            tmp += n;
            return tmp;
        }
        ConstBucketNvec operator-(difference_type const n) const {
            auto tmp = *this;
            tmp -= n;
            return tmp;
        }
        friend difference_type operator-(ConstBucketNvec const &a, ConstBucketNvec const &b) {
            assert(a.data_ == b.data_);
            assert(a.index_ == b.index_);
            return a.i_ - b.i_;
        }

      private:
        DataVec const &data() const { return *data_; }
        IndexVec const &index() const { return *index_; }

        std::size_t bucket_begin_idx() const { return to_idx(index()[i_]); }
        std::size_t bucket_end_idx() const { return to_idx(index()[i_ + 1U]); }
        bool is_inside_bucket(std::size_t const i) const { return bucket_begin_idx() + i < bucket_end_idx(); }

        DataVec const *data_{nullptr};
        IndexVec const *index_{nullptr};
        size_type i_{};
    };

    template <typename DataVec, typename IndexVec, typename SizeType> struct BucketNvec final {
        using size_type = SizeType;
        using index_value_type = typename IndexVec::value_type;
        using data_value_type = typename DataVec::value_type;

        using value_type = data_value_type;
        using iterator = typename DataVec::iterator;
        using const_iterator = typename DataVec::const_iterator;
        using reference = typename DataVec::reference;
        using const_reference = typename DataVec::const_reference;

        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = std::add_pointer_t<value_type>;

        BucketNvec() = default;

        BucketNvec(DataVec *data, IndexVec *index, index_value_type const i)
            : data_{data}, index_{index}, i_{static_cast<size_type>(to_idx(i))} {}

        template <typename T = std::decay_t<data_value_type>, typename = std::enable_if_t<std::is_same_v<T, char>>>
        std::string_view view() const {
            return std::string_view{begin(), size()};
        }

        value_type &front() {
            assert(!empty());
            return operator[](0);
        }

        value_type const &front() const {
            assert(!empty());
            return operator[](0);
        }

        value_type &back() {
            assert(!empty());
            return operator[](size() - 1U);
        }

        value_type const &back() const {
            assert(!empty());
            return operator[](size() - 1U);
        }

        bool empty() const { return begin() == end(); }

        value_type &at(std::size_t const i) {
            verify(i < size(), "bucket::at: index out of range");
            return *(begin() + i);
        }

        value_type const &at(std::size_t const i) const {
            verify(i < size(), "bucket::at: index out of range");
            return *(begin() + i);
        }

        value_type &operator[](std::size_t const i) {
            assert(is_inside_bucket(i));
            return data()[index()[i_] + i];
        }

        value_type const &operator[](std::size_t const i) const {
            assert(is_inside_bucket(i));
            return data()[index()[i_] + i];
        }

        BucketNvec operator*() const { return {data_, index_, i_}; }
        BucketNvec operator*() { return *this; }

        std::size_t size() const { return bucket_end_idx() - bucket_begin_idx(); }

        iterator begin() { return data().begin() + bucket_begin_idx(); }
        iterator end() { return data().begin() + bucket_end_idx(); }

        const_iterator begin() const { return data().begin() + bucket_begin_idx(); }
        const_iterator end() const { return data().begin() + bucket_end_idx(); }

        friend iterator begin(BucketNvec &b) { return b.begin(); }
        friend iterator end(BucketNvec &b) { return b.end(); }
        friend const_iterator begin(BucketNvec const &b) { return b.begin(); }
        friend const_iterator end(BucketNvec const &b) { return b.end(); }

        friend bool operator==(BucketNvec const &a, BucketNvec const &b) {
            assert(a.data_ == b.data_);
            assert(a.index_ == b.index_);
            return a.i_ == b.i_;
        }
        friend bool operator!=(BucketNvec const &a, BucketNvec const &b) {
            assert(a.data_ == b.data_);
            assert(a.index_ == b.index_);
            return a.i_ != b.i_;
        }

        BucketNvec &operator++() {
            ++i_;
            return *this;
        }
        BucketNvec operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        BucketNvec &operator--() {
            --i_;
            return *this;
        }
        BucketNvec operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }
        BucketNvec &operator+=(difference_type const n) {
            i_ += n;
            return *this;
        }
        BucketNvec &operator-=(difference_type const n) {
            i_ -= n;
            return *this;
        }
        BucketNvec operator+(difference_type const n) const {
            auto tmp = *this;
            tmp += n;
            return tmp;
        }
        BucketNvec operator-(difference_type const n) const {
            auto tmp = *this;
            tmp -= n;
            return tmp;
        }
        friend difference_type operator-(BucketNvec const &a, BucketNvec const &b) {
            assert(a.data_ == b.data_);
            assert(a.index_ == b.index_);
            return a.i_ - b.i_;
        }

      private:
        DataVec &data() const { return *data_; }
        IndexVec &index() const { return *index_; }

        index_value_type bucket_begin_idx() const { return to_idx(index()[i_]); }
        index_value_type bucket_end_idx() const { return to_idx(index()[i_ + 1U]); }
        bool is_inside_bucket(std::size_t const i) const { return bucket_begin_idx() + i < bucket_end_idx(); }

        size_type i_{};
        DataVec *data_{nullptr};
        IndexVec *index_{nullptr};
    };

    template <std::size_t Depth, typename DataVec, typename IndexVec, typename SizeType> struct ConstMetaBucket {
        using index_value_type = typename IndexVec::value_type;

        using const_iterator = std::conditional_t<Depth == 1U, ConstBucketNvec<DataVec, IndexVec, SizeType>,
                                                  ConstMetaBucket<Depth - 1U, DataVec, IndexVec, SizeType>>;
        using iterator = const_iterator;
        using difference_type = std::ptrdiff_t;
        using value_type = const_iterator;
        using pointer = void;
        using reference = ConstMetaBucket;
        using const_reference = ConstMetaBucket;
        using iterator_category = std::random_access_iterator_tag;
        using size_type = SizeType;

        ConstMetaBucket() = default;

        ConstMetaBucket(DataVec const *data, IndexVec const *index, index_value_type const i)
            : data_{data}, index_{index}, i_{i} {}

        index_value_type size() const { return index()[i_ + 1U] - index()[i_]; }

        iterator begin() const { return {data_, index_ - 1U, index()[i_]}; }
        const_iterator end() const { return {data_, index_ - 1U, index()[i_ + 1U]}; }

        friend iterator begin(ConstMetaBucket const &b) { return b.begin(); }
        friend iterator end(ConstMetaBucket const &b) { return b.end(); }

        reference operator*() const { return *this; }

        iterator operator[](size_type const i) { return begin() + i; }

        ConstMetaBucket &operator++() {
            ++i_;
            return *this;
        }
        ConstMetaBucket operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }
        ConstMetaBucket &operator--() {
            --i_;
            return *this;
        }
        ConstMetaBucket operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }
        ConstMetaBucket &operator+=(difference_type const n) {
            i_ += n;
            return *this;
        }
        ConstMetaBucket &operator-=(difference_type const n) {
            i_ -= n;
            return *this;
        }
        ConstMetaBucket operator+(difference_type const n) const {
            auto tmp = *this;
            tmp += n;
            return tmp;
        }
        ConstMetaBucket operator-(difference_type const n) const {
            auto tmp = *this;
            tmp -= n;
            return tmp;
        }

        friend bool operator==(ConstMetaBucket const &a, ConstMetaBucket const &b) { return a.i_ == b.i_; }

        friend bool operator!=(ConstMetaBucket const &a, ConstMetaBucket const &b) { return !(a == b); }

      private:
        IndexVec const &index() const { return *index_; }

        DataVec const *data_{nullptr};
        IndexVec const *index_{nullptr};
        index_value_type i_{};
    };

    template <std::size_t Depth, typename DataVec, typename IndexVec, typename SizeType> struct MetaBucket {
        using index_value_type = typename IndexVec::value_type;

        using iterator = std::conditional_t<Depth == 1U, BucketNvec<DataVec, IndexVec, SizeType>,
                                            MetaBucket<Depth - 1U, DataVec, IndexVec, SizeType>>;
        using const_iterator = std::conditional_t<Depth == 1U, ConstBucketNvec<DataVec, IndexVec, SizeType>,
                                                  ConstMetaBucket<Depth - 1U, DataVec, IndexVec, SizeType>>;

        using value_type = iterator;
        using iterator_category = std::random_access_iterator_tag;
        using reference = MetaBucket;
        using const_reference = ConstMetaBucket<Depth, DataVec, IndexVec, SizeType>;
        using difference_type = std::ptrdiff_t;
        using size_type = SizeType;

        MetaBucket() = default;

        MetaBucket(DataVec *data, IndexVec *index, index_value_type const i) : data_{data}, index_{index}, i_{i} {}

        index_value_type size() const { return index()[i_ + 1U] - index()[i_]; }

        iterator begin() { return {data_, index_ - 1U, index()[i_]}; }
        iterator end() { return {data_, index_ - 1U, index()[i_ + 1U]}; }

        const_iterator begin() const { return {data_, index_ - 1U, index()[i_]}; }
        const_iterator end() const { return {data_, index_ - 1U, index()[i_ + 1U]}; }

        friend iterator begin(MetaBucket &b) { return b.begin(); }
        friend iterator end(MetaBucket &b) { return b.end(); }

        friend const_iterator begin(MetaBucket const &b) { return b.begin(); }
        friend const_iterator end(MetaBucket const &b) { return b.end(); }

        reference operator*() const { return {data_, index_, i_}; }
        reference operator*() { return *this; }

        iterator operator[](size_type const i) { return begin() + i; }
        const_iterator operator[](size_type const i) const { return begin() + i; }

        operator ConstMetaBucket<Depth, DataVec, IndexVec, SizeType>() const { return {data_, index_, i_}; }

        MetaBucket &operator++() {
            ++i_;
            return *this;
        }
        MetaBucket operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        MetaBucket &operator--() {
            --i_;
            return *this;
        }
        MetaBucket operator--(int) {
            auto tmp = *this;
            --(*this);
            return tmp;
        }
        MetaBucket &operator+=(difference_type const n) {
            i_ += n;
            return *this;
        }
        MetaBucket &operator-=(difference_type const n) {
            i_ -= n;
            return *this;
        }
        MetaBucket operator+(difference_type const n) const {
            auto tmp = *this;
            tmp += n;
            return tmp;
        }
        MetaBucket operator-(difference_type const n) const {
            auto tmp = *this;
            tmp -= n;
            return tmp;
        }

        friend bool operator==(MetaBucket const &a, MetaBucket const &b) { return a.i_ == b.i_; }

        friend bool operator!=(MetaBucket const &a, MetaBucket const &b) { return !(a == b); }

      private:
        IndexVec &index() const { return *index_; }
        DataVec &data() const { return *data_; }

        DataVec *data_{nullptr};
        IndexVec *index_{nullptr};
        index_value_type i_{};
    };

    template <typename Key, typename DataVec, typename IndexVec, std::size_t N, typename SizeType = std::uint32_t>
    struct BasicNvec {
        using data_vec_t = DataVec;
        using index_vec_t = IndexVec;
        using size_type = SizeType;
        using data_value_type = typename DataVec::value_type;
        using index_value_type = typename IndexVec::value_type;

        using bucket_t = BucketNvec<DataVec, IndexVec, SizeType>;
        using const_bucket_t = ConstBucketNvec<DataVec, IndexVec, SizeType>;

        using iterator_category = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using iterator = MetaBucket<N - 1U, DataVec, IndexVec, SizeType>;
        using const_iterator = ConstMetaBucket<N - 1U, DataVec, IndexVec, SizeType>;
        using reference = iterator;
        using const_reference = const_iterator;
        using value_type = iterator;

        iterator begin() { return {&data_, &index_.back(), 0U}; }
        iterator end() { return {&data_, &index_.back(), size()}; }

        const_iterator begin() const { return {&data_, &index_.back(), 0U}; }
        const_iterator end() const { return {&data_, &index_.back(), size()}; }

        iterator front() { return begin(); }
        iterator back() { return begin() + size() - 1; }

        const_iterator front() const { return begin(); }
        const_iterator back() const { return begin() + size() - 1; }

        template <typename Container> void emplace_back(Container &&bucket) {
            if (index_[0].size() == 0U) {
                for (auto &i : index_) {
                    i.push_back(0U);
                }
            }
            add<N - 1>(bucket);
        }

        iterator operator[](Key const k) { return begin() + to_idx(k); }
        const_iterator operator[](Key const k) const { return begin() + to_idx(k); }

        size_type size() const { return index_[N - 1].size() == 0U ? 0U : (index_[N - 1].size() - 1U); }

        template <typename... Indices> size_type size(Key const first, Indices... rest) const {
            constexpr auto const I = sizeof...(Indices);
            verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
            if (sizeof...(Indices) == 0U) {
                return get_size<N - sizeof...(Indices) - 1>(first);
            } else {
                return get_size<N - sizeof...(Indices) - 1>(index_[I][first], rest...);
            }
        }

        template <typename... Indices> bucket_t at(Key const first, Indices... rest) {
            constexpr auto const I = sizeof...(Indices);
            static_assert(I == N - 1);
            verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
            return get_bucket(index_[I][to_idx(first)], rest...);
        }

        template <typename... Indices> const_bucket_t at(Key const first, Indices... rest) const {
            constexpr auto const I = sizeof...(Indices);
            static_assert(I == N - 1);
            verify(to_idx(first) < index_[I].size(), "nvec::at: index out of range");
            return get_bucket(index_[I][to_idx(first)], rest...);
        }

        template <typename... Rest>
        bucket_t get_bucket(index_value_type const bucket_start, index_value_type const i, Rest... rest) {
            return get_bucket<Rest...>(index_[sizeof...(Rest)][bucket_start + i], rest...);
        }

        bucket_t get_bucket(index_value_type const bucket_start, index_value_type const i) {
            return {&data_, &index_[0], bucket_start + i};
        }

        template <typename... Rest>
        const_bucket_t get_bucket(index_value_type const bucket_start, index_value_type const i, Rest... rest) const {
            return get_bucket<Rest...>(index_[sizeof...(Rest)][bucket_start + i], rest...);
        }

        const_bucket_t get_bucket(index_value_type const bucket_start, index_value_type const i) const {
            return {&data_, &index_[0], bucket_start + i};
        }

        template <std::size_t L, typename Container> void add(Container &&c) {
            if constexpr (L == 0) {
                index_[0].push_back(static_cast<size_type>(data_.size() + c.size()));
                for (auto &&x : c) {
                    data_.push_back(std::forward<decltype(x)>(x));
                }
            } else {
                index_[L].push_back(static_cast<size_type>(index_[L - 1].size() + c.size() - 1U));
                for (auto &&x : c) {
                    add<L - 1>(x);
                }
            }
        }

        template <std::size_t L, typename... Rest>
        size_type get_size(index_value_type const i, index_value_type const j, Rest... rest) const {
            if constexpr (sizeof...(Rest) == 0U) {
                return index_[L][i + j + 1] - index_[L][i + j];
            } else {
                return get_size<L>(index_[L][i + j], rest...);
            }
        }

        template <std::size_t L> size_type get_size(index_value_type const i) const {
            return index_[L][i + 1] - index_[L][i];
        }

        Array<IndexVec, N> index_;
        DataVec data_;
    };

    // Convenience alias
    template <typename K, typename V, std::size_t N, typename SizeType = std::uint32_t>
    using Nvec = BasicNvec<K, Vector<V>, Vector<base_t<K>>, N, SizeType>;

} // namespace bitcon
