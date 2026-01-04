#pragma once
#include <datapod/types/types.hpp>

#include <cinttypes>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "datapod/core/aligned_alloc.hpp"
#include "datapod/core/bit_counting.hpp"
#include "datapod/core/decay.hpp"
#include "datapod/core/exception.hpp"
#include "datapod/core/hash.hpp"
#include "datapod/pods/adapters/optional.hpp"
#include "datapod/pods/memory/ptr.hpp"

namespace datapod {

    // Generic hash-based container (Swiss table implementation)
    // Can be used as hash_set or hash_map:
    //   - hash_map: T = Pair<Key, Value>, GetKey returns entry.first
    //   - hash_set: T = T, GetKey returns entry (identity)
    //
    // Based on Google's Swiss Tables:
    // https://abseil.io/blog/20180927-swisstables
    template <typename T, template <typename> typename Ptr, typename GetKey, typename GetValue, typename Hash,
              typename Eq>
    struct HashStorage {
        using entry_t = T;
        using difference_type = datapod::isize;
        using size_type = hash_t;
        using key_type = decay_t<decltype(std::declval<GetKey>().operator()(std::declval<T>()))>;
        using mapped_type = decay_t<decltype(std::declval<GetValue>().operator()(std::declval<T>()))>;
        using group_t = datapod::u64;
        using h2_t = datapod::u8;
        static constexpr size_type const WIDTH = 8U;
        static constexpr datapod::usize const ALIGNMENT = alignof(T);

        template <typename Key> hash_t compute_hash(Key const &k) const { return static_cast<size_type>(Hash{}(k)); }

        enum ctrl_t : int8_t { EMPTY = -128, DELETED = -2, END = -1 };

        struct find_info {
            size_type offset_{}, probe_length_{};
        };

        struct probe_seq {
            constexpr probe_seq(size_type const hash, size_type const mask) noexcept
                : mask_{mask}, offset_{hash & mask_} {}
            size_type offset(size_type const i) const noexcept { return (offset_ + i) & mask_; }
            void next() noexcept {
                index_ += WIDTH;
                offset_ += index_;
                offset_ &= mask_;
            }
            size_type mask_, offset_, index_{0U};
        };

        struct bit_mask {
            static constexpr auto const SHIFT = 3U;

            constexpr explicit bit_mask(group_t const mask) noexcept : mask_{mask} {}

            bit_mask &operator++() noexcept {
                mask_ &= (mask_ - 1U);
                return *this;
            }

            size_type operator*() const noexcept { return trailing_zeros(); }

            explicit operator bool() const noexcept { return mask_ != 0U; }

            bit_mask begin() const noexcept { return *this; }
            bit_mask end() const noexcept { return bit_mask{0}; }

            size_type trailing_zeros() const noexcept { return ::datapod::trailing_zeros(mask_) >> SHIFT; }

            size_type leading_zeros() const noexcept {
                constexpr int total_significant_bits = 8 << SHIFT;
                constexpr int extra_bits = sizeof(group_t) * 8 - total_significant_bits;
                return ::datapod::leading_zeros(mask_ << extra_bits) >> SHIFT;
            }

            friend bool operator!=(bit_mask const &a, bit_mask const &b) noexcept { return a.mask_ != b.mask_; }

            group_t mask_;
        };

        struct group {
            static constexpr auto MSBS = 0x8080808080808080ULL;
            static constexpr auto LSBS = 0x0101010101010101ULL;
            static constexpr auto GAPS = 0x00FEFEFEFEFEFEFEULL;

            explicit group(ctrl_t const *pos) noexcept { std::memcpy(&ctrl_, pos, WIDTH); }

            bit_mask match(h2_t const hash) const noexcept {
                auto const x = ctrl_ ^ (LSBS * hash);
                return bit_mask{(x - LSBS) & ~x & MSBS};
            }

            bit_mask match_empty() const noexcept { return bit_mask{(ctrl_ & (~ctrl_ << 6U)) & MSBS}; }

            bit_mask match_empty_or_deleted() const noexcept { return bit_mask{(ctrl_ & (~ctrl_ << 7U)) & MSBS}; }

