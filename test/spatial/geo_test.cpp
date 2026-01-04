#include <doctest/doctest.h>

#include <cmath>
#include <datapod/pods/spatial/geo.hpp>

using namespace datapod;

TEST_SUITE("Geo") {
    TEST_CASE("Default construction") {
        Geo nav;
        CHECK(nav.latitude == 0.0);
        CHECK(nav.longitude == 0.0);
        CHECK(nav.altitude == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Geo nav{37.7749, -122.4194, 16.0}; // San Francisco
        CHECK(nav.latitude == 37.7749);
        CHECK(nav.longitude == -122.4194);
        CHECK(nav.altitude == 16.0);
    }

    TEST_CASE("is_set - false for origin") {
        Geo nav{0.0, 0.0, 0.0};
        CHECK_FALSE(nav.is_set());
    }

    TEST_CASE("is_set - true with latitude") {
        Geo nav{37.7749, 0.0, 0.0};
        CHECK(nav.is_set());
    }

    TEST_CASE("is_set - true with longitude") {
        Geo nav{0.0, -122.4194, 0.0};
        CHECK(nav.is_set());
    }

    TEST_CASE("is_set - true with altitude") {
        Geo nav{0.0, 0.0, 100.0};
        CHECK(nav.is_set());
    }

    TEST_CASE("has_altitude - true for normal altitude") {
        Geo nav{37.7749, -122.4194, 16.0};
        CHECK(nav.has_altitude());
    }

    TEST_CASE("has_altitude - false for NaN altitude") {
        Geo nav{37.7749, -122.4194, NAN};
        CHECK_FALSE(nav.has_altitude());
    }

    TEST_CASE("is_valid - true for valid coordinates") {
        Geo nav{37.7749, -122.4194, 16.0};
        CHECK(nav.is_valid());
    }

    TEST_CASE("is_valid - false for latitude > 90") {
        Geo nav{91.0, -122.4194, 16.0};
        CHECK_FALSE(nav.is_valid());
    }

    TEST_CASE("is_valid - false for latitude < -90") {
        Geo nav{-91.0, -122.4194, 16.0};
        CHECK_FALSE(nav.is_valid());
    }

    TEST_CASE("is_valid - false for longitude > 180") {
        Geo nav{37.7749, 181.0, 16.0};
        CHECK_FALSE(nav.is_valid());
    }

    TEST_CASE("is_valid - false for longitude < -180") {
        Geo nav{37.7749, -181.0, 16.0};
        CHECK_FALSE(nav.is_valid());
    }

    TEST_CASE("is_valid - boundary cases") {
        CHECK(Geo{90.0, 180.0, 0.0}.is_valid());
        CHECK(Geo{-90.0, -180.0, 0.0}.is_valid());
        CHECK(Geo{0.0, 0.0, 0.0}.is_valid());
    }

    TEST_CASE("distance_to - same location") {
        Geo nav{37.7749, -122.4194, 16.0};
        double dist = nav.distance_to(nav);
        CHECK(dist == doctest::Approx(0.0).epsilon(0.01));
    }

    TEST_CASE("distance_to - SF to NY (~4000 km)") {
        Geo sf{37.7749, -122.4194, 16.0};
        Geo ny{40.7128, -74.0060, 10.0};
        double dist = sf.distance_to(ny);
        // Approximate great circle distance
        CHECK(dist == doctest::Approx(4130000.0).epsilon(0.01)); // ~4130 km
    }

    TEST_CASE("distance_to - short distance") {
        Geo point1{37.4219, -122.0840, 0.0}; // Stanford
        Geo point2{37.4275, -122.1697, 0.0}; // ~7km away
        double dist = point1.distance_to(point2);
        CHECK(dist > 7000.0);
        CHECK(dist < 8000.0);
    }

    TEST_CASE("bearing_to - eastward") {
        Geo west{37.0, -122.0, 0.0};
        Geo east{37.0, -121.0, 0.0};
        double bearing = west.bearing_to(east);
        // Should be close to 90 degrees (π/2 radians) = East
        CHECK(bearing == doctest::Approx(M_PI / 2.0).epsilon(0.01));
    }

    TEST_CASE("bearing_to - northward") {
        Geo south{37.0, -122.0, 0.0};
        Geo north{38.0, -122.0, 0.0};
        double bearing = south.bearing_to(north);
        // Should be close to 0 degrees (0 radians) = North
        CHECK(bearing == doctest::Approx(0.0).epsilon(0.01));
    }

    TEST_CASE("bearing_to - range [0, 2π)") {
        Geo origin{37.0, -122.0, 0.0};
        Geo target{36.0, -121.0, 0.0};
        double bearing = origin.bearing_to(target);
        CHECK(bearing >= 0.0);
        CHECK(bearing < 2.0 * M_PI);
    }

    TEST_CASE("operator== equality") {
        Geo nav1{37.7749, -122.4194, 16.0};
        Geo nav2{37.7749, -122.4194, 16.0};
        CHECK(nav1 == nav2);
    }

    TEST_CASE("operator!= inequality - different latitude") {
        Geo nav1{37.7749, -122.4194, 16.0};
        Geo nav2{40.7128, -122.4194, 16.0};
        CHECK(nav1 != nav2);
    }

    TEST_CASE("operator!= inequality - different longitude") {
        Geo nav1{37.7749, -122.4194, 16.0};
        Geo nav2{37.7749, -74.0060, 16.0};
        CHECK(nav1 != nav2);
    }

    TEST_CASE("operator!= inequality - different altitude") {
        Geo nav1{37.7749, -122.4194, 16.0};
        Geo nav2{37.7749, -122.4194, 100.0};
        CHECK(nav1 != nav2);
    }

    TEST_CASE("members() reflection") {
        Geo nav;
        auto m = nav.members();
        CHECK(&std::get<0>(m) == &nav.latitude);
        CHECK(&std::get<1>(m) == &nav.longitude);
        CHECK(&std::get<2>(m) == &nav.altitude);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Geo>);
        CHECK(std::is_trivially_copyable_v<Geo>);
    }

    TEST_CASE("GPS waypoint navigation use case") {
        // Robot waypoint at Stanford University
        Geo waypoint{37.4219, -122.0840, 0.0};
        CHECK(waypoint.latitude == 37.4219);
        CHECK(waypoint.longitude == -122.0840);
        CHECK(waypoint.is_valid());
    }

    TEST_CASE("Drone altitude tracking") {
        // Drone at 100m altitude
        Geo drone_pos{37.4219, -122.0840, 100.0};
        CHECK(drone_pos.altitude == 100.0);
        CHECK(drone_pos.has_altitude());
    }

    TEST_CASE("Equator and prime meridian") {
        // Null Island (0°, 0°)
        Geo null_island{0.0, 0.0, 0.0};
        CHECK(null_island.is_valid());
        CHECK_FALSE(null_island.is_set()); // All zeros
    }

    TEST_CASE("Extreme valid coordinates") {
        // North Pole
        Geo north_pole{90.0, 0.0, 0.0};
        CHECK(north_pole.is_valid());

        // South Pole
        Geo south_pole{-90.0, 0.0, 0.0};
        CHECK(south_pole.is_valid());
    }

    TEST_CASE("Negative altitude (below sea level)") {
        // Dead Sea (lowest point on Earth, ~-430m)
        Geo dead_sea{31.5, 35.5, -430.0};
        CHECK(dead_sea.altitude == -430.0);
        CHECK(dead_sea.has_altitude());
        CHECK(dead_sea.is_valid());
    }
}
