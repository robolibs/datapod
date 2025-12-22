#pragma once

#include <algorithm>
#include <cstring>
#include <ostream>
#include <string_view>

namespace datapod {

    // Basic string with Small String Optimization (SSO)
    // Stores up to 23 characters inline before allocating
    template <typename Ptr = char *> class BasicString {
      public:
        using value_type = char;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
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
                capacity_ = len + 1;
                heap_data_ = new char[capacity_];
                std::memcpy(heap_data_, str, len);
                heap_data_[len] = '\0';
            }
        }

        // Constructor from string_view
        BasicString(std::string_view sv) : BasicString(sv.data(), sv.size()) {}

        // Copy constructor
        BasicString(BasicString const &other) : size_(other.size_), is_sso_(other.is_sso_) {
            if (is_sso_) {
                std::memcpy(sso_data_, other.sso_data_, size_ + 1);
            } else {
                capacity_ = other.capacity_;
                heap_data_ = new char[capacity_];
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

        // Element access
        reference operator[](size_type pos) noexcept { return data()[pos]; }

        const_reference operator[](size_type pos) const noexcept { return data()[pos]; }

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

        bool operator==(BasicString const &other) const noexcept { return compare(other) == 0; }

        bool operator!=(BasicString const &other) const noexcept { return compare(other) != 0; }

        bool operator<(BasicString const &other) const noexcept { return compare(other) < 0; }

        bool operator<=(BasicString const &other) const noexcept { return compare(other) <= 0; }

        bool operator>(BasicString const &other) const noexcept { return compare(other) > 0; }

        bool operator>=(BasicString const &other) const noexcept { return compare(other) >= 0; }

        // Stream output
        friend std::ostream &operator<<(std::ostream &os, BasicString const &str) {
            return os << std::string_view(str.data(), str.size());
        }

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

    using String = BasicString<char *>;

} // namespace datapod
