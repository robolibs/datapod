#include <doctest/doctest.h>

#include <datapod/pods/spatial/aabb.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("AABB - Default construction") {
    AABB aabb;
    CHECK(aabb.min_point.x == 0.0);
    CHECK(aabb.min_point.y == 0.0);
    CHECK(aabb.min_point.z == 0.0);
    CHECK(aabb.max_point.x == 0.0);
    CHECK(aabb.max_point.y == 0.0);
    CHECK(aabb.max_point.z == 0.0);
}

TEST_CASE("AABB - Aggregate initialization") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    CHECK(aabb.min_point.x == 0.0);
    CHECK(aabb.min_point.y == 0.0);
    CHECK(aabb.min_point.z == 0.0);
    CHECK(aabb.max_point.x == 10.0);
    CHECK(aabb.max_point.y == 10.0);
    CHECK(aabb.max_point.z == 10.0);
}

TEST_CASE("AABB - members() reflection") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    auto m = aabb.members();
    CHECK(&std::get<0>(m) == &aabb.min_point);
    CHECK(&std::get<1>(m) == &aabb.max_point);
}

TEST_CASE("AABB - const members() reflection") {
    const AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    auto m = aabb.members();
    CHECK(&std::get<0>(m) == &aabb.min_point);
    CHECK(&std::get<1>(m) == &aabb.max_point);
}

// ============================================================================
// TEST: Center Calculation
// ============================================================================

TEST_CASE("AABB - center of degenerate AABB") {
    AABB aabb{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    Point c = aabb.center();
    CHECK(c.x == doctest::Approx(0.0));
    CHECK(c.y == doctest::Approx(0.0));
    CHECK(c.z == doctest::Approx(0.0));
}

TEST_CASE("AABB - center of unit cube at origin") {
    AABB aabb{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
    Point c = aabb.center();
    CHECK(c.x == doctest::Approx(0.5));
    CHECK(c.y == doctest::Approx(0.5));
    CHECK(c.z == doctest::Approx(0.5));
}

TEST_CASE("AABB - center of centered cube") {
    AABB aabb{{-5.0, -5.0, -5.0}, {5.0, 5.0, 5.0}};
    Point c = aabb.center();
    CHECK(c.x == doctest::Approx(0.0));
    CHECK(c.y == doctest::Approx(0.0));
    CHECK(c.z == doctest::Approx(0.0));
}

TEST_CASE("AABB - center of offset box") {
    AABB aabb{{10.0, 20.0, 30.0}, {20.0, 30.0, 40.0}};
    Point c = aabb.center();
    CHECK(c.x == doctest::Approx(15.0));
    CHECK(c.y == doctest::Approx(25.0));
    CHECK(c.z == doctest::Approx(35.0));
}

// ============================================================================
// TEST: Volume Calculation
// ============================================================================

TEST_CASE("AABB - volume of degenerate AABB") {
    AABB aabb{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(aabb.volume() == doctest::Approx(0.0));
}

TEST_CASE("AABB - volume of unit cube") {
    AABB aabb{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
    CHECK(aabb.volume() == doctest::Approx(1.0));
}

TEST_CASE("AABB - volume of 2x3x4 box") {
    AABB aabb{{0.0, 0.0, 0.0}, {2.0, 3.0, 4.0}};
    CHECK(aabb.volume() == doctest::Approx(24.0));
}

TEST_CASE("AABB - volume of 10x10x10 cube") {
    AABB aabb{{-5.0, -5.0, -5.0}, {5.0, 5.0, 5.0}};
    CHECK(aabb.volume() == doctest::Approx(1000.0));
}

// ============================================================================
// TEST: Surface Area Calculation
// ============================================================================

TEST_CASE("AABB - surface area of degenerate AABB") {
    AABB aabb{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(aabb.surface_area() == doctest::Approx(0.0));
}

TEST_CASE("AABB - surface area of unit cube") {
    AABB aabb{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
    CHECK(aabb.surface_area() == doctest::Approx(6.0)); // 6 faces of 1x1
}

TEST_CASE("AABB - surface area of 2x2x2 cube") {
    AABB aabb{{0.0, 0.0, 0.0}, {2.0, 2.0, 2.0}};
    CHECK(aabb.surface_area() == doctest::Approx(24.0)); // 6 faces of 2x2
}

TEST_CASE("AABB - surface area of 2x3x4 box") {
    AABB aabb{{0.0, 0.0, 0.0}, {2.0, 3.0, 4.0}};
    // 2*(2*3 + 3*4 + 4*2) = 2*(6 + 12 + 8) = 52
    CHECK(aabb.surface_area() == doctest::Approx(52.0));
}

// ============================================================================
// TEST: Point Containment
// ============================================================================

TEST_CASE("AABB - contains min_point") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    CHECK(aabb.contains(aabb.min_point));
}

TEST_CASE("AABB - contains max_point") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    CHECK(aabb.contains(aabb.max_point));
}

TEST_CASE("AABB - contains center point") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point center{5.0, 5.0, 5.0};
    CHECK(aabb.contains(center));
}

TEST_CASE("AABB - contains point inside") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point inside{3.0, 4.0, 5.0};
    CHECK(aabb.contains(inside));
}

TEST_CASE("AABB - does not contain point outside X") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point outside{11.0, 5.0, 5.0};
    CHECK_FALSE(aabb.contains(outside));
}

TEST_CASE("AABB - does not contain point outside Y") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point outside{5.0, 11.0, 5.0};
    CHECK_FALSE(aabb.contains(outside));
}

