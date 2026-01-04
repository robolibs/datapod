#include <doctest/doctest.h>

#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/pods/temporal/circular_buffer.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction
// ============================================================================

TEST_CASE("CircularTimeBuffer - Default Construction") {
    CircularTimeBuffer<int, 10> buf;
    CHECK(buf.empty());
    CHECK(buf.get_size() == 0);
    CHECK(buf.capacity() == 10);
}

TEST_CASE("CircularTimeBuffer - Capacity is compile-time constant") {
    CircularTimeBuffer<double, 5> buf5;
    CircularTimeBuffer<double, 100> buf100;

    CHECK(buf5.capacity() == 5);
    CHECK(buf100.capacity() == 100);
}

// ============================================================================
// TEST: Capacity
// ============================================================================

TEST_CASE("CircularTimeBuffer - empty and full") {
    CircularTimeBuffer<int, 3> buf;

    CHECK(buf.empty());
    CHECK_FALSE(buf.full());

    buf.push(1000, 1);
    CHECK_FALSE(buf.empty());
    CHECK_FALSE(buf.full());

    buf.push(2000, 2);
    buf.push(3000, 3);
    CHECK_FALSE(buf.empty());
    CHECK(buf.full());
}

TEST_CASE("CircularTimeBuffer - clear") {
    CircularTimeBuffer<int, 5> buf;
    buf.push(1000, 1);
    buf.push(2000, 2);
    CHECK(buf.get_size() == 2);

    buf.clear();
    CHECK(buf.empty());
    CHECK(buf.get_size() == 0);
}

// ============================================================================
// TEST: Modifiers
// ============================================================================

TEST_CASE("CircularTimeBuffer - push timestamp and value") {
    CircularTimeBuffer<double, 5> buf;

    buf.push(1000, 1.0);
    buf.push(2000, 2.0);
    buf.push(3000, 3.0);

    CHECK(buf.get_size() == 3);
    CHECK(buf[0].timestamp == 1000);
    CHECK(buf[0].value == doctest::Approx(1.0));
    CHECK(buf[2].timestamp == 3000);
    CHECK(buf[2].value == doctest::Approx(3.0));
}

TEST_CASE("CircularTimeBuffer - push Stamp") {
    CircularTimeBuffer<int, 5> buf;

    Stamp<int> s1{1000, 10};
    Stamp<int> s2{2000, 20};

    buf.push(s1);
    buf.push(s2);

    CHECK(buf.get_size() == 2);
    CHECK(buf[0].value == 10);
    CHECK(buf[1].value == 20);
}

TEST_CASE("CircularTimeBuffer - overwrite when full") {
    CircularTimeBuffer<int, 3> buf;

    // Fill the buffer
    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);
    CHECK(buf.full());
    CHECK(buf.oldest().value == 1);

    // Push another - should overwrite oldest
    buf.push(4000, 4);
    CHECK(buf.full());
    CHECK(buf.get_size() == 3);
    CHECK(buf.oldest().value == 2); // 1 was overwritten
    CHECK(buf.newest().value == 4);

    // Check all values
    CHECK(buf[0].value == 2);
    CHECK(buf[1].value == 3);
    CHECK(buf[2].value == 4);
}

// ============================================================================
// TEST: Element Access
// ============================================================================

TEST_CASE("CircularTimeBuffer - operator[]") {
    CircularTimeBuffer<int, 5> buf;
    buf.push(1000, 10);
    buf.push(2000, 20);
    buf.push(3000, 30);

    auto s0 = buf[0];
    auto s1 = buf[1];
    auto s2 = buf[2];

    CHECK(s0.timestamp == 1000);
    CHECK(s0.value == 10);
    CHECK(s1.timestamp == 2000);
    CHECK(s1.value == 20);
    CHECK(s2.timestamp == 3000);
    CHECK(s2.value == 30);
}

TEST_CASE("CircularTimeBuffer - at with bounds check") {
    CircularTimeBuffer<int, 5> buf;
    buf.push(1000, 42);

    CHECK(buf.at(0).value == 42);
    CHECK_THROWS_AS(buf.at(1), std::out_of_range);
    CHECK_THROWS_AS(buf.at(5), std::out_of_range);
}

TEST_CASE("CircularTimeBuffer - newest and oldest") {
    CircularTimeBuffer<int, 5> buf;

    CHECK_THROWS_AS(buf.newest(), std::out_of_range);
    CHECK_THROWS_AS(buf.oldest(), std::out_of_range);

    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);

    CHECK(buf.oldest().timestamp == 1000);
    CHECK(buf.oldest().value == 1);
    CHECK(buf.newest().timestamp == 3000);
    CHECK(buf.newest().value == 3);
}

