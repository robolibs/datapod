#pragma once

/**
 * @file quaternion.hpp
 * @brief Spatial quaternion for 3D rotations
 *
 * This file provides the Quaternion struct which extends mat::Quaternion<double>
 * with spatial/robotics-specific functionality like Euler angle conversion.
 *
 * For pure mathematical quaternion operations, see: datapod/matrix/math/quaternion.hpp
 */

#include "datapod/pods/matrix/math/quaternion.hpp"
#include "datapod/pods/spatial/euler.hpp"

namespace datapod {

    /**
     * @brief Unit quaternion for 3D rotation - extends mat::Quaternion<double>
     *
     * Inherits all mathematical operations from mat::Quaternion<double> and adds
     * spatial-specific functionality like Euler angle conversion.
     *
     * Convention: (w, x, y, z) where w is the scalar (real) part.
     * Identity: Quaternion{1, 0, 0, 0} represents no rotation.
     *
     * Examples:
     *   Quaternion q{1.0, 0.0, 0.0, 0.0};  // Identity (no rotation)
     *   Quaternion q2 = Quaternion::from_euler(roll, pitch, yaw);
     *   Euler e = q2.to_euler();           // Convert to Euler angles
     *   auto rotated = q * q2;             // Compose rotations (Hamilton product)
     */
    struct Quaternion : public mat::Quaternion<double> {
        using Base = mat::Quaternion<double>;

        // Inherit constructors
        using Base::Base;

        // Default constructor - identity quaternion
        constexpr Quaternion() noexcept : Base(1.0, 0.0, 0.0, 0.0) {}

        // Constructor from base type (implicit conversion)
        constexpr Quaternion(const Base &q) noexcept : Base(q) {}
        constexpr Quaternion(Base &&q) noexcept : Base(std::move(q)) {}

        // Aggregate-style constructor
        constexpr Quaternion(double w_, double x_, double y_, double z_) noexcept : Base(w_, x_, y_, z_) {}

        // ===== SPATIAL-SPECIFIC METHODS =====

        /**
         * @brief Convert to Euler angles
         * @return Euler angles (roll, pitch, yaw in radians)
         */
        inline Euler to_euler() const noexcept {
            double r, p, y;
            Base::to_euler(r, p, y);
            return Euler{r, p, y};
        }

        /**
         * @brief Create quaternion from Euler angles
         * @param e Euler angles (roll, pitch, yaw in radians)
         * @return Unit quaternion representing the rotation
         */
        static inline Quaternion from_euler(const Euler &e) noexcept {
            return Quaternion{Base::from_euler(e.roll, e.pitch, e.yaw)};
        }

        // Re-expose static factories with correct return type
        static constexpr Quaternion identity() noexcept { return Quaternion{1.0, 0.0, 0.0, 0.0}; }

        static inline Quaternion from_axis_angle(double ax, double ay, double az, double angle) noexcept {
            return Quaternion{Base::from_axis_angle(ax, ay, az, angle)};
        }

        static inline Quaternion from_euler(double roll, double pitch, double yaw) noexcept {
            return Quaternion{Base::from_euler(roll, pitch, yaw)};
        }

        // Override operations that return Base to return Quaternion instead
        constexpr Quaternion conjugate() const noexcept { return Quaternion{Base::conjugate()}; }

        inline Quaternion inverse() const noexcept { return Quaternion{Base::inverse()}; }

        constexpr Quaternion unit_inverse() const noexcept { return Quaternion{Base::unit_inverse()}; }

        inline Quaternion normalized() const noexcept { return Quaternion{Base::normalized()}; }

        // Operators that return Quaternion
        constexpr Quaternion operator-() const noexcept { return Quaternion{Base::operator-()}; }
        constexpr Quaternion operator+() const noexcept { return *this; }
    };

    // Binary operators returning Quaternion
    inline Quaternion operator+(const Quaternion &a, const Quaternion &b) noexcept {
        return Quaternion{static_cast<const mat::Quaternion<double> &>(a) +
                          static_cast<const mat::Quaternion<double> &>(b)};
    }

    inline Quaternion operator-(const Quaternion &a, const Quaternion &b) noexcept {
        return Quaternion{static_cast<const mat::Quaternion<double> &>(a) -
                          static_cast<const mat::Quaternion<double> &>(b)};
    }

    inline Quaternion operator*(const Quaternion &a, const Quaternion &b) noexcept {
        return Quaternion{static_cast<const mat::Quaternion<double> &>(a) *
                          static_cast<const mat::Quaternion<double> &>(b)};
    }

    inline Quaternion operator/(const Quaternion &a, const Quaternion &b) noexcept {
        return Quaternion{static_cast<const mat::Quaternion<double> &>(a) /
                          static_cast<const mat::Quaternion<double> &>(b)};
    }

    inline Quaternion operator*(const Quaternion &q, double s) noexcept {
        return Quaternion{static_cast<const mat::Quaternion<double> &>(q) * s};
    }

    inline Quaternion operator*(double s, const Quaternion &q) noexcept { return q * s; }

    inline Quaternion operator/(const Quaternion &q, double s) noexcept {
        return Quaternion{static_cast<const mat::Quaternion<double> &>(q) / s};
    }

    // Interpolation returning Quaternion
    inline Quaternion lerp(const Quaternion &a, const Quaternion &b, double t) noexcept {
        return Quaternion{mat::lerp(static_cast<const mat::Quaternion<double> &>(a),
                                    static_cast<const mat::Quaternion<double> &>(b), t)};
    }

    inline Quaternion nlerp(const Quaternion &a, const Quaternion &b, double t) noexcept {
        return Quaternion{mat::nlerp(static_cast<const mat::Quaternion<double> &>(a),
                                     static_cast<const mat::Quaternion<double> &>(b), t)};
    }

    inline Quaternion slerp(const Quaternion &a, const Quaternion &b, double t) noexcept {
        return Quaternion{mat::slerp(static_cast<const mat::Quaternion<double> &>(a),
                                     static_cast<const mat::Quaternion<double> &>(b), t)};
    }

    // ===== EULER IMPLEMENTATION =====

    inline Quaternion Euler::to_quaternion() const noexcept { return Quaternion::from_euler(roll, pitch, yaw); }

    // ===== FLOAT VERSION =====

    /**
     * @brief Single-precision quaternion for 3D rotation
     */
    struct Quaternionf : public mat::Quaternion<float> {
        using Base = mat::Quaternion<float>;
        using Base::Base;

        constexpr Quaternionf() noexcept : Base(1.0f, 0.0f, 0.0f, 0.0f) {}
        constexpr Quaternionf(const Base &q) noexcept : Base(q) {}
        constexpr Quaternionf(float w_, float x_, float y_, float z_) noexcept : Base(w_, x_, y_, z_) {}

        static constexpr Quaternionf identity() noexcept { return Quaternionf{1.0f, 0.0f, 0.0f, 0.0f}; }

        constexpr Quaternionf conjugate() const noexcept { return Quaternionf{Base::conjugate()}; }
        inline Quaternionf inverse() const noexcept { return Quaternionf{Base::inverse()}; }
        inline Quaternionf normalized() const noexcept { return Quaternionf{Base::normalized()}; }
    };

} // namespace datapod
