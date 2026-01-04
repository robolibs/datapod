#include <doctest/doctest.h>

#include <datapod/pods/spatial/ring.hpp>

using namespace datapod;

TEST_CASE("Ring - Default construction") {
    Ring r;
    CHECK(r.points.empty());
}

TEST_CASE("Ring - members() reflection") {
    Ring r;
    auto m = r.members();
    CHECK(&std::get<0>(m) == &r.points);
}

TEST_CASE("Ring - const members() reflection") {
    const Ring r;
    auto m = r.members();
    CHECK(&std::get<0>(m) == &r.points);
}

TEST_CASE("Ring - length of empty ring") {
    Ring r;
    CHECK(r.length() == doctest::Approx(0.0));
}

TEST_CASE("Ring - length of square ring") {
    Ring r{
        {Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{0.0, 1.0, 0.0}, Point{0.0, 0.0, 0.0}}};
    CHECK(r.length() == doctest::Approx(4.0));
}

TEST_CASE("Ring - area of empty ring") {
    Ring r;
    CHECK(r.area() == doctest::Approx(0.0));
}

TEST_CASE("Ring - area of unit square") {
    Ring r{
        {Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{0.0, 1.0, 0.0}, Point{0.0, 0.0, 0.0}}};
    CHECK(r.area() == doctest::Approx(1.0));
}

TEST_CASE("Ring - area of 2x3 rectangle") {
    Ring r{
        {Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{2.0, 3.0, 0.0}, Point{0.0, 3.0, 0.0}, Point{0.0, 0.0, 0.0}}};
    CHECK(r.area() == doctest::Approx(6.0));
}

TEST_CASE("Ring - num_points") {
    Ring r{{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}, Point{2.0, 2.0, 2.0}}};
    CHECK(r.num_points() == 3);
}

TEST_CASE("Ring - empty returns true") {
    Ring r;
    CHECK(r.empty());
}

TEST_CASE("Ring - empty returns false") {
    Ring r{{Point{1.0, 2.0, 3.0}}};
    CHECK_FALSE(r.empty());
}

TEST_CASE("Ring - is_closed returns false for empty") {
    Ring r;
    CHECK_FALSE(r.is_closed());
}

TEST_CASE("Ring - is_closed returns false for too few points") {
    Ring r{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}}};
    CHECK_FALSE(r.is_closed());
}

TEST_CASE("Ring - is_closed returns false when not closed") {
    Ring r{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{1.0, 1.0, 0.0}}};
    CHECK_FALSE(r.is_closed());
}

TEST_CASE("Ring - is_closed returns true when properly closed") {
    Ring r{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{0.0, 0.0, 0.0}}};
    CHECK(r.is_closed());
}

TEST_CASE("Ring - is standard layout") { CHECK(std::is_standard_layout_v<Ring>); }

// Note: Ring is NOT trivially copyable because Vector has a non-trivial destructor
// This is correct - Vector needs to free memory
