#include <cmath>
#include <doctest/doctest.h>

#include "datapod/datapod.hpp"
#include "datapod/matrix.hpp"

using namespace datapod;
// Not using datapod::mat to avoid Vector conflict

// ============================================================================
// Complex Number Tests
// ============================================================================

TEST_SUITE("mat::complex") {
    TEST_CASE("construction") {
        mat::complexd z1;
        CHECK(z1.real == 0.0);
        CHECK(z1.imag == 0.0);

        mat::complexd z2{3.0, 4.0};
        CHECK(z2.real == 3.0);
        CHECK(z2.imag == 4.0);

        mat::complexd z3{5.0};
        CHECK(z3.real == 5.0);
        CHECK(z3.imag == 0.0);
    }

    TEST_CASE("magnitude") {
        mat::complexd z{3.0, 4.0};
        CHECK(z.magnitude() == doctest::Approx(5.0));
        CHECK(z.magnitude_squared() == doctest::Approx(25.0));
    }

    TEST_CASE("arithmetic") {
        mat::complexd a{1.0, 2.0};
        mat::complexd b{3.0, 4.0};

        auto sum = a + b;
        CHECK(sum.real == doctest::Approx(4.0));
        CHECK(sum.imag == doctest::Approx(6.0));

        auto diff = a - b;
        CHECK(diff.real == doctest::Approx(-2.0));
        CHECK(diff.imag == doctest::Approx(-2.0));

        auto prod = a * b;
        // (1+2i)(3+4i) = 3 + 4i + 6i + 8i² = 3 + 10i - 8 = -5 + 10i
        CHECK(prod.real == doctest::Approx(-5.0));
        CHECK(prod.imag == doctest::Approx(10.0));
    }

    TEST_CASE("conjugate") {
        mat::complexd z{3.0, 4.0};
        auto conj = z.conjugate();
        CHECK(conj.real == doctest::Approx(3.0));
        CHECK(conj.imag == doctest::Approx(-4.0));
    }

    TEST_CASE("polar form") {
        auto z = mat::complexd::from_polar(5.0, 0.0);
        CHECK(z.real == doctest::Approx(5.0));
        CHECK(z.imag == doctest::Approx(0.0));

        auto z2 = mat::complexd::from_polar(1.0, M_PI / 2);
        CHECK(z2.real == doctest::Approx(0.0));
        CHECK(z2.imag == doctest::Approx(1.0));
    }

    TEST_CASE("serialization") {
        mat::complexd original{3.14159, 2.71828};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::complexd>(buffer);

        CHECK(restored.real == doctest::Approx(original.real));
        CHECK(restored.imag == doctest::Approx(original.imag));
    }

    TEST_CASE("reflection") {
        mat::complexd z{1.0, 2.0};
        auto tuple = z.members();
        CHECK(std::get<0>(tuple) == doctest::Approx(1.0));
        CHECK(std::get<1>(tuple) == doctest::Approx(2.0));
    }
}

// ============================================================================
// Dual Number Tests (Automatic Differentiation)
// ============================================================================

TEST_SUITE("mat::dual") {
    TEST_CASE("construction") {
        mat::duald x = mat::duald::variable(3.0);
        CHECK(x.real == doctest::Approx(3.0));
        CHECK(x.eps == doctest::Approx(1.0));

        mat::duald c = mat::duald::constant(5.0);
        CHECK(c.real == doctest::Approx(5.0));
        CHECK(c.eps == doctest::Approx(0.0));
    }

    TEST_CASE("autodiff power") {
        // f(x) = x², f'(x) = 2x
        mat::duald x = mat::duald::variable(3.0);
        auto y = x * x;

        CHECK(y.value() == doctest::Approx(9.0));      // f(3) = 9
        CHECK(y.derivative() == doctest::Approx(6.0)); // f'(3) = 6
    }

    TEST_CASE("autodiff trig") {
        // f(x) = sin(x), f'(x) = cos(x)
        mat::duald x = mat::duald::variable(0.0);
        auto y = sin(x);

        CHECK(y.value() == doctest::Approx(0.0));      // sin(0) = 0
        CHECK(y.derivative() == doctest::Approx(1.0)); // cos(0) = 1
    }

    TEST_CASE("autodiff composite") {
        // f(x) = x² + sin(x), f'(x) = 2x + cos(x)
        mat::duald x = mat::duald::variable(M_PI);
        auto y = x * x + sin(x);

        double expected_val = M_PI * M_PI + std::sin(M_PI);
        double expected_deriv = 2 * M_PI + std::cos(M_PI);

        CHECK(y.value() == doctest::Approx(expected_val));
        CHECK(y.derivative() == doctest::Approx(expected_deriv));
    }

    TEST_CASE("serialization") {
        mat::duald original{2.5, 1.5};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::duald>(buffer);

        CHECK(restored.real == doctest::Approx(original.real));
        CHECK(restored.eps == doctest::Approx(original.eps));
    }
}

