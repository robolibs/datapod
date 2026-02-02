#pragma once
#include <datapod/types/types.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace datapod {

    // Basic string with Small String Optimization (SSO)
    // Stores up to 23 characters inline before allocating
    template <typename Ptr = char *> class BasicString {
      public:
        using value_type = char;
        using size_type = datapod::usize;
        using difference_type = datapod::isize;
        using reference = char &;
        using const_reference = char const &;
        using pointer = Ptr;
        using const_pointer = char const *;

        static constexpr size_type SSO_SIZE = 23;
        static constexpr size_type npos = static_cast<size_type>(-1);

        // Default constructor
        BasicString() noexcept : size_(0), is_sso_(true) { sso_data_[0] = '\0'; }

        // Constructor from C-string
        BasicString(char const *str) : BasicString(str, std::strlen(str)) {}

        // Constructor from C-string with length
        BasicString(char const *str, size_type len) : size_(len), is_sso_(len <= SSO_SIZE) {
            if (is_sso_) {
                std::memcpy(sso_data_, str, len);
                sso_data_[len] = '\0';
            } else {
                capacity_ = len;
                heap_data_ = new char[capacity_ + 1]; // +1 for null terminator
                std::memcpy(heap_data_, str, len);
                heap_data_[len] = '\0';
            }
        }

        // Constructor from string_view
        BasicString(std::string_view sv) : BasicString(sv.data(), sv.size()) {}

        // Constructor from std::string
        BasicString(std::string const &s) : BasicString(s.data(), s.size()) {}

        // Copy constructor
        BasicString(BasicString const &other) : size_(other.size_), is_sso_(other.is_sso_) {
            if (is_sso_) {
                std::memcpy(sso_data_, other.sso_data_, size_ + 1);
            } else {
                capacity_ = other.capacity_;
                heap_data_ = new char[capacity_ + 1]; // +1 for null terminator
                std::memcpy(heap_data_, other.heap_data_, size_ + 1);
            }
        }

        // Move constructor
        BasicString(BasicString &&other) noexcept : size_(other.size_), is_sso_(other.is_sso_) {
            if (is_sso_) {
                std::memcpy(sso_data_, other.sso_data_, size_ + 1);
            } else {
                heap_data_ = other.heap_data_;
                capacity_ = other.capacity_;
                other.heap_data_ = nullptr;
                other.size_ = 0;
                other.is_sso_ = true;
                other.sso_data_[0] = '\0';
            }
        }

        // Destructor
        ~BasicString() {
            if (!is_sso_ && heap_data_ != nullptr) {
                delete[] heap_data_;
            }
        }

        // Copy assignment
        BasicString &operator=(BasicString const &other) {
            if (this != &other) {
                BasicString tmp(other);
                swap(tmp);
            }
            return *this;
        }

        // Move assignment
        BasicString &operator=(BasicString &&other) noexcept {
            if (this != &other) {
                BasicString tmp(std::move(other));
                swap(tmp);
            }
            return *this;
        }

        // String view conversion
        operator std::string_view() const noexcept { return {data(), size_}; }

        // std::string implicit conversion
        operator std::string() const { return std::string(data(), size_); }

        // Element access
        reference operator[](size_type pos) noexcept { return data()[pos]; }

        const_reference operator[](size_type pos) const noexcept { return data()[pos]; }

        reference at(size_type pos) {
            if (pos >= size_)
                throw std::out_of_range("BasicString::at");
            return data()[pos];
        }

        const_reference at(size_type pos) const {
            if (pos >= size_)
                throw std::out_of_range("BasicString::at");
            return data()[pos];
        }

        reference front() noexcept { return data()[0]; }

        const_reference front() const noexcept { return data()[0]; }

        reference back() noexcept { return data()[size_ - 1]; }

        const_reference back() const noexcept { return data()[size_ - 1]; }

        char *data() noexcept { return is_sso_ ? sso_data_ : heap_data_; }

        char const *data() const noexcept { return is_sso_ ? sso_data_ : heap_data_; }

        char const *c_str() const noexcept { return data(); }

        // Zero-copy view into string data (for flatsim compatibility)
        std::string_view view() const noexcept { return std::string_view(data(), size_); }

        // Capacity
        bool empty() const noexcept { return size_ == 0; }

        size_type size() const noexcept { return size_; }

        size_type length() const noexcept { return size_; }

        size_type capacity() const noexcept { return is_sso_ ? SSO_SIZE : capacity_; }

        size_type max_size() const noexcept { return std::numeric_limits<size_type>::max() - 1; }

        void shrink_to_fit() {
            if (is_sso_ || size_ == capacity_)
                return; // Already optimal

            // Reallocate to exact size + 1 for null terminator
            char *new_data = new char[size_ + 1];
            std::memcpy(new_data, data(), size_ + 1);
            delete[] heap_data_;
            heap_data_ = new_data;
            capacity_ = size_;
        }

        // Operations
        void clear() noexcept {
            if (!is_sso_ && heap_data_ != nullptr) {
                delete[] heap_data_;
                heap_data_ = nullptr;
            }
            size_ = 0;
            is_sso_ = true;
            sso_data_[0] = '\0';
        }

        void swap(BasicString &other) noexcept {
            // Low-level swap to avoid recursion with assignment operators
            // Save original modes before swapping
            bool this_is_sso = is_sso_;
            bool other_is_sso = other.is_sso_;

            // Save capacity values if needed (before union manipulation)
            size_type this_capacity = this_is_sso ? 0 : capacity_;
            size_type other_capacity = other_is_sso ? 0 : other.capacity_;

            // Swap the union data based on original modes
            if (this_is_sso && other_is_sso) {
                // Both use SSO - swap sso_data
                char temp[SSO_SIZE + 1];
                std::memcpy(temp, sso_data_, SSO_SIZE + 1);
                std::memcpy(sso_data_, other.sso_data_, SSO_SIZE + 1);
                std::memcpy(other.sso_data_, temp, SSO_SIZE + 1);
            } else if (!this_is_sso && !other_is_sso) {
                // Both use heap - swap pointers and capacity
                std::swap(heap_data_, other.heap_data_);
                std::swap(capacity_, other.capacity_);
            } else {
                // Mixed mode - need to swap union carefully
                char temp_sso[SSO_SIZE + 1];
                char *temp_heap;
                size_type temp_cap;

                if (this_is_sso) {
                    // this uses SSO, other uses heap
                    std::memcpy(temp_sso, sso_data_, SSO_SIZE + 1);
                    temp_heap = other.heap_data_;
                    temp_cap = other.capacity_;
                    heap_data_ = temp_heap;
                    capacity_ = temp_cap;
                    std::memcpy(other.sso_data_, temp_sso, SSO_SIZE + 1);
                } else {
                    // this uses heap, other uses SSO
                    std::memcpy(temp_sso, other.sso_data_, SSO_SIZE + 1);
                    temp_heap = heap_data_;
                    temp_cap = capacity_;
                    other.heap_data_ = temp_heap;
                    other.capacity_ = temp_cap;
                    std::memcpy(sso_data_, temp_sso, SSO_SIZE + 1);
                }
            }

            // Now swap the metadata (size and mode)
            std::swap(size_, other.size_);
            std::swap(is_sso_, other.is_sso_);
        }

        // Comparison
        int compare(BasicString const &other) const noexcept {
            size_type min_size = std::min(size_, other.size_);
            int result = std::memcmp(data(), other.data(), min_size);
            if (result != 0) {
                return result;
            }
            if (size_ < other.size_) {
                return -1;
            }
            if (size_ > other.size_) {
                return 1;
            }
            return 0;
        }

        int compare(size_type pos1, size_type count1, BasicString const &str) const {
            return substr(pos1, count1).compare(str);
        }

        int compare(size_type pos1, size_type count1, BasicString const &str, size_type pos2, size_type count2) const {
            return substr(pos1, count1).compare(str.substr(pos2, count2));
        }

        int compare(char const *s) const { return compare(std::string_view(s)); }

        int compare(size_type pos1, size_type count1, char const *s) const {
            return substr(pos1, count1).compare(std::string_view(s));
        }

        int compare(size_type pos1, size_type count1, char const *s, size_type count2) const {
            return substr(pos1, count1).compare(std::string_view(s, count2));
        }

        int compare(std::string_view sv) const noexcept {
            size_type min_size = std::min(size_, sv.size());
            int result = std::memcmp(data(), sv.data(), min_size);
            if (result != 0) {
                return result;
            }
            if (size_ < sv.size()) {
                return -1;
            }
            if (size_ > sv.size()) {
                return 1;
            }
            return 0;
        }

        bool operator==(BasicString const &other) const noexcept { return compare(other) == 0; }
        bool operator==(char const *other) const noexcept { return compare(std::string_view(other)) == 0; }
        bool operator==(std::string_view other) const noexcept { return compare(other) == 0; }

        bool operator!=(BasicString const &other) const noexcept { return compare(other) != 0; }
        bool operator!=(char const *other) const noexcept { return compare(std::string_view(other)) != 0; }
        bool operator!=(std::string_view other) const noexcept { return compare(other) != 0; }

        bool operator<(BasicString const &other) const noexcept { return compare(other) < 0; }
        bool operator<(char const *other) const noexcept { return compare(std::string_view(other)) < 0; }
        bool operator<(std::string_view other) const noexcept { return compare(other) < 0; }

        bool operator<=(BasicString const &other) const noexcept { return compare(other) <= 0; }
        bool operator<=(char const *other) const noexcept { return compare(std::string_view(other)) <= 0; }
        bool operator<=(std::string_view other) const noexcept { return compare(other) <= 0; }

        bool operator>(BasicString const &other) const noexcept { return compare(other) > 0; }
        bool operator>(char const *other) const noexcept { return compare(std::string_view(other)) > 0; }
        bool operator>(std::string_view other) const noexcept { return compare(other) > 0; }

        bool operator>=(BasicString const &other) const noexcept { return compare(other) >= 0; }
        bool operator>=(char const *other) const noexcept { return compare(std::string_view(other)) >= 0; }
        bool operator>=(std::string_view other) const noexcept { return compare(other) >= 0; }

        // Modifiers - assign
        BasicString &assign(BasicString const &str) {
            if (this != &str) {
                clear();
                append(str);
            }
            return *this;
        }

        BasicString &assign(BasicString const &str, size_type subpos, size_type sublen = npos) {
            clear();
            if (subpos > str.size_)
                subpos = str.size_;
            size_type count = (sublen == npos || subpos + sublen > str.size_) ? str.size_ - subpos : sublen;
            return append(str.data() + subpos, count);
        }

        BasicString &assign(char const *s) { return assign(s, std::strlen(s)); }

        BasicString &assign(char const *s, size_type count) {
            clear();
            return append(s, count);
        }

        BasicString &assign(std::string_view sv) { return assign(sv.data(), sv.size()); }

        BasicString &assign(size_type count, char ch) {
            clear();
            resize(count, ch);
            return *this;
        }

        BasicString &assign(std::initializer_list<char> ilist) {
            clear();
            reserve(ilist.size());
            for (char ch : ilist)
                push_back(ch);
            return *this;
        }

        // Modifiers - reserve and resize
        void reserve(size_type new_cap) {
            if (new_cap <= capacity()) {
                return; // Already have enough capacity
            }

            // Allocate new buffer
            char *new_data = new char[new_cap + 1];
            std::memcpy(new_data, data(), size_ + 1);

            // Clean up old data if heap
            if (!is_sso_ && heap_data_ != nullptr) {
                delete[] heap_data_;
            }

            // Switch to heap mode
            heap_data_ = new_data;
            capacity_ = new_cap;
            is_sso_ = false;
        }

        void resize(size_type count, char ch = '\0') {
            if (count < size_) {
                // Shrink
                size_ = count;
                data()[size_] = '\0';
            } else if (count > size_) {
                // Check for overflow
                if (count > max_size()) {
                    throw std::length_error("BasicString::resize would exceed max_size()");
                }
                // Grow
                reserve(count);
                std::memset(data() + size_, ch, count - size_);
                size_ = count;
                data()[size_] = '\0';
            }
        }

        // Modifiers - append
        BasicString &append(BasicString const &str) { return append(str.data(), str.size()); }

        BasicString &append(char const *s) { return append(s, std::strlen(s)); }

        BasicString &append(char const *s, size_type count) {
            if (count == 0)
                return *this;

            // Check for overflow
            if (count > max_size() - size_) {
                throw std::length_error("BasicString::append would exceed max_size()");
            }

            size_type new_size = size_ + count;
            reserve(new_size);
            std::memcpy(data() + size_, s, count);
            size_ = new_size;
            data()[size_] = '\0';
            return *this;
        }

        BasicString &append(size_type count, char ch) {
            if (count == 0)
                return *this;

            // Check for overflow
            if (count > max_size() - size_) {
                throw std::length_error("BasicString::append would exceed max_size()");
            }

            size_type new_size = size_ + count;
            reserve(new_size);
            std::memset(data() + size_, ch, count);
            size_ = new_size;
            data()[size_] = '\0';
            return *this;
        }

        BasicString &append(std::string_view sv) { return append(sv.data(), sv.size()); }

        BasicString &operator+=(BasicString const &str) { return append(str); }

        BasicString &operator+=(char const *s) { return append(s); }

        BasicString &operator+=(char ch) {
            push_back(ch);
            return *this;
        }

        BasicString &operator+=(std::string_view sv) { return append(sv); }

        // Modifiers - push/pop
        void push_back(char ch) {
            reserve(size_ + 1);
            data()[size_] = ch;
            ++size_;
            data()[size_] = '\0';
        }

        void pop_back() noexcept {
            if (size_ > 0) {
                --size_;
                data()[size_] = '\0';
            }
        }

        // Modifiers - insert
        BasicString &insert(size_type index, size_type count, char ch) {
            if (index > size_)
                index = size_;
            if (count == 0)
                return *this;

            // Check for overflow
            if (count > max_size() - size_) {
                throw std::length_error("BasicString::insert would exceed max_size()");
            }

            size_type new_size = size_ + count;
            reserve(new_size);

            // Shift existing characters right
            std::memmove(data() + index + count, data() + index, size_ - index + 1);

            // Insert new characters
            std::memset(data() + index, ch, count);
            size_ = new_size;
            return *this;
        }

        BasicString &insert(size_type index, char const *s) { return insert(index, s, std::strlen(s)); }

        BasicString &insert(size_type index, char const *s, size_type count) {
            if (index > size_)
                index = size_;
            if (count == 0)
                return *this;

            // Check for overflow
            if (count > max_size() - size_) {
                throw std::length_error("BasicString::insert would exceed max_size()");
            }

            size_type new_size = size_ + count;
            reserve(new_size);

            // Shift existing characters right
            std::memmove(data() + index + count, data() + index, size_ - index + 1);

            // Insert new characters
            std::memcpy(data() + index, s, count);
            size_ = new_size;
            return *this;
        }

        BasicString &insert(size_type index, BasicString const &str) { return insert(index, str.data(), str.size()); }

        BasicString &insert(size_type index, std::string_view sv) { return insert(index, sv.data(), sv.size()); }

        // Modifiers - erase
        BasicString &erase(size_type index = 0, size_type count = npos) {
            if (index >= size_)
                return *this;

            if (count == npos || index + count >= size_) {
                // Erase to end
                size_ = index;
                data()[size_] = '\0';
            } else {
                // Erase in middle
                std::memmove(data() + index, data() + index + count, size_ - index - count + 1);
                size_ -= count;
            }
            return *this;
        }

        // Search - find
        size_type find(BasicString const &str, size_type pos = 0) const noexcept {
            return find(str.data(), pos, str.size());
        }

        size_type find(char const *s, size_type pos = 0) const { return find(s, pos, std::strlen(s)); }

        size_type find(char const *s, size_type pos, size_type count) const noexcept {
            if (count == 0)
                return pos <= size_ ? pos : npos;
            if (pos + count > size_)
                return npos;

            char const *haystack = data();
            for (size_type i = pos; i <= size_ - count; ++i) {
                if (std::memcmp(haystack + i, s, count) == 0) {
                    return i;
                }
            }
            return npos;
        }

        size_type find(char ch, size_type pos = 0) const noexcept {
            if (pos >= size_)
                return npos;

            char const *haystack = data();
            char const *found = static_cast<char const *>(std::memchr(haystack + pos, ch, size_ - pos));
            return found ? static_cast<size_type>(found - haystack) : npos;
        }

        size_type find(std::string_view sv, size_type pos = 0) const noexcept {
            return find(sv.data(), pos, sv.size());
        }

        // Search - rfind
        size_type rfind(BasicString const &str, size_type pos = npos) const noexcept {
            return rfind(str.data(), pos, str.size());
        }

        size_type rfind(char const *s, size_type pos = npos) const { return rfind(s, pos, std::strlen(s)); }

        size_type rfind(char const *s, size_type pos, size_type count) const noexcept {
            if (count == 0)
                return pos <= size_ ? pos : size_;
            if (count > size_)
                return npos;

            size_type start_pos = std::min(pos, size_ - count);
            char const *haystack = data();

            for (size_type i = start_pos + 1; i > 0; --i) {
                if (std::memcmp(haystack + i - 1, s, count) == 0) {
                    return i - 1;
                }
            }
            return npos;
        }

        size_type rfind(char ch, size_type pos = npos) const noexcept {
            if (size_ == 0)
                return npos;

            size_type start_pos = std::min(pos, size_ - 1);
            char const *haystack = data();

            for (size_type i = start_pos + 1; i > 0; --i) {
                if (haystack[i - 1] == ch) {
                    return i - 1;
                }
            }
            return npos;
        }

        size_type rfind(std::string_view sv, size_type pos = npos) const noexcept {
            return rfind(sv.data(), pos, sv.size());
        }

        // Search - C++20/23 style
        bool contains(std::string_view sv) const noexcept { return find(sv) != npos; }

        bool contains(char ch) const noexcept { return find(ch) != npos; }

        bool contains(char const *s) const { return find(s) != npos; }

        bool starts_with(std::string_view sv) const noexcept {
            return size_ >= sv.size() && std::memcmp(data(), sv.data(), sv.size()) == 0;
        }

        bool starts_with(char ch) const noexcept { return size_ > 0 && data()[0] == ch; }

        bool starts_with(char const *s) const { return starts_with(std::string_view(s)); }

        bool ends_with(std::string_view sv) const noexcept {
            return size_ >= sv.size() && std::memcmp(data() + size_ - sv.size(), sv.data(), sv.size()) == 0;
        }

        bool ends_with(char ch) const noexcept { return size_ > 0 && data()[size_ - 1] == ch; }

        bool ends_with(char const *s) const { return ends_with(std::string_view(s)); }

        // Search - find_first_of family
        size_type find_first_of(std::string_view sv, size_type pos = 0) const noexcept {
            if (pos >= size_)
                return npos;

            char const *haystack = data();
            for (size_type i = pos; i < size_; ++i) {
                for (char ch : sv) {
                    if (haystack[i] == ch) {
                        return i;
                    }
                }
            }
            return npos;
        }

        size_type find_first_of(char const *s, size_type pos = 0) const {
            return find_first_of(std::string_view(s), pos);
        }

        size_type find_first_of(char const *s, size_type pos, size_type count) const noexcept {
            return find_first_of(std::string_view(s, count), pos);
        }

        size_type find_first_of(char ch, size_type pos = 0) const noexcept {
            return find(ch, pos); // Same as find for single char
        }

        // Search - find_last_of family
        size_type find_last_of(std::string_view sv, size_type pos = npos) const noexcept {
            if (size_ == 0)
                return npos;

            size_type start_pos = std::min(pos, size_ - 1);
            char const *haystack = data();

            for (size_type i = start_pos + 1; i > 0; --i) {
                for (char ch : sv) {
                    if (haystack[i - 1] == ch) {
                        return i - 1;
                    }
                }
            }
            return npos;
        }

        size_type find_last_of(char const *s, size_type pos = npos) const {
            return find_last_of(std::string_view(s), pos);
        }

        size_type find_last_of(char const *s, size_type pos, size_type count) const noexcept {
            return find_last_of(std::string_view(s, count), pos);
        }

        size_type find_last_of(char ch, size_type pos = npos) const noexcept {
            return rfind(ch, pos); // Same as rfind for single char
        }

        // Search - find_first_not_of family
        size_type find_first_not_of(std::string_view sv, size_type pos = 0) const noexcept {
            if (pos >= size_)
                return npos;

            char const *haystack = data();
            for (size_type i = pos; i < size_; ++i) {
                bool found = false;
                for (char ch : sv) {
                    if (haystack[i] == ch) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return i;
                }
            }
            return npos;
        }

        size_type find_first_not_of(char const *s, size_type pos = 0) const {
            return find_first_not_of(std::string_view(s), pos);
        }

        size_type find_first_not_of(char const *s, size_type pos, size_type count) const noexcept {
            return find_first_not_of(std::string_view(s, count), pos);
        }

        size_type find_first_not_of(char ch, size_type pos = 0) const noexcept {
            if (pos >= size_)
                return npos;

            char const *haystack = data();
            for (size_type i = pos; i < size_; ++i) {
                if (haystack[i] != ch) {
                    return i;
                }
            }
            return npos;
        }

        // Search - find_last_not_of family
        size_type find_last_not_of(std::string_view sv, size_type pos = npos) const noexcept {
            if (size_ == 0)
                return npos;

            size_type start_pos = std::min(pos, size_ - 1);
            char const *haystack = data();

            for (size_type i = start_pos + 1; i > 0; --i) {
                bool found = false;
                for (char ch : sv) {
                    if (haystack[i - 1] == ch) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return i - 1;
                }
            }
            return npos;
        }

        size_type find_last_not_of(char const *s, size_type pos = npos) const {
            return find_last_not_of(std::string_view(s), pos);
        }

        size_type find_last_not_of(char const *s, size_type pos, size_type count) const noexcept {
            return find_last_not_of(std::string_view(s, count), pos);
        }

        size_type find_last_not_of(char ch, size_type pos = npos) const noexcept {
            if (size_ == 0)
                return npos;

            size_type start_pos = std::min(pos, size_ - 1);
            char const *haystack = data();

            for (size_type i = start_pos + 1; i > 0; --i) {
                if (haystack[i - 1] != ch) {
                    return i - 1;
                }
            }
            return npos;
        }

        // replace() methods
        BasicString &replace(size_type pos, size_type count, BasicString const &str) {
            return replace(pos, count, str.data(), str.size());
        }

        BasicString &replace(size_type pos, size_type count, char const *s) {
            return replace(pos, count, s, std::strlen(s));
        }

        BasicString &replace(size_type pos, size_type count, char const *s, size_type count2) {
            if (pos > size_)
                pos = size_;
            if (count > size_ - pos)
                count = size_ - pos;

            // Calculate new size and check for overflow
            if (count2 > count) {
                // Growing: check if count2 - count would overflow when added to size_
                size_type delta = count2 - count;
                if (delta > max_size() - size_) {
                    throw std::length_error("BasicString::replace would exceed max_size()");
                }
            }
            size_type new_size = size_ - count + count2;

            if (count2 == count) {
                // Same size, just overwrite
                std::memcpy(data() + pos, s, count2);
            } else if (count2 < count) {
                // Replacement is smaller, overwrite and shift left
                std::memcpy(data() + pos, s, count2);
                std::memmove(data() + pos + count2, data() + pos + count, size_ - pos - count);
                size_ = new_size;
                data()[size_] = '\0';
            } else {
                // Replacement is larger, need to grow
                reserve(new_size);
                // Shift right to make room
                std::memmove(data() + pos + count2, data() + pos + count, size_ - pos - count);
                // Copy replacement
                std::memcpy(data() + pos, s, count2);
                size_ = new_size;
                data()[size_] = '\0';
            }

            return *this;
        }

        BasicString &replace(size_type pos, size_type count, size_type count2, char ch) {
            if (pos > size_)
                pos = size_;
            if (count > size_ - pos)
                count = size_ - pos;

            // Check for overflow when growing
            if (count2 > count) {
                size_type delta = count2 - count;
                if (delta > max_size() - size_) {
                    throw std::length_error("BasicString::replace would exceed max_size()");
                }
            }
            size_type new_size = size_ - count + count2;

            if (count2 == count) {
                std::memset(data() + pos, ch, count2);
            } else if (count2 < count) {
                std::memset(data() + pos, ch, count2);
                std::memmove(data() + pos + count2, data() + pos + count, size_ - pos - count);
                size_ = new_size;
                data()[size_] = '\0';
            } else {
                reserve(new_size);
                std::memmove(data() + pos + count2, data() + pos + count, size_ - pos - count);
                std::memset(data() + pos, ch, count2);
                size_ = new_size;
                data()[size_] = '\0';
            }

            return *this;
        }

        BasicString &replace(size_type pos, size_type count, std::string_view sv) {
            return replace(pos, count, sv.data(), sv.size());
        }

        // Substring
        BasicString substr(size_type pos = 0, size_type count = npos) const {
            if (pos >= size_)
                return BasicString();

            size_type actual_count = std::min(count, size_ - pos);
            return BasicString(data() + pos, actual_count);
        }

        size_type copy(char *dest, size_type count, size_type pos = 0) const {
            if (pos > size_)
                throw std::out_of_range("BasicString::copy");
            size_type copy_count = std::min(count, size_ - pos);
            std::memcpy(dest, data() + pos, copy_count);
            return copy_count;
        }

        // Formatting - string concatenation via operator+
        friend BasicString operator+(BasicString const &lhs, BasicString const &rhs) {
            BasicString result;
            result.reserve(lhs.size() + rhs.size());
            result.append(lhs);
            result.append(rhs);
            return result;
        }

        friend BasicString operator+(BasicString const &lhs, char const *rhs) {
            BasicString result;
            size_type rhs_len = std::strlen(rhs);
            result.reserve(lhs.size() + rhs_len);
            result.append(lhs);
            result.append(rhs, rhs_len);
            return result;
        }

        friend BasicString operator+(char const *lhs, BasicString const &rhs) {
            BasicString result;
            size_type lhs_len = std::strlen(lhs);
            result.reserve(lhs_len + rhs.size());
            result.append(lhs, lhs_len);
            result.append(rhs);
            return result;
        }

        friend BasicString operator+(BasicString const &lhs, char rhs) {
            BasicString result;
            result.reserve(lhs.size() + 1);
            result.append(lhs);
            result.push_back(rhs);
            return result;
        }

        friend BasicString operator+(char lhs, BasicString const &rhs) {
            BasicString result;
            result.reserve(1 + rhs.size());
            result.push_back(lhs);
            result.append(rhs);
            return result;
        }

        friend BasicString operator+(BasicString const &lhs, std::string_view rhs) {
            BasicString result;
            result.reserve(lhs.size() + rhs.size());
            result.append(lhs);
            result.append(rhs);
            return result;
        }

        friend BasicString operator+(std::string_view lhs, BasicString const &rhs) {
            BasicString result;
            result.reserve(lhs.size() + rhs.size());
            result.append(lhs);
            result.append(rhs);
            return result;
        }

        friend BasicString operator+(BasicString const &lhs, std::string const &rhs) {
            BasicString result;
            result.reserve(lhs.size() + rhs.size());
            result.append(lhs);
            result.append(rhs.data(), rhs.size());
            return result;
        }

        friend BasicString operator+(std::string const &lhs, BasicString const &rhs) {
            BasicString result;
            result.reserve(lhs.size() + rhs.size());
            result.append(lhs.data(), lhs.size());
            result.append(rhs);
            return result;
        }

        // Formatting - stream-style append operator
        BasicString &operator<<(BasicString const &str) { return append(str); }

        BasicString &operator<<(char const *str) { return append(str); }

        BasicString &operator<<(char ch) {
            push_back(ch);
            return *this;
        }

        BasicString &operator<<(std::string_view sv) { return append(sv); }

        BasicString &operator<<(int value);
        BasicString &operator<<(long value);
        BasicString &operator<<(long long value);
        BasicString &operator<<(unsigned int value);
        BasicString &operator<<(unsigned long value);
        BasicString &operator<<(unsigned long long value);
        BasicString &operator<<(float value);
        BasicString &operator<<(double value);
        BasicString &operator<<(bool value);

        // Formatting - simple {} substitution (variadic template)
        template <typename... Args> static BasicString format(char const *fmt, Args const &...args) {
            BasicString result;
            format_impl(result, fmt, args...);
            return result;
        }

      private:
        // Helper for format() - base case
        static void format_impl(BasicString &result, char const *fmt) {
            // No more arguments, append rest of format string
            result.append(fmt);
        }

        // Helper for format() - recursive case
        template <typename T, typename... Args>
        static void format_impl(BasicString &result, char const *fmt, T const &arg, Args const &...args) {
            // Find next {}
            char const *p = fmt;
            while (*p) {
                if (*p == '{' && *(p + 1) == '}') {
                    // Found {}, append everything before it
                    result.append(fmt, p - fmt);
                    // Append the argument
                    result << arg;
                    // Continue with rest of format string
                    format_impl(result, p + 2, args...);
                    return;
                }
                ++p;
            }
            // No {} found, append rest and ignore remaining args
            result.append(fmt);
        }

      public:
        // Iterators
        using iterator = char *;
        using const_iterator = char const *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        iterator begin() noexcept { return data(); }
        const_iterator begin() const noexcept { return data(); }
        const_iterator cbegin() const noexcept { return data(); }

        iterator end() noexcept { return data() + size_; }
        const_iterator end() const noexcept { return data() + size_; }
        const_iterator cend() const noexcept { return data() + size_; }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // Iterator-based insert methods
        iterator insert(const_iterator pos, char ch) {
            size_type index = static_cast<size_type>(pos - begin());
            insert(index, 1, ch);
            return begin() + index;
        }

        iterator insert(const_iterator pos, size_type count, char ch) {
            size_type index = static_cast<size_type>(pos - begin());
            insert(index, count, ch);
            return begin() + index;
        }

        iterator insert(const_iterator pos, std::initializer_list<char> ilist) {
            size_type index = static_cast<size_type>(pos - begin());
            reserve(size_ + ilist.size());
            std::memmove(data() + index + ilist.size(), data() + index, size_ - index + 1);
            size_t i = 0;
            for (char ch : ilist) {
                data()[index + i++] = ch;
            }
            size_ += ilist.size();
            return begin() + index;
        }

        template <typename InputIt> iterator insert(const_iterator pos, InputIt first, InputIt last) {
            size_type index = static_cast<size_type>(pos - begin());
            size_type count = 0;
            InputIt it = first;
            while (it != last) {
                ++count;
                ++it;
            }
            reserve(size_ + count);
            std::memmove(data() + index + count, data() + index, size_ - index + 1);
            size_t i = 0;
            it = first;
            while (it != last) {
                data()[index + i++] = *it;
                ++it;
            }
            size_ += count;
            return begin() + index;
        }

        // Iterator-based erase methods
        iterator erase(const_iterator pos) {
            size_type index = static_cast<size_type>(pos - begin());
            erase(index, 1);
            return begin() + index;
        }

        iterator erase(const_iterator first, const_iterator last) {
            size_type index = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            erase(index, count);
            return begin() + index;
        }

        // Iterator-based replace methods
        BasicString &replace(const_iterator first, const_iterator last, BasicString const &str) {
            size_type pos = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            return replace(pos, count, str.data(), str.size());
        }

        BasicString &replace(const_iterator first, const_iterator last, char const *s, size_type count2) {
            size_type pos = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            return replace(pos, count, s, count2);
        }

        BasicString &replace(const_iterator first, const_iterator last, char const *s) {
            size_type pos = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            return replace(pos, count, s, std::strlen(s));
        }

        BasicString &replace(const_iterator first, const_iterator last, size_type count2, char ch) {
            size_type pos = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            return replace(pos, count, count2, ch);
        }

        BasicString &replace(const_iterator first, const_iterator last, std::string_view sv) {
            size_type pos = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            return replace(pos, count, sv.data(), sv.size());
        }

        BasicString &replace(const_iterator first, const_iterator last, std::initializer_list<char> ilist) {
            size_type pos = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            erase(pos, count);
            insert(begin() + pos, ilist);
            return *this;
        }

        template <typename InputIt>
        BasicString &replace(const_iterator first, const_iterator last, InputIt first2, InputIt last2) {
            size_type pos = static_cast<size_type>(first - begin());
            size_type count = static_cast<size_type>(last - first);
            erase(pos, count);
            insert(begin() + pos, first2, last2);
            return *this;
        }

        // Convenience methods - whitespace handling
        BasicString &trim() { return trim_right().trim_left(); }

        BasicString &trim_left() {
            size_type start = find_first_not_of(" \t\n\r\f\v");
            if (start == npos) {
                clear();
            } else if (start > 0) {
                erase(0, start);
            }
            return *this;
        }

        BasicString &trim_right() {
            size_type end = find_last_not_of(" \t\n\r\f\v");
            if (end == npos) {
                clear();
            } else if (end + 1 < size_) {
                erase(end + 1);
            }
            return *this;
        }

        BasicString trimmed() const {
            BasicString result(*this);
            result.trim();
            return result;
        }

        BasicString trimmed_left() const {
            BasicString result(*this);
            result.trim_left();
            return result;
        }

        BasicString trimmed_right() const {
            BasicString result(*this);
            result.trim_right();
            return result;
        }

        // Convenience methods - case conversion
        BasicString &to_lower() {
            for (size_type i = 0; i < size_; ++i) {
                if (data()[i] >= 'A' && data()[i] <= 'Z') {
                    data()[i] = static_cast<char>(data()[i] + 32);
                }
            }
            return *this;
        }

        BasicString &to_upper() {
            for (size_type i = 0; i < size_; ++i) {
                if (data()[i] >= 'a' && data()[i] <= 'z') {
                    data()[i] = static_cast<char>(data()[i] - 32);
                }
            }
            return *this;
        }

        BasicString to_lower_copy() const {
            BasicString result(*this);
            result.to_lower();
            return result;
        }

        BasicString to_upper_copy() const {
            BasicString result(*this);
            result.to_upper();
            return result;
        }

        // Convenience methods - repeat and reverse
        BasicString &repeat(size_type count) {
            if (count <= 1)
                return *this;
            if (count == 0) {
                clear();
                return *this;
            }

            BasicString original(*this);
            for (size_type i = 1; i < count; ++i) {
                append(original);
            }
            return *this;
        }

        static BasicString repeated(BasicString const &str, size_type count) {
            BasicString result;
            result.reserve(str.size() * count);
            for (size_type i = 0; i < count; ++i) {
                result.append(str);
            }
            return result;
        }

        BasicString &reverse() {
            for (size_type i = 0; i < size_ / 2; ++i) {
                std::swap(data()[i], data()[size_ - 1 - i]);
            }
            return *this;
        }

        BasicString reversed() const {
            BasicString result(*this);
            result.reverse();
            return result;
        }

        // Convenience methods - truncate and pad
        BasicString &truncate(size_type max_len, char ellipsis = '\0') {
            if (size_ > max_len) {
                if (ellipsis != '\0' && max_len >= 3) {
                    resize(max_len);
                    data()[max_len - 3] = '.';
                    data()[max_len - 2] = '.';
                    data()[max_len - 1] = '.';
                } else {
                    resize(max_len);
                }
            }
            return *this;
        }

        BasicString &pad_left(size_type total_width, char fill_char = ' ') {
            if (size_ >= total_width)
                return *this;
            insert(0, total_width - size_, fill_char);
            return *this;
        }

        BasicString &pad_right(size_type total_width, char fill_char = ' ') {
            if (size_ >= total_width)
                return *this;
            append(total_width - size_, fill_char);
            return *this;
        }

        // Convenience methods - capitalize and first/last char access helpers
        BasicString &capitalize() {
            if (!empty()) {
                to_lower();
                if (data()[0] >= 'a' && data()[0] <= 'z') {
                    data()[0] = static_cast<char>(data()[0] - 32);
                }
            }
            return *this;
        }

        BasicString capitalized() const {
            BasicString result(*this);
            result.capitalize();
            return result;
        }

        // Convenience methods - slice (Python-style)
        BasicString slice(int64_t start, int64_t end = std::numeric_limits<int64_t>::max()) const {
            // Handle negative indices
            int64_t adj_start = start;
            int64_t adj_end = end;

            if (adj_start < 0)
                adj_start += static_cast<int64_t>(size_);
            if (adj_end < 0)
                adj_end += static_cast<int64_t>(size_);

            // Clamp to valid range
            if (adj_start < 0)
                adj_start = 0;
            if (adj_end < 0)
                adj_end = 0;
            if (adj_start > static_cast<int64_t>(size_))
                adj_start = static_cast<int64_t>(size_);
            if (adj_end > static_cast<int64_t>(size_))
                adj_end = static_cast<int64_t>(size_);

            if (adj_start >= adj_end)
                return BasicString();

            return BasicString(data() + adj_start, static_cast<size_type>(adj_end - adj_start));
        }

        // Convenience methods - chomp (remove trailing newline)
        BasicString &chomp() {
            while (!empty() && (back() == '\n' || back() == '\r')) {
                pop_back();
            }
            return *this;
        }

        BasicString chomped() const {
            BasicString result(*this);
            result.chomp();
            return result;
        }

        // Convenience methods - count occurrences
        size_type count(std::string_view sv) const noexcept {
            if (sv.empty())
                return 0;

            size_type result = 0;
            size_type pos = 0;
            while ((pos = find(sv, pos)) != npos) {
                ++result;
                pos += sv.size();
            }
            return result;
        }

        size_type count(char ch) const noexcept {
            size_type result = 0;
            for (size_type i = 0; i < size_; ++i) {
                if (data()[i] == ch)
                    ++result;
            }
            return result;
        }

        // Convenience methods - join static method (for arrays of strings)
        template <typename It> static BasicString join(It first, It last, std::string_view separator = "") {
            BasicString result;
            if (first != last) {
                result.append(*first);
                ++first;
                for (; first != last; ++first) {
                    result.append(separator);
                    result.append(*first);
                }
            }
            return result;
        }

        // Convenience methods - split by delimiter (returns Vector<BasicString>)
        // Note: This requires Vector to be defined. Placeholder for now.
        // Vector<BasicString> split(std::string_view delim) const;

        // Stream output
        friend std::ostream &operator<<(std::ostream &os, BasicString const &str) {
            return os << std::string_view(str.data(), str.size());
        }

        // Reflection support - expose only non-union members to avoid UB.
        // The union-based SSO means only one of sso_data_ or heap_data_/capacity_
        // is active at a time. Exposing inactive union members via std::tie() is UB.
        // String content serialization is handled by specialized functions in
        // datapod/serialization/serialize.hpp that properly use size()/data().
        auto members() noexcept { return std::tie(size_, is_sso_); }
        auto members() const noexcept { return std::tie(size_, is_sso_); }

      private:
        size_type size_;
        bool is_sso_;

        union {
            char sso_data_[SSO_SIZE + 1]; // +1 for null terminator
            __extension__ struct {
                char *heap_data_;
                size_type capacity_;
            };
        };
    };

    // to_string() conversions for numeric types (zero-dependency implementation)
    namespace detail {
        // Helper to convert unsigned integer to string
        template <typename UInt> inline void uint_to_chars(char *buf, size_t &len, UInt value) {
            if (value == 0) {
                buf[0] = '0';
                len = 1;
                return;
            }

            char temp[64];
            size_t i = 0;
            while (value > 0) {
                temp[i++] = '0' + (value % 10);
                value /= 10;
            }

            len = i;
            for (size_t j = 0; j < i; ++j) {
                buf[j] = temp[i - 1 - j];
            }
        }

        // Helper to convert signed integer to string
        template <typename Int> inline void int_to_chars(char *buf, size_t &len, Int value) {
            if (value < 0) {
                buf[0] = '-';
                using UInt = typename std::make_unsigned<Int>::type;
                uint_to_chars(buf + 1, len, static_cast<UInt>(-value));
                ++len;
            } else {
                using UInt = typename std::make_unsigned<Int>::type;
                uint_to_chars(buf, len, static_cast<UInt>(value));
            }
        }

        // Helper to convert floating point to string (simple implementation)
        inline void float_to_chars(char *buf, size_t &len, double value, int precision = 6) {
            // Handle special cases
            if (value != value) { // NaN
                std::memcpy(buf, "nan", 3);
                len = 3;
                return;
            }
            if (value == std::numeric_limits<double>::infinity()) {
                std::memcpy(buf, "inf", 3);
                len = 3;
                return;
            }
            if (value == -std::numeric_limits<double>::infinity()) {
                std::memcpy(buf, "-inf", 4);
                len = 4;
                return;
            }

            // Handle sign
            size_t pos = 0;
            if (value < 0) {
                buf[pos++] = '-';
                value = -value;
            }

            // Get integer and fractional parts
            long long int_part = static_cast<long long>(value);
            double frac_part = value - int_part;

            // Convert integer part
            size_t int_len;
            int_to_chars(buf + pos, int_len, int_part);
            pos += int_len;

            // Add decimal point and fractional part
            if (precision > 0) {
                buf[pos++] = '.';
                for (int i = 0; i < precision; ++i) {
                    frac_part *= 10;
                    int digit = static_cast<int>(frac_part);
                    buf[pos++] = '0' + digit;
                    frac_part -= digit;
                }
            }

            len = pos;
        }
    } // namespace detail

    // to_string() for integral types
    template <typename Ptr = char *> inline BasicString<Ptr> to_string(int value) {
        char buf[64];
        size_t len;
        detail::int_to_chars(buf, len, value);
        return BasicString<Ptr>(buf, len);
    }

    template <typename Ptr = char *> inline BasicString<Ptr> to_string(long value) {
        char buf[64];
        size_t len;
        detail::int_to_chars(buf, len, value);
        return BasicString<Ptr>(buf, len);
    }

    template <typename Ptr = char *> inline BasicString<Ptr> to_string(long long value) {
        char buf[64];
        size_t len;
        detail::int_to_chars(buf, len, value);
        return BasicString<Ptr>(buf, len);
    }

    template <typename Ptr = char *> inline BasicString<Ptr> to_string(unsigned int value) {
        char buf[64];
        size_t len;
        detail::uint_to_chars(buf, len, value);
        return BasicString<Ptr>(buf, len);
    }

    template <typename Ptr = char *> inline BasicString<Ptr> to_string(unsigned long value) {
        char buf[64];
        size_t len;
        detail::uint_to_chars(buf, len, value);
        return BasicString<Ptr>(buf, len);
    }

    template <typename Ptr = char *> inline BasicString<Ptr> to_string(unsigned long long value) {
        char buf[64];
        size_t len;
        detail::uint_to_chars(buf, len, value);
        return BasicString<Ptr>(buf, len);
    }

    // to_string() for floating point types
    template <typename Ptr = char *> inline BasicString<Ptr> to_string(float value) {
        char buf[128];
        size_t len;
        detail::float_to_chars(buf, len, static_cast<double>(value), 6);
        return BasicString<Ptr>(buf, len);
    }

    template <typename Ptr = char *> inline BasicString<Ptr> to_string(double value) {
        char buf[128];
        size_t len;
        detail::float_to_chars(buf, len, value, 6);
        return BasicString<Ptr>(buf, len);
    }

    // to_string() for bool
    template <typename Ptr = char *> inline BasicString<Ptr> to_string(bool value) {
        return value ? BasicString<Ptr>("true", 4) : BasicString<Ptr>("false", 5);
    }

    // to_string() for char (single character string)
    template <typename Ptr = char *> inline BasicString<Ptr> to_string(char value) {
        return BasicString<Ptr>(&value, 1);
    }

    // Implementation of operator<< for numeric types (outside class definition)
    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(int value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(long value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(long long value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(unsigned int value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(unsigned long value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(unsigned long long value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(float value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(double value) {
        return append(to_string<Ptr>(value));
    }

    template <typename Ptr> BasicString<Ptr> &BasicString<Ptr>::operator<<(bool value) {
        return append(to_string<Ptr>(value));
    }

    using String = BasicString<char *>;

    namespace seq_string {
        /// Placeholder for template container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace seq_string

} // namespace datapod

// std::hash specialization for datapod::String
namespace std {
    template <> struct hash<datapod::String> {
        size_t operator()(datapod::String const &str) const noexcept {
            // Use FNV-1a hash algorithm (fast and good distribution)
            size_t hash = 14695981039346656037ULL; // FNV offset basis
            char const *data = str.data();
            size_t size = str.size();

            for (size_t i = 0; i < size; ++i) {
                hash ^= static_cast<size_t>(static_cast<unsigned char>(data[i]));
                hash *= 1099511628211ULL; // FNV prime
            }

            return hash;
        }
    };

    template <typename Ptr> struct hash<datapod::BasicString<Ptr>> {
        size_t operator()(datapod::BasicString<Ptr> const &str) const noexcept {
            size_t hash = 14695981039346656037ULL;
            char const *data = str.data();
            size_t size = str.size();

            for (size_t i = 0; i < size; ++i) {
                hash ^= static_cast<size_t>(static_cast<unsigned char>(data[i]));
                hash *= 1099511628211ULL;
            }

            return hash;
        }
    };
} // namespace std
