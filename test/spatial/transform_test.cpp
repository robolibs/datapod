#include <cmath>
#include <doctest/doctest.h>

#include "datapod/datapod.hpp"
#include "datapod/pods/spatial/transform.hpp"

using namespace datapod;

TEST_SUITE("Transform") {
    TEST_CASE("construction") {
        auto tf = Transform::identity();
        CHECK(tf.rw == doctest::Approx(1.0));
        CHECK(tf.rx == doctest::Approx(0.0));
        CHECK(tf.ry == doctest::Approx(0.0));
        CHECK(tf.rz == doctest::Approx(0.0));
        CHECK(tf.dw == doctest::Approx(0.0));
        CHECK(tf.dx == doctest::Approx(0.0));
        CHECK(tf.dy == doctest::Approx(0.0));
        CHECK(tf.dz == doctest::Approx(0.0));
    }

    TEST_CASE("from translation") {
        auto tf = Transform::from_translation(1.0, 2.0, 3.0);

        double tx, ty, tz;
        tf.get_translation(tx, ty, tz);

        CHECK(tx == doctest::Approx(1.0));
        CHECK(ty == doctest::Approx(2.0));
        CHECK(tz == doctest::Approx(3.0));
    }

    TEST_CASE("from rotation") {
        // Identity rotation
        auto tf = Transform::from_rotation(1.0, 0.0, 0.0, 0.0);

        double qw, qx, qy, qz;
        tf.get_rotation(qw, qx, qy, qz);

        CHECK(qw == doctest::Approx(1.0));
        CHECK(qx == doctest::Approx(0.0));
        CHECK(qy == doctest::Approx(0.0));
        CHECK(qz == doctest::Approx(0.0));
    }

    TEST_CASE("from rotation and translation") {
        // 90 degree rotation around Z axis + translation
        double angle = M_PI / 2.0;
        double qw = std::cos(angle / 2.0);
        double qz = std::sin(angle / 2.0);

        auto tf = Transform::from_rotation_translation(qw, 0.0, 0.0, qz, 1.0, 2.0, 3.0);

        double tx, ty, tz;
        tf.get_translation(tx, ty, tz);

        CHECK(tx == doctest::Approx(1.0));
        CHECK(ty == doctest::Approx(2.0));
        CHECK(tz == doctest::Approx(3.0));
    }

    TEST_CASE("apply - pure translation") {
        auto tf = Transform::from_translation(1.0, 0.0, 0.0);

        double px = 0.0, py = 0.0, pz = 0.0;
        tf.apply(px, py, pz);

        CHECK(px == doctest::Approx(1.0));
        CHECK(py == doctest::Approx(0.0));
        CHECK(pz == doctest::Approx(0.0));
    }

    TEST_CASE("apply - pure rotation 90 deg around Z") {
        double angle = M_PI / 2.0;
        double qw = std::cos(angle / 2.0);
        double qz = std::sin(angle / 2.0);

        auto tf = Transform::from_rotation(qw, 0.0, 0.0, qz);

        // Rotate point (1, 0, 0) by 90 degrees around Z -> (0, 1, 0)
        double px = 1.0, py = 0.0, pz = 0.0;
        tf.apply(px, py, pz);

        CHECK(px == doctest::Approx(0.0).epsilon(1e-10));
        CHECK(py == doctest::Approx(1.0));
        CHECK(pz == doctest::Approx(0.0));
    }

    TEST_CASE("apply - rotation + translation") {
        double angle = M_PI / 2.0;
        double qw = std::cos(angle / 2.0);
        double qz = std::sin(angle / 2.0);

        auto tf = Transform::from_rotation_translation(qw, 0.0, 0.0, qz, 10.0, 0.0, 0.0);

        // Rotate (1, 0, 0) by 90 deg around Z -> (0, 1, 0), then translate by (10, 0, 0) -> (10, 1, 0)
        double px = 1.0, py = 0.0, pz = 0.0;
        tf.apply(px, py, pz);

        CHECK(px == doctest::Approx(10.0));
        CHECK(py == doctest::Approx(1.0));
        CHECK(pz == doctest::Approx(0.0));
    }

    TEST_CASE("composition") {
        // Two translations should add
        auto t1 = Transform::from_translation(1.0, 0.0, 0.0);
        auto t2 = Transform::from_translation(0.0, 2.0, 0.0);

        auto t3 = t1 * t2;

        double px = 0.0, py = 0.0, pz = 0.0;
        t3.apply(px, py, pz);

        CHECK(px == doctest::Approx(1.0));
        CHECK(py == doctest::Approx(2.0));
        CHECK(pz == doctest::Approx(0.0));
    }

    TEST_CASE("interpolation") {
        auto t1 = Transform::from_translation(0.0, 0.0, 0.0);
        auto t2 = Transform::from_translation(10.0, 0.0, 0.0);

        auto mid = lerp(t1, t2, 0.5);

        double tx, ty, tz;
        mid.get_translation(tx, ty, tz);

        CHECK(tx == doctest::Approx(5.0));
        CHECK(ty == doctest::Approx(0.0));
        CHECK(tz == doctest::Approx(0.0));
    }

    TEST_CASE("is_set") {
        auto identity = Transform::identity();
        CHECK_FALSE(identity.is_set());

        auto translated = Transform::from_translation(1.0, 0.0, 0.0);
        CHECK(translated.is_set());
    }

    TEST_CASE("serialization") {
        auto original = Transform::from_rotation_translation(0.707, 0.0, 0.707, 0.0, // 90° rotation around Y
                                                             1.0, 2.0, 3.0           // translation
        );

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, Transform>(buffer);

        CHECK(restored.rw == doctest::Approx(original.rw));
        CHECK(restored.rx == doctest::Approx(original.rx));
        CHECK(restored.ry == doctest::Approx(original.ry));
        CHECK(restored.rz == doctest::Approx(original.rz));
        CHECK(restored.dw == doctest::Approx(original.dw));
        CHECK(restored.dx == doctest::Approx(original.dx));
        CHECK(restored.dy == doctest::Approx(original.dy));
        CHECK(restored.dz == doctest::Approx(original.dz));
    }

    TEST_CASE("members reflection") {
        Transform tf = Transform::from_translation(1.0, 2.0, 3.0);
        auto tuple = tf.members();

        CHECK(std::get<0>(tuple) == doctest::Approx(1.0)); // rw
        CHECK(std::get<5>(tuple) == doctest::Approx(0.5)); // dx = tx/2
    }

    // ========================================================================
    // TEST: Namespace Utilities
    // ========================================================================

    TEST_CASE("transform::identity - creates identity transform") {
        auto tf = transform::identity();
        CHECK(tf.rw == 1.0);
        CHECK(tf.rx == 0.0);
        CHECK(tf.ry == 0.0);
        CHECK(tf.rz == 0.0);
        CHECK_FALSE(tf.is_set());
    }

    TEST_CASE("transform::make - rotation only") {
        auto tf = transform::make(1.0, 0.0, 0.0, 0.0);
        CHECK(tf.rw == 1.0);
        CHECK(tf.rx == 0.0);
        CHECK(tf.ry == 0.0);
        CHECK(tf.rz == 0.0);
    }

    TEST_CASE("transform::make - translation only") {
        auto tf = transform::make(1.0, 2.0, 3.0);

        double tx, ty, tz;
        tf.get_translation(tx, ty, tz);

        CHECK(tx == doctest::Approx(1.0));
        CHECK(ty == doctest::Approx(2.0));
        CHECK(tz == doctest::Approx(3.0));
    }

    TEST_CASE("transform::make - rotation and translation") {
        auto tf = transform::make(1.0, 0.0, 0.0, 0.0, 1.0, 2.0, 3.0);

        double tx, ty, tz;
        tf.get_translation(tx, ty, tz);

        CHECK(tx == doctest::Approx(1.0));
        CHECK(ty == doctest::Approx(2.0));
        CHECK(tz == doctest::Approx(3.0));
    }
}
