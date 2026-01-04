#pragma once
#include <datapod/types/types.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "datapod/core/verify.hpp"

namespace datapod {

    // Simple memory buffer with RAII semantics
    struct Buffer final {
        constexpr Buffer() noexcept : buf_(nullptr), size_(0U) {}

        explicit Buffer(datapod::usize const size) : buf_(std::malloc(size)), size_(size) {
            verify(buf_ != nullptr, "buffer initialization failed");
        }

        explicit Buffer(char const *str) : Buffer(std::strlen(str)) { std::memcpy(buf_, str, size_); }

        Buffer(char const *str, datapod::usize size) : Buffer(size) { std::memcpy(buf_, str, size_); }

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

        datapod::usize size() const noexcept { return size_; }

        datapod::u8 *data() noexcept { return static_cast<datapod::u8 *>(buf_); }
        datapod::u8 const *data() const noexcept { return static_cast<datapod::u8 const *>(buf_); }

        datapod::u8 *begin() noexcept { return data(); }
        datapod::u8 *end() noexcept { return data() + size_; }

        datapod::u8 const *begin() const noexcept { return data(); }
        datapod::u8 const *end() const noexcept { return data() + size_; }

        datapod::u8 &operator[](datapod::usize const i) noexcept { return data()[i]; }
        datapod::u8 const &operator[](datapod::usize const i) const noexcept { return data()[i]; }

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
        datapod::usize size_;
    };

} // namespace datapod
