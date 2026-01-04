#include <doctest/doctest.h>

#include <datapod/pods/spatial/primitives/square.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Square - Default construction") {
    Square s;
    CHECK(s.center.x == 0.0);
    CHECK(s.center.y == 0.0);
    CHECK(s.center.z == 0.0);
    CHECK(s.side == 0.0);
}

TEST_CASE("Square - Aggregate initialization") {
    Square s{{5.0, 5.0, 0.0}, 10.0};
    CHECK(s.center.x == 5.0);
    CHECK(s.center.y == 5.0);
    CHECK(s.center.z == 0.0);
    CHECK(s.side == 10.0);
}

TEST_CASE("Square - members() reflection") {
    Square s{{5.0, 5.0, 0.0}, 10.0};
    auto m = s.members();
    CHECK(&std::get<0>(m) == &s.center);
    CHECK(&std::get<1>(m) == &s.side);
}

TEST_CASE("Square - const members() reflection") {
    const Square s{{5.0, 5.0, 0.0}, 10.0};
    auto m = s.members();
    CHECK(&std::get<0>(m) == &s.center);
    CHECK(&std::get<1>(m) == &s.side);
}

// ============================================================================
// TEST: Area Calculation
// ============================================================================

TEST_CASE("Square - area of zero side") {
    Square s{{0.0, 0.0, 0.0}, 0.0};
    CHECK(s.area() == doctest::Approx(0.0));
}

TEST_CASE("Square - area of unit square") {
    Square s{{0.0, 0.0, 0.0}, 1.0};
    CHECK(s.area() == doctest::Approx(1.0));
}

TEST_CASE("Square - area of side 5") {
    Square s{{0.0, 0.0, 0.0}, 5.0};
    CHECK(s.area() == doctest::Approx(25.0));
}

TEST_CASE("Square - area of side 10") {
    Square s{{5.0, 5.0, 0.0}, 10.0};
    CHECK(s.area() == doctest::Approx(100.0));
}

TEST_CASE("Square - area with fractional side") {
    Square s{{0.0, 0.0, 0.0}, 2.5};
    CHECK(s.area() == doctest::Approx(6.25));
}

// ============================================================================
// TEST: Perimeter Calculation
// ============================================================================

TEST_CASE("Square - perimeter of zero side") {
    Square s{{0.0, 0.0, 0.0}, 0.0};
    CHECK(s.perimeter() == doctest::Approx(0.0));
}

TEST_CASE("Square - perimeter of unit square") {
    Square s{{0.0, 0.0, 0.0}, 1.0};
    CHECK(s.perimeter() == doctest::Approx(4.0));
}

TEST_CASE("Square - perimeter of side 5") {
    Square s{{0.0, 0.0, 0.0}, 5.0};
    CHECK(s.perimeter() == doctest::Approx(20.0));
}

TEST_CASE("Square - perimeter of side 10") {
    Square s{{5.0, 5.0, 0.0}, 10.0};
    CHECK(s.perimeter() == doctest::Approx(40.0));
}

// ============================================================================
// TEST: Diagonal Calculation
// ============================================================================

TEST_CASE("Square - diagonal of zero side") {
    Square s{{0.0, 0.0, 0.0}, 0.0};
    CHECK(s.diagonal() == doctest::Approx(0.0));
}

TEST_CASE("Square - diagonal of unit square") {
    Square s{{0.0, 0.0, 0.0}, 1.0};
    CHECK(s.diagonal() == doctest::Approx(std::sqrt(2.0)));
}

TEST_CASE("Square - diagonal of side 5") {
    Square s{{0.0, 0.0, 0.0}, 5.0};
    CHECK(s.diagonal() == doctest::Approx(5.0 * std::sqrt(2.0)));
}

TEST_CASE("Square - diagonal of side 10") {
    Square s{{5.0, 5.0, 0.0}, 10.0};
    CHECK(s.diagonal() == doctest::Approx(10.0 * std::sqrt(2.0)));
}

// ============================================================================
// TEST: Point Containment
// ============================================================================

TEST_CASE("Square - contains center point") {
    Square s{{5.0, 5.0, 0.0}, 10.0};
    CHECK(s.contains(s.center));
}

