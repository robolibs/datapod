#pragma once

#include <algorithm>
#include <cstdint>
#include <datapod/sequential/vector.hpp>
#include <datapod/temporal/stamp.hpp>
#include <numeric>
#include <stdexcept>
#include <tuple>

namespace datapod {

    /**
     * @brief Columnar time-series container for high-performance analytics
     *
     * TimeSeries<T> stores timestamps and values in separate vectors (columnar layout)
     * for better cache efficiency, SIMD operations, and range queries. This is the
     * primary container for time-series data in datapod.
     *
     * @tparam T Value type (can be POD, struct, or class)
     *
     * Features:
     * - Columnar layout: Separate timestamp and value arrays
     * - Zero-copy range queries
     * - Automatic serialization via members()
     * - STL-compatible iterators
     * - Aggregation functions (mean, min, max, sum)
     * - Time-based sorting and validation
     *
     * Memory Layout:
     * @code
     * timestamps: [t0, t1, t2, t3, ...]  <- int64_t array
     * values:     [v0, v1, v2, v3, ...]  <- T array
     * @endcode
     *
     * Example:
     * @code
     * TimeSeries<double> temps;
     * temps.append(1000, 23.5);
     * temps.append(2000, 24.1);
     *
     * auto range = temps.query(1000, 3000);
     * double avg = temps.mean();
     * @endcode
     */
    template <typename T> struct TimeSeries {
        datapod::Vector<int64_t> timestamps; ///< Sorted timestamps (nanoseconds)
        datapod::Vector<T> values;           ///< Corresponding values

        // ========================================================================
        // REFLECTION & SERIALIZATION (REQUIRED)
        // ========================================================================

        /**
         * @brief Returns tuple of members for reflection and serialization
         *
         * This enables automatic serialization, deserialization, and reflection
         * through datapod's reflection system without manual registration.
         */
        auto members() { return std::tie(timestamps, values); }
        auto members() const { return std::tie(timestamps, values); }

        // ========================================================================
        // CONSTRUCTION
        // ========================================================================

        /// Default constructor
        TimeSeries() = default;

        /**
         * @brief Construct with reserved capacity
         * @param capacity Initial capacity to reserve
         */
        explicit TimeSeries(size_t capacity) { reserve(capacity); }

        /**
         * @brief Construct from vector of Stamp<T>
         * @param stamps Vector of timestamped values
         */
        explicit TimeSeries(datapod::Vector<Stamp<T>> const &stamps) {
            reserve(stamps.size());
            for (auto const &s : stamps) {
                timestamps.push_back(s.timestamp);
                values.push_back(s.value);
            }
        }

        // ========================================================================
        // CAPACITY
        // ========================================================================

        /**
         * @brief Number of data points
         * @return Size of the time series
         */
        size_t size() const noexcept { return timestamps.size(); }

        /**
         * @brief Check if empty
         * @return true if no data points
         */
        bool empty() const noexcept { return timestamps.empty(); }

        /**
         * @brief Reserve capacity for n data points
         * @param n Capacity to reserve
         */
        void reserve(size_t n) {
            timestamps.reserve(n);
            values.reserve(n);
        }

        /**
         * @brief Get current capacity
         * @return Capacity (minimum of timestamp and value capacities)
         */
        size_t capacity() const noexcept { return std::min(timestamps.capacity(), values.capacity()); }

        /**
         * @brief Clear all data points
         */
        void clear() {
            timestamps.clear();
            values.clear();
        }

        // ========================================================================
        // MODIFIERS
        // ========================================================================

        /**
         * @brief Append a timestamped value
         * @param ts Timestamp in nanoseconds
         * @param value Value to append
         *
         * Note: For best performance, append in chronological order.
         * Use sort_by_time() if data is appended out of order.
         */
        void append(int64_t ts, T const &value) {
            timestamps.push_back(ts);
            values.push_back(value);
        }

        /**
         * @brief Append a Stamp<T>
         * @param stamp Timestamped value
         */
        void append(Stamp<T> const &stamp) {
            timestamps.push_back(stamp.timestamp);
            values.push_back(stamp.value);
        }

        /**
         * @brief Append multiple stamps at once
         * @param stamps Vector of timestamped values
         */
        void append(datapod::Vector<Stamp<T>> const &stamps) {
            reserve(size() + stamps.size());
            for (auto const &s : stamps) {
                timestamps.push_back(s.timestamp);
                values.push_back(s.value);
            }
        }

        // ========================================================================
        // ELEMENT ACCESS
        // ========================================================================

        /**
         * @brief Access element at index (unchecked)
         * @param i Index
         * @return Stamp<T> containing timestamp and value
         */
        Stamp<T> operator[](size_t i) const { return {timestamps[i], values[i]}; }

        /**
         * @brief Access element at index (bounds checked)
         * @param i Index
         * @return Stamp<T> containing timestamp and value
         * @throws std::out_of_range if i >= size()
         */
        Stamp<T> at(size_t i) const {
            if (i >= size()) {
                throw std::out_of_range("TimeSeries::at: index out of range");
            }
            return {timestamps[i], values[i]};
        }

        /**
         * @brief Get first element
         * @return First Stamp<T>
         */
        Stamp<T> front() const { return {timestamps.front(), values.front()}; }

        /**
         * @brief Get last element
         * @return Last Stamp<T>
         */
        Stamp<T> back() const { return {timestamps.back(), values.back()}; }

        // ========================================================================
        // TIME RANGE QUERIES
        // ========================================================================

        /**
         * @brief Result of a time range query (zero-copy view)
         */
        struct Range {
            int64_t const *times; ///< Pointer to first timestamp
            T const *values;      ///< Pointer to first value
            size_t count;         ///< Number of elements in range

            /// Check if range is empty
            bool empty() const { return count == 0; }

            /// Get element at index within range
            Stamp<T> operator[](size_t i) const { return {times[i], values[i]}; }
        };

        /**
         * @brief Query time range [start, end)
         * @param start Start timestamp (inclusive)
         * @param end End timestamp (exclusive)
         * @return Range view (zero-copy)
         *
         * Note: TimeSeries must be sorted by time for accurate results.
         * Use sort_by_time() if needed.
         */
        Range query(int64_t start, int64_t end) const {
            if (empty()) {
                return {nullptr, nullptr, 0};
            }

            // Binary search for range bounds
            auto it_start = std::lower_bound(timestamps.begin(), timestamps.end(), start);
            auto it_end = std::lower_bound(timestamps.begin(), timestamps.end(), end);

            size_t idx_start = it_start - timestamps.begin();
            size_t count = it_end - it_start;

            if (count == 0) {
                return {nullptr, nullptr, 0};
            }

            return {&timestamps[idx_start], &values[idx_start], count};
        }

        // ========================================================================
        // SORTING & VALIDATION
        // ========================================================================

        /**
         * @brief Check if sorted by timestamp
         * @return true if timestamps are in ascending order
         */
        bool is_sorted() const noexcept {
            if (size() <= 1)
                return true;

            for (size_t i = 1; i < size(); ++i) {
                if (timestamps[i] < timestamps[i - 1]) {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Sort by timestamp (ascending)
         *
         * This sorts both timestamps and values arrays together,
         * maintaining the correspondence between them.
         */
        void sort_by_time() {
            if (size() <= 1)
                return;

            // Create index array
            datapod::Vector<size_t> indices(size());
            for (size_t i = 0; i < size(); ++i) {
                indices[i] = i;
            }

            // Sort indices by timestamp
            std::sort(indices.begin(), indices.end(),
                      [this](size_t a, size_t b) { return timestamps[a] < timestamps[b]; });

            // Reorder both arrays
            datapod::Vector<int64_t> sorted_times(size());
            datapod::Vector<T> sorted_values(size());

            for (size_t i = 0; i < size(); ++i) {
                sorted_times[i] = timestamps[indices[i]];
                sorted_values[i] = values[indices[i]];
            }

            timestamps = std::move(sorted_times);
            values = std::move(sorted_values);
        }

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

            T sum = std::accumulate(values.begin(), values.end(), T{});
            return sum / static_cast<T>(size());
        }

        /**
         * @brief Calculate sum of values
         * @return Sum of all values
         * @note Requires T to support addition
         */
        T sum() const { return std::accumulate(values.begin(), values.end(), T{}); }

        /**
         * @brief Find minimum value
         * @return Minimum value
         * @note Requires T to support operator<
         */
        T min() const {
            if (empty()) {
                return T{};
            }
            return *std::min_element(values.begin(), values.end());
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
            return *std::max_element(values.begin(), values.end());
        }

        /**
         * @brief Find timestamp of minimum value
         * @return Timestamp where minimum occurs
         */
        int64_t time_at_min() const {
            if (empty()) {
                return 0;
            }
            auto it = std::min_element(values.begin(), values.end());
            size_t idx = it - values.begin();
            return timestamps[idx];
        }

        /**
         * @brief Find timestamp of maximum value
         * @return Timestamp where maximum occurs
         */
        int64_t time_at_max() const {
            if (empty()) {
                return 0;
            }
            auto it = std::max_element(values.begin(), values.end());
            size_t idx = it - values.begin();
            return timestamps[idx];
        }

        // ========================================================================
        // TIME UTILITIES
        // ========================================================================

        /**
         * @brief Get time span of the series
         * @return Duration in nanoseconds (last - first timestamp)
         */
        int64_t duration() const noexcept {
            if (size() < 2)
                return 0;
            return timestamps.back() - timestamps.front();
        }

        /**
         * @brief Get first timestamp
         * @return First timestamp, or 0 if empty
         */
        int64_t start_time() const noexcept { return empty() ? 0 : timestamps.front(); }

        /**
         * @brief Get last timestamp
         * @return Last timestamp, or 0 if empty
         */
        int64_t end_time() const noexcept { return empty() ? 0 : timestamps.back(); }

        // ========================================================================
        // RESAMPLING & DOWNSAMPLING
        // ========================================================================

        /**
         * @brief Downsample by taking every Nth point
         * @param n Step size (take every nth point)
         * @return New downsampled TimeSeries
         */
        TimeSeries<T> downsample(size_t n) const {
            if (n == 0 || n == 1)
                return *this;

            TimeSeries<T> result;
            result.reserve((size() + n - 1) / n);

            for (size_t i = 0; i < size(); i += n) {
                result.append(timestamps[i], values[i]);
            }

            return result;
        }

        // ========================================================================
        // CONVERSION
        // ========================================================================

        /**
         * @brief Convert to vector of Stamp<T>
         * @return Vector of timestamped values
         */
        datapod::Vector<Stamp<T>> to_stamps() const {
            datapod::Vector<Stamp<T>> result;
            result.reserve(size());

            for (size_t i = 0; i < size(); ++i) {
                result.push_back({timestamps[i], values[i]});
            }

            return result;
        }
    };

} // namespace datapod
