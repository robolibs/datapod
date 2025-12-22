/**
 * @file stamp_usage.cpp
 * @brief Comprehensive examples of using Stamp<T> for timestamped data
 *
 * Demonstrates:
 * - Basic timestamped values
 * - Sensor readings (robotics)
 * - Event logging
 * - Financial tick data
 * - Time series operations
 * - Reflection and serialization
 */

#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/temporal/stamp.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

using namespace datapod;

// ============================================================================
// Example 1: Basic Timestamped Values
// ============================================================================

void example_basic_usage() {
    std::cout << "=== Example 1: Basic Usage ===\n";

    // Create with explicit timestamp
    Stamp<double> temp1{1234567890000000LL, 23.5};
    std::cout << "Temperature reading: " << temp1.value << "°C at " << temp1.seconds() << " seconds\n";

    // Create with current timestamp
    Stamp<double> temp2{24.2};
    std::cout << "Current temperature: " << temp2.value << "°C\n";

    // Using factory methods
    auto temp3 = Stamp<double>::from_seconds(1.5, 25.1);
    auto temp4 = Stamp<double>::from_milliseconds(2500, 25.8);

    std::cout << "temp3 at " << temp3.milliseconds() << " ms\n";
    std::cout << "temp4 at " << temp4.milliseconds() << " ms\n";

    // Type aliases
    StampedDouble pressure{1000000000, 1013.25};
    StampedFloat humidity{2000000000, 65.5f};
    StampedInt count{3000000000, 42};

    std::cout << "Pressure: " << pressure.value << " hPa\n";
    std::cout << "Humidity: " << humidity.value << "%\n";
    std::cout << "Count: " << count.value << "\n";

    std::cout << "\n";
}

// ============================================================================
// Example 2: Sensor Readings (Robotics)
// ============================================================================

struct IMUReading {
    double accel_x, accel_y, accel_z;
    double gyro_x, gyro_y, gyro_z;

    auto members() { return std::tie(accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z); }
};

