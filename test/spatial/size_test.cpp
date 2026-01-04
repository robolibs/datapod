#include <doctest/doctest.h>

#include <datapod/pods/spatial/size.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Size - Default construction") {
    Size s;
    CHECK(s.x == 0.0);
    CHECK(s.y == 0.0);
    CHECK(s.z == 0.0);
}

TEST_CASE("Size - Aggregate initialization") {
    Size s{10.0, 20.0, 30.0};
    CHECK(s.x == 10.0);
    CHECK(s.y == 20.0);
    CHECK(s.z == 30.0);
}

TEST_CASE("Size - members() reflection") {
    Size s{10.0, 20.0, 30.0};
    auto m = s.members();
    CHECK(&std::get<0>(m) == &s.x);
    CHECK(&std::get<1>(m) == &s.y);
    CHECK(&std::get<2>(m) == &s.z);
}

TEST_CASE("Size - const members() reflection") {
    const Size s{10.0, 20.0, 30.0};
    auto m = s.members();
    CHECK(&std::get<0>(m) == &s.x);
    CHECK(&std::get<1>(m) == &s.y);
    CHECK(&std::get<2>(m) == &s.z);
}

// ============================================================================
// TEST: Volume and Area
// ============================================================================

TEST_CASE("Size - volume of cube") {
    Size s{10.0, 10.0, 10.0};
    CHECK(s.volume() == doctest::Approx(1000.0));
}

TEST_CASE("Size - volume of box") {
    Size s{2.0, 3.0, 4.0};
    CHECK(s.volume() == doctest::Approx(24.0));
}

TEST_CASE("Size - volume of zero size") {
    Size s{0.0, 0.0, 0.0};
    CHECK(s.volume() == doctest::Approx(0.0));
}

TEST_CASE("Size - area_xy") {
    Size s{5.0, 4.0, 10.0};
    CHECK(s.area_xy() == doctest::Approx(20.0));
}

TEST_CASE("Size - area_xz") {
    Size s{5.0, 10.0, 3.0};
    CHECK(s.area_xz() == doctest::Approx(15.0));
}

TEST_CASE("Size - area_yz") {
    Size s{10.0, 6.0, 4.0};
    CHECK(s.area_yz() == doctest::Approx(24.0));
}

TEST_CASE("Size - diagonal 3D") {
    Size s{3.0, 4.0, 0.0};
    CHECK(s.diagonal() == doctest::Approx(5.0)); // 3-4-5 triangle
}

TEST_CASE("Size - diagonal 3D cube") {
    Size s{1.0, 1.0, 1.0};
    CHECK(s.diagonal() == doctest::Approx(std::sqrt(3.0)));
}

TEST_CASE("Size - diagonal_2d") {
    Size s{3.0, 4.0, 100.0};
    CHECK(s.diagonal_2d() == doctest::Approx(5.0)); // Ignores Z
}

TEST_CASE("Size - diagonal_2d square") {
    Size s{10.0, 10.0, 0.0};
    CHECK(s.diagonal_2d() == doctest::Approx(10.0 * std::sqrt(2.0)));
}

// ============================================================================
// TEST: Utility
// ============================================================================

TEST_CASE("Size - is_set returns false at origin") {
    Size s{0.0, 0.0, 0.0};
    CHECK_FALSE(s.is_set());
}

TEST_CASE("Size - is_set returns true with X") {
    Size s{1.0, 0.0, 0.0};
    CHECK(s.is_set());
}

TEST_CASE("Size - is_set returns true with Y") {
    Size s{0.0, 1.0, 0.0};
    CHECK(s.is_set());
}

TEST_CASE("Size - is_set returns true with Z") {
    Size s{0.0, 0.0, 1.0};
    CHECK(s.is_set());
}

TEST_CASE("Size - is_set returns true with all coords") {
    Size s{10.0, 20.0, 30.0};
    CHECK(s.is_set());
}

// ============================================================================
// TEST: Operators
// ============================================================================

TEST_CASE("Size - operator+ adds components") {
    Size s1{10.0, 20.0, 30.0};
    Size s2{1.0, 2.0, 3.0};
    Size result = s1 + s2;
    CHECK(result.x == 11.0);
    CHECK(result.y == 22.0);
    CHECK(result.z == 33.0);
}

TEST_CASE("Size - operator- subtracts components") {
    Size s1{10.0, 20.0, 30.0};
    Size s2{1.0, 2.0, 3.0};
    Size result = s1 - s2;
    CHECK(result.x == 9.0);
    CHECK(result.y == 18.0);
    CHECK(result.z == 27.0);
}

