#pragma once

#include <cstdint>
#include <tuple>

#include "../sequential/vector.hpp"
#include "stamp.hpp"

namespace datapod {

    /**
     * @brief TimeWindow - Time range representation (POD)
     *
     * Simple time range for queries and filters.
     * Represents a half-open interval [start, end).
     *
     * Pure aggregate struct for time range queries.
     * Use aggregate initialization: TimeWindow{start_ns, end_ns}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - start: Start time (inclusive) in nanoseconds since epoch
     * - end: End time (exclusive) in nanoseconds since epoch
     *
     * Use cases:
     * - Time-based filtering of data
     * - Query ranges for time series
     * - Event filtering by time
     * - Log analysis time bounds
     */
    struct TimeWindow {
        int64_t start = 0; // Start time (inclusive) [ns]
        int64_t end = 0;   // End time (exclusive) [ns]

        auto members() noexcept { return std::tie(start, end); }
        auto members() const noexcept { return std::tie(start, end); }

        // Construction
        TimeWindow() = default;
        TimeWindow(int64_t s, int64_t e) : start(s), end(e) {}

        // Queries
        inline bool contains(int64_t ts) const noexcept { return ts >= start && ts < end; }

        inline bool overlaps(const TimeWindow &other) const noexcept { return start < other.end && other.start < end; }

        inline int64_t duration() const noexcept { return end - start; }

        inline bool is_valid() const noexcept { return start <= end; }

        // Utilities - Create windows relative to current time
        static inline TimeWindow last_n_seconds(int64_t n) noexcept {
            const int64_t now_ns = Stamp<int>::now();
            const int64_t ns_per_sec = 1'000'000'000LL;
            return TimeWindow{now_ns - (n * ns_per_sec), now_ns};
        }

        static inline TimeWindow last_n_minutes(int64_t n) noexcept {
            const int64_t now_ns = Stamp<int>::now();
            const int64_t ns_per_min = 60'000'000'000LL;
            return TimeWindow{now_ns - (n * ns_per_min), now_ns};
        }

        static inline TimeWindow last_n_hours(int64_t n) noexcept {
            const int64_t now_ns = Stamp<int>::now();
            const int64_t ns_per_hour = 3'600'000'000'000LL;
            return TimeWindow{now_ns - (n * ns_per_hour), now_ns};
        }

        // Comparison
        inline bool operator==(const TimeWindow &other) const noexcept {
            return start == other.start && end == other.end;
        }

        inline bool operator!=(const TimeWindow &other) const noexcept { return !(*this == other); }
    };

    /**
     * @brief SlidingWindow<T> - Real-time sliding window (POD when T is POD)
     *
     * Sliding window for stream processing and moving averages.
     * Automatically expires old data as new data arrives.
     *
     * Template struct for time-based windowed data.
     * Use aggregate initialization or methods.
     * Fully serializable and reflectable.
     *
     * Fields:
     * - window_size_ns: Window duration in nanoseconds
     * - slide_interval_ns: Slide/step amount in nanoseconds
     * - buffer: Timestamped data buffer
     *
     * Use cases:
     * - Moving averages (SMA, EMA)
     * - Real-time stream analytics
     * - Recent data buffering
     * - Anomaly detection windows
     */
    template <typename T> struct SlidingWindow {
        int64_t window_size_ns = 0;    // Window duration [ns]
        int64_t slide_interval_ns = 0; // Slide/step amount [ns]
        Vector<Stamp<T>> buffer;       // Timestamped data

        auto members() noexcept { return std::tie(window_size_ns, slide_interval_ns, buffer); }
        auto members() const noexcept { return std::tie(window_size_ns, slide_interval_ns, buffer); }

        // Construction
        SlidingWindow() = default;
        SlidingWindow(int64_t window_size, int64_t slide_interval = 0)
            : window_size_ns(window_size), slide_interval_ns(slide_interval > 0 ? slide_interval : window_size) {}

        // Modifiers
        inline void insert(int64_t ts, const T &value) {
            buffer.push_back(Stamp<T>{ts, value});
            expire_old(ts);
        }

        inline void insert(const Stamp<T> &stamped) {
            buffer.push_back(stamped);
            expire_old(stamped.timestamp);
        }

        inline void expire_old(int64_t current_time) noexcept {
            const int64_t cutoff = current_time - window_size_ns;

            // Remove expired entries from front
            size_t i = 0;
            while (i < buffer.size() && buffer[i].timestamp < cutoff) {
                ++i;
            }

            if (i > 0) {
                // Shift remaining elements
                for (size_t j = 0; j < buffer.size() - i; ++j) {
                    buffer[j] = buffer[j + i];
                }
                buffer.resize(buffer.size() - i);
            }
        }

        inline void clear() noexcept { buffer.clear(); }

        // Queries
        inline size_t size() const noexcept { return buffer.size(); }

        inline bool empty() const noexcept { return buffer.empty(); }

        inline const Vector<Stamp<T>> &data() const noexcept { return buffer; }

        // Aggregations (requires T to support arithmetic)
        inline T sum() const noexcept {
            T result{};
            for (const auto &stamp : buffer) {
                result = result + stamp.value;
            }
            return result;
        }

        inline T mean() const noexcept {
            if (buffer.empty())
                return T{};
            return sum() / static_cast<double>(buffer.size());
        }

        inline T min() const noexcept {
            if (buffer.empty())
                return T{};
            T result = buffer[0].value;
            for (size_t i = 1; i < buffer.size(); ++i) {
                if (buffer[i].value < result) {
                    result = buffer[i].value;
                }
            }
            return result;
        }

        inline T max() const noexcept {
            if (buffer.empty())
                return T{};
            T result = buffer[0].value;
            for (size_t i = 1; i < buffer.size(); ++i) {
                if (buffer[i].value > result) {
                    result = buffer[i].value;
                }
            }
            return result;
        }

        // Get current window range
        inline TimeWindow current_window(int64_t current_time) const noexcept {
            return TimeWindow{current_time - window_size_ns, current_time};
        }

        // Comparison
        inline bool operator==(const SlidingWindow &other) const noexcept {
            return window_size_ns == other.window_size_ns && slide_interval_ns == other.slide_interval_ns &&
                   buffer == other.buffer;
        }

        inline bool operator!=(const SlidingWindow &other) const noexcept { return !(*this == other); }
    };

