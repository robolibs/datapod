#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>
#include <type_traits>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <datapod/adapters/error.hpp>
#include <datapod/adapters/result.hpp>
#include <datapod/sequential/string.hpp>
#include <datapod/sequential/vector.hpp>

namespace datapod {

    struct SPSC {};
    struct MPMC {};
    struct SPMC {};

    template <typename Policy, typename T> class RingBuffer;

    template <typename T> class RingBuffer<SPSC, T> {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable for RingBuffer");

      public:
        RingBuffer() noexcept
            : header_(nullptr), buffer_(nullptr), owns_memory_(false), is_shm_(false), shm_fd_(-1), shm_size_(0) {}
        explicit RingBuffer(size_t capacity);
        static Result<RingBuffer, Error> create_shm(const String &name, size_t capacity);
        static Result<RingBuffer, Error> attach_shm(const String &name);
        ~RingBuffer();

        RingBuffer(RingBuffer &&other) noexcept;
        RingBuffer &operator=(RingBuffer &&other) noexcept;
        RingBuffer(const RingBuffer &) = delete;
        RingBuffer &operator=(const RingBuffer &) = delete;

        inline Result<bool, Error> push(const T &item);
        inline Result<bool, Error> push(T &&item);

        template <typename... Args> inline Result<bool, Error> emplace(Args &&...args);

        inline Result<T, Error> pop();
        inline Result<const T *, Error> peek() const;

        inline bool empty() const noexcept;
        inline bool full() const noexcept;
        inline size_t size() const noexcept;
        inline size_t capacity() const noexcept;

        struct Snapshot {
            uint64_t write_pos;
            uint64_t read_pos;
            uint64_t capacity;
            uint32_t magic;
            uint32_t version;

            auto members() noexcept { return std::tie(write_pos, read_pos, capacity, magic, version); }
            auto members() const noexcept { return std::tie(write_pos, read_pos, capacity, magic, version); }
        };

        inline Snapshot snapshot() const noexcept;
        inline auto members() const noexcept;

        struct SnapshotWithData {
            uint64_t write_pos;
            uint64_t read_pos;
            uint64_t capacity;
            uint32_t magic;
            uint32_t version;
            Vector<T> data;

            auto members() noexcept { return std::tie(write_pos, read_pos, capacity, magic, version, data); }
            auto members() const noexcept { return std::tie(write_pos, read_pos, capacity, magic, version, data); }
        };

        inline SnapshotWithData snapshot_with_data() const;
        static Result<RingBuffer, Error> from_snapshot(const SnapshotWithData &snap);
        inline Vector<T> drain();

      private:
        struct alignas(64) Header {
            std::atomic<uint64_t> write_pos;
            uint8_t padding1[64 - sizeof(std::atomic<uint64_t>)];

            std::atomic<uint64_t> read_pos;
            uint8_t padding2[64 - sizeof(std::atomic<uint64_t>)];

            uint64_t capacity;
            uint32_t magic;
            uint32_t version;

            Header() : write_pos(0), read_pos(0), capacity(0), magic(0x53505343), version(1) {}
        };

        Header *header_;
        T *buffer_;
        bool owns_memory_;
        bool is_shm_;
        int shm_fd_;
        size_t shm_size_;
        String shm_name_;

        static inline size_t calculate_shm_size(size_t capacity) noexcept {
            return sizeof(Header) + capacity * sizeof(T);
        }

        inline void init_header(size_t capacity) noexcept {
            header_->write_pos.store(0, std::memory_order_relaxed);
            header_->read_pos.store(0, std::memory_order_relaxed);
            header_->capacity = capacity;
            header_->magic = 0x53505343;
            header_->version = 1;
        }

        inline bool verify_header() const noexcept { return header_->magic == 0x53505343 && header_->version == 1; }
    };

    // ============================================================================
    // IMPLEMENTATION
    // ============================================================================

