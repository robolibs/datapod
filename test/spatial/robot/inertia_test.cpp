#include <doctest/doctest.h>

#include <datapod/spatial/robot/inertia.hpp>

using namespace datapod;

TEST_SUITE("Inertia") {
    TEST_CASE("Default construction") {
        Inertia inert;
        CHECK(inert.m == 0.0);
        CHECK(inert.com.x == 0.0);
        CHECK(inert.ixx == 0.0);
        CHECK(inert.iyy == 0.0);
        CHECK(inert.izz == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Point com{0.1, 0.0, 0.05};
        Inertia inert{10.0, com, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};

        CHECK(inert.m == 10.0);
        CHECK(inert.com.x == 0.1);
        CHECK(inert.ixx == 0.5);
        CHECK(inert.iyy == 0.6);
        CHECK(inert.izz == 0.7);
    }

    TEST_CASE("is_set - false for zero inertia") {
        Inertia inert;
        CHECK_FALSE(inert.is_set());
    }

    TEST_CASE("is_set - true with mass") {
        Inertia inert{5.0, Point{}};
        CHECK(inert.is_set());
    }

    TEST_CASE("is_set - true with inertia tensor") {
        Inertia inert{0.0, Point{}, 0.1, 0.0, 0.0, 0.1, 0.0, 0.1};
        CHECK(inert.is_set());
    }

    TEST_CASE("trace calculation") {
        Inertia inert{0.0, Point{}, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(inert.trace() == doctest::Approx(1.8));
    }

    TEST_CASE("is_diagonal - true for diagonal tensor") {
        Inertia inert{10.0, Point{}, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(inert.is_diagonal());
    }

    TEST_CASE("is_diagonal - false for non-diagonal tensor") {
        Inertia inert{10.0, Point{}, 0.5, 0.1, 0.0, 0.6, 0.0, 0.7};
        CHECK_FALSE(inert.is_diagonal());
    }

    TEST_CASE("operator== equality") {
        Inertia i1{10.0, Point{0.1, 0.0, 0.0}, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        Inertia i2{10.0, Point{0.1, 0.0, 0.0}, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(i1 == i2);
    }

    TEST_CASE("operator!= inequality") {
        Inertia i1{10.0, Point{0.1, 0.0, 0.0}, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        Inertia i2{11.0, Point{0.1, 0.0, 0.0}, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(i1 != i2);
    }

    TEST_CASE("members() reflection") {
        Inertia inert;
        auto m = inert.members();
        CHECK(&std::get<0>(m) == &inert.m);
        CHECK(&std::get<1>(m) == &inert.com);
        CHECK(&std::get<2>(m) == &inert.ixx);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Inertia>);
        CHECK(std::is_trivially_copyable_v<Inertia>);
    }

    TEST_CASE("Cylinder inertia use case") {
        // Solid cylinder: mass=5kg, radius=0.1m, height=0.5m
        // Ixx = Iyy = (1/12)*m*h² + (1/4)*m*r²
        // Izz = (1/2)*m*r²
        double m = 5.0;
        double r = 0.1;
        double h = 0.5;
        double ixx = (1.0 / 12.0) * m * h * h + (1.0 / 4.0) * m * r * r;
        double izz = (1.0 / 2.0) * m * r * r;

        Inertia cylinder{m, Point{}, ixx, 0.0, 0.0, ixx, 0.0, izz};

        CHECK(cylinder.m == 5.0);
        CHECK(cylinder.ixx == doctest::Approx(ixx));
        CHECK(cylinder.izz == doctest::Approx(izz));
        CHECK(cylinder.is_diagonal());
    }

    TEST_CASE("Point mass inertia use case") {
        // Point mass at (1, 0, 0)
        double mass = 2.0;
        Point pos{1.0, 0.0, 0.0};

        // I = m * r²
        double ixx = 0.0;                    // Along x-axis
        double iyy = mass * (pos.x * pos.x); // About y-axis
        double izz = mass * (pos.x * pos.x); // About z-axis

        Inertia point_mass{mass, pos, ixx, 0.0, 0.0, iyy, 0.0, izz};

        CHECK(point_mass.m == 2.0);
        CHECK(point_mass.com.x == 1.0);
        CHECK(point_mass.iyy == doctest::Approx(2.0));
        CHECK(point_mass.izz == doctest::Approx(2.0));
    }
}
