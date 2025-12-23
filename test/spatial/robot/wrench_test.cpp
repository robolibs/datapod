#include <doctest/doctest.h>

#include <datapod/spatial/robot/wrench.hpp>

using namespace datapod;

TEST_SUITE("Wrench") {
    TEST_CASE("Default construction") {
        Wrench w;
        CHECK(w.force.x == 0.0);
        CHECK(w.force.y == 0.0);
        CHECK(w.force.z == 0.0);
        CHECK(w.torque.x == 0.0);
        CHECK(w.torque.y == 0.0);
        CHECK(w.torque.z == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Point force{10.0, 0.0, 0.0};
        Point torque{0.0, 0.0, 5.0};
        Wrench w{force, torque};

        CHECK(w.force.x == 10.0);
        CHECK(w.torque.z == 5.0);
    }

    TEST_CASE("is_set - false for zero wrench") {
        Wrench w;
        CHECK_FALSE(w.is_set());
    }

    TEST_CASE("is_set - true with force") {
        Wrench w{Point{10.0, 0.0, 0.0}, Point{}};
        CHECK(w.is_set());
    }

    TEST_CASE("is_set - true with torque") {
        Wrench w{Point{}, Point{0.0, 0.0, 5.0}};
        CHECK(w.is_set());
    }

    TEST_CASE("force_magnitude") {
        Wrench w{Point{3.0, 4.0, 0.0}, Point{}};
        CHECK(w.force_magnitude() == doctest::Approx(5.0));
    }

    TEST_CASE("torque_magnitude") {
        Wrench w{Point{}, Point{0.0, 3.0, 4.0}};
        CHECK(w.torque_magnitude() == doctest::Approx(5.0));
    }

    TEST_CASE("operator+ addition") {
        Wrench w1{Point{10.0, 0.0, 0.0}, Point{0.0, 0.0, 5.0}};
        Wrench w2{Point{5.0, 0.0, 0.0}, Point{0.0, 0.0, 3.0}};
        auto result = w1 + w2;

        CHECK(result.force.x == 15.0);
        CHECK(result.torque.z == 8.0);
    }

    TEST_CASE("operator- subtraction") {
        Wrench w1{Point{10.0, 0.0, 0.0}, Point{0.0, 0.0, 5.0}};
        Wrench w2{Point{3.0, 0.0, 0.0}, Point{0.0, 0.0, 2.0}};
        auto result = w1 - w2;

        CHECK(result.force.x == 7.0);
        CHECK(result.torque.z == 3.0);
    }

    TEST_CASE("operator* scaling") {
        Wrench w{Point{10.0, 0.0, 0.0}, Point{0.0, 0.0, 5.0}};
        auto result = w * 2.0;

        CHECK(result.force.x == 20.0);
        CHECK(result.torque.z == 10.0);
    }

    TEST_CASE("operator/ division") {
        Wrench w{Point{10.0, 0.0, 0.0}, Point{0.0, 0.0, 10.0}};
        auto result = w / 2.0;

        CHECK(result.force.x == 5.0);
        CHECK(result.torque.z == 5.0);
    }

    TEST_CASE("operator== equality") {
        Wrench w1{Point{10.0, 0.0, 0.0}, Point{0.0, 0.0, 5.0}};
        Wrench w2{Point{10.0, 0.0, 0.0}, Point{0.0, 0.0, 5.0}};
        CHECK(w1 == w2);
    }

    TEST_CASE("operator!= inequality") {
        Wrench w1{Point{10.0, 0.0, 0.0}, Point{0.0, 0.0, 5.0}};
        Wrench w2{Point{20.0, 0.0, 0.0}, Point{0.0, 0.0, 5.0}};
        CHECK(w1 != w2);
    }

    TEST_CASE("members() reflection") {
        Wrench w;
        auto m = w.members();
        CHECK(&std::get<0>(m) == &w.force);
        CHECK(&std::get<1>(m) == &w.torque);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Wrench>);
        CHECK(std::is_trivially_copyable_v<Wrench>);
    }

    TEST_CASE("Force/torque sensor use case") {
        // Measured wrench: 100N downward, 10 N⋅m torque
        Wrench sensor_reading{Point{0.0, 0.0, -100.0}, Point{0.0, 0.0, 10.0}};
        CHECK(sensor_reading.force.z == -100.0);
        CHECK(sensor_reading.torque.z == 10.0);
        CHECK(sensor_reading.force_magnitude() == doctest::Approx(100.0));
    }
}
