#include <doctest/doctest.h>

#include <datapod/pods/spatial/quaternion.hpp> // This includes euler.hpp and implements conversions

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Quaternion - Default construction") {
    Quaternion q;
    CHECK(q.w == 1.0);
    CHECK(q.x == 0.0);
    CHECK(q.y == 0.0);
    CHECK(q.z == 0.0);
}

TEST_CASE("Quaternion - Aggregate initialization") {
    Quaternion q{0.5, 0.5, 0.5, 0.5};
    CHECK(q.w == 0.5);
    CHECK(q.x == 0.5);
    CHECK(q.y == 0.5);
    CHECK(q.z == 0.5);
}

TEST_CASE("Quaternion - members() reflection") {
    Quaternion q{0.5, 0.5, 0.5, 0.5};
    auto m = q.members();
    CHECK(&std::get<0>(m) == &q.w);
    CHECK(&std::get<1>(m) == &q.x);
    CHECK(&std::get<2>(m) == &q.y);
    CHECK(&std::get<3>(m) == &q.z);
}

TEST_CASE("Quaternion - const members() reflection") {
    const Quaternion q{0.5, 0.5, 0.5, 0.5};
    auto m = q.members();
    CHECK(&std::get<0>(m) == &q.w);
    CHECK(&std::get<1>(m) == &q.x);
    CHECK(&std::get<2>(m) == &q.y);
    CHECK(&std::get<3>(m) == &q.z);
}

// ============================================================================
// TEST: Utility
// ============================================================================

TEST_CASE("Quaternion - is_set returns false for identity") {
    Quaternion q{1.0, 0.0, 0.0, 0.0};
    CHECK_FALSE(q.is_set());
}

TEST_CASE("Quaternion - is_set returns true with rotation") {
    Quaternion q{0.9, 0.1, 0.0, 0.0};
    CHECK(q.is_set());
}

TEST_CASE("Quaternion - magnitude of identity") {
    Quaternion q{1.0, 0.0, 0.0, 0.0};
    CHECK(q.magnitude() == doctest::Approx(1.0));
}

TEST_CASE("Quaternion - magnitude of unit quaternion") {
    Quaternion q{0.5, 0.5, 0.5, 0.5};
    CHECK(q.magnitude() == doctest::Approx(1.0));
}

TEST_CASE("Quaternion - magnitude of scaled quaternion") {
    Quaternion q{2.0, 0.0, 0.0, 0.0};
    CHECK(q.magnitude() == doctest::Approx(2.0));
}

TEST_CASE("Quaternion - normalized identity") {
    Quaternion q{1.0, 0.0, 0.0, 0.0};
    Quaternion n = q.normalized();
    CHECK(n.w == doctest::Approx(1.0));
    CHECK(n.x == doctest::Approx(0.0));
    CHECK(n.y == doctest::Approx(0.0));
    CHECK(n.z == doctest::Approx(0.0));
}

TEST_CASE("Quaternion - normalized scales to unit length") {
    Quaternion q{2.0, 0.0, 0.0, 0.0};
    Quaternion n = q.normalized();
    CHECK(n.magnitude() == doctest::Approx(1.0));
    CHECK(n.w == doctest::Approx(1.0));
}

TEST_CASE("Quaternion - normalized handles zero quaternion") {
    Quaternion q{0.0, 0.0, 0.0, 0.0};
    Quaternion n = q.normalized();
    CHECK(n.w == doctest::Approx(1.0)); // Returns identity
    CHECK(n.x == doctest::Approx(0.0));
    CHECK(n.y == doctest::Approx(0.0));
    CHECK(n.z == doctest::Approx(0.0));
}

TEST_CASE("Quaternion - conjugate of identity") {
    Quaternion q{1.0, 0.0, 0.0, 0.0};
    Quaternion c = q.conjugate();
    CHECK(c.w == doctest::Approx(1.0));
    CHECK(c.x == doctest::Approx(0.0));
    CHECK(c.y == doctest::Approx(0.0));
    CHECK(c.z == doctest::Approx(0.0));
}

TEST_CASE("Quaternion - conjugate flips imaginary parts") {
    Quaternion q{0.5, 0.5, 0.5, 0.5};
    Quaternion c = q.conjugate();
    CHECK(c.w == doctest::Approx(0.5));
    CHECK(c.x == doctest::Approx(-0.5));
    CHECK(c.y == doctest::Approx(-0.5));
    CHECK(c.z == doctest::Approx(-0.5));
}

// ============================================================================
// TEST: Operators
// ============================================================================

TEST_CASE("Quaternion - operator* identity quaternion") {
    Quaternion q1{1.0, 0.0, 0.0, 0.0};
    Quaternion q2{0.5, 0.5, 0.5, 0.5};
    Quaternion result = q1 * q2;
    CHECK(result.w == doctest::Approx(0.5));
    CHECK(result.x == doctest::Approx(0.5));
    CHECK(result.y == doctest::Approx(0.5));
    CHECK(result.z == doctest::Approx(0.5));
}

