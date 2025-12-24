#include <doctest/doctest.h>

#include <datapod/spatial/primitives/circle.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Circle - Default construction") {
    Circle c;
    CHECK(c.center.x == 0.0);
    CHECK(c.center.y == 0.0);
    CHECK(c.center.z == 0.0);
    CHECK(c.radius == 0.0);
}

TEST_CASE("Circle - Aggregate initialization") {
    Circle c{{1.0, 2.0, 3.0}, 5.0};
    CHECK(c.center.x == 1.0);
    CHECK(c.center.y == 2.0);
    CHECK(c.center.z == 3.0);
    CHECK(c.radius == 5.0);
}

TEST_CASE("Circle - members() reflection") {
    Circle c{{1.0, 2.0, 3.0}, 5.0};
    auto m = c.members();
    CHECK(&std::get<0>(m) == &c.center);
    CHECK(&std::get<1>(m) == &c.radius);
}

TEST_CASE("Circle - const members() reflection") {
    const Circle c{{1.0, 2.0, 3.0}, 5.0};
    auto m = c.members();
    CHECK(&std::get<0>(m) == &c.center);
    CHECK(&std::get<1>(m) == &c.radius);
}

// ============================================================================
// TEST: Geometric Properties
// ============================================================================

TEST_CASE("Circle - area of zero radius") {
    Circle c{{0.0, 0.0, 0.0}, 0.0};
    CHECK(c.area() == doctest::Approx(0.0));
}

TEST_CASE("Circle - area of unit circle") {
    Circle c{{0.0, 0.0, 0.0}, 1.0};
    CHECK(c.area() == doctest::Approx(M_PI));
}

TEST_CASE("Circle - area of radius 2") {
    Circle c{{0.0, 0.0, 0.0}, 2.0};
    CHECK(c.area() == doctest::Approx(4.0 * M_PI));
}

TEST_CASE("Circle - area of radius 5") {
    Circle c{{5.0, 5.0, 0.0}, 5.0};
    CHECK(c.area() == doctest::Approx(25.0 * M_PI));
}

TEST_CASE("Circle - perimeter of zero radius") {
    Circle c{{0.0, 0.0, 0.0}, 0.0};
    CHECK(c.perimeter() == doctest::Approx(0.0));
}

TEST_CASE("Circle - perimeter of unit circle") {
    Circle c{{0.0, 0.0, 0.0}, 1.0};
    CHECK(c.perimeter() == doctest::Approx(2.0 * M_PI));
}

TEST_CASE("Circle - perimeter of radius 3") {
    Circle c{{0.0, 0.0, 0.0}, 3.0};
    CHECK(c.perimeter() == doctest::Approx(6.0 * M_PI));
}

// ============================================================================
// TEST: Containment
// ============================================================================

TEST_CASE("Circle - contains center point") {
    Circle c{{5.0, 5.0, 0.0}, 10.0};
    Point p{5.0, 5.0, 0.0};
    CHECK(c.contains(p));
}

TEST_CASE("Circle - contains point inside") {
    Circle c{{0.0, 0.0, 0.0}, 10.0};
    Point p{3.0, 4.0, 0.0}; // 5 units from center
    CHECK(c.contains(p));
}

TEST_CASE("Circle - contains point on boundary") {
    Circle c{{0.0, 0.0, 0.0}, 5.0};
    Point p{3.0, 4.0, 0.0}; // exactly 5 units from center
    CHECK(c.contains(p));
}

TEST_CASE("Circle - does not contain point outside") {
    Circle c{{0.0, 0.0, 0.0}, 5.0};
    Point p{10.0, 0.0, 0.0};
    CHECK_FALSE(c.contains(p));
}

TEST_CASE("Circle - does not contain point far outside") {
    Circle c{{0.0, 0.0, 0.0}, 1.0};
    Point p{100.0, 100.0, 100.0};
    CHECK_FALSE(c.contains(p));
}

TEST_CASE("Circle - contains in 3D space") {
    Circle c{{5.0, 5.0, 5.0}, 10.0};
    Point p{5.0, 5.0, 10.0}; // 5 units away in Z
    CHECK(c.contains(p));
}

TEST_CASE("Circle - does not contain in 3D space") {
    Circle c{{0.0, 0.0, 0.0}, 5.0};
    Point p{3.0, 3.0, 3.0}; // sqrt(27) > 5
    CHECK_FALSE(c.contains(p));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Circle - is standard layout") { CHECK(std::is_standard_layout_v<Circle>); }

TEST_CASE("Circle - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Circle>); }