// ============================================================================
// Fraction Tests
// ============================================================================

TEST_SUITE("mat::fraction") {
    TEST_CASE("construction") {
        mat::fraction32 f{1, 2};
        CHECK(f.num == 1);
        CHECK(f.den == 2);

        // Auto-reduce
        mat::fraction32 g{4, 8};
        CHECK(g.num == 1);
        CHECK(g.den == 2);
    }

    TEST_CASE("arithmetic") {
        mat::fraction32 a{1, 2};
        mat::fraction32 b{1, 3};

        auto sum = a + b; // 1/2 + 1/3 = 5/6
        CHECK(sum.num == 5);
        CHECK(sum.den == 6);

        auto prod = a * b; // 1/2 * 1/3 = 1/6
        CHECK(prod.num == 1);
        CHECK(prod.den == 6);
    }

    TEST_CASE("conversion") {
        mat::fraction32 f{1, 4};
        CHECK(f.to_double() == doctest::Approx(0.25));

        auto g = mat::fraction32::from_double(0.333333, 1000);
        CHECK(g.num == 1);
        CHECK(g.den == 3);
    }

    TEST_CASE("serialization") {
        mat::fraction64 original{355, 113}; // Approximation of pi

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::fraction64>(buffer);

        CHECK(restored.num == original.num);
        CHECK(restored.den == original.den);
    }
}

// ============================================================================
// Interval Tests
// ============================================================================

TEST_SUITE("mat::interval") {
    TEST_CASE("construction") {
        mat::intervald i{1.0, 2.0};
        CHECK(i.lo == doctest::Approx(1.0));
        CHECK(i.hi == doctest::Approx(2.0));

        auto point = mat::intervald::point(5.0);
        CHECK(point.lo == doctest::Approx(5.0));
        CHECK(point.hi == doctest::Approx(5.0));
    }

    TEST_CASE("arithmetic") {
        mat::intervald a{1.0, 2.0};
        mat::intervald b{3.0, 4.0};

        auto sum = a + b; // [4, 6]
        CHECK(sum.lo == doctest::Approx(4.0));
        CHECK(sum.hi == doctest::Approx(6.0));

        auto prod = a * b; // [3, 8]
        CHECK(prod.lo == doctest::Approx(3.0));
        CHECK(prod.hi == doctest::Approx(8.0));
    }

    TEST_CASE("properties") {
        mat::intervald i{1.0, 5.0};
        CHECK(i.width() == doctest::Approx(4.0));
        CHECK(i.midpoint() == doctest::Approx(3.0));
        CHECK(i.contains(3.0));
        CHECK_FALSE(i.contains(6.0));
    }

    TEST_CASE("serialization") {
        mat::intervald original{-1.5, 2.5};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::intervald>(buffer);

        CHECK(restored.lo == doctest::Approx(original.lo));
        CHECK(restored.hi == doctest::Approx(original.hi));
    }
}

// ============================================================================
// Polynomial Tests
// ============================================================================

TEST_SUITE("mat::polynomial") {
    TEST_CASE("construction") {
        mat::quadraticd p{1.0, 2.0, 3.0}; // 1 + 2x + 3x²
        CHECK(p[0] == doctest::Approx(1.0));
        CHECK(p[1] == doctest::Approx(2.0));
        CHECK(p[2] == doctest::Approx(3.0));
    }

    TEST_CASE("evaluation") {
        mat::quadraticd p{1.0, 2.0, 3.0}; // 1 + 2x + 3x²

        // p(2) = 1 + 4 + 12 = 17
        CHECK(p.eval(2.0) == doctest::Approx(17.0));
        CHECK(p(2.0) == doctest::Approx(17.0)); // Call operator
    }

    TEST_CASE("derivative") {
        mat::quadraticd p{1.0, 2.0, 3.0}; // 1 + 2x + 3x²
        auto dp = p.derivative();         // 2 + 6x

        CHECK(dp[0] == doctest::Approx(2.0));
        CHECK(dp[1] == doctest::Approx(6.0));
    }

    TEST_CASE("integral") {
        mat::lineard p{2.0, 3.0}; // 2 + 3x
        auto ip = p.integral();   // 2x + 1.5x²

        CHECK(ip[0] == doctest::Approx(0.0));
        CHECK(ip[1] == doctest::Approx(2.0));
        CHECK(ip[2] == doctest::Approx(1.5));
    }

    TEST_CASE("multiplication") {
        mat::lineard p{1.0, 1.0};  // 1 + x
        mat::lineard q{1.0, -1.0}; // 1 - x
        auto r = p * q;            // (1+x)(1-x) = 1 - x²

        CHECK(r[0] == doctest::Approx(1.0));
        CHECK(r[1] == doctest::Approx(0.0));
        CHECK(r[2] == doctest::Approx(-1.0));
    }

    TEST_CASE("serialization") {
        mat::cubicd original{1.0, 2.0, 3.0, 4.0};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::cubicd>(buffer);

        for (size_t i = 0; i < 4; ++i) {
            CHECK(restored[i] == doctest::Approx(original[i]));
        }
    }
}