void example_sensor_readings() {
    std::cout << "=== Example 2: Sensor Readings (Robotics) ===\n";

    std::vector<Stamp<IMUReading>> imu_data;

    // Simulate IMU readings over time
    for (int i = 0; i < 5; ++i) {
        auto ts = Stamp<IMUReading>::now();
        IMUReading reading{
            0.1 * i,  0.2 * i,  9.81, // Accelerometer
            0.01 * i, 0.02 * i, 0.0   // Gyroscope
        };
        imu_data.push_back({ts, reading});
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Collected " << imu_data.size() << " IMU readings\n";

    // Calculate time span
    auto duration_ns = imu_data.back().timestamp - imu_data.front().timestamp;
    auto duration_ms = duration_ns / 1'000'000;
    std::cout << "Data collection took " << duration_ms << " ms\n";

    // Calculate average acceleration
    double avg_accel_z = 0.0;
    for (const auto &reading : imu_data) {
        avg_accel_z += reading.value.accel_z;
    }
    avg_accel_z /= imu_data.size();
    std::cout << "Average Z acceleration: " << avg_accel_z << " m/s²\n";

    std::cout << "\n";
}

// ============================================================================
// Example 3: Event Logging
// ============================================================================

enum class LogLevel : uint8_t { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

struct LogEvent {
    LogLevel level;
    const char *message;
    const char *component;
};

void example_event_logging() {
    std::cout << "=== Example 3: Event Logging ===\n";

    std::vector<Stamp<LogEvent>> logs;

    // Log some events
    logs.push_back({Stamp<LogEvent>::now(), {LogLevel::INFO, "System started", "Main"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    logs.push_back({Stamp<LogEvent>::now(), {LogLevel::DEBUG, "Loading config", "Config"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    logs.push_back({Stamp<LogEvent>::now(), {LogLevel::WARN, "High memory usage", "Memory"}});
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    logs.push_back({Stamp<LogEvent>::now(), {LogLevel::ERROR, "Connection failed", "Network"}});

    // Print log events
    const char *level_names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    for (const auto &log : logs) {
        std::cout << std::setw(12) << log.milliseconds() << " ms | " << std::setw(6)
                  << level_names[static_cast<int>(log.value.level)] << " | " << std::setw(10) << log.value.component
                  << " | " << log.value.message << "\n";
    }

    std::cout << "\n";
}

// ============================================================================
// Example 4: Financial Tick Data
// ============================================================================

struct Tick {
    double price;
    int64_t volume;
    char side; // 'B'=bid, 'A'=ask, 'T'=trade

    auto members() { return std::tie(price, volume, side); }
};

void example_financial_ticks() {
    std::cout << "=== Example 4: Financial Tick Data ===\n";

    std::vector<Stamp<Tick>> ticks = {
        {1000000000, {100.50, 1000, 'T'}}, {1001000000, {100.52, 500, 'T'}}, {1002000000, {100.48, 750, 'T'}},
        {1003000000, {100.55, 1200, 'T'}}, {1004000000, {100.49, 900, 'T'}},
    };

    std::cout << "Tick data:\n";
    std::cout << std::fixed << std::setprecision(2);
    for (const auto &tick : ticks) {
        std::cout << "  " << tick.seconds() << "s | "
                  << "$" << tick.value.price << " x " << tick.value.volume << "\n";
    }

    // Calculate VWAP (Volume-Weighted Average Price)
    double total_value = 0.0;
    int64_t total_volume = 0;
    for (const auto &tick : ticks) {
        total_value += tick.value.price * tick.value.volume;
        total_volume += tick.value.volume;
    }
    double vwap = total_value / total_volume;

    std::cout << "\nVolume-Weighted Average Price (VWAP): $" << vwap << "\n";

    // Find min and max prices
    auto [min_it, max_it] = std::minmax_element(
        ticks.begin(), ticks.end(), [](const auto &a, const auto &b) { return a.value.price < b.value.price; });

    std::cout << "Price range: $" << min_it->value.price << " - $" << max_it->value.price << "\n";

    std::cout << "\n";
}

// ============================================================================
// Example 5: Time Series Operations
// ============================================================================

void example_time_series_operations() {
    std::cout << "=== Example 5: Time Series Operations ===\n";

    // Create unsorted time series
    std::vector<Stamp<double>> temps = {
        {3000, 24.5}, {1000, 23.1}, {4000, 25.2}, {2000, 23.8}, {5000, 25.8},
    };

    std::cout << "Unsorted temperatures:\n";
    for (const auto &t : temps) {
        std::cout << "  t=" << t.timestamp << ": " << t.value << "°C\n";
    }

    // Sort by timestamp
    std::sort(temps.begin(), temps.end());

    std::cout << "\nSorted temperatures:\n";
    for (const auto &t : temps) {
        std::cout << "  t=" << t.timestamp << ": " << t.value << "°C\n";
    }

    // Time range query using binary search
    int64_t query_start = 2000;
    int64_t query_end = 4000;

    auto start_it = std::lower_bound(temps.begin(), temps.end(), Stamp<double>{query_start, 0});
    auto end_it = std::upper_bound(temps.begin(), temps.end(), Stamp<double>{query_end, 0});

    std::cout << "\nTemperatures in range [" << query_start << ", " << query_end << "]:\n";
    for (auto it = start_it; it != end_it; ++it) {
        std::cout << "  t=" << it->timestamp << ": " << it->value << "°C\n";
    }

    // Calculate statistics
    double avg =
        std::accumulate(temps.begin(), temps.end(), 0.0, [](double sum, const auto &t) { return sum + t.value; }) /
        temps.size();

    std::cout << "\nAverage temperature: " << avg << "°C\n";

    std::cout << "\n";
}

// ============================================================================
// Example 6: Reflection and Serialization
// ============================================================================

void example_reflection() {
    std::cout << "=== Example 6: Reflection and Serialization ===\n";

    struct Position {
        double x, y, z;
        auto members() { return std::tie(x, y, z); }
    };

    Stamp<Position> pos{1234567890, {1.5, 2.3, 0.8}};

    // Access via members() - manual
    std::cout << "Manual members() access:\n";
    auto tuple = pos.members();
    std::cout << "  timestamp: " << std::get<0>(tuple) << "\n";
    std::cout << "  position.x: " << std::get<1>(tuple).x << "\n";

    // Access via to_tuple() - automatic reflection
    std::cout << "\nAutomatic reflection via to_tuple():\n";
    auto reflected = to_tuple(pos);
    std::cout << "  timestamp: " << std::get<0>(reflected) << "\n";
    std::cout << "  position.x: " << std::get<1>(reflected).x << "\n";

    // Use for_each_field to iterate
    std::cout << "\nIterate fields with for_each_field():\n";
    int field_count = 0;
    for_each_field(pos, [&field_count](auto &field) { std::cout << "  Field " << field_count++ << " found\n"; });

    std::cout << "\n";
}

// ============================================================================
// Example 7: Practical Robotics Scenario
// ============================================================================

struct Pose3D {
    double x, y, z;          // Position
    double roll, pitch, yaw; // Orientation

    auto members() { return std::tie(x, y, z, roll, pitch, yaw); }
};

void example_robotics_trajectory() {
    std::cout << "=== Example 7: Robotics Trajectory ===\n";

    // Robot trajectory over time
    std::vector<Stamp<Pose3D>> trajectory;

    for (int i = 0; i < 10; ++i) {
        double t = i * 0.1; // Time in seconds
        Pose3D pose{
            t * 1.0,                   // Moving forward at 1 m/s
            0.5 * sin(t),              // Sinusoidal side motion
            0.0,                       // Ground level
            0.0,          0.0, t * 0.1 // Rotating slowly
        };

        trajectory.push_back({Stamp<Pose3D>::now(), pose});

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::cout << "Recorded " << trajectory.size() << " poses\n";

    // Calculate total distance traveled
    double total_distance = 0.0;
    for (size_t i = 1; i < trajectory.size(); ++i) {
        double dx = trajectory[i].value.x - trajectory[i - 1].value.x;
        double dy = trajectory[i].value.y - trajectory[i - 1].value.y;
        double dz = trajectory[i].value.z - trajectory[i - 1].value.z;
        total_distance += std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    auto duration_s = (trajectory.back().timestamp - trajectory.front().timestamp) / 1e9;
    double avg_speed = total_distance / duration_s;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Total distance: " << total_distance << " m\n";
    std::cout << "Duration: " << duration_s << " s\n";
    std::cout << "Average speed: " << avg_speed << " m/s\n";

    std::cout << "\n";
}

// ============================================================================
// Example 8: Comparison and Sorting
// ============================================================================

void example_comparison() {
    std::cout << "=== Example 8: Comparison and Sorting ===\n";

    Stamp<int> s1{1000, 10};
    Stamp<int> s2{2000, 20};
    Stamp<int> s3{1000, 30}; // Same timestamp as s1

    std::cout << "s1.timestamp = " << s1.timestamp << ", value = " << s1.value << "\n";
    std::cout << "s2.timestamp = " << s2.timestamp << ", value = " << s2.value << "\n";
    std::cout << "s3.timestamp = " << s3.timestamp << ", value = " << s3.value << "\n\n";

    std::cout << "Comparisons (by timestamp only):\n";
    std::cout << "  s1 < s2: " << (s1 < s2 ? "true" : "false") << "\n";
    std::cout << "  s1 == s3: " << (s1 == s3 ? "true" : "false") << " (same timestamp, different values)\n";
    std::cout << "  s2 > s1: " << (s2 > s1 ? "true" : "false") << "\n";

    std::cout << "\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         Stamp<T> Usage Examples - datapod library          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    example_basic_usage();
    example_sensor_readings();
    example_event_logging();
    example_financial_ticks();
    example_time_series_operations();
    example_reflection();
    example_robotics_trajectory();
    example_comparison();

    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     All examples complete!                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    return 0;
}