    template <typename T>
    RingBuffer<SPSC, T>::RingBuffer(size_t capacity) : owns_memory_(true), is_shm_(false), shm_fd_(-1), shm_size_(0) {
        if (capacity == 0)
            capacity = 1;

        size_t total_size = calculate_shm_size(capacity);
        void *mem = std::aligned_alloc(64, total_size);
        if (!mem) {
            header_ = nullptr;
            buffer_ = nullptr;
            return;
        }

        header_ = new (mem) Header();
        buffer_ = reinterpret_cast<T *>(static_cast<uint8_t *>(mem) + sizeof(Header));
        init_header(capacity);
    }

    template <typename T>
    Result<RingBuffer<SPSC, T>, Error> RingBuffer<SPSC, T>::create_shm(const String &name, size_t capacity) {
        if (capacity == 0) {
            return Result<RingBuffer, Error>::err(Error::invalid_argument("Capacity must be > 0"));
        }

        if (name.empty() || name[0] != '/') {
            return Result<RingBuffer, Error>::err(Error::invalid_argument("Shared memory name must start with '/'"));
        }

        size_t total_size = calculate_shm_size(capacity);
        int fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
        if (fd < 0) {
            if (errno == EEXIST) {
                return Result<RingBuffer, Error>::err(Error::already_exists("Shared memory already exists"));
            }
            return Result<RingBuffer, Error>::err(Error::io_error("shm_open failed"));
        }

        if (ftruncate(fd, total_size) < 0) {
            close(fd);
            shm_unlink(name.c_str());
            return Result<RingBuffer, Error>::err(Error::io_error("ftruncate failed"));
        }

        void *addr = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            close(fd);
            shm_unlink(name.c_str());
            return Result<RingBuffer, Error>::err(Error::io_error("mmap failed"));
        }

        RingBuffer ring;
        ring.header_ = new (addr) Header();
        ring.buffer_ = reinterpret_cast<T *>(static_cast<uint8_t *>(addr) + sizeof(Header));
        ring.owns_memory_ = true;
        ring.is_shm_ = true;
        ring.shm_fd_ = fd;
        ring.shm_size_ = total_size;
        ring.shm_name_ = name;
        ring.init_header(capacity);

