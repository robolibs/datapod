#pragma once

#include <cmath>
#include <tuple>

namespace datapod {

    /**
     * @brief Rigid body transform (rotation + translation) using dual quaternion - POD
     *
     * A Transform combines rotation (quaternion) and translation in a single
     * algebraic structure. It's the most compact representation for rigid body
     * transforms and allows smooth interpolation (ScLERP).
     *
     * Internally uses dual quaternion representation:
     * q = qr + ε·qd (real quaternion + epsilon * dual quaternion)
     * where qr encodes rotation and qd encodes translation (combined with qr).
     *
     * Fully serializable via members().
     *
     * Examples:
     *   Transform tf;
     *   tf = Transform::from_rotation_translation(quat, translation);
     *   tf = Transform::from_translation(1.0, 2.0, 3.0);
     *   auto interpolated = lerp(tf1, tf2, 0.5);
     *
     * Comparison with Quaternion:
     *   - Quaternion: rotation only (4 components, SO(3))
     *   - Transform: rotation + translation (8 components, SE(3))
     */
    struct Transform {
        // Real quaternion (rotation): w + xi + yj + zk
        double rw = 1.0; // Real part w
        double rx = 0.0; // i component
        double ry = 0.0; // j component
        double rz = 0.0; // k component

        // Dual quaternion (translation encoded): w' + x'i + y'j + z'k
        double dw = 0.0; // Dual w
        double dx = 0.0; // Dual i
        double dy = 0.0; // Dual j
        double dz = 0.0; // Dual k

        // ===== SERIALIZATION (required for datapod) =====
        auto members() noexcept { return std::tie(rw, rx, ry, rz, dw, dx, dy, dz); }
        auto members() const noexcept { return std::tie(rw, rx, ry, rz, dw, dx, dy, dz); }