    /**
     * @brief TumblingWindow<T> - Non-overlapping batch window (POD when T is POD)
     *
     * Non-overlapping time windows for batch processing.
     * Data is collected until window completes, then flushed.
     *
     * Template struct for batch-oriented windowing.
     * Fully serializable and reflectable.
     *
     * Fields:
     * - window_size_ns: Fixed window duration
     * - current_window_start: Start time of current window
     * - current_batch: Data in current window
     *
     * Use cases:
     * - Periodic batch processing
     * - Time-bucketed aggregation
     * - Micro-batching in stream processing
     * - Hourly/daily rollups
     */
    template <typename T> struct TumblingWindow {
        int64_t window_size_ns = 0;       // Fixed window size [ns]
        int64_t current_window_start = 0; // Start of current window [ns]
        Vector<Stamp<T>> current_batch;   // Current batch data

        auto members() noexcept { return std::tie(window_size_ns, current_window_start, current_batch); }
        auto members() const noexcept { return std::tie(window_size_ns, current_window_start, current_batch); }

        // Construction
        TumblingWindow() = default;
        explicit TumblingWindow(int64_t window_size) : window_size_ns(window_size), current_window_start(0) {}

        // Modifiers
        inline void insert(int64_t ts, const T &value) {
            // Initialize window on first insert
            if (current_window_start == 0) {
                current_window_start = (ts / window_size_ns) * window_size_ns;
            }

            // Check if we need to advance to next window
            while (ts >= current_window_start + window_size_ns) {
                current_window_start += window_size_ns;
                current_batch.clear();
            }

            current_batch.push_back(Stamp<T>{ts, value});
        }

        inline void insert(const Stamp<T> &stamped) { insert(stamped.timestamp, stamped.value); }

        // Window management
        inline bool is_window_complete(int64_t current_time) const noexcept {
            if (current_window_start == 0)
                return false;
            return current_time >= current_window_start + window_size_ns;
        }

        inline Vector<Stamp<T>> flush() noexcept {
            Vector<Stamp<T>> result = current_batch;
            current_batch.clear();
            current_window_start += window_size_ns;
            return result;
        }

        inline void clear() noexcept {
            current_batch.clear();
            current_window_start = 0;
        }

        // Queries
        inline size_t size() const noexcept { return current_batch.size(); }

        inline bool empty() const noexcept { return current_batch.empty(); }

        inline const Vector<Stamp<T>> &data() const noexcept { return current_batch; }

        inline TimeWindow current_window() const noexcept {
            return TimeWindow{current_window_start, current_window_start + window_size_ns};
        }

        // Comparison
        inline bool operator==(const TumblingWindow &other) const noexcept {
            return window_size_ns == other.window_size_ns && current_window_start == other.current_window_start &&
                   current_batch == other.current_batch;
        }

        inline bool operator!=(const TumblingWindow &other) const noexcept { return !(*this == other); }
    };

    namespace window {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace window

} // namespace datapod
