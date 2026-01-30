#include <doctest/doctest.h>

#include <datapod/pods/spatial/robot/inertial.hpp>

using namespace datapod;
using namespace datapod::robot;

TEST_SUITE("Inertial") {
    TEST_CASE("Default construction") {
        Inertial inert;
        CHECK(inert.mass == 0.0);
        CHECK(inert.origin.point.x == 0.0);
        CHECK(inert.ixx == 0.0);
        CHECK(inert.iyy == 0.0);
        CHECK(inert.izz == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Pose origin = pose::make(Point{0.1, 0.0, 0.05});
        Inertial inert{origin, 10.0, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};

        CHECK(inert.mass == 10.0);
        CHECK(inert.origin.point.x == 0.1);
        CHECK(inert.ixx == 0.5);
        CHECK(inert.iyy == 0.6);
        CHECK(inert.izz == 0.7);
    }

    TEST_CASE("is_set - false for zero inertia") {
        Inertial inert;
        CHECK_FALSE(inert.is_set());
    }

    TEST_CASE("is_set - true with mass") {
        Inertial inert{pose::identity(), 5.0};
        CHECK(inert.is_set());
    }

    TEST_CASE("is_set - true with inertia tensor") {
        Inertial inert{pose::identity(), 0.0, 0.1, 0.0, 0.0, 0.1, 0.0, 0.1};
        CHECK(inert.is_set());
    }

    TEST_CASE("trace calculation") {
        Inertial inert{pose::identity(), 0.0, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(inert.trace() == doctest::Approx(1.8));
    }

    TEST_CASE("is_diagonal - true for diagonal tensor") {
        Inertial inert{pose::identity(), 10.0, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(inert.is_diagonal());
    }

    TEST_CASE("is_diagonal - false for non-diagonal tensor") {
        Inertial inert{pose::identity(), 10.0, 0.5, 0.1, 0.0, 0.6, 0.0, 0.7};
        CHECK_FALSE(inert.is_diagonal());
    }

    TEST_CASE("operator== equality") {
        Pose origin = pose::make(Point{0.1, 0.0, 0.0});
        Inertial i1{origin, 10.0, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        Inertial i2{origin, 10.0, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(i1 == i2);
    }

    TEST_CASE("operator!= inequality") {
        Pose origin = pose::make(Point{0.1, 0.0, 0.0});
        Inertial i1{origin, 10.0, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        Inertial i2{origin, 11.0, 0.5, 0.0, 0.0, 0.6, 0.0, 0.7};
        CHECK(i1 != i2);
    }

    TEST_CASE("members() reflection") {
        Inertial inert;
        auto m = inert.members();
        CHECK(&std::get<0>(m) == &inert.origin);
        CHECK(&std::get<1>(m) == &inert.mass);
        CHECK(&std::get<2>(m) == &inert.ixx);
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

        Inertial cylinder{pose::identity(), m, ixx, 0.0, 0.0, ixx, 0.0, izz};

        CHECK(cylinder.mass == 5.0);
        CHECK(cylinder.ixx == doctest::Approx(ixx));
        CHECK(cylinder.izz == doctest::Approx(izz));
        CHECK(cylinder.is_diagonal());
    }

    TEST_CASE("Point mass inertia use case") {
        // Point mass at (1, 0, 0)
        double mass = 2.0;
        Point pos{1.0, 0.0, 0.0};

        // I = m * r²
        double ixx = 0.0;                  // Along x-axis
        double iyy = mass * (pos.x * pos.x); // About y-axis
        double izz = mass * (pos.x * pos.x); // About z-axis

        Inertial point_mass{pose::make(pos), mass, ixx, 0.0, 0.0, iyy, 0.0, izz};

        CHECK(point_mass.mass == 2.0);
        CHECK(point_mass.origin.point.x == 1.0);
        CHECK(point_mass.iyy == doctest::Approx(2.0));
        CHECK(point_mass.izz == doctest::Approx(2.0));
    }

    TEST_CASE("Factory functions") {
        SUBCASE("sphere") {
            auto s = inertial::sphere(5.0, 0.1);
            CHECK(s.mass == 5.0);
            CHECK(s.is_diagonal());
        }

        SUBCASE("box") {
            auto b = inertial::box(5.0, 0.2, 0.3, 0.4);
            CHECK(b.mass == 5.0);
            CHECK(b.is_diagonal());
        }

        SUBCASE("cylinder") {
            auto c = inertial::cylinder(5.0, 0.1, 0.5);
            CHECK(c.mass == 5.0);
            CHECK(c.is_diagonal());
        }

        SUBCASE("point_mass") {
            auto p = inertial::point_mass(2.0, Point{1.0, 0.0, 0.0});
            CHECK(p.mass == 2.0);
            CHECK(p.origin.point.x == 1.0);
        }
    }
}
