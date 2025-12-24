#include <doctest/doctest.h>

#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/temporal/time_series.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction
// ============================================================================

TEST_CASE("TimeSeries - Default Construction") {
    TimeSeries<double> ts;
    CHECK(ts.empty());
    CHECK(ts.size() == 0);
}

TEST_CASE("TimeSeries - Construction with capacity") {
    TimeSeries<int> ts(100);
    CHECK(ts.empty());
    CHECK(ts.capacity() >= 100);
}

TEST_CASE("TimeSeries - Construction from Stamps") {
    Vector<Stamp<double>> stamps = {{1000, 1.0}, {2000, 2.0}, {3000, 3.0}};

    TimeSeries<double> ts(stamps);
    CHECK(ts.size() == 3);
    CHECK(ts[0].timestamp == 1000);
    CHECK(ts[0].value == doctest::Approx(1.0));
}

// ============================================================================
// TEST: Capacity
// ============================================================================

TEST_CASE("TimeSeries - size and empty") {
    TimeSeries<int> ts;
    CHECK(ts.empty());
    CHECK(ts.size() == 0);

    ts.append(1000, 42);
    CHECK_FALSE(ts.empty());
    CHECK(ts.size() == 1);
}

TEST_CASE("TimeSeries - reserve") {
    TimeSeries<double> ts;
    ts.reserve(1000);
    CHECK(ts.capacity() >= 1000);
    CHECK(ts.empty());
}

TEST_CASE("TimeSeries - clear") {
    TimeSeries<int> ts;
    ts.append(1000, 1);
    ts.append(2000, 2);
    CHECK(ts.size() == 2);

    ts.clear();
    CHECK(ts.empty());
    CHECK(ts.size() == 0);
}

// ============================================================================
// TEST: Modifiers
// ============================================================================

TEST_CASE("TimeSeries - append timestamp and value") {
    TimeSeries<double> ts;
    ts.append(1000, 23.5);
    ts.append(2000, 24.1);

    CHECK(ts.size() == 2);
    CHECK(ts[0].timestamp == 1000);
    CHECK(ts[0].value == doctest::Approx(23.5));
    CHECK(ts[1].timestamp == 2000);
    CHECK(ts[1].value == doctest::Approx(24.1));
}

TEST_CASE("TimeSeries - append Stamp") {
    TimeSeries<int> ts;
    Stamp<int> s1{1000, 42};
    Stamp<int> s2{2000, 84};

    ts.append(s1);
    ts.append(s2);

    CHECK(ts.size() == 2);
    CHECK(ts[0].value == 42);
    CHECK(ts[1].value == 84);
}

TEST_CASE("TimeSeries - append multiple stamps") {
    TimeSeries<double> ts;
    Vector<Stamp<double>> stamps = {{1000, 1.0}, {2000, 2.0}, {3000, 3.0}};

    ts.append(stamps);
    CHECK(ts.size() == 3);
}

// ============================================================================
// TEST: Element Access
// ============================================================================

TEST_CASE("TimeSeries - operator[]") {
    TimeSeries<int> ts;
    ts.append(1000, 10);
    ts.append(2000, 20);

    auto s0 = ts[0];
    auto s1 = ts[1];

    CHECK(s0.timestamp == 1000);
    CHECK(s0.value == 10);
    CHECK(s1.timestamp == 2000);
    CHECK(s1.value == 20);
}

TEST_CASE("TimeSeries - at with bounds check") {
    TimeSeries<int> ts;
    ts.append(1000, 42);

    CHECK(ts.at(0).value == 42);
    CHECK_THROWS_AS(ts.at(1), std::out_of_range);
}

TEST_CASE("TimeSeries - front and back") {
    TimeSeries<double> ts;
    ts.append(1000, 1.0);
    ts.append(2000, 2.0);
    ts.append(3000, 3.0);

    CHECK(ts.front().value == doctest::Approx(1.0));
    CHECK(ts.back().value == doctest::Approx(3.0));
}

// ============================================================================
// TEST: Time Range Queries
// ============================================================================

TEST_CASE("TimeSeries - query empty series") {
    TimeSeries<int> ts;
    auto range = ts.query(1000, 3000);

    CHECK(range.empty());
    CHECK(range.count == 0);
}

TEST_CASE("TimeSeries - query full range") {
    TimeSeries<int> ts;
    ts.append(1000, 1);
    ts.append(2000, 2);
    ts.append(3000, 3);
    ts.append(4000, 4);
    ts.append(5000, 5);

    auto range = ts.query(1000, 6000);

    CHECK(range.count == 5);
    CHECK(range[0].value == 1);
    CHECK(range[4].value == 5);
}

