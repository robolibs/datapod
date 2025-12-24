#include <doctest/doctest.h>

#include <datapod/spatial/linestring.hpp>

using namespace datapod;

TEST_CASE("Linestring - Default construction") {
    Linestring ls;
    CHECK(ls.points.empty());
}

TEST_CASE("Linestring - Aggregate initialization") {
    Linestring ls{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{3.0, 4.0, 0.0}}};
    CHECK(ls.points.size() == 3);
}

TEST_CASE("Linestring - members() reflection") {
    Linestring ls{{Point{1.0, 2.0, 3.0}}};
    auto m = ls.members();
    CHECK(&std::get<0>(m) == &ls.points);
}

TEST_CASE("Linestring - const members() reflection") {
    const Linestring ls{{Point{1.0, 2.0, 3.0}}};
    auto m = ls.members();
    CHECK(&std::get<0>(m) == &ls.points);
}

TEST_CASE("Linestring - length of empty linestring") {
    Linestring ls;
    CHECK(ls.length() == doctest::Approx(0.0));
}

TEST_CASE("Linestring - length of single point") {
    Linestring ls{{Point{1.0, 2.0, 3.0}}};
    CHECK(ls.length() == doctest::Approx(0.0));
}

TEST_CASE("Linestring - length of two points") {
    Linestring ls{{Point{0.0, 0.0, 0.0}, Point{3.0, 4.0, 0.0}}};
    CHECK(ls.length() == doctest::Approx(5.0)); // 3-4-5 triangle
}

TEST_CASE("Linestring - length of multiple segments") {
    Linestring ls{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{3.0, 4.0, 0.0}}};
    CHECK(ls.length() == doctest::Approx(7.0)); // 3 + 4
}

TEST_CASE("Linestring - num_points") {
    Linestring ls{{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}, Point{2.0, 2.0, 2.0}}};
    CHECK(ls.num_points() == 3);
}

TEST_CASE("Linestring - empty returns true for no points") {
    Linestring ls;
    CHECK(ls.empty());
}

TEST_CASE("Linestring - empty returns false with points") {
    Linestring ls{{Point{1.0, 2.0, 3.0}}};
    CHECK_FALSE(ls.empty());
}

TEST_CASE("Linestring - is standard layout") { CHECK(std::is_standard_layout_v<Linestring>); }

// Note: Linestring is NOT trivially copyable because Vector has a non-trivial destructor
// This is correct - Vector needs to free memory