            datapod::usize count_leading_empty_or_deleted() const noexcept {
                return (trailing_zeros(((~ctrl_ & (ctrl_ >> 7U)) | GAPS) + 1U) + 7U) >> 3U;
            }

            group_t ctrl_;
        };

        struct iterator {
            using iterator_category = std::forward_iterator_tag;
            using value_type = HashStorage::entry_t;
            using reference = HashStorage::entry_t &;
            using pointer = HashStorage::entry_t *;
            using difference_type = datapod::isize;

            constexpr iterator() noexcept = default;

            reference operator*() const noexcept { return *entry_; }
            pointer operator->() const noexcept { return entry_; }

            iterator &operator++() noexcept {
                ++ctrl_;
                ++entry_;
                skip_empty_or_deleted();
                return *this;
            }

            iterator operator++(int) noexcept {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            friend bool operator==(iterator const &a, iterator const &b) noexcept { return a.ctrl_ == b.ctrl_; }
            friend bool operator!=(iterator const &a, iterator const &b) noexcept { return !(a == b); }

            constexpr iterator(ctrl_t *const ctrl) noexcept : ctrl_(ctrl) {}
            constexpr iterator(ctrl_t *const ctrl, T *const entry) noexcept : ctrl_(ctrl), entry_(entry) {}

            void skip_empty_or_deleted() noexcept {
                while (is_empty_or_deleted(*ctrl_)) {
                    auto const shift = group{ctrl_}.count_leading_empty_or_deleted();
                    ctrl_ += shift;
                    entry_ += shift;
                }
            }

            ctrl_t *ctrl_{nullptr};
            T *entry_{nullptr};
        };

        struct const_iterator {
            using iterator_category = std::forward_iterator_tag;
            using value_type = HashStorage::entry_t;
            using reference = HashStorage::entry_t const &;
            using pointer = HashStorage::entry_t const *;
            using difference_type = datapod::isize;

            constexpr const_iterator() noexcept = default;
            const_iterator(iterator i) noexcept : inner_(std::move(i)) {}

            reference operator*() const noexcept { return *inner_; }
            pointer operator->() const noexcept { return inner_.operator->(); }

            const_iterator &operator++() noexcept {
                ++inner_;
                return *this;
            }
            const_iterator operator++(int) noexcept { return inner_++; }

            friend bool operator==(const const_iterator &a, const const_iterator &b) noexcept {
                return a.inner_ == b.inner_;
            }
            friend bool operator!=(const const_iterator &a, const const_iterator &b) noexcept { return !(a == b); }

            const_iterator(ctrl_t const *ctrl, T const *entry) noexcept
                : inner_(const_cast<ctrl_t *>(ctrl), const_cast<T *>(entry)) {}

            iterator inner_;
        };

        static ctrl_t *empty_group() noexcept {
            alignas(16) static constexpr ctrl_t empty_group_data[] = {END,   EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
                                                                      EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
                                                                      EMPTY, EMPTY, EMPTY, EMPTY};
            return const_cast<ctrl_t *>(empty_group_data);
        }

        static constexpr bool is_empty(ctrl_t const c) noexcept { return c == EMPTY; }
        static constexpr bool is_full(ctrl_t const c) noexcept { return c >= 0; }
        static constexpr bool is_deleted(ctrl_t const c) noexcept { return c == DELETED; }
        static constexpr bool is_empty_or_deleted(ctrl_t const c) noexcept { return c < END; }

        static constexpr datapod::usize normalize_capacity(size_type const n) noexcept {
            return n == 0U ? 1U : ~size_type{} >> leading_zeros(n);
        }

        static constexpr size_type h1(size_type const hash) noexcept { return (hash >> 7U) ^ 16777619U; }

        static constexpr h2_t h2(size_type const hash) noexcept { return hash & 0x7FU; }

        static constexpr size_type capacity_to_growth(size_type const capacity) noexcept {
            return (capacity == 7U) ? 6U : capacity - (capacity / 8U);
        }

        constexpr HashStorage() = default;