        // Identity transformation
        static constexpr Transform identity() noexcept { return Transform{1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; }

        // From rotation quaternion only (no translation)
        static constexpr Transform from_rotation(double qw, double qx, double qy, double qz) noexcept {
            return Transform{qw, qx, qy, qz, 0.0, 0.0, 0.0, 0.0};
        }

        // From translation only (identity rotation)
        static constexpr Transform from_translation(double tx, double ty, double tz) noexcept {
            // For pure translation: qd = 0.5 * t * qr (where qr = identity)
            return Transform{1.0, 0.0, 0.0, 0.0, 0.0, tx / 2.0, ty / 2.0, tz / 2.0};
        }

        // From rotation quaternion and translation vector
        static constexpr Transform from_rotation_translation(double qw, double qx, double qy, double qz, double tx,
                                                             double ty, double tz) noexcept {
            // qd = 0.5 * t * qr where t is pure quaternion (0, tx, ty, tz)
            double dw_ = 0.5 * (-tx * qx - ty * qy - tz * qz);
            double dx_ = 0.5 * (tx * qw + ty * qz - tz * qy);
            double dy_ = 0.5 * (-tx * qz + ty * qw + tz * qx);
            double dz_ = 0.5 * (tx * qy - ty * qx + tz * qw);
            return Transform{qw, qx, qy, qz, dw_, dx_, dy_, dz_};
        }

        // Extract rotation quaternion
        constexpr void get_rotation(double &qw, double &qx, double &qy, double &qz) const noexcept {
            qw = rw;
            qx = rx;
            qy = ry;
            qz = rz;
        }

        // Extract translation vector
        constexpr void get_translation(double &tx, double &ty, double &tz) const noexcept {
            // t = 2 * qd * conj(qr)
            tx = 2.0 * (dx * rw - dw * rx + dz * ry - dy * rz);
            ty = 2.0 * (dy * rw - dz * rx - dw * ry + dx * rz);
            tz = 2.0 * (dz * rw + dy * rx - dx * ry - dw * rz);
        }

        // Properties
        inline double rotation_norm() const noexcept { return std::sqrt(rw * rw + rx * rx + ry * ry + rz * rz); }

        inline bool is_set() const noexcept {
            return rw != 1.0 || rx != 0.0 || ry != 0.0 || rz != 0.0 || dw != 0.0 || dx != 0.0 || dy != 0.0 || dz != 0.0;
        }

        // Conjugate (for rigid body inverse)
        constexpr Transform conjugate() const noexcept { return Transform{rw, -rx, -ry, -rz, dw, -dx, -dy, -dz}; }

        // Inverse transform
        inline Transform inverse() const noexcept {
            // For unit dual quaternion: inverse = conjugate
            Transform inv = conjugate();
            // Negate dual part for proper inverse
            inv.dw = -inv.dw;
            inv.dx = -inv.dx;
            inv.dy = -inv.dy;
            inv.dz = -inv.dz;
            return inv;
        }

        // Normalize (ensures rotation part has unit length)
        inline Transform normalized() const noexcept {
            double norm = rotation_norm();
            double inv_norm = 1.0 / norm;
            double dot = rw * dw + rx * dx + ry * dy + rz * dz;
            return Transform{rw * inv_norm,
                             rx * inv_norm,
                             ry * inv_norm,
                             rz * inv_norm,
                             (dw - rw * dot * inv_norm * inv_norm) * inv_norm,
                             (dx - rx * dot * inv_norm * inv_norm) * inv_norm,
                             (dy - ry * dot * inv_norm * inv_norm) * inv_norm,
                             (dz - rz * dot * inv_norm * inv_norm) * inv_norm};
        }

        // Composition: this * other (apply other first, then this)
        inline Transform operator*(const Transform &other) const noexcept {
            // Real part: qr1 * qr2
            double nrw = rw * other.rw - rx * other.rx - ry * other.ry - rz * other.rz;
            double nrx = rw * other.rx + rx * other.rw + ry * other.rz - rz * other.ry;
            double nry = rw * other.ry - rx * other.rz + ry * other.rw + rz * other.rx;
            double nrz = rw * other.rz + rx * other.ry - ry * other.rx + rz * other.rw;

            // Dual part: qr1 * qd2 + qd1 * qr2
            double ndw = rw * other.dw - rx * other.dx - ry * other.dy - rz * other.dz + dw * other.rw - dx * other.rx -
                         dy * other.ry - dz * other.rz;
            double ndx = rw * other.dx + rx * other.dw + ry * other.dz - rz * other.dy + dw * other.rx + dx * other.rw +
                         dy * other.rz - dz * other.ry;
            double ndy = rw * other.dy - rx * other.dz + ry * other.dw + rz * other.dx + dw * other.ry - dx * other.rz +
                         dy * other.rw + dz * other.rx;
            double ndz = rw * other.dz + rx * other.dy - ry * other.dx + rz * other.dw + dw * other.rz + dx * other.ry -
                         dy * other.rx + dz * other.rw;

            return Transform{nrw, nrx, nry, nrz, ndw, ndx, ndy, ndz};
        }

        inline Transform &operator*=(const Transform &other) noexcept {
            *this = *this * other;
            return *this;
        }

        // Transform a point (apply rotation and translation)
        inline void apply(double &px, double &py, double &pz) const noexcept {
            double tx, ty, tz;
            get_translation(tx, ty, tz);

            // Rotate point by quaternion: p' = q * p * conj(q)
            // Using optimized formula
            double t0 = 2.0 * (ry * pz - rz * py);
            double t1 = 2.0 * (rz * px - rx * pz);
            double t2 = 2.0 * (rx * py - ry * px);

            double rx_ = px + rw * t0 + (ry * t2 - rz * t1);
            double ry_ = py + rw * t1 + (rz * t0 - rx * t2);
            double rz_ = pz + rw * t2 + (rx * t1 - ry * t0);

            px = rx_ + tx;
            py = ry_ + ty;
            pz = rz_ + tz;
        }

        // Comparison
        inline bool operator==(const Transform &other) const noexcept {
            return rw == other.rw && rx == other.rx && ry == other.ry && rz == other.rz && dw == other.dw &&
                   dx == other.dx && dy == other.dy && dz == other.dz;
        }

        inline bool operator!=(const Transform &other) const noexcept { return !(*this == other); }
    };

    // Screw Linear Interpolation (ScLERP) - smooth interpolation between transforms
    inline Transform lerp(const Transform &t1, const Transform &t2, double t) noexcept {
        // Ensure shortest path
        double dot = t1.rw * t2.rw + t1.rx * t2.rx + t1.ry * t2.ry + t1.rz * t2.rz;
        double sign = dot < 0.0 ? -1.0 : 1.0;

        // Linear blend and normalize
        Transform result{t1.rw + t * (sign * t2.rw - t1.rw), t1.rx + t * (sign * t2.rx - t1.rx),
                         t1.ry + t * (sign * t2.ry - t1.ry), t1.rz + t * (sign * t2.rz - t1.rz),
                         t1.dw + t * (sign * t2.dw - t1.dw), t1.dx + t * (sign * t2.dx - t1.dx),
                         t1.dy + t * (sign * t2.dy - t1.dy), t1.dz + t * (sign * t2.dz - t1.dz)};

        return result.normalized();
    }

    namespace transform {
        /// Create an identity transform
        inline Transform identity() noexcept { return Transform::identity(); }

        /// Create a transform from rotation quaternion only
        inline Transform make(double qw, double qx, double qy, double qz) noexcept {
            return Transform::from_rotation(qw, qx, qy, qz);
        }

        /// Create a transform from translation only
        inline Transform make(double tx, double ty, double tz) noexcept {
            return Transform::from_translation(tx, ty, tz);
        }

        /// Create a transform from rotation and translation
        inline Transform make(double qw, double qx, double qy, double qz, double tx, double ty, double tz) noexcept {
            return Transform::from_rotation_translation(qw, qx, qy, qz, tx, ty, tz);
        }
    } // namespace transform

} // namespace datapod
