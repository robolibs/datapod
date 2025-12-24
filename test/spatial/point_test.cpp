#include <doctest/doctest.h>

#include <datapod/spatial/point.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Point - Default construction") {
    Point p;
    CHECK(p.x == 0.0);
    CHECK(p.y == 0.0);
    CHECK(p.z == 0.0);
}

TEST_CASE("Point - Aggregate initialization") {
    Point p{1.0, 2.0, 3.0};
    CHECK(p.x == 1.0);
    CHECK(p.y == 2.0);
    CHECK(p.z == 3.0);
}

TEST_CASE("Point - members() reflection") {
    Point p{1.0, 2.0, 3.0};
    auto m = p.members();
    CHECK(&std::get<0>(m) == &p.x);
    CHECK(&std::get<1>(m) == &p.y);
    CHECK(&std::get<2>(m) == &p.z);
}

TEST_CASE("Point - const members() reflection") {
    const Point p{1.0, 2.0, 3.0};
    auto m = p.members();
    CHECK(&std::get<0>(m) == &p.x);
    CHECK(&std::get<1>(m) == &p.y);
    CHECK(&std::get<2>(m) == &p.z);
}

// ============================================================================
// TEST: Magnitude and Distance
// ============================================================================

TEST_CASE("Point - magnitude at origin") {
    Point p{0.0, 0.0, 0.0};
    CHECK(p.magnitude() == 0.0);
}

TEST_CASE("Point - magnitude on X axis") {
    Point p{3.0, 0.0, 0.0};
    CHECK(p.magnitude() == doctest::Approx(3.0));
}

TEST_CASE("Point - magnitude on Y axis") {
    Point p{0.0, 4.0, 0.0};
    CHECK(p.magnitude() == doctest::Approx(4.0));
}

TEST_CASE("Point - magnitude on Z axis") {
    Point p{0.0, 0.0, 5.0};
    CHECK(p.magnitude() == doctest::Approx(5.0));
}

TEST_CASE("Point - magnitude 3-4-5 triangle") {
    Point p{3.0, 4.0, 0.0};
    CHECK(p.magnitude() == doctest::Approx(5.0));
}

TEST_CASE("Point - magnitude 3D") {
    Point p{1.0, 2.0, 2.0};
    CHECK(p.magnitude() == doctest::Approx(3.0)); // sqrt(1+4+4) = 3
}

TEST_CASE("Point - distance_to same point") {
    Point p1{1.0, 2.0, 3.0};
    Point p2{1.0, 2.0, 3.0};
    CHECK(p1.distance_to(p2) == doctest::Approx(0.0));
}

TEST_CASE("Point - distance_to along X axis") {
    Point p1{0.0, 0.0, 0.0};
    Point p2{5.0, 0.0, 0.0};
    CHECK(p1.distance_to(p2) == doctest::Approx(5.0));
}

TEST_CASE("Point - distance_to 3-4-5 triangle") {
    Point p1{0.0, 0.0, 0.0};
    Point p2{3.0, 4.0, 0.0};
    CHECK(p1.distance_to(p2) == doctest::Approx(5.0));
}

TEST_CASE("Point - distance_to 3D") {
    Point p1{1.0, 2.0, 3.0};
    Point p2{4.0, 6.0, 3.0};
    CHECK(p1.distance_to(p2) == doctest::Approx(5.0)); // sqrt(9+16) = 5
}

TEST_CASE("Point - distance_to_2d ignores Z") {
    Point p1{0.0, 0.0, 0.0};
    Point p2{3.0, 4.0, 100.0};
    CHECK(p1.distance_to_2d(p2) == doctest::Approx(5.0)); // Only X,Y matter
}

TEST_CASE("Point - distance_to_2d same XY different Z") {
    Point p1{1.0, 2.0, 10.0};
    Point p2{1.0, 2.0, 50.0};
    CHECK(p1.distance_to_2d(p2) == doctest::Approx(0.0));
}

// ============================================================================
// TEST: Utility
// ============================================================================

TEST_CASE("Point - is_set returns false at origin") {
    Point p{0.0, 0.0, 0.0};
    CHECK_FALSE(p.is_set());
}

TEST_CASE("Point - is_set returns true with X") {
    Point p{1.0, 0.0, 0.0};
    CHECK(p.is_set());
}

TEST_CASE("Point - is_set returns true with Y") {
    Point p{0.0, 1.0, 0.0};
    CHECK(p.is_set());
}

TEST_CASE("Point - is_set returns true with Z") {
    Point p{0.0, 0.0, 1.0};
    CHECK(p.is_set());
}

TEST_CASE("Point - is_set returns true with all coords") {
    Point p{1.0, 2.0, 3.0};
    CHECK(p.is_set());
}

// ============================================================================
// TEST: Operators
// ============================================================================

TEST_CASE("Point - operator+ adds components") {
    Point p1{1.0, 2.0, 3.0};
    Point p2{4.0, 5.0, 6.0};
    Point result = p1 + p2;
    CHECK(result.x == 5.0);
    CHECK(result.y == 7.0);
    CHECK(result.z == 9.0);
}

TEST_CASE("Point - operator- subtracts components") {
    Point p1{10.0, 8.0, 6.0};
    Point p2{1.0, 2.0, 3.0};
    Point result = p1 - p2;
    CHECK(result.x == 9.0);
    CHECK(result.y == 6.0);
    CHECK(result.z == 3.0);
}

TEST_CASE("Point - operator* scales by scalar") {
    Point p{1.0, 2.0, 3.0};
    Point result = p * 2.0;
    CHECK(result.x == 2.0);
    CHECK(result.y == 4.0);
    CHECK(result.z == 6.0);
}

TEST_CASE("Point - operator/ divides by scalar") {
    Point p{10.0, 20.0, 30.0};
    Point result = p / 2.0;
    CHECK(result.x == 5.0);
    CHECK(result.y == 10.0);
    CHECK(result.z == 15.0);
}

TEST_CASE("Point - operator== true for same values") {
    Point p1{1.0, 2.0, 3.0};
    Point p2{1.0, 2.0, 3.0};
    CHECK(p1 == p2);
}

TEST_CASE("Point - operator== false for different values") {
    Point p1{1.0, 2.0, 3.0};
    Point p2{1.0, 2.0, 4.0};
    CHECK_FALSE(p1 == p2);
}

TEST_CASE("Point - operator!= false for same values") {
    Point p1{1.0, 2.0, 3.0};
    Point p2{1.0, 2.0, 3.0};
    CHECK_FALSE(p1 != p2);
}

TEST_CASE("Point - operator!= true for different values") {
    Point p1{1.0, 2.0, 3.0};
    Point p2{1.0, 2.0, 4.0};
    CHECK(p1 != p2);
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Point - is standard layout") { CHECK(std::is_standard_layout_v<Point>); }

TEST_CASE("Point - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Point>); }
