#pragma once

#include <cmath>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Dual quaternion for rigid body transformations (rotation + translation) - POD
         *
         * A dual quaternion combines rotation (quaternion) and translation in a single
         * algebraic structure. It's the most compact representation for rigid body
         * transforms and allows smooth interpolation (ScLERP).
         *
         * Structure: q = qr + ε·qd (real quaternion + epsilon * dual quaternion)
         * where qr encodes rotation and qd encodes translation (combined with qr).
         *
         * Fully serializable via members().
         *
         * Examples:
         *   dual_quaternion<double> dq;
         *   dq = dual_quaternion<double>::from_rotation_translation(quat, translation);
         *   auto interpolated = sclerp(dq1, dq2, 0.5);
         */
        template <typename T> struct dual_quaternion {
            static_assert(std::is_floating_point_v<T>, "dual_quaternion<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            // Real quaternion (rotation): w + xi + yj + zk
            T rw{1}; // Real part w
            T rx{};  // i component
            T ry{};  // j component
            T rz{};  // k component

            // Dual quaternion (translation encoded): w' + x'i + y'j + z'k
            T dw{}; // Dual w
            T dx{}; // Dual i
            T dy{}; // Dual j
            T dz{}; // Dual k

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(rw, rx, ry, rz, dw, dx, dy, dz); }
            auto members() const noexcept { return std::tie(rw, rx, ry, rz, dw, dx, dy, dz); }

            // Construction
            constexpr dual_quaternion() noexcept = default;

            constexpr dual_quaternion(T rw_, T rx_, T ry_, T rz_, T dw_, T dx_, T dy_, T dz_) noexcept
                : rw(rw_), rx(rx_), ry(ry_), rz(rz_), dw(dw_), dx(dx_), dy(dy_), dz(dz_) {}

            // From rotation quaternion only (no translation)
            static constexpr dual_quaternion from_rotation(T qw, T qx, T qy, T qz) noexcept {
                return dual_quaternion{qw, qx, qy, qz, T{0}, T{0}, T{0}, T{0}};
            }

            // From translation only (identity rotation)
            static constexpr dual_quaternion from_translation(T tx, T ty, T tz) noexcept {
                // For pure translation: qd = 0.5 * t * qr (where qr = identity)
                return dual_quaternion{T{1}, T{0}, T{0}, T{0}, T{0}, tx / T{2}, ty / T{2}, tz / T{2}};
            }

            // From rotation quaternion and translation vector
            static constexpr dual_quaternion from_rotation_translation(T qw, T qx, T qy, T qz, T tx, T ty,
                                                                       T tz) noexcept {
                // qd = 0.5 * t * qr where t is pure quaternion (0, tx, ty, tz)
                // t * qr = (0 + tx*i + ty*j + tz*k) * (qw + qx*i + qy*j + qz*k)
                T dw_ = T{0.5} * (-tx * qx - ty * qy - tz * qz);
                T dx_ = T{0.5} * (tx * qw + ty * qz - tz * qy);
                T dy_ = T{0.5} * (-tx * qz + ty * qw + tz * qx);
                T dz_ = T{0.5} * (tx * qy - ty * qx + tz * qw);
                return dual_quaternion{qw, qx, qy, qz, dw_, dx_, dy_, dz_};
            }

            // Identity transformation
            static constexpr dual_quaternion identity() noexcept {
                return dual_quaternion{T{1}, T{0}, T{0}, T{0}, T{0}, T{0}, T{0}, T{0}};
            }

            // Extract rotation quaternion (normalized)
            constexpr void get_rotation(T &qw, T &qx, T &qy, T &qz) const noexcept {
                qw = rw;
                qx = rx;
                qy = ry;
                qz = rz;
            }

            // Extract translation vector
            constexpr void get_translation(T &tx, T &ty, T &tz) const noexcept {
                // t = 2 * qd * conj(qr)
                // conj(qr) = (rw, -rx, -ry, -rz)
                T tw = T{2} * (dw * rw + dx * rx + dy * ry + dz * rz);
                tx = T{2} * (dx * rw - dw * rx + dz * ry - dy * rz);
                ty = T{2} * (dy * rw - dz * rx - dw * ry + dx * rz);
                tz = T{2} * (dz * rw + dy * rx - dx * ry - dw * rz);
                (void)tw; // tw should be 0 for valid dual quaternion
            }

            // Properties
            inline T real_norm() const noexcept { return std::sqrt(rw * rw + rx * rx + ry * ry + rz * rz); }

            constexpr bool is_set() const noexcept {
                return rw != T{1} || rx != T{0} || ry != T{0} || rz != T{0} || dw != T{0} || dx != T{0} || dy != T{0} ||
                       dz != T{0};
            }

            // Conjugate (for rigid body inverse)
            constexpr dual_quaternion conjugate() const noexcept {
                return dual_quaternion{rw, -rx, -ry, -rz, dw, -dx, -dy, -dz};
            }

            // Dual conjugate
            constexpr dual_quaternion dual_conjugate() const noexcept {
                return dual_quaternion{rw, rx, ry, rz, -dw, -dx, -dy, -dz};
            }

            // Full conjugate (both)
            constexpr dual_quaternion full_conjugate() const noexcept {
                return dual_quaternion{rw, -rx, -ry, -rz, -dw, dx, dy, dz};
            }

            // Normalize (ensures real part has unit length)
            inline dual_quaternion normalized() const noexcept {
                T norm = real_norm();
                T inv_norm = T{1} / norm;
                T dot = rw * dw + rx * dx + ry * dy + rz * dz;
                return dual_quaternion{rw * inv_norm,
                                       rx * inv_norm,
                                       ry * inv_norm,
                                       rz * inv_norm,
                                       (dw - rw * dot * inv_norm * inv_norm) * inv_norm,
                                       (dx - rx * dot * inv_norm * inv_norm) * inv_norm,
                                       (dy - ry * dot * inv_norm * inv_norm) * inv_norm,
                                       (dz - rz * dot * inv_norm * inv_norm) * inv_norm};
            }

            // Multiplication (composition of transforms)
            constexpr dual_quaternion operator*(const dual_quaternion &other) const noexcept {
                // Real part: qr1 * qr2
                T nrw = rw * other.rw - rx * other.rx - ry * other.ry - rz * other.rz;
                T nrx = rw * other.rx + rx * other.rw + ry * other.rz - rz * other.ry;
                T nry = rw * other.ry - rx * other.rz + ry * other.rw + rz * other.rx;
                T nrz = rw * other.rz + rx * other.ry - ry * other.rx + rz * other.rw;

                // Dual part: qr1 * qd2 + qd1 * qr2
                T ndw = rw * other.dw - rx * other.dx - ry * other.dy - rz * other.dz + dw * other.rw - dx * other.rx -
                        dy * other.ry - dz * other.rz;
                T ndx = rw * other.dx + rx * other.dw + ry * other.dz - rz * other.dy + dw * other.rx + dx * other.rw +
                        dy * other.rz - dz * other.ry;
                T ndy = rw * other.dy - rx * other.dz + ry * other.dw + rz * other.dx + dw * other.ry - dx * other.rz +
                        dy * other.rw + dz * other.rx;
                T ndz = rw * other.dz + rx * other.dy - ry * other.dx + rz * other.dw + dw * other.rz + dx * other.ry -
                        dy * other.rx + dz * other.rw;

                return dual_quaternion{nrw, nrx, nry, nrz, ndw, ndx, ndy, ndz};
            }

            constexpr dual_quaternion &operator*=(const dual_quaternion &other) noexcept {
                *this = *this * other;
                return *this;
            }

            // Addition
            constexpr dual_quaternion operator+(const dual_quaternion &other) const noexcept {
                return dual_quaternion{rw + other.rw, rx + other.rx, ry + other.ry, rz + other.rz,
                                       dw + other.dw, dx + other.dx, dy + other.dy, dz + other.dz};
            }

            constexpr dual_quaternion &operator+=(const dual_quaternion &other) noexcept {
                rw += other.rw;
                rx += other.rx;
                ry += other.ry;
                rz += other.rz;
                dw += other.dw;
                dx += other.dx;
                dy += other.dy;
                dz += other.dz;
                return *this;
            }

            // Scalar multiplication
            constexpr dual_quaternion operator*(T s) const noexcept {
                return dual_quaternion{rw * s, rx * s, ry * s, rz * s, dw * s, dx * s, dy * s, dz * s};
            }

            constexpr dual_quaternion &operator*=(T s) noexcept {
                rw *= s;
                rx *= s;
                ry *= s;
                rz *= s;
                dw *= s;
                dx *= s;
                dy *= s;
                dz *= s;
                return *this;
            }

            // Transform a point (apply rotation and translation)
            constexpr void transform_point(T &px, T &py, T &pz) const noexcept {
                // Create dual quaternion for point: (1 + ε(0,p))
                // Result: dq * p_dq * conjugate(dq)
                // Simplified: just apply rotation then translation
                T tx, ty, tz;
                get_translation(tx, ty, tz);

                // Rotate point by quaternion: p' = q * p * conj(q)
                T qw = rw, qx = rx, qy = ry, qz = rz;

                // v = 2 * cross(q.xyz, p) + p
                T t0 = T{2} * (qy * pz - qz * py);
                T t1 = T{2} * (qz * px - qx * pz);
                T t2 = T{2} * (qx * py - qy * px);

                T rx_ = px + qw * t0 + (qy * t2 - qz * t1);
                T ry_ = py + qw * t1 + (qz * t0 - qx * t2);
                T rz_ = pz + qw * t2 + (qx * t1 - qy * t0);

                px = rx_ + tx;
                py = ry_ + ty;
                pz = rz_ + tz;
            }

            // Comparison
            constexpr bool operator==(const dual_quaternion &other) const noexcept {
                return rw == other.rw && rx == other.rx && ry == other.ry && rz == other.rz && dw == other.dw &&
                       dx == other.dx && dy == other.dy && dz == other.dz;
            }

            constexpr bool operator!=(const dual_quaternion &other) const noexcept { return !(*this == other); }
        };

        // Scalar multiplication (left)
        template <typename T> constexpr dual_quaternion<T> operator*(T s, const dual_quaternion<T> &dq) noexcept {
            return dq * s;
        }

        // Screw Linear Interpolation (ScLERP)
        template <typename T>
        inline dual_quaternion<T> sclerp(const dual_quaternion<T> &dq1, const dual_quaternion<T> &dq2, T t) noexcept {
            // Ensure shortest path
            T dot = dq1.rw * dq2.rw + dq1.rx * dq2.rx + dq1.ry * dq2.ry + dq1.rz * dq2.rz;
            dual_quaternion<T> q2 = dot < T{0} ? dq2 * T{-1} : dq2;

            // Linear blend and normalize
            dual_quaternion<T> result{dq1.rw + t * (q2.rw - dq1.rw), dq1.rx + t * (q2.rx - dq1.rx),
                                      dq1.ry + t * (q2.ry - dq1.ry), dq1.rz + t * (q2.rz - dq1.rz),
                                      dq1.dw + t * (q2.dw - dq1.dw), dq1.dx + t * (q2.dx - dq1.dx),
                                      dq1.dy + t * (q2.dy - dq1.dy), dq1.dz + t * (q2.dz - dq1.dz)};

            return result.normalized();
        }

        // Type traits
        template <typename T> struct is_dual_quaternion : std::false_type {};
        template <typename T> struct is_dual_quaternion<dual_quaternion<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_dual_quaternion_v = is_dual_quaternion<T>::value;

        // Type aliases
        using dual_quaternionf = dual_quaternion<float>;
        using dual_quaterniond = dual_quaternion<double>;

    } // namespace mat
} // namespace datapod
