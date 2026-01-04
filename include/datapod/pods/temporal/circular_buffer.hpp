#pragma once

#include <cstdint>
#include <datapod/pods/sequential/array.hpp>
#include <datapod/pods/temporal/stamp.hpp>
#include <stdexcept>
#include <tuple>

namespace datapod {

    /**
     * @brief Fixed-size circular buffer for recent time-series data
     *
     * CircularTimeBuffer<T, N> maintains the most recent N timestamped values
     * using a fixed-size circular buffer. Perfect for embedded systems and
     * real-time robotics where dynamic allocation is undesirable.
     *
     * @tparam T Value type (can be POD, struct, or class)
     * @tparam N Buffer capacity (fixed at compile time)
     *
     * Features:
     * - No dynamic allocation (stack-friendly)
     * - Fixed memory footprint
     * - O(1) push operation
     * - Chronological iteration (oldest to newest)
     * - Full serialization via members()
     *
     * Memory Layout:
     * @code
     * timestamps: [t0, t1, t2, ..., tN-1]  <- Fixed array of int64_t
     * values:     [v0, v1, v2, ..., vN-1]  <- Fixed array of T
     * head:       index where next write occurs
     * size:       current number of elements (0 to N)
     * @endcode
     *
     * Example:
     * @code
     * CircularTimeBuffer<double, 100> sensor_history;
     * sensor_history.push(timestamp, temperature);
     * auto latest = sensor_history.newest();
     * @endcode
     */
    template <typename T, size_t N> struct CircularTimeBuffer {
        static_assert(N > 0, "CircularTimeBuffer capacity must be > 0");

        datapod::Array<int64_t, N> timestamps; ///< Circular timestamp buffer
        datapod::Array<T, N> values;           ///< Circular value buffer
        size_t head;                           ///< Next write position
        size_t size;                           ///< Current number of elements

        // ========================================================================
        // REFLECTION & SERIALIZATION (REQUIRED)
        // ========================================================================

        /**
         * @brief Returns tuple of members for reflection and serialization
         *
         * This enables automatic serialization, deserialization, and reflection
         * through datapod's reflection system without manual registration.
         */
        auto members() { return std::tie(timestamps, values, head, size); }
        auto members() const { return std::tie(timestamps, values, head, size); }

        // ========================================================================
        // CONSTRUCTION
        // ========================================================================

        /**
         * @brief Default constructor - creates empty buffer
         */
        CircularTimeBuffer() : head(0), size(0) {}

        // ========================================================================
        // CAPACITY
        // ========================================================================

        /**
         * @brief Get maximum capacity (compile-time constant)
         * @return Capacity N
         */
        static constexpr size_t capacity() noexcept { return N; }

        /**
         * @brief Get current number of elements
         * @return Number of elements (0 to N)
         */
        size_t get_size() const noexcept { return size; }

        /**
         * @brief Check if buffer is empty
         * @return true if no elements
         */
        bool empty() const noexcept { return size == 0; }

        /**
         * @brief Check if buffer is full
         * @return true if size == capacity
         */
        bool full() const noexcept { return size == N; }

        /**
         * @brief Clear all elements
         */
        void clear() noexcept {
            head = 0;
            size = 0;
        }

        // ========================================================================
        // MODIFIERS
        // ========================================================================

        /**
         * @brief Push a timestamped value
         * @param ts Timestamp in nanoseconds
         * @param value Value to store
         *
         * If buffer is full, overwrites the oldest element.
         */
        void push(int64_t ts, T const &value) {
            timestamps[head] = ts;
            values[head] = value;
            head = (head + 1) % N;
            if (size < N) {
                size++;
            }
        }

        /**
         * @brief Push a Stamp<T>
         * @param stamp Timestamped value
         */
        void push(Stamp<T> const &stamp) { push(stamp.timestamp, stamp.value); }

        // ========================================================================
        // ELEMENT ACCESS (Logical Indexing)
        // ========================================================================

        /**
         * @brief Access element by logical index (0 = oldest)
         * @param i Logical index (0 to size-1)
         * @return Stamp<T> at logical index i
         *
         * Logical index 0 is the oldest element, size-1 is the newest.
         */
        Stamp<T> operator[](size_t i) const {
            size_t physical_idx = get_physical_index(i);
            return {timestamps[physical_idx], values[physical_idx]};
        }

        /**
         * @brief Access element with bounds checking
         * @param i Logical index
         * @return Stamp<T> at logical index i
         * @throws std::out_of_range if i >= size
         */
        Stamp<T> at(size_t i) const {
            if (i >= size) {
                throw std::out_of_range("CircularTimeBuffer::at: index out of range");
            }
            return operator[](i);
        }

        /**
         * @brief Get newest (most recent) element
         * @return Most recently pushed Stamp<T>
         * @throws std::out_of_range if empty
         */
        Stamp<T> newest() const {
            if (empty()) {
                throw std::out_of_range("CircularTimeBuffer::newest: buffer is empty");
            }
            size_t idx = (head + N - 1) % N;
            return {timestamps[idx], values[idx]};
        }

        /**
         * @brief Get oldest element
         * @return Oldest Stamp<T> in buffer
         * @throws std::out_of_range if empty
         */
        Stamp<T> oldest() const {
            if (empty()) {
                throw std::out_of_range("CircularTimeBuffer::oldest: buffer is empty");
            }
            size_t idx = get_physical_index(0);
            return {timestamps[idx], values[idx]};
        }

        // ========================================================================
        // ITERATION
        // ========================================================================

        /**
         * @brief Iterator for chronological traversal (oldest to newest)
         */
        struct iterator {
            CircularTimeBuffer const *buffer;
            size_t logical_index;

            iterator(CircularTimeBuffer const *buf, size_t idx) : buffer(buf), logical_index(idx) {}

            Stamp<T> operator*() const { return (*buffer)[logical_index]; }

            iterator &operator++() {
                ++logical_index;
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                ++logical_index;
                return tmp;
            }

            bool operator==(iterator const &other) const { return logical_index == other.logical_index; }

            bool operator!=(iterator const &other) const { return logical_index != other.logical_index; }
        };

        /**
         * @brief Get iterator to oldest element
         * @return Iterator to beginning (oldest)
         */
        iterator begin() const { return iterator(this, 0); }

        /**
         * @brief Get iterator to past-the-end
         * @return Iterator to end
         */
        iterator end() const { return iterator(this, size); }

        // ========================================================================
        // AGGREGATIONS
        // ========================================================================

        /**
         * @brief Calculate mean of values
         * @return Mean value
         * @note Requires T to support addition and division by size_t
         */
        T mean() const {
            if (empty()) {
                return T{};
            }

            T sum = T{};
            for (size_t i = 0; i < size; ++i) {
                sum = sum + values[get_physical_index(i)];
            }
            return sum / static_cast<T>(size);
        }

        /**
         * @brief Find minimum value
         * @return Minimum value
         * @note Requires T to support operator<
         */
        T min() const {
            if (empty()) {
                return T{};
            }

            T min_val = values[get_physical_index(0)];
            for (size_t i = 1; i < size; ++i) {
                T val = values[get_physical_index(i)];
                if (val < min_val) {
                    min_val = val;
                }
            }
            return min_val;
        }

        /**
         * @brief Find maximum value
         * @return Maximum value
         * @note Requires T to support operator<
         */
        T max() const {
            if (empty()) {
                return T{};
            }

            T max_val = values[get_physical_index(0)];
            for (size_t i = 1; i < size; ++i) {
                T val = values[get_physical_index(i)];
                if (max_val < val) {
                    max_val = val;
                }
            }
            return max_val;
        }

        // ========================================================================
        // TIME UTILITIES
        // ========================================================================

        /**
         * @brief Get time span of the buffer
         * @return Duration in nanoseconds (newest - oldest timestamp)
         */
        int64_t duration() const noexcept {
            if (size < 2)
                return 0;
            return newest().timestamp - oldest().timestamp;
        }

        /**
         * @brief Get oldest timestamp
         * @return Oldest timestamp, or 0 if empty
         */
        int64_t start_time() const noexcept { return empty() ? 0 : oldest().timestamp; }

        /**
         * @brief Get newest timestamp
         * @return Newest timestamp, or 0 if empty
         */
        int64_t end_time() const noexcept { return empty() ? 0 : newest().timestamp; }

      private:
        // ========================================================================
        // INTERNAL HELPERS
        // ========================================================================

        /**
         * @brief Convert logical index to physical index
         * @param logical Logical index (0 = oldest)
         * @return Physical index in circular arrays
         *
         * When buffer is not full: physical = logical
         * When buffer is full: physical = (head + logical) % N
         */
        size_t get_physical_index(size_t logical) const noexcept {
            if (size < N) {
                // Not full yet, logical == physical
                return logical;
            } else {
                // Full, need to wrap around
                return (head + logical) % N;
            }
        }
    };

} // namespace datapod