        HashStorage(std::initializer_list<T> init) { insert(init.begin(), init.end()); }

        HashStorage(HashStorage &&other) noexcept
            : entries_{other.entries_}, ctrl_{other.ctrl_}, size_{other.size_}, capacity_{other.capacity_},
              growth_left_{other.growth_left_}, self_allocated_{other.self_allocated_} {
            other.reset();
        }

        HashStorage(HashStorage const &other) {
            if (other.size() != 0U) {
                for (const auto &v : other) {
                    emplace(v);
                }
            }
        }

        HashStorage &operator=(HashStorage &&other) noexcept {
            if (&other == this) {
                return *this;
            }
            entries_ = other.entries_;
            ctrl_ = other.ctrl_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            growth_left_ = other.growth_left_;
            self_allocated_ = other.self_allocated_;
            other.reset();
            return *this;
        }

        HashStorage &operator=(HashStorage const &other) {
            if (&other == this) {
                return *this;
            }
            clear();
            if (other.size() == 0U) {
                return *this;
            }
            for (const auto &v : other) {
                emplace(v);
            }
            return *this;
        }

        ~HashStorage() { clear(); }

        void set_empty_key(key_type const &) noexcept {}
        void set_deleted_key(key_type const &) noexcept {}

        // operator[]
        template <typename Key> mapped_type &bracket_operator_impl(Key &&key) {
            auto const res = find_or_prepare_insert(std::forward<Key>(key));
            if (res.second) {
                new (entries_ + res.first) T{static_cast<key_type>(key), mapped_type{}};
            }
            return GetValue{}(entries_[res.first]);
        }

        template <typename Key> mapped_type &operator[](Key &&key) {
            return bracket_operator_impl(std::forward<Key>(key));
        }

        mapped_type &operator[](key_type const &key) { return bracket_operator_impl(key); }

        // get()
        template <typename Key> Optional<mapped_type> get_impl(Key &&key) const {
            auto it = const_cast<HashStorage *>(this)->find(std::forward<Key>(key));
            if (it != end()) {
                return Optional<mapped_type>(GetValue{}(*it));
            }
            return Optional<mapped_type>();
        }

        template <typename Key> Optional<mapped_type> get(Key &&key) const { return get_impl(std::forward<Key>(key)); }

        // at()
        template <typename Key> mapped_type &at_impl(Key &&key) {
            auto const it = find(std::forward<Key>(key));
            if (it == end()) {
                throw_exception(std::out_of_range{"HashStorage::at() key not found"});
            }
            return GetValue{}(*it);
        }

        mapped_type &at(key_type const &key) { return at_impl(key); }

        mapped_type const &at(key_type const &key) const { return const_cast<HashStorage *>(this)->at(key); }

        template <typename Key> mapped_type &at(Key &&key) { return at_impl(std::forward<Key>(key)); }

        template <typename Key> mapped_type const &at(Key &&key) const {
            return const_cast<HashStorage *>(this)->at(std::forward<Key>(key));
        }

        // find()
        template <typename Key> iterator find_impl(Key &&key) {
            auto const hash = compute_hash(key);
            for (auto seq = probe_seq{h1(hash), capacity_}; true; seq.next()) {
                group g{ctrl_ + seq.offset_};
                for (auto const i : g.match(h2(hash))) {
                    if (Eq{}(GetKey()(entries_[seq.offset(i)]), key)) {
                        return iterator_at(seq.offset(i));
                    }
                }
                if (g.match_empty()) {
                    return end();
                }
            }
        }

        template <typename Key> const_iterator find(Key &&key) const {
            return const_cast<HashStorage *>(this)->find_impl(std::forward<Key>(key));
        }

        template <typename Key> iterator find(Key &&key) { return find_impl(std::forward<Key>(key)); }

        const_iterator find(key_type const &key) const noexcept {
            return const_cast<HashStorage *>(this)->find_impl(key);
        }

        iterator find(key_type const &key) noexcept { return find_impl(key); }

        template <class InputIt> void insert(InputIt first, InputIt last) {
            for (; first != last; ++first) {
                emplace(*first);
            }
        }

