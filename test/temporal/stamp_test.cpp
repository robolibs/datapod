#include <doctest/doctest.h>

#include <algorithm>
#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/pods/temporal/stamp.hpp>
#include <thread>
#include <vector>

using namespace datapod;

// ============================================================================
// TEST: Construction
// ============================================================================

TEST_CASE("Stamp - Default Construction") {
    Stamp<int> s;
    // Default constructed, values are uninitialized (expected)
    CHECK(true); // Just verify it compiles
}

TEST_CASE("Stamp - Construction with timestamp and value") {
    Stamp<double> s{1234567890000000LL, 23.5};
    CHECK(s.timestamp == 1234567890000000LL);
    CHECK(s.value == doctest::Approx(23.5));
}

TEST_CASE("Stamp - Construction with value only (uses current time)") {
    auto before = Stamp<int>::now();
    Stamp<int> s{42};
    auto after = Stamp<int>::now();

    CHECK(s.value == 42);
    CHECK(s.timestamp >= before);
    CHECK(s.timestamp <= after);
}

TEST_CASE("Stamp - Construction with struct value") {
    struct Vec3 {
        double x, y, z;
        auto members() { return std::tie(x, y, z); }
    };

    Stamp<Vec3> s{1000000, {1.0, 2.0, 3.0}};
    CHECK(s.timestamp == 1000000);
    CHECK(s.value.x == doctest::Approx(1.0));
    CHECK(s.value.y == doctest::Approx(2.0));
    CHECK(s.value.z == doctest::Approx(3.0));
}

// ============================================================================
// TEST: Time Utilities
// ============================================================================

TEST_CASE("Stamp - now() returns valid timestamp") {
    auto ts1 = Stamp<int>::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto ts2 = Stamp<int>::now();

    CHECK(ts2 > ts1);
    CHECK((ts2 - ts1) >= 10'000'000); // At least 10ms
}

