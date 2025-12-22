#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <cstdint>
#include <limits>
#include <string_view>

#include "datapod/core/next_power_of_2.hpp"
#include "datapod/core/verify.hpp"

namespace datapod {

    // Memory-mapped file wrapper
    struct Mmap {
        static constexpr auto const OFFSET = 0ULL;
        static constexpr auto const ENTIRE_FILE = std::numeric_limits<std::size_t>::max();

        enum class Protection { READ, WRITE, MODIFY };

        Mmap() = default;

        explicit Mmap(char const *path, Protection const prot = Protection::WRITE) : prot_{prot} {
            open_file(path);
            size_ = file_size();
            used_size_ = size_;
            if (size_ != 0U) {
                addr_ = map();
            }
        }

        ~Mmap() {
            if (addr_ != nullptr) {
                sync();
                size_ = used_size_;
                unmap();
                if (size_ != file_size()) {
                    resize_file();
                }
            }
            close_file();
        }

        Mmap(Mmap const &) = delete;
        Mmap &operator=(Mmap const &) = delete;

        Mmap(Mmap &&o) noexcept
            : prot_{o.prot_}, size_{o.size_}, used_size_{o.used_size_}, addr_{o.addr_}
#ifdef _WIN32
              ,
              file_handle_{o.file_handle_}, file_mapping_{o.file_mapping_}
#else
              ,
              fd_{o.fd_}
#endif
        {
            o.addr_ = nullptr;
#ifdef _WIN32
            o.file_handle_ = INVALID_HANDLE_VALUE;
            o.file_mapping_ = nullptr;
#else
            o.fd_ = -1;
#endif
        }

        Mmap &operator=(Mmap &&o) noexcept {
            if (this != &o) {
                if (addr_ != nullptr) {
                    sync();
                    unmap();
                }
                close_file();

                prot_ = o.prot_;
                size_ = o.size_;
                used_size_ = o.used_size_;
                addr_ = o.addr_;
#ifdef _WIN32
                file_handle_ = o.file_handle_;
                file_mapping_ = o.file_mapping_;
                o.file_handle_ = INVALID_HANDLE_VALUE;
                o.file_mapping_ = nullptr;
#else
                fd_ = o.fd_;
                o.fd_ = -1;
#endif
                o.addr_ = nullptr;
            }
            return *this;
        }

        void sync() {
            if ((prot_ == Protection::WRITE || prot_ == Protection::MODIFY) && addr_ != nullptr) {
#ifdef _WIN32
                verify(::FlushViewOfFile(addr_, size_) != 0, "flush error");
                verify(::FlushFileBuffers(file_handle_) != 0, "flush error");
#else
                verify(::msync(addr_, size_, MS_SYNC) == 0, "sync error");
#endif
            }
        }

        void resize(std::size_t const new_size) {
            verify(prot_ == Protection::WRITE || prot_ == Protection::MODIFY, "read-only not resizable");
            if (size_ < new_size) {
                resize_map(next_power_of_two(new_size));
            }
            used_size_ = new_size;
        }

        void reserve(std::size_t const new_size) {
            verify(prot_ == Protection::WRITE || prot_ == Protection::MODIFY, "read-only not resizable");
            if (size_ < new_size) {
                resize_map(next_power_of_two(new_size));
            }
        }

        std::size_t size() const noexcept { return used_size_; }

        std::string_view view() const noexcept { return {static_cast<char const *>(addr_), size()}; }

        std::uint8_t *data() noexcept { return static_cast<std::uint8_t *>(addr_); }
        std::uint8_t const *data() const noexcept { return static_cast<std::uint8_t const *>(addr_); }

        std::uint8_t *begin() noexcept { return data(); }
        std::uint8_t *end() noexcept { return data() + used_size_; }
        std::uint8_t const *begin() const noexcept { return data(); }
        std::uint8_t const *end() const noexcept { return data() + used_size_; }

        std::uint8_t &operator[](std::size_t const i) noexcept { return data()[i]; }
        std::uint8_t const &operator[](std::size_t const i) const noexcept { return data()[i]; }