        // erase()
        template <typename Key> datapod::usize erase_impl(Key &&key) {
            auto it = find(std::forward<Key>(key));
            if (it == end()) {
                return 0U;
            }
            erase(it);
            return 1U;
        }

        datapod::usize erase(key_type const &k) { return erase_impl(k); }

        template <typename Key> datapod::usize erase(Key &&key) { return erase_impl(std::forward<Key>(key)); }

        void erase(iterator const it) noexcept {
            it.entry_->~T();
            erase_meta_only(it);
        }

        std::pair<iterator, bool> insert(T const &entry) { return emplace(entry); }

        template <typename... Args> std::pair<iterator, bool> emplace(Args &&...args) {
            auto entry = T{std::forward<Args>(args)...};
            auto res = find_or_prepare_insert(GetKey()(entry));
            if (res.second) {
                new (entries_ + res.first) T{std::move(entry)};
            }
            return {iterator_at(res.first), res.second};
        }

        iterator begin() noexcept {
            auto it = iterator_at(0U);
            if (ctrl_ != nullptr) {
                it.skip_empty_or_deleted();
            }
            return it;
        }
        iterator end() noexcept { return {ctrl_ + capacity_}; }

        const_iterator begin() const noexcept { return const_cast<HashStorage *>(this)->begin(); }
        const_iterator end() const noexcept { return const_cast<HashStorage *>(this)->end(); }
        const_iterator cbegin() const noexcept { return begin(); }
        const_iterator cend() const noexcept { return end(); }

        friend iterator begin(HashStorage &h) noexcept { return h.begin(); }
        friend const_iterator begin(HashStorage const &h) noexcept { return h.begin(); }
        friend const_iterator cbegin(HashStorage const &h) noexcept { return h.begin(); }
        friend iterator end(HashStorage &h) noexcept { return h.end(); }
        friend const_iterator end(HashStorage const &h) noexcept { return h.end(); }
        friend const_iterator cend(HashStorage const &h) noexcept { return h.end(); }

        bool empty() const noexcept { return size() == 0U; }
        size_type size() const noexcept { return size_; }
        size_type capacity() const noexcept { return capacity_; }
        size_type max_size() const noexcept { return std::numeric_limits<datapod::usize>::max(); }

        bool was_never_full(datapod::usize const index) const noexcept {
            auto const index_before = (index - WIDTH) & capacity_;
            auto const empty_after = group{ctrl_ + index}.match_empty();
            auto const empty_before = group{ctrl_ + index_before}.match_empty();
            return empty_before && empty_after && (empty_after.trailing_zeros() + empty_before.leading_zeros()) < WIDTH;
        }

        void erase_meta_only(const_iterator it) noexcept {
            --size_;
            auto const index = static_cast<datapod::usize>(it.inner_.ctrl_ - ctrl_);
            auto const wnf = was_never_full(index);
            set_ctrl(index, static_cast<h2_t>(wnf ? EMPTY : DELETED));
            growth_left_ += wnf;
        }

        void clear() {
            if (capacity_ == 0U) {
                return;
            }

            for (size_type i = 0U; i != capacity_; ++i) {
                if (is_full(ctrl_[i])) {
                    entries_[i].~T();
                }
            }

            if (self_allocated_) {
                aligned_free(ALIGNMENT, entries_);
            }

            partial_reset();
        }

        template <typename Key> std::pair<size_type, bool> find_or_prepare_insert(Key &&key) {
            auto const hash = compute_hash(key);
            // Search for key - if found, return its position
            for (auto seq = probe_seq{h1(hash), capacity_}; true; seq.next()) {
                group g{ctrl_ + seq.offset_};
                // Check if key already exists in this group
                for (auto const i : g.match(h2(hash))) {
                    if (Eq{}(GetKey()(entries_[seq.offset(i)]), key)) {
                        return {seq.offset(i), false}; // Found existing key
                    }
                }
                // Check if we've found an empty slot (means key doesn't exist)
                if (g.match_empty()) {
                    break; // Key not found, proceed to insertion
                }
            }
            // Key doesn't exist - prepare slot for insertion
            return {prepare_insert(hash), true};
        }