TEST_CASE("Square - contains point inside") {
    Square s{{0.0, 0.0, 0.0}, 10.0};
    Point p{2.0, 3.0, 0.0};
    CHECK(s.contains(p));
}

TEST_CASE("Square - contains point on edge") {
    Square s{{0.0, 0.0, 0.0}, 10.0};
    Point p{5.0, 0.0, 0.0}; // On right edge
    CHECK(s.contains(p));
}

TEST_CASE("Square - contains corner point") {
    Square s{{0.0, 0.0, 0.0}, 10.0};
    Point corner{5.0, 5.0, 0.0}; // Top-right corner
    CHECK(s.contains(corner));
}

TEST_CASE("Square - does not contain point outside") {
    Square s{{0.0, 0.0, 0.0}, 10.0};
    Point outside{10.0, 10.0, 0.0};
    CHECK_FALSE(s.contains(outside));
}

TEST_CASE("Square - does not contain point far outside") {
    Square s{{0.0, 0.0, 0.0}, 1.0};
    Point outside{100.0, 100.0, 0.0};
    CHECK_FALSE(s.contains(outside));
}

TEST_CASE("Square - contains with negative center") {
    Square s{{-5.0, -5.0, 0.0}, 4.0};
    Point inside{-4.0, -4.0, 0.0};
    CHECK(s.contains(inside));
}

TEST_CASE("Square - containment boundary check left edge") {
    Square s{{0.0, 0.0, 0.0}, 10.0};
    Point onEdge{-5.0, 0.0, 0.0}; // On left edge
    CHECK(s.contains(onEdge));
}

TEST_CASE("Square - containment boundary check top edge") {
    Square s{{0.0, 0.0, 0.0}, 10.0};
    Point onEdge{0.0, 5.0, 0.0}; // On top edge
    CHECK(s.contains(onEdge));
}

// ============================================================================
// TEST: get_corners
// ============================================================================

TEST_CASE("Square - get_corners returns all four corners") {
    Square s{{0.0, 0.0, 0.0}, 10.0};
    auto corners = s.get_corners();
    CHECK(corners.size() == 4);
    // Check bottom-left corner
    CHECK(corners[0].x == doctest::Approx(-5.0));
    CHECK(corners[0].y == doctest::Approx(-5.0));
    // Check bottom-right corner
    CHECK(corners[1].x == doctest::Approx(5.0));
    CHECK(corners[1].y == doctest::Approx(-5.0));
    // Check top-right corner
    CHECK(corners[2].x == doctest::Approx(5.0));
    CHECK(corners[2].y == doctest::Approx(5.0));
    // Check top-left corner
    CHECK(corners[3].x == doctest::Approx(-5.0));
    CHECK(corners[3].y == doctest::Approx(5.0));
}

TEST_CASE("Square - get_corners with offset center") {
    Square s{{10.0, 20.0, 0.0}, 6.0};
    auto corners = s.get_corners();
    CHECK(corners[0].x == doctest::Approx(7.0));  // bottom-left x
    CHECK(corners[0].y == doctest::Approx(17.0)); // bottom-left y
    CHECK(corners[2].x == doctest::Approx(13.0)); // top-right x
    CHECK(corners[2].y == doctest::Approx(23.0)); // top-right y
}

TEST_CASE("Square - get_corners preserves Z coordinate") {
    Square s{{5.0, 5.0, 10.0}, 4.0};
    auto corners = s.get_corners();
    CHECK(corners[0].z == 10.0);
    CHECK(corners[1].z == 10.0);
    CHECK(corners[2].z == 10.0);
    CHECK(corners[3].z == 10.0);
}

TEST_CASE("Square - get_corners maintains order") {
    Square s{{0.0, 0.0, 0.0}, 8.0};
    auto corners = s.get_corners();
    // Order: bottom-left, bottom-right, top-right, top-left
    CHECK(corners[0].y < corners[2].y); // bottom < top
    CHECK(corners[0].y < corners[3].y); // bottom < top
    CHECK(corners[0].x < corners[1].x); // left < right
    CHECK(corners[3].x < corners[2].x); // left < right
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Square - is standard layout") { CHECK(std::is_standard_layout_v<Square>); }

TEST_CASE("Square - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Square>); }