TEST_CASE("TimeSeries - query partial range") {
    TimeSeries<int> ts;
    ts.append(1000, 1);
    ts.append(2000, 2);
    ts.append(3000, 3);
    ts.append(4000, 4);
    ts.append(5000, 5);

    auto range = ts.query(2000, 4000);

    CHECK(range.count == 2); // 2000 and 3000 (4000 is exclusive)
    CHECK(range[0].value == 2);
    CHECK(range[1].value == 3);
}

TEST_CASE("TimeSeries - query no matches") {
    TimeSeries<int> ts;
    ts.append(1000, 1);
    ts.append(5000, 5);

    auto range = ts.query(2000, 3000);
    CHECK(range.empty());
}

// ============================================================================
// TEST: Sorting & Validation
// ============================================================================

TEST_CASE("TimeSeries - is_sorted on empty") {
    TimeSeries<int> ts;
    CHECK(ts.is_sorted());
}

TEST_CASE("TimeSeries - is_sorted on single element") {
    TimeSeries<int> ts;
    ts.append(1000, 1);
    CHECK(ts.is_sorted());
}

TEST_CASE("TimeSeries - is_sorted when sorted") {
    TimeSeries<int> ts;
    ts.append(1000, 1);
    ts.append(2000, 2);
    ts.append(3000, 3);
    CHECK(ts.is_sorted());
}

TEST_CASE("TimeSeries - is_sorted when not sorted") {
    TimeSeries<int> ts;
    ts.append(3000, 3);
    ts.append(1000, 1);
    ts.append(2000, 2);
    CHECK_FALSE(ts.is_sorted());
}

TEST_CASE("TimeSeries - sort_by_time") {
    TimeSeries<int> ts;
    ts.append(3000, 3);
    ts.append(1000, 1);
    ts.append(5000, 5);
    ts.append(2000, 2);
    ts.append(4000, 4);

    CHECK_FALSE(ts.is_sorted());

    ts.sort_by_time();

    CHECK(ts.is_sorted());
    CHECK(ts[0].timestamp == 1000);
    CHECK(ts[1].timestamp == 2000);
    CHECK(ts[2].timestamp == 3000);
    CHECK(ts[3].timestamp == 4000);
    CHECK(ts[4].timestamp == 5000);

    // Values should follow their timestamps
    CHECK(ts[0].value == 1);
    CHECK(ts[1].value == 2);
    CHECK(ts[2].value == 3);
    CHECK(ts[3].value == 4);
    CHECK(ts[4].value == 5);
}

// ============================================================================
// TEST: Aggregations
// ============================================================================

TEST_CASE("TimeSeries - mean") {
    TimeSeries<double> ts;
    ts.append(1000, 10.0);
    ts.append(2000, 20.0);
    ts.append(3000, 30.0);

    CHECK(ts.mean() == doctest::Approx(20.0));
}

TEST_CASE("TimeSeries - sum") {
    TimeSeries<int> ts;
    ts.append(1000, 10);
    ts.append(2000, 20);
    ts.append(3000, 30);

    CHECK(ts.sum() == 60);
}

TEST_CASE("TimeSeries - min and max") {
    TimeSeries<double> ts;
    ts.append(1000, 5.0);
    ts.append(2000, 2.0);
    ts.append(3000, 8.0);
    ts.append(4000, 1.0);
    ts.append(5000, 6.0);

    CHECK(ts.min() == doctest::Approx(1.0));
    CHECK(ts.max() == doctest::Approx(8.0));
}

TEST_CASE("TimeSeries - time_at_min and time_at_max") {
    TimeSeries<double> ts;
    ts.append(1000, 5.0);
    ts.append(2000, 2.0);
    ts.append(3000, 8.0);
    ts.append(4000, 1.0);
    ts.append(5000, 6.0);

    CHECK(ts.time_at_min() == 4000);
    CHECK(ts.time_at_max() == 3000);
}

// ============================================================================
// TEST: Time Utilities
// ============================================================================

TEST_CASE("TimeSeries - duration") {
    TimeSeries<int> ts;
    CHECK(ts.duration() == 0);

    ts.append(1000, 1);
    CHECK(ts.duration() == 0);

    ts.append(3000, 3);
    CHECK(ts.duration() == 2000);
}

TEST_CASE("TimeSeries - start_time and end_time") {
    TimeSeries<int> ts;
    CHECK(ts.start_time() == 0);
    CHECK(ts.end_time() == 0);

    ts.append(1000, 1);
    ts.append(5000, 5);

    CHECK(ts.start_time() == 1000);
    CHECK(ts.end_time() == 5000);
}

// ============================================================================
// TEST: Resampling & Downsampling
// ============================================================================

TEST_CASE("TimeSeries - downsample") {
    TimeSeries<int> ts;
    for (int i = 0; i < 10; ++i) {
        ts.append(i * 1000, i);
    }

    auto ts2 = ts.downsample(2);
    CHECK(ts2.size() == 5);
    CHECK(ts2[0].value == 0);
    CHECK(ts2[1].value == 2);
    CHECK(ts2[2].value == 4);
    CHECK(ts2[3].value == 6);
    CHECK(ts2[4].value == 8);
}