TEST_CASE("AABB - does not contain point outside Z") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point outside{5.0, 5.0, 11.0};
    CHECK_FALSE(aabb.contains(outside));
}

TEST_CASE("AABB - does not contain point below minimum") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point below{-1.0, 5.0, 5.0};
    CHECK_FALSE(aabb.contains(below));
}

// ============================================================================
// TEST: AABB Intersection
// ============================================================================

TEST_CASE("AABB - intersects with itself") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    CHECK(aabb.intersects(aabb));
}

TEST_CASE("AABB - intersects with overlapping AABB") {
    AABB aabb1{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB aabb2{{5.0, 5.0, 5.0}, {15.0, 15.0, 15.0}};
    CHECK(aabb1.intersects(aabb2));
    CHECK(aabb2.intersects(aabb1));
}

TEST_CASE("AABB - intersects with contained AABB") {
    AABB outer{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB inner{{2.0, 2.0, 2.0}, {8.0, 8.0, 8.0}};
    CHECK(outer.intersects(inner));
    CHECK(inner.intersects(outer));
}

TEST_CASE("AABB - does not intersect separated X") {
    AABB aabb1{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB aabb2{{11.0, 0.0, 0.0}, {20.0, 10.0, 10.0}};
    CHECK_FALSE(aabb1.intersects(aabb2));
    CHECK_FALSE(aabb2.intersects(aabb1));
}

TEST_CASE("AABB - does not intersect separated Y") {
    AABB aabb1{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB aabb2{{0.0, 11.0, 0.0}, {10.0, 20.0, 10.0}};
    CHECK_FALSE(aabb1.intersects(aabb2));
}

TEST_CASE("AABB - does not intersect separated Z") {
    AABB aabb1{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB aabb2{{0.0, 0.0, 11.0}, {10.0, 10.0, 20.0}};
    CHECK_FALSE(aabb1.intersects(aabb2));
}

TEST_CASE("AABB - intersects edge touching") {
    AABB aabb1{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB aabb2{{10.0, 0.0, 0.0}, {20.0, 10.0, 10.0}};
    CHECK(aabb1.intersects(aabb2));
}

// ============================================================================
// TEST: Expand with Point
// ============================================================================

TEST_CASE("AABB - expand with point inside does not change bounds") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point inside{5.0, 5.0, 5.0};
    aabb.expand(inside);
    CHECK(aabb.min_point.x == doctest::Approx(0.0));
    CHECK(aabb.min_point.y == doctest::Approx(0.0));
    CHECK(aabb.min_point.z == doctest::Approx(0.0));
    CHECK(aabb.max_point.x == doctest::Approx(10.0));
    CHECK(aabb.max_point.y == doctest::Approx(10.0));
    CHECK(aabb.max_point.z == doctest::Approx(10.0));
}

TEST_CASE("AABB - expand with point outside increases max") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point outside{15.0, 15.0, 15.0};
    aabb.expand(outside);
    CHECK(aabb.max_point.x == doctest::Approx(15.0));
    CHECK(aabb.max_point.y == doctest::Approx(15.0));
    CHECK(aabb.max_point.z == doctest::Approx(15.0));
}

TEST_CASE("AABB - expand with point below decreases min") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    Point below{-5.0, -5.0, -5.0};
    aabb.expand(below);
    CHECK(aabb.min_point.x == doctest::Approx(-5.0));
    CHECK(aabb.min_point.y == doctest::Approx(-5.0));
    CHECK(aabb.min_point.z == doctest::Approx(-5.0));
}

// ============================================================================
// TEST: Expand with AABB
// ============================================================================

TEST_CASE("AABB - expand with contained AABB does not change bounds") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB inner{{2.0, 2.0, 2.0}, {8.0, 8.0, 8.0}};
    aabb.expand(inner);
    CHECK(aabb.min_point.x == doctest::Approx(0.0));
    CHECK(aabb.max_point.x == doctest::Approx(10.0));
}

TEST_CASE("AABB - expand with overlapping AABB increases bounds") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB other{{5.0, 5.0, 5.0}, {15.0, 15.0, 15.0}};
    aabb.expand(other);
    CHECK(aabb.min_point.x == doctest::Approx(0.0));
    CHECK(aabb.min_point.y == doctest::Approx(0.0));
    CHECK(aabb.min_point.z == doctest::Approx(0.0));
    CHECK(aabb.max_point.x == doctest::Approx(15.0));
    CHECK(aabb.max_point.y == doctest::Approx(15.0));
    CHECK(aabb.max_point.z == doctest::Approx(15.0));
}

TEST_CASE("AABB - expand with non-overlapping AABB") {
    AABB aabb{{0.0, 0.0, 0.0}, {10.0, 10.0, 10.0}};
    AABB other{{-5.0, -5.0, -5.0}, {5.0, 5.0, 5.0}};
    aabb.expand(other);
    CHECK(aabb.min_point.x == doctest::Approx(-5.0));
    CHECK(aabb.min_point.y == doctest::Approx(-5.0));
    CHECK(aabb.min_point.z == doctest::Approx(-5.0));
    CHECK(aabb.max_point.x == doctest::Approx(10.0));
    CHECK(aabb.max_point.y == doctest::Approx(10.0));
    CHECK(aabb.max_point.z == doctest::Approx(10.0));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("AABB - is standard layout") { CHECK(std::is_standard_layout_v<AABB>); }

TEST_CASE("AABB - is trivially copyable") { CHECK(std::is_trivially_copyable_v<AABB>); }
