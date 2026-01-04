#include <doctest/doctest.h>

#include "datapod/adapters/variant.hpp"

using namespace datapod;

TEST_SUITE("Variant Advanced Operations") {

    // ========================================================================
    // holds_alternative Tests
    // ========================================================================

    TEST_CASE("holds_alternative - correct type") {
        Variant<int, float, double> v(42);
        
        CHECK(holds_alternative<int>(v));
        CHECK_FALSE(holds_alternative<float>(v));
        CHECK_FALSE(holds_alternative<double>(v));
    }

    TEST_CASE("holds_alternative - after assignment") {
        Variant<int, float, double> v(42);
        CHECK(holds_alternative<int>(v));
        
        v = 3.14f;
        CHECK_FALSE(holds_alternative<int>(v));
        CHECK(holds_alternative<float>(v));
        CHECK_FALSE(holds_alternative<double>(v));
    }

    TEST_CASE("holds_alternative - empty variant") {
        Variant<int, float> v;
        
        CHECK_FALSE(holds_alternative<int>(v));
        CHECK_FALSE(holds_alternative<float>(v));
    }

    // ========================================================================
    // visit_at Tests
    // ========================================================================

    TEST_CASE("visit_at - correct index") {
        Variant<int, float, double> v(42);
        
        int result = visit_at<0>([](int x) { return x * 2; }, v);
        CHECK(result == 84);
    }

    TEST_CASE("visit_at - const variant") {
        const Variant<int, float, double> v(3.14f);
        
        float result = visit_at<1>([](float x) { return x * 2.0f; }, v);
        CHECK(result == doctest::Approx(6.28f));
    }

    TEST_CASE("visit_at - mutation") {
        Variant<int, float, double> v(42);
        
        visit_at<0>([](int &x) { x = 100; }, v);
        CHECK(get<int>(v) == 100);
    }

    TEST_CASE("visit_at - wrong index throws") {
        Variant<int, float, double> v(42);  // holds int at index 0
        
        CHECK_THROWS_AS(visit_at<1>([](float) { return 0.0f; }, v), std::runtime_error);
    }

    TEST_CASE("visit_at - different return types") {
        Variant<int, float, double> v(42);
        
        auto result = visit_at<0>([](int x) -> double { return x * 1.5; }, v);
        CHECK(result == doctest::Approx(63.0));
    }

    // ========================================================================
    // Comparison Operators Tests
    // ========================================================================

    TEST_CASE("Comparison - equal variants") {
        Variant<int, float> v1(42);
        Variant<int, float> v2(42);
        
        CHECK(v1 == v2);
        CHECK_FALSE(v1 != v2);
        CHECK_FALSE(v1 < v2);
        CHECK_FALSE(v1 > v2);
        CHECK(v1 <= v2);
        CHECK(v1 >= v2);
    }

    TEST_CASE("Comparison - different values same type") {
        Variant<int, float> v1(42);
        Variant<int, float> v2(100);
        
        CHECK_FALSE(v1 == v2);
        CHECK(v1 != v2);
        CHECK(v1 < v2);
        CHECK_FALSE(v1 > v2);
        CHECK(v1 <= v2);
        CHECK_FALSE(v1 >= v2);
    }

    TEST_CASE("Comparison - different types") {
        Variant<int, float> v1(42);      // index 0
        Variant<int, float> v2(3.14f);   // index 1
        
        CHECK_FALSE(v1 == v2);
        CHECK(v1 != v2);
        CHECK(v1 < v2);  // index 0 < index 1
        CHECK_FALSE(v1 > v2);
    }

    TEST_CASE("Comparison - with empty variant") {
        Variant<int, float> v1(42);
        Variant<int, float> v2;  // empty
        
        CHECK_FALSE(v1 == v2);
        CHECK(v1 != v2);
    }

    // ========================================================================
    // Combined Tests
    // ========================================================================

    TEST_CASE("holds_alternative with visit_at") {
        Variant<int, float, double> v(3.14f);
        
        if (holds_alternative<float>(v)) {
            float result = visit_at<1>([](float x) { return x * 2.0f; }, v);
            CHECK(result == doctest::Approx(6.28f));
        } else {
            FAIL("Should hold float");
        }
    }

    TEST_CASE("Type-safe access pattern") {
        Variant<int, float, double> v(42);
        
        if (holds_alternative<int>(v)) {
            int value = get<int>(v);
            CHECK(value == 42);
        } else if (holds_alternative<float>(v)) {
            FAIL("Should not hold float");
        } else if (holds_alternative<double>(v)) {
            FAIL("Should not hold double");
        }
    }

    TEST_CASE("visit_at with different types") {
        Variant<int, float, double> v1(42);
        Variant<int, float, double> v2(3.14f);
        Variant<int, float, double> v3(2.718);
        
        auto process = [](auto x) { return static_cast<double>(x) * 2.0; };
        
        double r1 = visit_at<0>(process, v1);
        double r2 = visit_at<1>(process, v2);
        double r3 = visit_at<2>(process, v3);
        
        CHECK(r1 == doctest::Approx(84.0));
        CHECK(r2 == doctest::Approx(6.28));
        CHECK(r3 == doctest::Approx(5.436));
    }

}