TEST_CASE("TimeSeries - downsample with n=1") {
    TimeSeries<int> ts;
    ts.append(1000, 1);
    ts.append(2000, 2);

    auto ts2 = ts.downsample(1);
    CHECK(ts2.size() == ts.size());
}

// ============================================================================
// TEST: Conversion
// ============================================================================

TEST_CASE("TimeSeries - to_stamps") {
    TimeSeries<int> ts;
    ts.append(1000, 10);
    ts.append(2000, 20);
    ts.append(3000, 30);

    auto stamps = ts.to_stamps();

    CHECK(stamps.size() == 3);
    CHECK(stamps[0].timestamp == 1000);
    CHECK(stamps[0].value == 10);
    CHECK(stamps[2].timestamp == 3000);
    CHECK(stamps[2].value == 30);
}

// ============================================================================
// TEST: Reflection & Serialization
// ============================================================================

TEST_CASE("TimeSeries - has members() for reflection") {
    TimeSeries<double> ts;
    ts.append(1000, 1.5);
    ts.append(2000, 2.5);

    auto tuple = ts.members();
    auto &times = std::get<0>(tuple);
    auto &vals = std::get<1>(tuple);

    CHECK(times.size() == 2);
    CHECK(vals.size() == 2);
    CHECK(times[0] == 1000);
    CHECK(vals[0] == doctest::Approx(1.5));
}

TEST_CASE("TimeSeries - works with to_tuple reflection") {
    TimeSeries<int> ts;
    ts.append(1000, 42);

    auto tuple = to_tuple(ts);
    auto &times = std::get<0>(tuple);
    auto &vals = std::get<1>(tuple);

    CHECK(times.size() == 1);
    CHECK(vals.size() == 1);
}

TEST_CASE("TimeSeries - works with for_each_field reflection") {
    TimeSeries<int> ts;
    ts.append(1000, 42);

    int count = 0;
    for_each_field(ts, [&count](auto &field) { count++; });

    CHECK(count == 2); // timestamps + values
}

// ============================================================================
// TEST: Practical Use Cases
// ============================================================================

TEST_CASE("TimeSeries - Temperature sensor data") {
    TimeSeries<double> temps;

    // Simulate hourly temperature readings
    for (int hour = 0; hour < 24; ++hour) {
        int64_t ts = hour * 3600 * 1'000'000'000LL;                 // Hours to nanoseconds
        double temp = 20.0 + 5.0 * std::sin(hour * 3.14159 / 12.0); // Sinusoidal variation
        temps.append(ts, temp);
    }

    CHECK(temps.size() == 24);
    CHECK(temps.duration() == 23 * 3600 * 1'000'000'000LL);

    // Find hottest and coldest times
    auto min_temp = temps.min();
    auto max_temp = temps.max();

    CHECK(min_temp < 16.0);
    CHECK(max_temp > 24.0);
}

TEST_CASE("TimeSeries - Stock prices") {
    TimeSeries<double> prices;

    prices.append(1000, 100.0);
    prices.append(2000, 101.5);
    prices.append(3000, 99.8);
    prices.append(4000, 102.3);
    prices.append(5000, 101.0);

    // Calculate price range
    double range = prices.max() - prices.min();
    CHECK(range == doctest::Approx(2.5).epsilon(0.01));

    // Query specific time window
    auto window = prices.query(2000, 4000);
    CHECK(window.count == 2);
}

TEST_CASE("TimeSeries - Filtering with struct") {
    struct SensorReading {
        double temperature;
        double humidity;

        auto members() { return std::tie(temperature, humidity); }
    };

    TimeSeries<SensorReading> sensors;
    sensors.append(1000, {23.5, 65.0});
    sensors.append(2000, {24.1, 66.5});
    sensors.append(3000, {23.8, 64.2});

    CHECK(sensors.size() == 3);
    CHECK(sensors[1].value.temperature == doctest::Approx(24.1));
    CHECK(sensors[2].value.humidity == doctest::Approx(64.2));
}

TEST_CASE("TimeSeries - Unsorted data workflow") {
    TimeSeries<int> ts;

    // Data arrives out of order
    ts.append(3000, 3);
    ts.append(1000, 1);
    ts.append(5000, 5);
    ts.append(2000, 2);
    ts.append(4000, 4);

    CHECK_FALSE(ts.is_sorted());

    // Sort before querying
    ts.sort_by_time();
    CHECK(ts.is_sorted());

    // Now queries work correctly
    auto range = ts.query(2000, 4000);
    CHECK(range.count == 2);
    CHECK(range[0].value == 2);
    CHECK(range[1].value == 3);
}
