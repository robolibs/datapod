#include <doctest/doctest.h>

#include <datapod/spatial/quaternion.hpp> // This includes euler.hpp and implements conversions

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Euler - Default construction") {
    Euler e;
    CHECK(e.roll == 0.0);
    CHECK(e.pitch == 0.0);
    CHECK(e.yaw == 0.0);
}

TEST_CASE("Euler - Aggregate initialization") {
    Euler e{0.1, 0.2, 0.3};
    CHECK(e.roll == 0.1);
    CHECK(e.pitch == 0.2);
    CHECK(e.yaw == 0.3);
}

TEST_CASE("Euler - members() reflection") {
    Euler e{0.1, 0.2, 0.3};
    auto m = e.members();
    CHECK(&std::get<0>(m) == &e.roll);
    CHECK(&std::get<1>(m) == &e.pitch);
    CHECK(&std::get<2>(m) == &e.yaw);
}

TEST_CASE("Euler - const members() reflection") {
    const Euler e{0.1, 0.2, 0.3};
    auto m = e.members();
    CHECK(&std::get<0>(m) == &e.roll);
    CHECK(&std::get<1>(m) == &e.pitch);
    CHECK(&std::get<2>(m) == &e.yaw);
}

// ============================================================================
// TEST: Utility
// ============================================================================

TEST_CASE("Euler - is_set returns false at origin") {
    Euler e{0.0, 0.0, 0.0};
    CHECK_FALSE(e.is_set());
}

TEST_CASE("Euler - is_set returns true with roll") {
    Euler e{0.1, 0.0, 0.0};
    CHECK(e.is_set());
}

TEST_CASE("Euler - is_set returns true with pitch") {
    Euler e{0.0, 0.1, 0.0};
    CHECK(e.is_set());
}

TEST_CASE("Euler - is_set returns true with yaw") {
    Euler e{0.0, 0.0, 0.1};
    CHECK(e.is_set());
}

TEST_CASE("Euler - yaw_cos") {
    Euler e{0.0, 0.0, 1.5707963267948966}; // 90 degrees
    CHECK(e.yaw_cos() == doctest::Approx(0.0).epsilon(1e-10));
}

TEST_CASE("Euler - yaw_sin") {
    Euler e{0.0, 0.0, 1.5707963267948966}; // 90 degrees
    CHECK(e.yaw_sin() == doctest::Approx(1.0));
}

TEST_CASE("Euler - yaw_cos and yaw_sin at 45 degrees") {
    Euler e{0.0, 0.0, 0.7853981633974483}; // 45 degrees
    CHECK(e.yaw_cos() == doctest::Approx(0.7071067811865476));
    CHECK(e.yaw_sin() == doctest::Approx(0.7071067811865476));
}

// ============================================================================
// TEST: Normalization
// ============================================================================

TEST_CASE("Euler - normalized keeps angles in range") {
    Euler e{0.1, 0.2, 0.3};
    Euler n = e.normalized();
    CHECK(n.roll == doctest::Approx(0.1));
    CHECK(n.pitch == doctest::Approx(0.2));
    CHECK(n.yaw == doctest::Approx(0.3));
}

TEST_CASE("Euler - normalized wraps positive overflow") {
    constexpr double PI = 3.14159265358979323846;
    Euler e{PI + 0.5, 0.0, 0.0};
    Euler n = e.normalized();
    CHECK(n.roll == doctest::Approx(-PI + 0.5));
}

TEST_CASE("Euler - normalized wraps negative overflow") {
    constexpr double PI = 3.14159265358979323846;
    Euler e{-PI - 0.5, 0.0, 0.0};
    Euler n = e.normalized();
    CHECK(n.roll == doctest::Approx(PI - 0.5));
}

TEST_CASE("Euler - normalized handles multiple wraps") {
    constexpr double PI = 3.14159265358979323846;
    Euler e{10.0 * PI, 0.0, 0.0};
    Euler n = e.normalized();
    CHECK(std::abs(n.roll) < PI);
}

// ============================================================================
// TEST: Operators
// ============================================================================

TEST_CASE("Euler - operator+ adds components") {
    Euler e1{0.1, 0.2, 0.3};
    Euler e2{0.4, 0.5, 0.6};
    Euler result = e1 + e2;
    CHECK(result.roll == doctest::Approx(0.5));
    CHECK(result.pitch == doctest::Approx(0.7));
    CHECK(result.yaw == doctest::Approx(0.9));
}

TEST_CASE("Euler - operator- subtracts components") {
    Euler e1{0.5, 0.7, 0.9};
    Euler e2{0.1, 0.2, 0.3};
    Euler result = e1 - e2;
    CHECK(result.roll == doctest::Approx(0.4));
    CHECK(result.pitch == doctest::Approx(0.5));
    CHECK(result.yaw == doctest::Approx(0.6));
}

TEST_CASE("Euler - operator* scales by scalar") {
    Euler e{0.1, 0.2, 0.3};
    Euler result = e * 2.0;
    CHECK(result.roll == doctest::Approx(0.2));
    CHECK(result.pitch == doctest::Approx(0.4));
    CHECK(result.yaw == doctest::Approx(0.6));
}

TEST_CASE("Euler - operator== true for same values") {
    Euler e1{0.1, 0.2, 0.3};
    Euler e2{0.1, 0.2, 0.3};
    CHECK(e1 == e2);
}

TEST_CASE("Euler - operator== false for different values") {
    Euler e1{0.1, 0.2, 0.3};
    Euler e2{0.1, 0.2, 0.4};
    CHECK_FALSE(e1 == e2);
}

TEST_CASE("Euler - operator!= false for same values") {
    Euler e1{0.1, 0.2, 0.3};
    Euler e2{0.1, 0.2, 0.3};
    CHECK_FALSE(e1 != e2);
}

TEST_CASE("Euler - operator!= true for different values") {
    Euler e1{0.1, 0.2, 0.3};
    Euler e2{0.1, 0.2, 0.4};
    CHECK(e1 != e2);
}

// ============================================================================
// TEST: Conversions
// ============================================================================

TEST_CASE("Euler - to_quaternion identity") {
    Euler e{0.0, 0.0, 0.0};
    Quaternion q = e.to_quaternion();
    CHECK(q.w == doctest::Approx(1.0));
    CHECK(q.x == doctest::Approx(0.0));
    CHECK(q.y == doctest::Approx(0.0));
    CHECK(q.z == doctest::Approx(0.0));
}

TEST_CASE("Euler - to_quaternion 90 degree yaw") {
    constexpr double PI = 3.14159265358979323846;
    Euler e{0.0, 0.0, PI / 2.0}; // 90 degrees yaw
    Quaternion q = e.to_quaternion();
    CHECK(q.w == doctest::Approx(0.7071067811865476));
    CHECK(q.x == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(q.y == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(q.z == doctest::Approx(0.7071067811865476));
}

TEST_CASE("Euler - to_quaternion and back") {
    Euler e{0.1, 0.2, 0.3};
    Quaternion q = e.to_quaternion();
    Euler e2 = q.to_euler();
    CHECK(e2.roll == doctest::Approx(e.roll));
    CHECK(e2.pitch == doctest::Approx(e.pitch));
    CHECK(e2.yaw == doctest::Approx(e.yaw));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Euler - is standard layout") { CHECK(std::is_standard_layout_v<Euler>); }

TEST_CASE("Euler - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Euler>); }