TEST_CASE("Quaternion - operator* commutative for identity") {
    Quaternion q1{1.0, 0.0, 0.0, 0.0};
    Quaternion q2{0.5, 0.5, 0.5, 0.5};
    Quaternion r1 = q1 * q2;
    Quaternion r2 = q2 * q1;
    CHECK(r1.w == doctest::Approx(r2.w));
    CHECK(r1.x == doctest::Approx(r2.x));
    CHECK(r1.y == doctest::Approx(r2.y));
    CHECK(r1.z == doctest::Approx(r2.z));
}

TEST_CASE("Quaternion - operator* with conjugate gives magnitude squared") {
    Quaternion q{0.5, 0.5, 0.5, 0.5};
    Quaternion c = q.conjugate();
    Quaternion result = q * c;
    // q * q_conj = (|q|^2, 0, 0, 0) for unit quaternion = (1, 0, 0, 0)
    CHECK(result.w == doctest::Approx(1.0));
    CHECK(result.x == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(result.y == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(result.z == doctest::Approx(0.0).epsilon(1e-10));
}

TEST_CASE("Quaternion - operator== true for same values") {
    Quaternion q1{0.5, 0.5, 0.5, 0.5};
    Quaternion q2{0.5, 0.5, 0.5, 0.5};
    CHECK(q1 == q2);
}

TEST_CASE("Quaternion - operator== false for different values") {
    Quaternion q1{0.5, 0.5, 0.5, 0.5};
    Quaternion q2{0.5, 0.5, 0.5, 0.6};
    CHECK_FALSE(q1 == q2);
}

TEST_CASE("Quaternion - operator!= false for same values") {
    Quaternion q1{0.5, 0.5, 0.5, 0.5};
    Quaternion q2{0.5, 0.5, 0.5, 0.5};
    CHECK_FALSE(q1 != q2);
}

TEST_CASE("Quaternion - operator!= true for different values") {
    Quaternion q1{0.5, 0.5, 0.5, 0.5};
    Quaternion q2{0.5, 0.5, 0.5, 0.6};
    CHECK(q1 != q2);
}

// ============================================================================
// TEST: Conversions
// ============================================================================

TEST_CASE("Quaternion - to_euler identity") {
    Quaternion q{1.0, 0.0, 0.0, 0.0};
    Euler e = q.to_euler();
    CHECK(e.roll == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(e.pitch == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(e.yaw == doctest::Approx(0.0).epsilon(1e-10));
}

TEST_CASE("Quaternion - to_euler 90 degree yaw") {
    // 90 degree yaw quaternion: cos(45°) + sin(45°)*k
    Quaternion q{0.7071067811865476, 0.0, 0.0, 0.7071067811865476};
    Euler e = q.to_euler();
    CHECK(e.roll == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(e.pitch == doctest::Approx(0.0).epsilon(1e-10));
    CHECK(e.yaw == doctest::Approx(1.5707963267948966)); // PI/2
}

TEST_CASE("Quaternion - to_euler and back") {
    Quaternion q{0.9238795325112867, 0.2209424458507589, 0.1766635829950186, 0.2588190451025208};
    Euler e = q.to_euler();
    Quaternion q2 = e.to_quaternion();
    // Conversion may have small numerical errors
    CHECK(q2.w == doctest::Approx(q.w).epsilon(0.001));
    CHECK(q2.x == doctest::Approx(q.x).epsilon(0.001));
    CHECK(q2.y == doctest::Approx(q.y).epsilon(0.001));
    CHECK(q2.z == doctest::Approx(q.z).epsilon(0.001));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Quaternion - is standard layout") { CHECK(std::is_standard_layout_v<Quaternion>); }

TEST_CASE("Quaternion - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Quaternion>); }

// ============================================================================
// TEST: Namespace Utilities
// ============================================================================

TEST_CASE("quaternion::make - creates Quaternion from double components") {
    auto q = datapod::quaternion::make(1.0, 0.0, 0.0, 0.0);
    CHECK(q.w == 1.0);
    CHECK(q.x == 0.0);
    CHECK(q.y == 0.0);
    CHECK(q.z == 0.0);
}

TEST_CASE("quaternion::make - creates Quaternion with all components") {
    auto q = datapod::quaternion::make(0.5, 0.5, 0.5, 0.5);
    CHECK(q.w == 0.5);
    CHECK(q.x == 0.5);
    CHECK(q.y == 0.5);
    CHECK(q.z == 0.5);
}

TEST_CASE("quaternion::make - creates Quaternionf from float components") {
    auto q = datapod::quaternion::make(1.0f, 0.0f, 0.0f, 0.0f);
    CHECK(q.w == 1.0f);
    CHECK(q.x == 0.0f);
    CHECK(q.y == 0.0f);
    CHECK(q.z == 0.0f);
}

TEST_CASE("quaternion::make - float overload returns Quaternionf type") {
    auto q = datapod::quaternion::make(0.5f, 0.5f, 0.5f, 0.5f);
    static_assert(std::is_same_v<decltype(q), Quaternionf>);
    CHECK(q.w == 0.5f);
    CHECK(q.x == 0.5f);
    CHECK(q.y == 0.5f);
    CHECK(q.z == 0.5f);
}