// ============================================================================
// Phasor Tests
// ============================================================================

TEST_SUITE("mat::phasor") {
    TEST_CASE("construction") {
        mat::phasord v{120.0, 0.0};
        CHECK(v.mag == doctest::Approx(120.0));
        CHECK(v.phase == doctest::Approx(0.0));

        auto from_rect = mat::phasord::from_rectangular(3.0, 4.0);
        CHECK(from_rect.mag == doctest::Approx(5.0));
    }

    TEST_CASE("multiplication") {
        mat::phasord a{10.0, 0.5};
        mat::phasord b{5.0, 0.3};

        auto c = a * b;
        CHECK(c.mag == doctest::Approx(50.0));
        CHECK(c.phase == doctest::Approx(0.8));
    }

    TEST_CASE("power calculations") {
        mat::phasord voltage{120.0, 0.0};
        mat::phasord current{10.0, -0.5236}; // 30° lagging

        double power_factor = voltage.power_factor(current);
        double real_power = voltage.real_power(current);

        CHECK(power_factor == doctest::Approx(std::cos(0.5236)));
        CHECK(real_power == doctest::Approx(120.0 * 10.0 * power_factor));
    }

    TEST_CASE("serialization") {
        mat::phasord original{100.0, 1.5708};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::phasord>(buffer);

        CHECK(restored.mag == doctest::Approx(original.mag));
        CHECK(restored.phase == doctest::Approx(original.phase));
    }
}

// ============================================================================
// Modular Arithmetic Tests
// ============================================================================

TEST_SUITE("mat::modular") {
    TEST_CASE("basic arithmetic") {
        mat::mod_1e9_7 a{5};
        mat::mod_1e9_7 b{3};

        auto sum = a + b;
        CHECK(sum.val == 8);

        mat::mod_1e9_7 c{1000000006}; // -1 mod (10^9+7)
        auto sum2 = c + mat::mod_1e9_7{2};
        CHECK(sum2.val == 1);
    }

    TEST_CASE("multiplication") {
        mat::mod32<7> a{5};
        mat::mod32<7> b{4};

        auto prod = a * b; // 20 mod 7 = 6
        CHECK(prod.val == 6);
    }

    TEST_CASE("inverse") {
        mat::mod32<7> a{5};
        auto inv = a.inverse(); // 5 * inv ≡ 1 (mod 7)

        auto product = a * inv;
        CHECK(product.val == 1);
    }

    TEST_CASE("power") {
        mat::mod32<13> a{2};
        auto result = a.pow(10); // 2^10 = 1024 = 78*13 + 10

        CHECK(result.val == 1024 % 13);
    }

    TEST_CASE("serialization") {
        mat::mod_1e9_7 original{123456789};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::mod_1e9_7>(buffer);

        CHECK(restored.val == original.val);
    }
}

// ============================================================================
// Octonion Tests
// ============================================================================

