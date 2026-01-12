#pragma once
#include <datapod/types/types.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>

#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    // Basic byte buffer container for raw binary data
    template <typename Vec = Vector<u8>> class BasicBytes {
      public:
        using value_type = u8;
        using size_type = typename Vec::size_type;
        using difference_type = typename Vec::difference_type;
        using reference = u8 &;
        using const_reference = u8 const &;
        using pointer = u8 *;
        using const_pointer = u8 const *;
        using iterator = u8 *;
        using const_iterator = u8 const *;

        // Constructors
        constexpr BasicBytes() noexcept = default;

        explicit BasicBytes(size_type count) : data_(count) {}

        BasicBytes(size_type count, u8 value) : data_(count, value) {}

        // Constructor from raw pointer and size
        BasicBytes(u8 const *ptr, size_type size) {
            data_.reserve(size);
            for (size_type i = 0; i < size; ++i) {
                data_.push_back(ptr[i]);
            }
        }

        // Constructor from void pointer and size (for convenience)
        BasicBytes(void const *ptr, size_type size) : BasicBytes(static_cast<u8 const *>(ptr), size) {}

        // Copy constructor
        BasicBytes(BasicBytes const &other) = default;

        // Move constructor
        BasicBytes(BasicBytes &&other) noexcept = default;

        // Range constructor
        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        BasicBytes(InputIt first, InputIt last) {
            data_.reserve(std::distance(first, last));
            for (auto it = first; it != last; ++it) {
                data_.push_back(static_cast<u8>(*it));
            }
        }

        // Initializer list constructor
        BasicBytes(std::initializer_list<u8> init) : data_(init) {}

        // Destructor
        ~BasicBytes() = default;

        // Assignment operators
        BasicBytes &operator=(BasicBytes const &other) = default;

        BasicBytes &operator=(BasicBytes &&other) noexcept = default;

        BasicBytes &operator=(std::initializer_list<u8> ilist) {
            data_ = ilist;
            return *this;
        }

        // Element access
        reference operator[](size_type pos) noexcept { return data_[pos]; }

        const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

        reference at(size_type pos) { return data_.at(pos); }

        const_reference at(size_type pos) const { return data_.at(pos); }

        reference front() noexcept { return data_.front(); }

        const_reference front() const noexcept { return data_.front(); }

        reference back() noexcept { return data_.back(); }

        const_reference back() const noexcept { return data_.back(); }

        u8 *data() noexcept { return data_.data(); }

        u8 const *data() const noexcept { return data_.data(); }

        // Raw data as void pointer (for interoperability)
        void *void_data() noexcept { return static_cast<void *>(data_.data()); }

        void const *void_data() const noexcept { return static_cast<void const *>(data_.data()); }

        // Iterators
        iterator begin() noexcept { return data_.begin(); }

        const_iterator begin() const noexcept { return data_.begin(); }

        const_iterator cbegin() const noexcept { return data_.cbegin(); }

        iterator end() noexcept { return data_.end(); }

        const_iterator end() const noexcept { return data_.end(); }

        const_iterator cend() const noexcept { return data_.cend(); }

        // Capacity
        bool empty() const noexcept { return data_.empty(); }

        size_type size() const noexcept { return data_.size(); }

        size_type max_size() const noexcept { return data_.max_size(); }

        size_type capacity() const noexcept { return data_.capacity(); }

        void reserve(size_type new_cap) { data_.reserve(new_cap); }

        void shrink_to_fit() { data_.shrink_to_fit(); }

        // Modifiers
        void clear() noexcept { data_.clear(); }

        void push_back(u8 value) { data_.push_back(value); }

        void pop_back() { data_.pop_back(); }

        template <typename... Args> reference emplace_back(Args &&...args) {
            return data_.emplace_back(std::forward<Args>(args)...);
        }

        // Append raw bytes
        void append(u8 const *ptr, size_type count) {
            reserve(size() + count);
            for (size_type i = 0; i < count; ++i) {
                push_back(ptr[i]);
            }
        }

        void append(void const *ptr, size_type count) { append(static_cast<u8 const *>(ptr), count); }

        // Append from another Bytes
        void append(BasicBytes const &other) {
            reserve(size() + other.size());
            for (size_type i = 0; i < other.size(); ++i) {
                push_back(other[i]);
            }
        }

        // Insert single byte
        iterator insert(const_iterator pos, u8 value) { return data_.insert(pos, value); }

        // Insert range
        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        iterator insert(const_iterator pos, InputIt first, InputIt last) {
            return data_.insert(pos, first, last);
        }

        // Erase single byte
        iterator erase(const_iterator pos) { return data_.erase(const_cast<iterator>(pos)); }

        // Erase range
        iterator erase(const_iterator first, const_iterator last) {
            return data_.erase(const_cast<iterator>(first), const_cast<iterator>(last));
        }

        void resize(size_type count) { data_.resize(count); }

        void resize(size_type count, u8 value) { data_.resize(count, value); }

        void swap(BasicBytes &other) noexcept { data_.swap(other.data_); }

        // Byte-level operations
        void zero() { std::memset(data(), 0, size()); }

        void fill(u8 value) { std::memset(data(), value, size()); }

        // Comparison
        bool operator==(BasicBytes const &other) const noexcept {
            if (size() != other.size()) {
                return false;
            }
            return std::memcmp(data(), other.data(), size()) == 0;
        }

        bool operator!=(BasicBytes const &other) const noexcept { return !(*this == other); }

        bool operator<(BasicBytes const &other) const noexcept {
            size_type min_size = std::min(size(), other.size());
            int cmp = std::memcmp(data(), other.data(), min_size);
            if (cmp != 0) {
                return cmp < 0;
            }
            return size() < other.size();
        }

        bool operator<=(BasicBytes const &other) const noexcept { return !(other < *this); }

        bool operator>(BasicBytes const &other) const noexcept { return other < *this; }

        bool operator>=(BasicBytes const &other) const noexcept { return !(*this < other); }

        // Find byte
        size_type find(u8 byte, size_type pos = 0) const noexcept {
            if (pos >= size()) {
                return npos;
            }
            u8 const *found = static_cast<u8 const *>(std::memchr(data() + pos, byte, size() - pos));
            return found ? static_cast<size_type>(found - data()) : npos;
        }

        // Find subsequence
        size_type find(BasicBytes const &sub, size_type pos = 0) const noexcept {
            if (sub.empty()) {
                return pos <= size() ? pos : npos;
            }
            if (pos + sub.size() > size()) {
                return npos;
            }

            for (size_type i = pos; i <= size() - sub.size(); ++i) {
                bool match = true;
                for (size_type j = 0; j < sub.size(); ++j) {
                    if (data_[i + j] != sub[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    return i;
                }
            }
            return npos;
        }

        // Rfind byte
        size_type rfind(u8 byte, size_type pos = npos) const noexcept {
            if (size() == 0) {
                return npos;
            }
            size_type start_pos = std::min(pos, size() - 1);
            for (size_type i = start_pos + 1; i > 0; --i) {
                if (data_[i - 1] == byte) {
                    return i - 1;
                }
            }
            return npos;
        }

        // Contains
        bool contains(u8 byte) const noexcept { return find(byte) != npos; }

        bool contains(BasicBytes const &sub) const noexcept { return find(sub) != npos; }

        // Starts with
        bool starts_with(u8 byte) const noexcept { return size() > 0 && data_[0] == byte; }

        bool starts_with(BasicBytes const &prefix) const noexcept {
            return size() >= prefix.size() && std::memcmp(data(), prefix.data(), prefix.size()) == 0;
        }

        // Ends with
        bool ends_with(u8 byte) const noexcept { return size() > 0 && data_[size() - 1] == byte; }

        bool ends_with(BasicBytes const &suffix) const noexcept {
            return size() >= suffix.size() &&
                   std::memcmp(data() + size() - suffix.size(), suffix.data(), suffix.size()) == 0;
        }

        // Sub-bytes
        BasicBytes substr(size_type pos = 0, size_type count = npos) const {
            if (pos >= size()) {
                return BasicBytes();
            }
            size_type actual_count = std::min(count, size() - pos);
            return BasicBytes(data() + pos, actual_count);
        }

        // Concatenation
        friend BasicBytes operator+(BasicBytes const &lhs, BasicBytes const &rhs) {
            BasicBytes result;
            result.reserve(lhs.size() + rhs.size());
            result.append(lhs);
            result.append(rhs);
            return result;
        }

        friend BasicBytes operator+(BasicBytes const &lhs, u8 rhs) {
            BasicBytes result(lhs);
            result.push_back(rhs);
            return result;
        }

        // Static constants
        static constexpr size_type npos = static_cast<size_type>(-1);

        // Serialization support
        auto members() noexcept { return std::tie(data_); }

      private:
        Vec data_;
    };

    using Bytes = BasicBytes<>;

    namespace seq_bytes {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace seq_bytes

} // namespace datapod

// std::hash specialization for datapod::Bytes
namespace std {
    template <typename Vec> struct hash<datapod::BasicBytes<Vec>> {
        size_t operator()(datapod::BasicBytes<Vec> const &bytes) const noexcept {
            // Use FNV-1a hash algorithm (fast and good distribution)
            size_t hash = 14695981039346656037ULL; // FNV offset basis
            datapod::u8 const *data = bytes.data();
            size_t size = bytes.size();

            for (size_t i = 0; i < size; ++i) {
                hash ^= static_cast<size_t>(data[i]);
                hash *= 1099511628211ULL; // FNV prime
            }

            return hash;
        }
    };

    template <> struct hash<datapod::Bytes> {
        size_t operator()(datapod::Bytes const &bytes) const noexcept {
            // Use FNV-1a hash algorithm (fast and good distribution)
            size_t hash = 14695981039346656037ULL; // FNV offset basis
            datapod::u8 const *data = bytes.data();
            size_t size = bytes.size();

            for (size_t i = 0; i < size; ++i) {
                hash ^= static_cast<size_t>(data[i]);
                hash *= 1099511628211ULL; // FNV prime
            }

            return hash;
        }
    };
} // namespace std