TEST_CASE("Size - operator* scales by scalar") {
    Size s{10.0, 20.0, 30.0};
    Size result = s * 2.0;
    CHECK(result.x == 20.0);
    CHECK(result.y == 40.0);
    CHECK(result.z == 60.0);
}

TEST_CASE("Size - operator/ divides by scalar") {
    Size s{10.0, 20.0, 30.0};
    Size result = s / 2.0;
    CHECK(result.x == 5.0);
    CHECK(result.y == 10.0);
    CHECK(result.z == 15.0);
}

TEST_CASE("Size - operator* component-wise multiply") {
    Size s1{2.0, 3.0, 4.0};
    Size s2{5.0, 6.0, 7.0};
    Size result = s1 * s2;
    CHECK(result.x == 10.0);
    CHECK(result.y == 18.0);
    CHECK(result.z == 28.0);
}

TEST_CASE("Size - operator== true for same values") {
    Size s1{10.0, 20.0, 30.0};
    Size s2{10.0, 20.0, 30.0};
    CHECK(s1 == s2);
}

TEST_CASE("Size - operator== false for different values") {
    Size s1{10.0, 20.0, 30.0};
    Size s2{10.0, 20.0, 31.0};
    CHECK_FALSE(s1 == s2);
}

TEST_CASE("Size - operator!= false for same values") {
    Size s1{10.0, 20.0, 30.0};
    Size s2{10.0, 20.0, 30.0};
    CHECK_FALSE(s1 != s2);
}

TEST_CASE("Size - operator!= true for different values") {
    Size s1{10.0, 20.0, 30.0};
    Size s2{10.0, 20.0, 31.0};
    CHECK(s1 != s2);
}

// ============================================================================
// TEST: Min/Max Helpers
// ============================================================================

TEST_CASE("Size - abs with positive values") {
    Size s{10.0, 20.0, 30.0};
    Size result = s.abs();
    CHECK(result.x == 10.0);
    CHECK(result.y == 20.0);
    CHECK(result.z == 30.0);
}

TEST_CASE("Size - abs with negative values") {
    Size s{-10.0, -20.0, -30.0};
    Size result = s.abs();
    CHECK(result.x == 10.0);
    CHECK(result.y == 20.0);
    CHECK(result.z == 30.0);
}

TEST_CASE("Size - abs with mixed values") {
    Size s{-10.0, 20.0, -30.0};
    Size result = s.abs();
    CHECK(result.x == 10.0);
    CHECK(result.y == 20.0);
    CHECK(result.z == 30.0);
}

TEST_CASE("Size - max selects larger components") {
    Size s1{10.0, 5.0, 30.0};
    Size s2{8.0, 15.0, 20.0};
    Size result = s1.max(s2);
    CHECK(result.x == 10.0);
    CHECK(result.y == 15.0);
    CHECK(result.z == 30.0);
}

TEST_CASE("Size - min selects smaller components") {
    Size s1{10.0, 5.0, 30.0};
    Size s2{8.0, 15.0, 20.0};
    Size result = s1.min(s2);
    CHECK(result.x == 8.0);
    CHECK(result.y == 5.0);
    CHECK(result.z == 20.0);
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Size - is standard layout") { CHECK(std::is_standard_layout_v<Size>); }

TEST_CASE("Size - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Size>); }

// ============================================================================
// TEST: Namespace Utilities
// ============================================================================

TEST_CASE("size::make - 2D size") {
    auto s = size::make(10.0, 20.0);
    CHECK(s.x == 10.0);
    CHECK(s.y == 20.0);
    CHECK(s.z == 0.0);
}

TEST_CASE("size::make - 3D size") {
    auto s = size::make(10.0, 20.0, 30.0);
    CHECK(s.x == 10.0);
    CHECK(s.y == 20.0);
    CHECK(s.z == 30.0);
}

TEST_CASE("size::uniform - creates uniform size") {
    auto s = size::uniform(5.0);
    CHECK(s.x == 5.0);
    CHECK(s.y == 5.0);
    CHECK(s.z == 5.0);
}

TEST_CASE("size::zero - creates zero size") {
    auto s = size::zero();
    CHECK(s.x == 0.0);
    CHECK(s.y == 0.0);
    CHECK(s.z == 0.0);
    CHECK_FALSE(s.is_set());
}