TEST_CASE("Stamp - age() returns elapsed time") {
    Stamp<int> s{Stamp<int>::now() - 1'000'000'000, 42}; // 1 second ago
    auto age = s.age();

    CHECK(age >= 1'000'000'000); // At least 1 second
    CHECK(age < 2'000'000'000);  // Less than 2 seconds
}

TEST_CASE("Stamp - seconds() converts timestamp") {
    Stamp<int> s{5'000'000'000, 42}; // 5 seconds in nanoseconds
    CHECK(s.seconds() == doctest::Approx(5.0));
}

TEST_CASE("Stamp - milliseconds() converts timestamp") {
    Stamp<int> s{1'500'000'000, 42}; // 1.5 seconds = 1500 ms
    CHECK(s.milliseconds() == 1500);
}

TEST_CASE("Stamp - microseconds() converts timestamp") {
    Stamp<int> s{2'500'000, 42}; // 2.5 ms = 2500 microseconds
    CHECK(s.microseconds() == 2500);
}

// ============================================================================
// TEST: Comparison Operators
// ============================================================================

TEST_CASE("Stamp - operator< compares by timestamp") {
    Stamp<int> s1{1000, 1};
    Stamp<int> s2{2000, 2};
    Stamp<int> s3{1000, 3}; // Same timestamp, different value

    CHECK(s1 < s2);
    CHECK_FALSE(s2 < s1);
    CHECK_FALSE(s1 < s3); // Equal timestamps
}

TEST_CASE("Stamp - operator> compares by timestamp") {
    Stamp<int> s1{1000, 1};
    Stamp<int> s2{2000, 2};

    CHECK(s2 > s1);
    CHECK_FALSE(s1 > s2);
}

TEST_CASE("Stamp - operator<= compares by timestamp") {
    Stamp<int> s1{1000, 1};
    Stamp<int> s2{2000, 2};
    Stamp<int> s3{1000, 3};

    CHECK(s1 <= s2);
    CHECK(s1 <= s3);
    CHECK_FALSE(s2 <= s1);
}

TEST_CASE("Stamp - operator>= compares by timestamp") {
    Stamp<int> s1{1000, 1};
    Stamp<int> s2{2000, 2};
    Stamp<int> s3{1000, 3};

    CHECK(s2 >= s1);
    CHECK(s1 >= s3);
    CHECK_FALSE(s1 >= s2);
}

TEST_CASE("Stamp - operator== compares by timestamp") {
    Stamp<int> s1{1000, 1};
    Stamp<int> s2{1000, 2}; // Same timestamp, different value
    Stamp<int> s3{2000, 1};

    CHECK(s1 == s2); // Equal timestamps
    CHECK_FALSE(s1 == s3);
}

TEST_CASE("Stamp - operator!= compares by timestamp") {
    Stamp<int> s1{1000, 1};
    Stamp<int> s2{1000, 2}; // Same timestamp, different value
    Stamp<int> s3{2000, 1};

    CHECK(s1 != s3);
    CHECK_FALSE(s1 != s2); // Equal timestamps
}

// ============================================================================
// TEST: Factory Methods
// ============================================================================

TEST_CASE("Stamp - from_seconds() creates stamp from double seconds") {
    auto s = Stamp<int>::from_seconds(2.5, 42);
    CHECK(s.timestamp == 2'500'000'000);
    CHECK(s.value == 42);
}

TEST_CASE("Stamp - from_milliseconds() creates stamp from milliseconds") {
    auto s = Stamp<int>::from_milliseconds(1500, 42);
    CHECK(s.timestamp == 1'500'000'000);
    CHECK(s.value == 42);
}

TEST_CASE("Stamp - from_microseconds() creates stamp from microseconds") {
    auto s = Stamp<int>::from_microseconds(2500, 42);
    CHECK(s.timestamp == 2'500'000);
    CHECK(s.value == 42);
}

// ============================================================================
// TEST: STL Compatibility
// ============================================================================

TEST_CASE("Stamp - works with std::vector") {
    std::vector<Stamp<double>> data;
    data.push_back({1000, 1.0});
    data.push_back({2000, 2.0});
    data.push_back({3000, 3.0});

    CHECK(data.size() == 3);
    CHECK(data[0].timestamp == 1000);
    CHECK(data[1].value == doctest::Approx(2.0));
}

TEST_CASE("Stamp - works with std::sort") {
    std::vector<Stamp<int>> data = {{3000, 3}, {1000, 1}, {2000, 2}};

    std::sort(data.begin(), data.end());

    CHECK(data[0].timestamp == 1000);
    CHECK(data[1].timestamp == 2000);
    CHECK(data[2].timestamp == 3000);
}

TEST_CASE("Stamp - works with std::lower_bound for time range queries") {
    std::vector<Stamp<int>> data = {{1000, 1}, {2000, 2}, {3000, 3}, {4000, 4}, {5000, 5}};

    // Find first element >= 2500
    auto it = std::lower_bound(data.begin(), data.end(), Stamp<int>{2500, 0});

    CHECK(it != data.end());
    CHECK(it->timestamp == 3000);
}

TEST_CASE("Stamp - works with std::upper_bound for time range queries") {
    std::vector<Stamp<int>> data = {{1000, 1}, {2000, 2}, {3000, 3}, {4000, 4}, {5000, 5}};

    // Find first element > 3000
    auto it = std::upper_bound(data.begin(), data.end(), Stamp<int>{3000, 0});

    CHECK(it != data.end());
    CHECK(it->timestamp == 4000);
}

// ============================================================================
// TEST: Reflection & Serialization
// ============================================================================

TEST_CASE("Stamp - has members() for reflection") {
    Stamp<double> s{1234567890, 23.5};

    auto tuple = s.members();
    CHECK(std::get<0>(tuple) == 1234567890);
    CHECK(std::get<1>(tuple) == doctest::Approx(23.5));
}

TEST_CASE("Stamp - works with to_tuple reflection") {
    Stamp<int> s{1000, 42};

    auto tuple = to_tuple(s);
    CHECK(std::get<0>(tuple) == 1000);
    CHECK(std::get<1>(tuple) == 42);
}

TEST_CASE("Stamp - works with for_each_field reflection") {
    Stamp<int> s{1000, 42};

    int count = 0;
    for_each_field(s, [&count](auto &field) { count++; });

    CHECK(count == 2); // timestamp + value
}

TEST_CASE("Stamp - const members() works") {
    const Stamp<int> s{1000, 42};

    auto tuple = s.members();
    CHECK(std::get<0>(tuple) == 1000);
    CHECK(std::get<1>(tuple) == 42);
}

// ============================================================================
// TEST: Type Aliases
// ============================================================================

TEST_CASE("Stamp - StampedDouble alias works") {
    StampedDouble s{1000, 3.14};
    CHECK(s.timestamp == 1000);
    CHECK(s.value == doctest::Approx(3.14));
}

TEST_CASE("Stamp - StampedFloat alias works") {
    StampedFloat s{1000, 2.71f};
    CHECK(s.timestamp == 1000);
    CHECK(s.value == doctest::Approx(2.71f));
}

TEST_CASE("Stamp - StampedInt alias works") {
    StampedInt s{1000, 42};
    CHECK(s.timestamp == 1000);
    CHECK(s.value == 42);
}

TEST_CASE("Stamp - StampedLong alias works") {
    StampedLong s{1000, 9876543210LL};
    CHECK(s.timestamp == 1000);
    CHECK(s.value == 9876543210LL);
}

// ============================================================================
// TEST: Practical Use Cases
// ============================================================================

TEST_CASE("Stamp - Sensor reading time series") {
    struct SensorReading {
        double temperature;
        double humidity;
        auto members() { return std::tie(temperature, humidity); }
    };

    std::vector<Stamp<SensorReading>> readings;
    readings.push_back({1000, {23.5, 65.0}});
    readings.push_back({2000, {23.8, 66.2}});
    readings.push_back({3000, {24.1, 67.5}});

    CHECK(readings.size() == 3);
    CHECK(readings[1].value.temperature == doctest::Approx(23.8));
}

TEST_CASE("Stamp - Event logging") {
    struct LogEvent {
        int level;
        const char *message;
    };

    Stamp<LogEvent> event{Stamp<LogEvent>::now(), {1, "System started"}};

    CHECK(event.value.level == 1);
    CHECK(std::string(event.value.message) == "System started");
}

TEST_CASE("Stamp - Financial tick data") {
    struct Tick {
        double price;
        int64_t volume;
        auto members() { return std::tie(price, volume); }
    };

    std::vector<Stamp<Tick>> ticks;
    ticks.push_back({1000, {100.50, 1000}});
    ticks.push_back({1001, {100.52, 500}});
    ticks.push_back({1002, {100.48, 750}});

    // Calculate VWAP
    double total_value = 0.0;
    int64_t total_volume = 0;
    for (const auto &tick : ticks) {
        total_value += tick.value.price * tick.value.volume;
        total_volume += tick.value.volume;
    }
    double vwap = total_value / total_volume;

    CHECK(vwap == doctest::Approx(100.5).epsilon(0.01));
}

TEST_CASE("Stamp - Robotics position tracking") {
    struct Position {
        double x, y, z;
        auto members() { return std::tie(x, y, z); }
    };

    std::vector<Stamp<Position>> trajectory;
    trajectory.push_back({1'000'000'000, {0.0, 0.0, 0.0}}); // 1 second
    trajectory.push_back({2'000'000'000, {1.0, 0.5, 0.1}}); // 2 seconds
    trajectory.push_back({3'000'000'000, {2.0, 1.0, 0.2}}); // 3 seconds

    // Calculate average velocity
    auto dt = (trajectory.back().timestamp - trajectory.front().timestamp) / 1e9;
    auto dx = trajectory.back().value.x - trajectory.front().value.x;
    auto velocity = dx / dt;

    CHECK(velocity == doctest::Approx(1.0)); // 2.0m / 2.0s = 1.0 m/s
}