TEST_SUITE("mat::octonion") {
    TEST_CASE("construction") {
        mat::octoniond o{1, 2, 3, 4, 5, 6, 7, 8};
        CHECK(o.e0 == doctest::Approx(1.0));
        CHECK(o.e7 == doctest::Approx(8.0));
    }

    TEST_CASE("conjugate") {
        mat::octoniond o{1, 2, 3, 4, 5, 6, 7, 8};
        auto conj = o.conjugate();

        CHECK(conj.e0 == doctest::Approx(1.0));
        CHECK(conj.e1 == doctest::Approx(-2.0));
        CHECK(conj.e7 == doctest::Approx(-8.0));
    }

    TEST_CASE("norm") {
        mat::octoniond o{1, 0, 0, 0, 0, 0, 0, 0};
        CHECK(o.norm() == doctest::Approx(1.0));

        mat::octoniond o2{1, 1, 1, 1, 1, 1, 1, 1};
        CHECK(o2.norm() == doctest::Approx(std::sqrt(8.0)));
    }

    TEST_CASE("multiplication") {
        // Unit octonions multiplication
        mat::octoniond e1 = mat::octoniond::unit(1); // i
        mat::octoniond e2 = mat::octoniond::unit(2); // j

        auto prod = e1 * e2; // i * j = k
        CHECK(prod.e3 == doctest::Approx(1.0));
    }

    TEST_CASE("serialization") {
        mat::octoniond original{1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::octoniond>(buffer);

        CHECK(restored.e0 == doctest::Approx(original.e0));
        CHECK(restored.e7 == doctest::Approx(original.e7));
    }
}

// ============================================================================
// BigInt Tests
// ============================================================================

TEST_SUITE("mat::bigint") {
    TEST_CASE("construction") {
        mat::uint256 x = mat::uint256::from_u64(12345);
        CHECK(x.to_u64() == 12345);
        CHECK(x.fits_u64());
    }

    TEST_CASE("addition") {
        mat::uint128 a = mat::uint128::from_u64(0xFFFFFFFFFFFFFFFF);
        mat::uint128 b = mat::uint128::from_u64(1);

        auto c = a + b; // Should overflow to second limb
        CHECK(c.limbs[0] == 0);
        CHECK(c.limbs[1] == 1);
    }

    TEST_CASE("multiplication") {
        mat::uint128 a = mat::uint128::from_u64(1000000);
        mat::uint128 b = mat::uint128::from_u64(1000000);

        auto c = a * b;
        CHECK(c.to_u64() == 1000000000000ULL);
    }

    TEST_CASE("bit operations") {
        mat::uint256 x = mat::uint256::from_u64(1);
        x <<= 100;

        CHECK(x.get_bit(100));
        CHECK_FALSE(x.get_bit(99));
        CHECK(x.bit_width() == 101);
    }

    TEST_CASE("comparison") {
        mat::uint256 a = mat::uint256::from_u64(100);
        mat::uint256 b = mat::uint256::from_u64(200);

        CHECK(a < b);
        CHECK(b > a);
        CHECK(a != b);
    }

    TEST_CASE("serialization") {
        mat::uint256 original;
        original.limbs[0] = 0x123456789ABCDEF0;
        original.limbs[1] = 0xFEDCBA9876543210;
        original.limbs[2] = 0x1111111111111111;
        original.limbs[3] = 0x2222222222222222;

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, mat::uint256>(buffer);

        CHECK(restored == original);
    }
}

// ============================================================================
// Members() verification - ensure all types are serializable
// ============================================================================

TEST_CASE("all types have members") {
    // This test verifies that all types compile with members()
    mat::complexd z{1, 2};
    mat::duald d{1, 2};
    mat::fraction32 f{1, 2};
    mat::intervald i{1, 2};
    mat::quadraticd p{1, 2, 3};
    mat::phasord ph{1, 2};
    mat::mod_1e9_7 m{123};
    mat::octoniond o{1, 2, 3, 4, 5, 6, 7, 8};
    mat::uint256 b;

    // All should have members() method
    auto z_members = z.members();
    auto d_members = d.members();
    auto f_members = f.members();
    auto i_members = i.members();
    auto p_members = p.members();
    auto ph_members = ph.members();
    auto m_members = m.members();
    auto o_members = o.members();
    auto b_members = b.members();

    // Suppress unused variable warnings
    (void)z_members;
    (void)d_members;
    (void)f_members;
    (void)i_members;
    (void)p_members;
    (void)ph_members;
    (void)m_members;
    (void)o_members;
    (void)b_members;

    CHECK(true);
}

// ============================================================================
// Type Traits Tests
// ============================================================================

TEST_CASE("type traits") {
    CHECK(mat::is_complex_v<mat::complexd>);
    CHECK_FALSE(mat::is_complex_v<double>);

    CHECK(mat::is_dual_v<mat::duald>);
    CHECK_FALSE(mat::is_dual_v<double>);

    CHECK(mat::is_fraction_v<mat::fraction32>);
    CHECK_FALSE(mat::is_fraction_v<int>);

    CHECK(mat::is_interval_v<mat::intervald>);
    CHECK_FALSE(mat::is_interval_v<double>);

    CHECK(mat::is_polynomial_v<mat::quadraticd>);
    CHECK_FALSE(mat::is_polynomial_v<double>);

    CHECK(mat::is_phasor_v<mat::phasord>);
    CHECK_FALSE(mat::is_phasor_v<double>);

    CHECK(mat::is_modular_v<mat::mod_1e9_7>);
    CHECK_FALSE(mat::is_modular_v<int>);

    CHECK(mat::is_octonion_v<mat::octoniond>);
    CHECK_FALSE(mat::is_octonion_v<double>);

    CHECK(mat::is_bigint_v<mat::uint256>);
    CHECK_FALSE(mat::is_bigint_v<int>);
}