        find_info find_first_non_full(size_type const hash) const noexcept {
            // Probe until we find an empty or deleted slot
            // Note: Swiss tables guarantee we'll always find one if growth logic is correct
            for (auto seq = probe_seq{h1(hash), capacity_};; seq.next()) {
                auto const mask = group{ctrl_ + seq.offset_}.match_empty_or_deleted();
                if (mask) {
                    return {seq.offset(*mask), seq.index_};
                }
                // NOTE: If growth_left calculation is correct, we should always find a slot
                // If we don't, the table has a serious bug
            }
            // Unreachable, but needed to silence compiler warning
            return {0, 0};
        }

        size_type prepare_insert(size_type const hash) {
            // Check if we need to grow BEFORE finding a slot
            if (growth_left_ == 0U) {
                rehash_and_grow_if_necessary();
            }

            auto target = find_first_non_full(hash);
            ++size_;
            growth_left_ -= (is_empty(ctrl_[target.offset_]) ? 1U : 0U);
            set_ctrl(target.offset_, h2(hash));
            return target.offset_;
        }

        void set_ctrl(size_type const i, h2_t const c) noexcept {
            ctrl_[i] = static_cast<ctrl_t>(c);
            // For wraparound: mirror the first WIDTH-1 bytes after the END marker
            // ctrl_[capacity] is the END marker and should never be modified
            // ctrl_[capacity+1..capacity+WIDTH-1] mirrors ctrl_[0..WIDTH-2]
            if (i < WIDTH - 1U) {
                ctrl_[capacity_ + 1U + i] = static_cast<ctrl_t>(c);
            }
        }

        void rehash_and_grow_if_necessary() { resize(capacity_ == 0U ? 1U : capacity_ * 2U + 1U); }

        void reset_growth_left() noexcept { growth_left_ = capacity_to_growth(capacity_) - size_; }

        void reset_ctrl() noexcept {
            std::memset(ctrl_, EMPTY, static_cast<datapod::usize>(capacity_ + WIDTH + 1U));
            ctrl_[capacity_] = END;
        }

        void initialize_entries() {
            self_allocated_ = true;
            auto const size = static_cast<size_type>(capacity_ * sizeof(T) + (capacity_ + 1U + WIDTH) * sizeof(ctrl_t));
            entries_ = reinterpret_cast<T *>(aligned_alloc(ALIGNMENT, static_cast<datapod::usize>(size)));
            if (entries_ == nullptr) {
                throw_exception(std::bad_alloc{});
            }
            ctrl_ = reinterpret_cast<ctrl_t *>(reinterpret_cast<datapod::u8 *>(entries_) + capacity_ * sizeof(T));
            reset_ctrl();
            reset_growth_left();
        }

        void resize(size_type const new_capacity) {
            auto const old_ctrl = ctrl_;
            auto const old_entries = entries_;
            auto const old_capacity = capacity_;
            auto const old_self_allocated = self_allocated_;

            capacity_ = new_capacity;
            initialize_entries();

            for (size_type i = 0U; i != old_capacity; ++i) {
                if (is_full(old_ctrl[i])) {
                    auto const hash = compute_hash(GetKey()(old_entries[i]));
                    auto const target = find_first_non_full(hash);
                    auto const new_index = target.offset_;
                    set_ctrl(new_index, h2(hash));
                    new (entries_ + new_index) T{std::move(old_entries[i])};
                    old_entries[i].~T();
                }
            }

            if (old_capacity != 0U && old_self_allocated) {
                aligned_free(ALIGNMENT, old_entries);
            }
        }

        void partial_reset() noexcept {
            entries_ = nullptr;
            ctrl_ = empty_group();
            size_ = 0U;
            capacity_ = 0U;
            growth_left_ = 0U;
        }

        void reset() noexcept {
            partial_reset();
            self_allocated_ = false;
        }

        void rehash() { resize(capacity_); }

