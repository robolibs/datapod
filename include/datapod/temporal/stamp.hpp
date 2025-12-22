#pragma once

#include <chrono>
#include <cstdint>
#include <tuple>

namespace datapod {

    /**
     * @brief Pairs any data type with a timestamp for time-series applications
     *
     * Stamp<T> is the fundamental building block for time-series data, pairing
     * a nanosecond-precision timestamp with any value type. Used extensively in
     * robotics (sensor readings), events (logs), measurements (IoT), financial
     * data (ticks), and multimedia (frame timestamps).
     *
     * @tparam T Value type (can be POD, struct, or class)
     *
     * Features:
     * - Nanosecond precision timestamps (int64_t)
     * - POD-compatible when T is POD
     * - Automatic serialization via members()
     * - Timestamp-based comparison operators
     * - Utility functions for time conversion
     *
     * Example:
     * @code
     * // Sensor reading
     * Stamp<double> temp{Stamp<double>::now(), 23.5};
     *
     * // Robotics position
     * Stamp<Vec3> position{timestamp, {x, y, z}};
     *
     * // Time series
     * std::vector<Stamp<double>> sensor_data;
     * sensor_data.push_back({ts, value});
     * @endcode
     */
    template <typename T> struct Stamp {
        int64_t timestamp; ///< Nanoseconds since Unix epoch
        T value;           ///< Associated value

        // ========================================================================
        // REFLECTION & SERIALIZATION (REQUIRED)
        // ========================================================================

        /**
         * @brief Returns tuple of members for reflection and serialization
         *
         * This enables automatic serialization, deserialization, and reflection
         * through datapod's reflection system without manual registration.
         */
        auto members() { return std::tie(timestamp, value); }
        auto members() const { return std::tie(timestamp, value); }

        // ========================================================================
        // CONSTRUCTION
        // ========================================================================

        /// Default constructor
        Stamp() = default;

        /**
         * @brief Construct with explicit timestamp and value
         * @param ts Timestamp in nanoseconds since Unix epoch
         * @param val Value to store
         */
        constexpr Stamp(int64_t ts, T const &val) : timestamp(ts), value(val) {}

        /**
         * @brief Construct with value, using current time
         * @param val Value to store
         */
        explicit Stamp(T const &val) : timestamp(now()), value(val) {}

        // ========================================================================
        // TIME UTILITIES
        // ========================================================================

        /**
         * @brief Get current time in nanoseconds since Unix epoch
         * @return Current timestamp in nanoseconds
         */
        static int64_t now() noexcept {
            using namespace std::chrono;
            return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
        }

        /**
         * @brief Get age of this timestamp (time elapsed since)
         * @return Age in nanoseconds
         */
        int64_t age() const noexcept { return now() - timestamp; }

        /**
         * @brief Convert timestamp to seconds (as double)
         * @return Timestamp in seconds with fractional part
         */
        double seconds() const noexcept { return static_cast<double>(timestamp) / 1e9; }

        /**
         * @brief Convert timestamp to milliseconds
         * @return Timestamp in milliseconds
         */
        int64_t milliseconds() const noexcept { return timestamp / 1'000'000; }

        /**
         * @brief Convert timestamp to microseconds
         * @return Timestamp in microseconds
         */
        int64_t microseconds() const noexcept { return timestamp / 1'000; }

        // ========================================================================
        // COMPARISON OPERATORS (by timestamp)
        // ========================================================================

        /// Less than comparison (by timestamp)
        constexpr bool operator<(Stamp const &other) const noexcept { return timestamp < other.timestamp; }

        /// Greater than comparison (by timestamp)
        constexpr bool operator>(Stamp const &other) const noexcept { return timestamp > other.timestamp; }

        /// Less than or equal comparison (by timestamp)
        constexpr bool operator<=(Stamp const &other) const noexcept { return timestamp <= other.timestamp; }

        /// Greater than or equal comparison (by timestamp)
        constexpr bool operator>=(Stamp const &other) const noexcept { return timestamp >= other.timestamp; }

        /// Equality comparison (by timestamp)
        constexpr bool operator==(Stamp const &other) const noexcept { return timestamp == other.timestamp; }

        /// Inequality comparison (by timestamp)
        constexpr bool operator!=(Stamp const &other) const noexcept { return timestamp != other.timestamp; }

        // ========================================================================
        // FACTORY METHODS
        // ========================================================================

        /**
         * @brief Create stamp from seconds (double) and value
         * @param seconds Time in seconds (can have fractional part)
         * @param val Value to store
         * @return Stamp with converted timestamp
         */
        static constexpr Stamp from_seconds(double seconds, T const &val) {
            return Stamp{static_cast<int64_t>(seconds * 1e9), val};
        }

        /**
         * @brief Create stamp from milliseconds and value
         * @param milliseconds Time in milliseconds
         * @param val Value to store
         * @return Stamp with converted timestamp
         */
        static constexpr Stamp from_milliseconds(int64_t milliseconds, T const &val) {
            return Stamp{milliseconds * 1'000'000, val};
        }

        /**
         * @brief Create stamp from microseconds and value
         * @param microseconds Time in microseconds
         * @param val Value to store
         * @return Stamp with converted timestamp
         */
        static constexpr Stamp from_microseconds(int64_t microseconds, T const &val) {
            return Stamp{microseconds * 1'000, val};
        }
    };

    // ============================================================================
    // CONVENIENCE ALIASES
    // ============================================================================

    /// Timestamped double value (common for sensor readings)
    using StampedDouble = Stamp<double>;

    /// Timestamped float value
    using StampedFloat = Stamp<float>;

    /// Timestamped int32 value
    using StampedInt = Stamp<int32_t>;

    /// Timestamped int64 value
    using StampedLong = Stamp<int64_t>;

} // namespace datapod