        return Result<RingBuffer, Error>::ok(std::move(ring));
    }

    template <typename T> Result<RingBuffer<SPSC, T>, Error> RingBuffer<SPSC, T>::attach_shm(const String &name) {
        if (name.empty() || name[0] != '/') {
            return Result<RingBuffer, Error>::err(Error::invalid_argument("Shared memory name must start with '/'"));
        }

        int fd = shm_open(name.c_str(), O_RDWR, 0666);
        if (fd < 0) {
            return Result<RingBuffer, Error>::err(Error::not_found("Shared memory not found"));
        }

        struct stat st;
        if (fstat(fd, &st) < 0) {
            close(fd);
            return Result<RingBuffer, Error>::err(Error::io_error("fstat failed"));
        }
        size_t total_size = st.st_size;

        void *addr = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            close(fd);
            return Result<RingBuffer, Error>::err(Error::io_error("mmap failed"));
        }

        RingBuffer ring;
        ring.header_ = static_cast<Header *>(addr);
        ring.buffer_ = reinterpret_cast<T *>(static_cast<uint8_t *>(addr) + sizeof(Header));
        ring.owns_memory_ = false;
        ring.is_shm_ = true;
        ring.shm_fd_ = fd;
        ring.shm_size_ = total_size;
        ring.shm_name_ = name;

        if (!ring.verify_header()) {
            munmap(addr, total_size);
            close(fd);
            return Result<RingBuffer, Error>::err(
                Error::invalid_argument("Invalid ring buffer header (magic mismatch)"));
        }

        return Result<RingBuffer, Error>::ok(std::move(ring));
    }

    template <typename T> RingBuffer<SPSC, T>::~RingBuffer() {
        if (is_shm_) {
            if (header_)
                munmap(header_, shm_size_);
            if (shm_fd_ >= 0)
                close(shm_fd_);
            if (owns_memory_ && !shm_name_.empty())
                shm_unlink(shm_name_.c_str());
        } else {
            if (header_)
                std::free(header_);
        }
    }

    template <typename T>
    RingBuffer<SPSC, T>::RingBuffer(RingBuffer &&other) noexcept
        : header_(other.header_), buffer_(other.buffer_), owns_memory_(other.owns_memory_), is_shm_(other.is_shm_),
          shm_fd_(other.shm_fd_), shm_size_(other.shm_size_), shm_name_(std::move(other.shm_name_)) {
        other.header_ = nullptr;
        other.buffer_ = nullptr;
        other.shm_fd_ = -1;
    }

    template <typename T> RingBuffer<SPSC, T> &RingBuffer<SPSC, T>::operator=(RingBuffer &&other) noexcept {
        if (this != &other) {
            this->~RingBuffer();
            header_ = other.header_;
            buffer_ = other.buffer_;
            owns_memory_ = other.owns_memory_;
            is_shm_ = other.is_shm_;
            shm_fd_ = other.shm_fd_;
            shm_size_ = other.shm_size_;
            shm_name_ = std::move(other.shm_name_);
            other.header_ = nullptr;
            other.buffer_ = nullptr;
            other.shm_fd_ = -1;
        }
        return *this;
    }

    template <typename T> inline Result<bool, Error> RingBuffer<SPSC, T>::push(const T &item) {
        uint64_t w = header_->write_pos.load(std::memory_order_relaxed);
        uint64_t r = header_->read_pos.load(std::memory_order_acquire);
        if (w - r >= header_->capacity) {
            return Result<bool, Error>::err(Error::timeout("Ring buffer full"));
        }
        size_t idx = w % header_->capacity;
        buffer_[idx] = item;
        std::atomic_thread_fence(std::memory_order_release);
        header_->write_pos.store(w + 1, std::memory_order_release);
        return Result<bool, Error>::ok(true);
    }

    template <typename T> inline Result<bool, Error> RingBuffer<SPSC, T>::push(T &&item) {
        uint64_t w = header_->write_pos.load(std::memory_order_relaxed);
        uint64_t r = header_->read_pos.load(std::memory_order_acquire);
        if (w - r >= header_->capacity) {
            return Result<bool, Error>::err(Error::timeout("Ring buffer full"));
        }
        size_t idx = w % header_->capacity;
        buffer_[idx] = std::move(item);
        std::atomic_thread_fence(std::memory_order_release);
        header_->write_pos.store(w + 1, std::memory_order_release);
        return Result<bool, Error>::ok(true);
    }

    template <typename T>
    template <typename... Args>
    inline Result<bool, Error> RingBuffer<SPSC, T>::emplace(Args &&...args) {
        uint64_t w = header_->write_pos.load(std::memory_order_relaxed);
        uint64_t r = header_->read_pos.load(std::memory_order_acquire);
        if (w - r >= header_->capacity) {
            return Result<bool, Error>::err(Error::timeout("Ring buffer full"));
        }
        size_t idx = w % header_->capacity;
        new (&buffer_[idx]) T(std::forward<Args>(args)...);
        std::atomic_thread_fence(std::memory_order_release);
        header_->write_pos.store(w + 1, std::memory_order_release);
        return Result<bool, Error>::ok(true);
    }

    template <typename T> inline Result<T, Error> RingBuffer<SPSC, T>::pop() {
        uint64_t r = header_->read_pos.load(std::memory_order_relaxed);
        uint64_t w = header_->write_pos.load(std::memory_order_acquire);
        if (w == r) {
            return Result<T, Error>::err(Error::timeout("Ring buffer empty"));
        }
        size_t idx = r % header_->capacity;
        T item = buffer_[idx];
        header_->read_pos.store(r + 1, std::memory_order_release);
        return Result<T, Error>::ok(std::move(item));
    }

    template <typename T> inline Result<const T *, Error> RingBuffer<SPSC, T>::peek() const {
        uint64_t r = header_->read_pos.load(std::memory_order_relaxed);
        uint64_t w = header_->write_pos.load(std::memory_order_acquire);
        if (w == r) {
            return Result<const T *, Error>::err(Error::timeout("Ring buffer empty"));
        }
        size_t idx = r % header_->capacity;
        return Result<const T *, Error>::ok(&buffer_[idx]);
    }

    template <typename T> inline bool RingBuffer<SPSC, T>::empty() const noexcept {
        uint64_t r = header_->read_pos.load(std::memory_order_relaxed);
        uint64_t w = header_->write_pos.load(std::memory_order_acquire);
        return w == r;
    }

    template <typename T> inline bool RingBuffer<SPSC, T>::full() const noexcept {
        uint64_t w = header_->write_pos.load(std::memory_order_relaxed);
        uint64_t r = header_->read_pos.load(std::memory_order_acquire);
        return (w - r) >= header_->capacity;
    }

    template <typename T> inline size_t RingBuffer<SPSC, T>::size() const noexcept {
        uint64_t w = header_->write_pos.load(std::memory_order_acquire);
        uint64_t r = header_->read_pos.load(std::memory_order_acquire);
        return w - r;
    }

    template <typename T> inline size_t RingBuffer<SPSC, T>::capacity() const noexcept { return header_->capacity; }

    template <typename T> inline typename RingBuffer<SPSC, T>::Snapshot RingBuffer<SPSC, T>::snapshot() const noexcept {
        return Snapshot{header_->write_pos.load(std::memory_order_acquire),
                        header_->read_pos.load(std::memory_order_acquire), header_->capacity, header_->magic,
                        header_->version};
    }

    template <typename T> inline auto RingBuffer<SPSC, T>::members() const noexcept {
        auto snap = snapshot();
        return std::make_tuple(snap.write_pos, snap.read_pos, snap.capacity, snap.magic, snap.version);
    }

    template <typename T>
    inline typename RingBuffer<SPSC, T>::SnapshotWithData RingBuffer<SPSC, T>::snapshot_with_data() const {
        uint64_t r = header_->read_pos.load(std::memory_order_acquire);
        uint64_t w = header_->write_pos.load(std::memory_order_acquire);

        SnapshotWithData snap;
        snap.write_pos = w;
        snap.read_pos = r;
        snap.capacity = header_->capacity;
        snap.magic = header_->magic;
        snap.version = header_->version;

        size_t count = w - r;
        snap.data.reserve(count);

        for (uint64_t pos = r; pos < w; pos++) {
            size_t idx = pos % header_->capacity;
            snap.data.push_back(buffer_[idx]);
        }

        return snap;
    }

    template <typename T>
    Result<RingBuffer<SPSC, T>, Error> RingBuffer<SPSC, T>::from_snapshot(const SnapshotWithData &snap) {
        if (snap.magic != 0x53505343) {
            return Result<RingBuffer, Error>::err(Error::invalid_argument("Invalid snapshot magic number"));
        }

        if (snap.capacity == 0) {
            return Result<RingBuffer, Error>::err(Error::invalid_argument("Invalid snapshot capacity"));
        }

        RingBuffer ring(snap.capacity);

        for (const auto &item : snap.data) {
            auto result = ring.push(item);
            if (!result.is_ok()) {
                return Result<RingBuffer, Error>::err(Error::io_error("Failed to restore snapshot: ring buffer full"));
            }
        }

        return Result<RingBuffer, Error>::ok(std::move(ring));
    }

    template <typename T> inline Vector<T> RingBuffer<SPSC, T>::drain() {
        Vector<T> result;
        while (true) {
            auto item = pop();
            if (!item.is_ok())
                break;
            result.push_back(std::move(item.value()));
        }
        return result;
    }

} // namespace datapod
