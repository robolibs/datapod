#include <doctest/doctest.h>

#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/pods/spatial/primitives/segment.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction
// ============================================================================

TEST_CASE("Segment - Default Construction") {
    Segment s;
    CHECK(s.start.x == 0.0);
    CHECK(s.start.y == 0.0);
    CHECK(s.start.z == 0.0);
    CHECK(s.end.x == 0.0);
    CHECK(s.end.y == 0.0);
    CHECK(s.end.z == 0.0);
}

TEST_CASE("Segment - Construction with points") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    CHECK(s.start.x == 1.0);
    CHECK(s.start.y == 2.0);
    CHECK(s.start.z == 3.0);
    CHECK(s.end.x == 4.0);
    CHECK(s.end.y == 5.0);
    CHECK(s.end.z == 6.0);
}

// ============================================================================
// TEST: Reflection
// ============================================================================

TEST_CASE("Segment - members() reflection") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    auto m = s.members();

    CHECK(&std::get<0>(m) == &s.start);
    CHECK(&std::get<1>(m) == &s.end);
}

TEST_CASE("Segment - const members() reflection") {
    const Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    auto m = s.members();

    CHECK(&std::get<0>(m) == &s.start);
    CHECK(&std::get<1>(m) == &s.end);
}

TEST_CASE("Segment - to_tuple conversion") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    auto t = to_tuple(s);

    auto &[start, end] = t;
    CHECK(start.x == 1.0);
    CHECK(end.x == 4.0);
}

TEST_CASE("Segment - for_each_field iteration") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};

    int field_count = 0;
    for_each_field(s, [&field_count](auto &field) { field_count++; });

    CHECK(field_count == 2); // start and end
}

// ============================================================================
// TEST: Geometric Properties
// ============================================================================

TEST_CASE("Segment - length of zero segment") {
    Segment s{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(s.length() == doctest::Approx(0.0));
}

TEST_CASE("Segment - length on X axis") {
    Segment s{{0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}};
    CHECK(s.length() == doctest::Approx(5.0));
}

TEST_CASE("Segment - length 3-4-5 triangle") {
    Segment s{{0.0, 0.0, 0.0}, {3.0, 4.0, 0.0}};
    CHECK(s.length() == doctest::Approx(5.0));
}

TEST_CASE("Segment - length 3D") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 6.0, 3.0}};
    CHECK(s.length() == doctest::Approx(5.0));
}

TEST_CASE("Segment - midpoint on X axis") {
    Segment s{{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point mid = s.midpoint();
    CHECK(mid.x == doctest::Approx(5.0));
    CHECK(mid.y == doctest::Approx(0.0));
    CHECK(mid.z == doctest::Approx(0.0));
}

TEST_CASE("Segment - midpoint 3D") {
    Segment s{{1.0, 2.0, 3.0}, {5.0, 6.0, 7.0}};
    Point mid = s.midpoint();
    CHECK(mid.x == doctest::Approx(3.0));
    CHECK(mid.y == doctest::Approx(4.0));
    CHECK(mid.z == doctest::Approx(5.0));
}

// ============================================================================
// TEST: Distance Queries
// ============================================================================

TEST_CASE("Segment - closest_point on segment") {
    Segment s{{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point p{5.0, 5.0, 0.0};
    Point closest = s.closest_point(p);
    CHECK(closest.x == doctest::Approx(5.0));
    CHECK(closest.y == doctest::Approx(0.0));
    CHECK(closest.z == doctest::Approx(0.0));
}

TEST_CASE("Segment - closest_point before start") {
    Segment s{{5.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point p{0.0, 0.0, 0.0};
    Point closest = s.closest_point(p);
    CHECK(closest.x == doctest::Approx(5.0));
    CHECK(closest.y == doctest::Approx(0.0));
    CHECK(closest.z == doctest::Approx(0.0));
}

TEST_CASE("Segment - closest_point after end") {
    Segment s{{0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}};
    Point p{10.0, 0.0, 0.0};
    Point closest = s.closest_point(p);
    CHECK(closest.x == doctest::Approx(5.0));
    CHECK(closest.y == doctest::Approx(0.0));
    CHECK(closest.z == doctest::Approx(0.0));
}

TEST_CASE("Segment - closest_point at start") {
    Segment s{{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point p{0.0, 5.0, 0.0};
    Point closest = s.closest_point(p);
    CHECK(closest.x == doctest::Approx(0.0));
    CHECK(closest.y == doctest::Approx(0.0));
    CHECK(closest.z == doctest::Approx(0.0));
}

TEST_CASE("Segment - closest_point at end") {
    Segment s{{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point p{10.0, 5.0, 0.0};
    Point closest = s.closest_point(p);
    CHECK(closest.x == doctest::Approx(10.0));
    CHECK(closest.y == doctest::Approx(0.0));
    CHECK(closest.z == doctest::Approx(0.0));
}

TEST_CASE("Segment - closest_point degenerate segment") {
    Segment s{{5.0, 5.0, 5.0}, {5.0, 5.0, 5.0}};
    Point p{10.0, 10.0, 10.0};
    Point closest = s.closest_point(p);
    CHECK(closest.x == doctest::Approx(5.0));
    CHECK(closest.y == doctest::Approx(5.0));
    CHECK(closest.z == doctest::Approx(5.0));
}

TEST_CASE("Segment - distance_to point on segment") {
    Segment s{{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point p{5.0, 3.0, 0.0};
    CHECK(s.distance_to(p) == doctest::Approx(3.0));
}

TEST_CASE("Segment - distance_to point at perpendicular") {
    Segment s{{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point p{5.0, 4.0, 3.0};
    CHECK(s.distance_to(p) == doctest::Approx(5.0)); // 3-4-5 triangle
}

TEST_CASE("Segment - distance_to point before start") {
    Segment s{{5.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    Point p{0.0, 0.0, 0.0};
    CHECK(s.distance_to(p) == doctest::Approx(5.0));
}

TEST_CASE("Segment - distance_to point after end") {
    Segment s{{0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}};
    Point p{10.0, 0.0, 0.0};
    CHECK(s.distance_to(p) == doctest::Approx(5.0));
}

// ============================================================================
// TEST: POD properties
// ============================================================================

TEST_CASE("Segment - is standard layout") { CHECK(std::is_standard_layout_v<Segment>); }

TEST_CASE("Segment - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Segment>); }