        iterator iterator_at(size_type const i) noexcept { return {ctrl_ + i, entries_ + i}; }
        const_iterator iterator_at(size_type const i) const noexcept { return {ctrl_ + i, entries_ + i}; }

        // Lookup - contains()
        template <typename Key> bool contains(Key &&key) const { return find(std::forward<Key>(key)) != end(); }

        bool contains(key_type const &key) const { return find(key) != end(); }

        // Lookup - count()
        template <typename Key> size_type count(Key &&key) const { return contains(std::forward<Key>(key)) ? 1U : 0U; }

        size_type count(key_type const &key) const { return contains(key) ? 1U : 0U; }

        // Modifiers - swap()
        void swap(HashStorage &other) noexcept {
            std::swap(entries_, other.entries_);
            std::swap(ctrl_, other.ctrl_);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
            std::swap(growth_left_, other.growth_left_);
            std::swap(self_allocated_, other.self_allocated_);
        }

        // Capacity - reserve()
        void reserve(size_type count) {
            if (count > capacity_) {
                resize(normalize_capacity(count));
            }
        }

        // Capacity - rehash() with count parameter
        void rehash(size_type count) {
            if (count > capacity_) {
                resize(normalize_capacity(count));
            } else {
                resize(capacity_);
            }
        }

        // Capacity - bucket_count()
        size_type bucket_count() const noexcept { return capacity_; }

        // Capacity - load_factor()
        float load_factor() const noexcept {
            return capacity_ == 0U ? 0.0f : static_cast<float>(size_) / static_cast<float>(capacity_);
        }

        // Capacity - max_load_factor()
        float max_load_factor() const noexcept {
            // Swiss tables target ~87.5% load factor (7/8)
            return 0.875f;
        }

        // C++17 - insert_or_assign()
        template <typename M> std::pair<iterator, bool> insert_or_assign(key_type const &key, M &&obj) {
            auto res = find_or_prepare_insert(key);
            if (res.second) {
                // New insertion
                new (entries_ + res.first) T{key, std::forward<M>(obj)};
            } else {
                // Update existing
                GetValue{}(entries_[res.first]) = std::forward<M>(obj);
            }
            return {iterator_at(res.first), res.second};
        }

        template <typename M> std::pair<iterator, bool> insert_or_assign(key_type &&key, M &&obj) {
            auto res = find_or_prepare_insert(key);
            if (res.second) {
                // New insertion
                new (entries_ + res.first) T{std::move(key), std::forward<M>(obj)};
            } else {
                // Update existing
                GetValue{}(entries_[res.first]) = std::forward<M>(obj);
            }
            return {iterator_at(res.first), res.second};
        }

        // C++17 - try_emplace()
        template <typename... Args> std::pair<iterator, bool> try_emplace(key_type const &key, Args &&...args) {
            auto res = find_or_prepare_insert(key);
            if (res.second) {
                // Only emplace if key doesn't exist
                new (entries_ + res.first) T{key, mapped_type{std::forward<Args>(args)...}};
            }
            return {iterator_at(res.first), res.second};
        }

        template <typename... Args> std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args) {
            auto res = find_or_prepare_insert(key);
            if (res.second) {
                // Only emplace if key doesn't exist
                new (entries_ + res.first) T{std::move(key), mapped_type{std::forward<Args>(args)...}};
            }
            return {iterator_at(res.first), res.second};
        }

        bool operator==(HashStorage const &b) const noexcept {
            if (size() != b.size()) {
                return false;
            }
            for (auto const &el : *this) {
                auto const it = b.find(GetKey()(el));
                if (it == b.end() || GetValue()(el) != GetValue()(*it)) {
                    return false;
                }
            }
            return true;
        }

        // Serialization support
        auto members() noexcept { return std::tie(entries_, ctrl_, size_, capacity_, growth_left_, self_allocated_); }

        Ptr<T> entries_{nullptr};
        Ptr<ctrl_t> ctrl_{empty_group()};
        size_type size_{0U}, capacity_{0U}, growth_left_{0U};
        bool self_allocated_{false};
    };

    namespace hash_storage {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace hash_storage

} // namespace datapod