      private:
        void open_file(char const *path) {
#ifdef _WIN32
            DWORD access = GENERIC_READ;
            DWORD share = FILE_SHARE_READ;
            DWORD creation = OPEN_EXISTING;

            if (prot_ == Protection::WRITE) {
                access = GENERIC_READ | GENERIC_WRITE;
                creation = CREATE_ALWAYS;
            } else if (prot_ == Protection::MODIFY) {
                access = GENERIC_READ | GENERIC_WRITE;
                creation = OPEN_ALWAYS;
            }

            file_handle_ = ::CreateFileA(path, access, share, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
            verify(file_handle_ != INVALID_HANDLE_VALUE, "open file error");
#else
            int flags = O_RDONLY;
            if (prot_ == Protection::WRITE) {
                flags = O_RDWR | O_CREAT | O_TRUNC;
            } else if (prot_ == Protection::MODIFY) {
                flags = O_RDWR | O_CREAT;
            }
            fd_ = ::open(path, flags, 0644);
            verify(fd_ != -1, "open file error");
#endif
        }

        void close_file() {
#ifdef _WIN32
            if (file_handle_ != INVALID_HANDLE_VALUE) {
                ::CloseHandle(file_handle_);
                file_handle_ = INVALID_HANDLE_VALUE;
            }
#else
            if (fd_ != -1) {
                ::close(fd_);
                fd_ = -1;
            }
#endif
        }

        std::size_t file_size() const {
#ifdef _WIN32
            LARGE_INTEGER size;
            if (::GetFileSizeEx(file_handle_, &size)) {
                return static_cast<std::size_t>(size.QuadPart);
            }
            return 0;
#else
            struct stat st;
            if (::fstat(fd_, &st) == 0) {
                return static_cast<std::size_t>(st.st_size);
            }
            return 0;
#endif
        }

        void unmap() {
#ifdef _WIN32
            if (addr_ != nullptr) {
                verify(::UnmapViewOfFile(addr_), "unmap error");
                addr_ = nullptr;

                if (file_mapping_ != nullptr) {
                    verify(::CloseHandle(file_mapping_), "close file mapping error");
                    file_mapping_ = nullptr;
                }
            }
#else
            if (addr_ != nullptr) {
                ::munmap(addr_, size_);
                addr_ = nullptr;
            }
#endif
        }

        void *map() {
#ifdef _WIN32
            auto const size_low = static_cast<DWORD>(size_);
#ifdef _WIN64
            auto const size_high = static_cast<DWORD>(size_ >> 32U);
#else
            auto const size_high = static_cast<DWORD>(0U);
#endif
            const auto fm =
                ::CreateFileMapping(file_handle_, nullptr, prot_ == Protection::READ ? PAGE_READONLY : PAGE_READWRITE,
                                    size_high, size_low, nullptr);
            verify(fm != nullptr, "file mapping error");
            file_mapping_ = fm;

            auto const addr =
                ::MapViewOfFile(fm, prot_ == Protection::READ ? FILE_MAP_READ : FILE_MAP_WRITE, OFFSET, OFFSET, size_);
            verify(addr != nullptr, "map error");

            return addr;
#else
            auto const addr = ::mmap(nullptr, size_, prot_ == Protection::READ ? PROT_READ : PROT_READ | PROT_WRITE,
                                     MAP_SHARED, fd_, OFFSET);
            verify(addr != MAP_FAILED, "map error");
            return addr;
#endif
        }

        void resize_file() {
            if (prot_ == Protection::READ) {
                return;
            }

#ifdef _WIN32
            LARGE_INTEGER distance;
            distance.QuadPart = static_cast<LONGLONG>(size_);
            verify(::SetFilePointerEx(file_handle_, distance, nullptr, FILE_BEGIN), "resize error");
            verify(::SetEndOfFile(file_handle_), "resize set eof error");
#else
            verify(::ftruncate(fd_, static_cast<off_t>(size_)) == 0, "resize error");
#endif
        }

        void resize_map(std::size_t const new_size) {
            if (prot_ == Protection::READ) {
                return;
            }

            unmap();
            size_ = new_size;
            resize_file();
            addr_ = map();
        }

        Protection prot_{Protection::READ};
        std::size_t size_{0};
        std::size_t used_size_{0};
        void *addr_{nullptr};

#ifdef _WIN32
        HANDLE file_handle_{INVALID_HANDLE_VALUE};
        HANDLE file_mapping_{nullptr};
#else
        int fd_{-1};
#endif
    };

} // namespace datapod