TEST_CASE("CircularTimeBuffer - newest and oldest after wrapping") {
    CircularTimeBuffer<int, 3> buf;

    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);
    buf.push(4000, 4); // Wraps around
    buf.push(5000, 5); // Wraps around again

    CHECK(buf.oldest().value == 3);
    CHECK(buf.newest().value == 5);
}

// ============================================================================
// TEST: Iteration
// ============================================================================

TEST_CASE("CircularTimeBuffer - iteration in chronological order") {
    CircularTimeBuffer<int, 5> buf;
    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);

    int expected_value = 1;
    for (auto stamp : buf) {
        CHECK(stamp.value == expected_value);
        expected_value++;
    }
}

TEST_CASE("CircularTimeBuffer - iteration after wrapping") {
    CircularTimeBuffer<int, 3> buf;

    // Fill and wrap
    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);
    buf.push(4000, 4);
    buf.push(5000, 5);

    // Should iterate 3, 4, 5 (oldest to newest)
    int expected_values[] = {3, 4, 5};
    int idx = 0;
    for (auto stamp : buf) {
        CHECK(stamp.value == expected_values[idx]);
        idx++;
    }
    CHECK(idx == 3);
}

TEST_CASE("CircularTimeBuffer - iterator increment") {
    CircularTimeBuffer<int, 5> buf;
    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);

    auto it = buf.begin();
    CHECK((*it).value == 1);

    ++it;
    CHECK((*it).value == 2);

    it++;
    CHECK((*it).value == 3);

    ++it;
    CHECK(it == buf.end());
}

// ============================================================================
// TEST: Aggregations
// ============================================================================

TEST_CASE("CircularTimeBuffer - mean") {
    CircularTimeBuffer<double, 5> buf;
    buf.push(1000, 10.0);
    buf.push(2000, 20.0);
    buf.push(3000, 30.0);

    CHECK(buf.mean() == doctest::Approx(20.0));
}

TEST_CASE("CircularTimeBuffer - min and max") {
    CircularTimeBuffer<double, 5> buf;
    buf.push(1000, 5.0);
    buf.push(2000, 2.0);
    buf.push(3000, 8.0);
    buf.push(4000, 1.0);
    buf.push(5000, 6.0);

    CHECK(buf.min() == doctest::Approx(1.0));
    CHECK(buf.max() == doctest::Approx(8.0));
}

TEST_CASE("CircularTimeBuffer - aggregations on empty buffer") {
    CircularTimeBuffer<double, 5> buf;

    // Should return default-constructed values
    CHECK(buf.mean() == doctest::Approx(0.0));
    CHECK(buf.min() == doctest::Approx(0.0));
    CHECK(buf.max() == doctest::Approx(0.0));
}

// ============================================================================
// TEST: Time Utilities
// ============================================================================

TEST_CASE("CircularTimeBuffer - duration") {
    CircularTimeBuffer<int, 5> buf;
    CHECK(buf.duration() == 0);

    buf.push(1000, 1);
    CHECK(buf.duration() == 0);

    buf.push(3000, 3);
    CHECK(buf.duration() == 2000);

    buf.push(5000, 5);
    CHECK(buf.duration() == 4000);
}

TEST_CASE("CircularTimeBuffer - start_time and end_time") {
    CircularTimeBuffer<int, 5> buf;
    CHECK(buf.start_time() == 0);
    CHECK(buf.end_time() == 0);

    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);

    CHECK(buf.start_time() == 1000);
    CHECK(buf.end_time() == 3000);
}

TEST_CASE("CircularTimeBuffer - time utilities after wrapping") {
    CircularTimeBuffer<int, 3> buf;

    buf.push(1000, 1);
    buf.push(2000, 2);
    buf.push(3000, 3);
    buf.push(4000, 4);

    CHECK(buf.start_time() == 2000); // Oldest is now 2
    CHECK(buf.end_time() == 4000);   // Newest is 4
    CHECK(buf.duration() == 2000);
}

// ============================================================================
// TEST: Reflection & Serialization
// ============================================================================

TEST_CASE("CircularTimeBuffer - has members() for reflection") {
    CircularTimeBuffer<double, 5> buf;
    buf.push(1000, 1.5);
    buf.push(2000, 2.5);

    auto tuple = buf.members();
    auto &times = std::get<0>(tuple);
    auto &vals = std::get<1>(tuple);
    auto &head = std::get<2>(tuple);
    auto &size = std::get<3>(tuple);

    CHECK(size == 2);
    CHECK(head == 2);
    CHECK(times[0] == 1000);
    CHECK(vals[0] == doctest::Approx(1.5));
}

