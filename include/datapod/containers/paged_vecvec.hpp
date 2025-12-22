#pragma once

#include <cassert>
#include <iterator>
#include <string_view>

#include "datapod/containers/paged.hpp"
#include "datapod/containers/vector.hpp"
#include "datapod/core/strong.hpp"
#include "datapod/core/verify.hpp"

namespace datapod {

    template <typename Index, typename PagedAlloc, typename Key> struct PagedVecvec {
        using index_t = Index;
        using data_t = PagedAlloc;

        using page_t = typename PagedAlloc::page_t;
        using size_type = typename PagedAlloc::size_type;
        using data_value_type = typename PagedAlloc::value_type;

        struct ConstBucket final {
            using size_type = typename PagedAlloc::size_type;
            using data_value_type = typename PagedAlloc::value_type;

            using value_type = data_value_type;
            using iterator = typename PagedAlloc::const_iterator;
            using const_iterator = typename PagedAlloc::const_iterator;
            using reference = typename PagedAlloc::const_reference;
            using const_reference = typename PagedAlloc::const_reference;

            using iterator_category = std::random_access_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using pointer = std::add_pointer_t<value_type>;

            ConstBucket(PagedVecvec const *pv, Key const i) : pv_{pv}, i_{i} {}

            template <typename T = std::decay_t<data_value_type>, typename = std::enable_if_t<std::is_same_v<T, char>>>
            std::string_view view() const {
                return std::string_view{begin(), size()};
            }

            const_iterator begin() const { return pv_->data(i_); }
            const_iterator end() const { return pv_->data(i_) + size(); }
            friend const_iterator begin(ConstBucket const &b) { return b.begin(); }
            friend const_iterator end(ConstBucket const &b) { return b.end(); }

            value_type const &operator[](std::size_t const i) const {
                assert(i < size());
                return *(begin() + i);
            }

            value_type const &at(std::size_t const i) const {
                verify(i < size(), "paged_vecvec: const_bucket::at: index out of range");
                return *(begin() + i);
            }

            value_type const &front() const {
                assert(!empty());
                return (*this)[0];
            }

            value_type const &back() const {
                assert(!empty());
                return (*this)[size() - 1U];
            }

            ConstBucket operator*() const { return *this; }

            size_type size() const { return pv_->page(i_).size_; }
            bool empty() const { return size() == 0U; }

            friend bool operator==(ConstBucket const &a, ConstBucket const &b) {
                assert(a.pv_ == b.pv_);
                return a.i_ == b.i_;
            }

            friend bool operator!=(ConstBucket const &a, ConstBucket const &b) {
                assert(a.pv_ == b.pv_);
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
                assert(a.pv_ == b.pv_);
                return a.i_ - b.i_;
            }

          private:
            PagedVecvec const *pv_;
            Key i_;
        };

        struct Bucket final {
            using size_type = typename PagedAlloc::size_type;
            using index_value_type = typename PagedAlloc::page_t;
            using data_value_type = typename PagedAlloc::value_type;

            using value_type = data_value_type;
            using iterator = typename PagedAlloc::iterator;
            using const_iterator = typename PagedAlloc::iterator;
            using reference = typename PagedAlloc::reference;
            using const_reference = typename PagedAlloc::const_reference;

            using iterator_category = std::random_access_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using pointer = std::add_pointer_t<value_type>;

            Bucket(PagedVecvec *pv, Key const i) : pv_{pv}, i_{i} {}

            value_type &front() {
                assert(!empty());
                return (*this)[0];
            }

            value_type &back() {
                assert(!empty());
                return (*this)[size() - 1U];
            }

            value_type const &front() const {
                assert(!empty());
                return (*this)[0];
            }

            value_type const &back() const {
                assert(!empty());
                return (*this)[size() - 1U];
            }

            void push_back(data_value_type const &x) {
                auto &p = pv_->page(i_);
                p = pv_->paged_.resize_page(p, static_cast<typename PagedAlloc::page_size_type>(p.size_ + 1U));
                (*this)[size() - 1U] = x;
            }

            template <typename... Ts> void emplace_back(Ts... args) {
                push_back(data_value_type{std::forward<Ts>(args)...});
            }

            template <typename Arg> iterator insert(iterator const it, Arg &&el) {
                auto const old_offset = std::distance(begin(), it);
                auto const old_size = size();
                push_back(data_value_type{el});
                return std::rotate(begin() + old_offset, begin() + old_size, end());
            }

            template <typename T = std::decay_t<data_value_type>, typename = std::enable_if_t<std::is_same_v<T, char>>>
            std::string_view view() const {
                return std::string_view{begin(), static_cast<std::size_t>(size())};
            }

            iterator begin() { return pv_->data(i_); }
            iterator end() { return pv_->data(i_) + size(); }
            const_iterator begin() const { return pv_->data(i_); }
            const_iterator end() const { return pv_->data(i_) + size(); }
            friend iterator begin(Bucket const &b) { return const_cast<Bucket &>(b).begin(); }
            friend iterator end(Bucket const &b) { return const_cast<Bucket &>(b).end(); }
            friend iterator begin(Bucket &b) { return b.begin(); }
            friend iterator end(Bucket &b) { return b.end(); }

            value_type &operator[](std::size_t const i) {
                assert(i < size());
                return *(begin() + i);
            }

            value_type const &operator[](std::size_t const i) const {
                assert(i < size());
                return *(begin() + i);
            }

            value_type const &at(std::size_t const i) const {
                verify(i < size(), "bucket::at: index out of range");
                return *(begin() + i);
            }

            value_type &at(std::size_t const i) {
                verify(i < size(), "bucket::at: index out of range");
                return *(begin() + i);
            }

            Bucket operator*() const { return *this; }

            operator ConstBucket() const { return {pv_, i_}; }

            size_type size() const { return pv_->page(i_).size_; }
            bool empty() const { return size() == 0U; }

            friend bool operator==(Bucket const &a, Bucket const &b) {
                assert(a.pv_ == b.pv_);
                return a.i_ == b.i_;
            }

            friend bool operator!=(Bucket const &a, Bucket const &b) {
                assert(a.pv_ == b.pv_);
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
            Bucket operator*() { return *this; }
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
                assert(a.pv_ == b.pv_);
                return a.i_ - b.i_;
            }

          private:
            PagedVecvec *pv_;
            Key i_;
        };

        using value_type = Bucket;
        using iterator = Bucket;
        using const_iterator = ConstBucket;

        Bucket operator[](Key const i) { return {this, i}; }
        ConstBucket operator[](Key const i) const { return {this, i}; }

        page_t &page(Key const i) { return idx_[to_idx(i)]; }
        page_t const &page(Key const i) const { return idx_[to_idx(i)]; }

        data_value_type const *data(Key const i) const { return data(idx_[to_idx(i)]); }
        data_value_type *data(Key const i) { return data(idx_[to_idx(i)]); }
        data_value_type const *data(page_t const &p) const { return paged_.data(p); }
        data_value_type *data(page_t const &p) { return paged_.data(p); }

        ConstBucket at(Key const i) const {
            verify(to_idx(i) < idx_.size(), "paged_vecvec::at: index out of range");
            return operator[](i);
        }

        Bucket at(Key const i) {
            verify(to_idx(i) < idx_.size(), "paged_vecvec::at: index out of range");
            return operator[](i);
        }

        Bucket front() { return at(Key{0}); }
        Bucket back() { return at(Key{static_cast<typename Key::value_t>(size() - 1)}); }

        ConstBucket front() const { return at(Key{0}); }
        ConstBucket back() const { return at(Key{static_cast<typename Key::value_t>(size() - 1)}); }

        base_t<Key> size() const { return idx_.size(); }
        bool empty() const { return idx_.empty(); }

        Bucket begin() { return empty() ? Bucket{nullptr, Key{}} : front(); }
        Bucket end() {
            return empty() ? Bucket{nullptr, Key{}} : operator[](Key{static_cast<typename Key::value_t>(size())});
        }
        ConstBucket begin() const { return empty() ? ConstBucket{nullptr, Key{}} : front(); }
        ConstBucket end() const {
            return empty() ? ConstBucket{nullptr, Key{}} : operator[](Key{static_cast<typename Key::value_t>(size())});
        }

        friend Bucket begin(PagedVecvec &m) { return m.begin(); }
        friend Bucket end(PagedVecvec &m) { return m.end(); }
        friend ConstBucket begin(PagedVecvec const &m) { return m.begin(); }
        friend ConstBucket end(PagedVecvec const &m) { return m.end(); }

        template <typename Container, typename = std::enable_if_t<std::is_convertible_v<
                                          decltype(*std::declval<Container>().begin()), data_value_type>>>
        void emplace_back(Container &&bucket) {
            auto p = paged_.create_page(static_cast<typename PagedAlloc::page_size_type>(bucket.size()));
            paged_.copy(p, std::begin(bucket), std::end(bucket));
            idx_.emplace_back(p);
        }

        template <typename X>
        std::enable_if_t<std::is_convertible_v<std::decay_t<X>, data_value_type>>
        emplace_back(std::initializer_list<X> &&x) {
            emplace_back(x);
        }

        void emplace_back() { emplace_back(std::initializer_list<data_value_type>{}); }

        void emplace_back_empty() { idx_.emplace_back(paged_.create_page(0U)); }

        template <typename T = data_value_type, typename = std::enable_if_t<std::is_convertible_v<T, char const>>>
        void emplace_back(char const *s) {
            return emplace_back(std::string_view{s});
        }

        template <typename Container, typename = std::enable_if_t<std::is_convertible_v<
                                          decltype(*std::declval<Container>().begin()), data_value_type>>>
        void insert(Key const &k, Container &&bucket) {
            auto p = paged_.create_page(static_cast<typename PagedAlloc::page_size_type>(bucket.size()));
            paged_.copy(p, std::begin(bucket), std::end(bucket));
            idx_.insert(idx_.begin() + to_idx(k), p);
        }

        void resize(size_type const size) {
            for (auto i = size; i < idx_.size(); ++i) {
                paged_.free_page(idx_[i]);
            }
            idx_.resize(size);
        }

        void clear() {
            paged_.clear();
            idx_.clear();
        }

        PagedAlloc paged_;
        Index idx_;
    };

    // Convenience alias
    template <typename K, typename V, typename SizeType = std::size_t>
    using PagedVecvecTyped = PagedVecvec<Vector<Page<SizeType, std::uint16_t>>, Paged<Vector<V>>, K>;

} // namespace datapod
