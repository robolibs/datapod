#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "datagram/core/verify.hpp"

namespace datagram {

    // Simple memory buffer with RAII semantics
    struct Buffer final {
        constexpr Buffer() noexcept : buf_(nullptr), size_(0U) {}

        explicit Buffer(std::size_t const size) : buf_(std::malloc(size)), size_(size) {
            verify(buf_ != nullptr, "buffer initialization failed");
        }

        explicit Buffer(char const *str) : Buffer(std::strlen(str)) { std::memcpy(buf_, str, size_); }

        Buffer(char const *str, std::size_t size) : Buffer(size) { std::memcpy(buf_, str, size_); }

        ~Buffer() { free(); }

        Buffer(Buffer const &) = delete;
        Buffer &operator=(Buffer const &) = delete;

        Buffer(Buffer &&o) noexcept : buf_(o.buf_), size_(o.size_) {
            if (&o != this) {
                o.reset();
            }
        }

        Buffer &operator=(Buffer &&o) noexcept {
            if (&o == this) {
                return *this;
            }
            if (buf_ != nullptr) {
                free();
            }
            buf_ = o.buf_;
            size_ = o.size_;
            o.reset();
            return *this;
        }

        std::size_t size() const noexcept { return size_; }

        std::uint8_t *data() noexcept { return static_cast<std::uint8_t *>(buf_); }
        std::uint8_t const *data() const noexcept { return static_cast<std::uint8_t const *>(buf_); }

        std::uint8_t *begin() noexcept { return data(); }
        std::uint8_t *end() noexcept { return data() + size_; }

        std::uint8_t const *begin() const noexcept { return data(); }
        std::uint8_t const *end() const noexcept { return data() + size_; }

        std::uint8_t &operator[](std::size_t const i) noexcept { return data()[i]; }
        std::uint8_t const &operator[](std::size_t const i) const noexcept { return data()[i]; }

        void reset() noexcept {
            buf_ = nullptr;
            size_ = 0U;
        }

        void free() noexcept {
            if (buf_ != nullptr) {
                std::free(buf_);
                reset();
            }
        }

        void *buf_;
        std::size_t size_;
    };

} // namespace datagram