TEST_CASE("CircularTimeBuffer - works with to_tuple reflection") {
    CircularTimeBuffer<int, 5> buf;
    buf.push(1000, 42);

    auto tuple = to_tuple(buf);
    auto &size = std::get<3>(tuple);

    CHECK(size == 1);
}

TEST_CASE("CircularTimeBuffer - works with for_each_field reflection") {
    CircularTimeBuffer<int, 5> buf;
    buf.push(1000, 42);

    int count = 0;
    for_each_field(buf, [&count](auto &field) { count++; });

    CHECK(count == 4); // timestamps, values, head, size
}

// ============================================================================
// TEST: Practical Use Cases
// ============================================================================

TEST_CASE("CircularTimeBuffer - IMU sensor history") {
    struct IMU {
        double accel_x, accel_y, accel_z;
        double gyro_x, gyro_y, gyro_z;

        auto members() { return std::tie(accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z); }
    };

    CircularTimeBuffer<IMU, 100> imu_history;

    // Simulate 150 readings (will wrap)
    for (int i = 0; i < 150; ++i) {
        IMU reading{0.1 * i, 0.2 * i, 9.81, 0.01 * i, 0.02 * i, 0.0};
        imu_history.push(i * 1000, reading);
    }

    CHECK(imu_history.full());
    CHECK(imu_history.get_size() == 100);

    // Oldest should be reading #50
    CHECK(imu_history.oldest().value.accel_x == doctest::Approx(0.1 * 50));

    // Newest should be reading #149
    CHECK(imu_history.newest().value.accel_x == doctest::Approx(0.1 * 149));
}

TEST_CASE("CircularTimeBuffer - Temperature sensor rolling window") {
    CircularTimeBuffer<double, 10> temps;

    // Simulate temperature readings
    for (int i = 0; i < 20; ++i) {
        double temp = 20.0 + i * 0.5;
        temps.push(i * 1'000'000'000LL, temp);
    }

    // Should have last 10 readings
    CHECK(temps.get_size() == 10);
    CHECK(temps.oldest().value == doctest::Approx(25.0)); // Reading #10
    CHECK(temps.newest().value == doctest::Approx(29.5)); // Reading #19

    // Calculate rolling average
    double avg = temps.mean();
    CHECK(avg == doctest::Approx(27.25));
}

TEST_CASE("CircularTimeBuffer - Event log") {
    struct Event {
        int level;
        int code;
    };

    CircularTimeBuffer<Event, 5> event_log;

    event_log.push(1000, {1, 100}); // INFO
    event_log.push(2000, {2, 200}); // WARN
    event_log.push(3000, {3, 300}); // ERROR
    event_log.push(4000, {1, 101}); // INFO
    event_log.push(5000, {2, 201}); // WARN
    event_log.push(6000, {3, 301}); // ERROR - wraps

    CHECK(event_log.get_size() == 5);

    // Oldest event should be second one (2, 200)
    CHECK(event_log.oldest().value.level == 2);
    CHECK(event_log.oldest().value.code == 200);

    // Newest event should be last one (3, 301)
    CHECK(event_log.newest().value.level == 3);
    CHECK(event_log.newest().value.code == 301);
}

TEST_CASE("CircularTimeBuffer - Fixed-size sensor window") {
    CircularTimeBuffer<double, 50> sensor;

    // Add data until full
    for (int i = 0; i < 50; ++i) {
        sensor.push(i * 100, i * 1.0);
    }

    CHECK(sensor.full());

    // Get statistics on the window
    double min_val = sensor.min();
    double max_val = sensor.max();
    double avg_val = sensor.mean();

    CHECK(min_val == doctest::Approx(0.0));
    CHECK(max_val == doctest::Approx(49.0));
    CHECK(avg_val == doctest::Approx(24.5));
}

TEST_CASE("CircularTimeBuffer - Comparison with different sizes") {
    CircularTimeBuffer<int, 5> small;
    CircularTimeBuffer<int, 100> large;

    // Fill both
    for (int i = 0; i < 10; ++i) {
        small.push(i * 1000, i);
        large.push(i * 1000, i);
    }

    CHECK(small.get_size() == 5);  // Wrapped
    CHECK(large.get_size() == 10); // Not full yet

    CHECK(small.oldest().value == 5);
    CHECK(large.oldest().value == 0);
}
