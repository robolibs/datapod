#pragma once

#include <cstdint>
#include <tuple>

#include "../associative/map.hpp"
#include "../sequential/string.hpp"
#include "../sequential/vector.hpp"

namespace datapod {

    /**
     * @brief MultiSeries - Multiple time series with shared timestamp index (POD)
     *
     * DataFrame-like structure for multi-variate time series data.
     * All series share the same timestamp index for aligned analysis.
     *
     * Pure aggregate struct for columnar multi-series storage.
     * Fully serializable and reflectable.
     *
     * Fields:
     * - timestamps: Shared time index for all series
     * - series: Map of series name to values vector
     *
     * Use cases:
     * - Multi-sensor data (temperature, humidity, pressure)
     * - Financial data (OHLCV across multiple symbols)
     * - Robot telemetry (position, velocity, battery)
     * - Multi-variate analytics
     * - Correlation analysis
     *
     * Example:
     * ```cpp
     * MultiSeries ms;
     * ms.add_series("temperature");
     * ms.add_series("humidity");
     * ms.append(timestamp1, {{"temperature", 23.5}, {"humidity", 65.0}});
     * ```
     */
    struct MultiSeries {
        Vector<int64_t> timestamps;         // Shared timestamp index
        Map<String, Vector<double>> series; // Series name → values

        auto members() noexcept { return std::tie(timestamps, series); }
        auto members() const noexcept { return std::tie(timestamps, series); }

        // Construction
        MultiSeries() = default;

        // Series management
        inline void add_series(const String &name) {
            if (!series.contains(name)) {
                series[name] = Vector<double>();
                // Resize to match existing timestamps if any
                if (!timestamps.empty()) {
                    series[name].resize(timestamps.size(), 0.0);
                }
            }
        }

        inline void remove_series(const String &name) { series.erase(name); }

        inline bool has_series(const String &name) const noexcept { return series.contains(name); }

        inline size_t num_series() const noexcept { return series.size(); }

        // Data insertion
        inline void append(int64_t ts, const Map<String, double> &values) {
            timestamps.push_back(ts);

            // Append values to each series
            for (auto &[name, vec] : series) {
                auto it = values.find(name);
                if (it != values.end()) {
                    vec.push_back(it->second);
                } else {
                    vec.push_back(0.0); // Default value for missing data
                }
            }
        }

        inline void reserve(size_t n) {
            timestamps.reserve(n);
            for (auto &[name, vec] : series) {
                vec.reserve(n);
            }
        }

        inline void clear() noexcept {
            timestamps.clear();
            for (auto &[name, vec] : series) {
                vec.clear();
            }
        }

        // Access
        inline size_t size() const noexcept { return timestamps.size(); }

        inline bool empty() const noexcept { return timestamps.empty(); }

        inline const Vector<double> &operator[](const String &name) const { return series.at(name); }

        inline Vector<double> &operator[](const String &name) { return series.at(name); }

        inline const Vector<double> *get(const String &name) const noexcept {
            auto it = series.find(name);
            return it != series.end() ? &it->second : nullptr;
        }

        // Queries - Range query result
        struct Range {
            const int64_t *times;
            size_t count;
            size_t start_idx;
        };

        inline Range query(int64_t start, int64_t end) const noexcept {
            size_t start_idx = 0;
            size_t end_idx = timestamps.size();

            // Binary search for start
            size_t left = 0, right = timestamps.size();
            while (left < right) {
                size_t mid = left + (right - left) / 2;
                if (timestamps[mid] < start) {
                    left = mid + 1;
                } else {
                    right = mid;
                }
            }
            start_idx = left;

            // Binary search for end
            left = start_idx;
            right = timestamps.size();
            while (left < right) {
                size_t mid = left + (right - left) / 2;
                if (timestamps[mid] < end) {
                    left = mid + 1;
                } else {
                    right = mid;
                }
            }
            end_idx = left;

            return Range{timestamps.data() + start_idx, end_idx - start_idx, start_idx};
        }

        // Get values for a specific series in a time range
        inline const double *get_range_values(const String &name, const Range &range) const noexcept {
            auto it = series.find(name);
            if (it == series.end())
                return nullptr;
            return it->second.data() + range.start_idx;
        }

        // Aggregations for a specific series
        inline double mean(const String &name) const noexcept {
            auto vec_ptr = get(name);
            if (!vec_ptr || vec_ptr->empty())
                return 0.0;

            double sum = 0.0;
            for (const auto &val : *vec_ptr) {
                sum += val;
            }
            return sum / static_cast<double>(vec_ptr->size());
        }

        inline double min(const String &name) const noexcept {
            auto vec_ptr = get(name);
            if (!vec_ptr || vec_ptr->empty())
                return 0.0;

            double result = (*vec_ptr)[0];
            for (size_t i = 1; i < vec_ptr->size(); ++i) {
                if ((*vec_ptr)[i] < result) {
                    result = (*vec_ptr)[i];
                }
            }
            return result;
        }

        inline double max(const String &name) const noexcept {
            auto vec_ptr = get(name);
            if (!vec_ptr || vec_ptr->empty())
                return 0.0;

            double result = (*vec_ptr)[0];
            for (size_t i = 1; i < vec_ptr->size(); ++i) {
                if ((*vec_ptr)[i] > result) {
                    result = (*vec_ptr)[i];
                }
            }
            return result;
        }

        // Utilities
        inline bool is_sorted() const noexcept {
            for (size_t i = 1; i < timestamps.size(); ++i) {
                if (timestamps[i] < timestamps[i - 1]) {
                    return false;
                }
            }
            return true;
        }

        // Comparison
        inline bool operator==(const MultiSeries &other) const noexcept {
            return timestamps == other.timestamps && series == other.series;
        }

        inline bool operator!=(const MultiSeries &other) const noexcept { return !(*this == other); }
    };

} // namespace datapod
