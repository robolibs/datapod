#pragma once

#include <cmath>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Quaternion - 4-dimensional hypercomplex number - POD
         *
         * Quaternions extend complex numbers to 4 dimensions. They form a
         * non-commutative division algebra. Primary uses:
         *   - 3D rotations (unit quaternions)
         *   - Computer graphics
         *   - Robotics and aerospace
         *   - Signal processing
         *
         * q = w + xi + yj + zk
         * where i² = j² = k² = ijk = -1
         *
         * Convention: (w, x, y, z) where w is the scalar (real) part.
         * This matches Eigen, ROS, and most robotics conventions.
         *
         * Fully serializable via members().
         *
         * Examples:
         *   Quaternion<double> q{1, 0, 0, 0};     // Identity Quaternion
         *   Quaternion<float> q2{0.707, 0, 0.707, 0}; // 90° rotation about Y
         *   auto conj = q.conjugate();
         *   auto prod = q * q2;                   // Hamilton product
         */
        template <typename T> struct Quaternion {
            static_assert(std::is_floating_point_v<T>, "Quaternion<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T w{}; // Scalar (real) part
            T x{}; // Imaginary i
            T y{}; // Imaginary j
            T z{}; // Imaginary k

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(w, x, y, z); }
            auto members() const noexcept { return std::tie(w, x, y, z); }

            // ===== CONSTRUCTION =====
            constexpr Quaternion() noexcept = default;

            constexpr Quaternion(T scalar) noexcept : w(scalar), x{}, y{}, z{} {}

            constexpr Quaternion(T w_, T x_, T y_, T z_) noexcept : w(w_), x(x_), y(y_), z(z_) {}

            // From scalar and vector parts
            constexpr Quaternion(T scalar, T vx, T vy, T vz, bool /*tag*/) noexcept : w(scalar), x(vx), y(vy), z(vz) {}

            // ===== STATIC FACTORIES =====

            // Identity Quaternion (represents no rotation)
            static constexpr Quaternion identity() noexcept { return Quaternion{T{1}, T{0}, T{0}, T{0}}; }

            // Unit imaginary quaternions
            static constexpr Quaternion i() noexcept { return Quaternion{T{0}, T{1}, T{0}, T{0}}; }
            static constexpr Quaternion j() noexcept { return Quaternion{T{0}, T{0}, T{1}, T{0}}; }
            static constexpr Quaternion k() noexcept { return Quaternion{T{0}, T{0}, T{0}, T{1}}; }

            // From axis-angle representation (for 3D rotations)
            // axis should be a unit vector, angle is in radians
            static inline Quaternion from_axis_angle(T ax, T ay, T az, T angle) noexcept {
                T half = angle / T{2};
                T s = std::sin(half);
                return Quaternion{std::cos(half), ax * s, ay * s, az * s};
            }

            // From Euler angles (ZYX convention: yaw, pitch, roll)
            static inline Quaternion from_euler(T roll, T pitch, T yaw) noexcept {
                T cr = std::cos(roll * T{0.5});
                T sr = std::sin(roll * T{0.5});
                T cp = std::cos(pitch * T{0.5});
                T sp = std::sin(pitch * T{0.5});
                T cy = std::cos(yaw * T{0.5});
                T sy = std::sin(yaw * T{0.5});

                return Quaternion{cr * cp * cy + sr * sp * sy,  // w
                                  sr * cp * cy - cr * sp * sy,  // x
                                  cr * sp * cy + sr * cp * sy,  // y
                                  cr * cp * sy - sr * sp * cy}; // z
            }

            // ===== PROPERTIES =====

            constexpr T scalar() const noexcept { return w; }

            constexpr T norm_squared() const noexcept { return w * w + x * x + y * y + z * z; }

            inline T norm() const noexcept { return std::sqrt(norm_squared()); }
            inline T magnitude() const noexcept { return norm(); }

            // ===== UTILITY =====

            constexpr bool is_identity() const noexcept { return w == T{1} && x == T{0} && y == T{0} && z == T{0}; }

            constexpr bool is_real() const noexcept { return x == T{0} && y == T{0} && z == T{0}; }

            constexpr bool is_pure() const noexcept { return w == T{0}; } // Pure imaginary Quaternion

            constexpr bool is_set() const noexcept { return w != T{1} || x != T{0} || y != T{0} || z != T{0}; }

            inline bool is_unit(T tolerance = T{1e-6}) const noexcept {
                return std::abs(norm_squared() - T{1}) < tolerance;
            }

            // ===== QUATERNION OPERATIONS =====

            // Conjugate: negate imaginary parts
            constexpr Quaternion conjugate() const noexcept { return Quaternion{w, -x, -y, -z}; }

            // Inverse: conj / |q|²
            inline Quaternion inverse() const noexcept {
                T n2 = norm_squared();
                return Quaternion{w / n2, -x / n2, -y / n2, -z / n2};
            }

            // For unit quaternions, inverse == conjugate (faster)
            constexpr Quaternion unit_inverse() const noexcept { return conjugate(); }

            // Normalized (unit Quaternion)
            inline Quaternion normalized() const noexcept {
                T n = norm();
                if (n < T{1e-10}) {
                    return identity();
                }
                return Quaternion{w / n, x / n, y / n, z / n};
            }

            // ===== ROTATION OPERATIONS (for unit quaternions) =====

            // Rotate a vector (vx, vy, vz) by this Quaternion
            // q * v * q^-1 where v = (0, vx, vy, vz)
            inline void rotate_vector(T &vx, T &vy, T &vz) const noexcept {
                // Optimized Rodrigues rotation formula
                T qw = w, qx = x, qy = y, qz = z;

                // t = 2 * cross(q.xyz, v)
                T tx = T{2} * (qy * vz - qz * vy);
                T ty = T{2} * (qz * vx - qx * vz);
                T tz = T{2} * (qx * vy - qy * vx);

                // v' = v + qw * t + cross(q.xyz, t)
                vx = vx + qw * tx + (qy * tz - qz * ty);
                vy = vy + qw * ty + (qz * tx - qx * tz);
                vz = vz + qw * tz + (qx * ty - qy * tx);
            }

            // Convert to Euler angles (roll, pitch, yaw in radians)
            inline void to_euler(T &roll, T &pitch, T &yaw) const noexcept {
                // Roll (x-axis rotation)
                T sinr_cosp = T{2} * (w * x + y * z);
                T cosr_cosp = T{1} - T{2} * (x * x + y * y);
                roll = std::atan2(sinr_cosp, cosr_cosp);

                // Pitch (y-axis rotation)
                T sinp = T{2} * (w * y - z * x);
                if (std::abs(sinp) >= T{1}) {
                    pitch = std::copysign(T{1.5707963267948966}, sinp); // 90 degrees
                } else {
                    pitch = std::asin(sinp);
                }

                // Yaw (z-axis rotation)
                T siny_cosp = T{2} * (w * z + x * y);
                T cosy_cosp = T{1} - T{2} * (y * y + z * z);
                yaw = std::atan2(siny_cosp, cosy_cosp);
            }

            // Convert to axis-angle representation
            inline void to_axis_angle(T &ax, T &ay, T &az, T &angle) const noexcept {
                angle = T{2} * std::acos(w);
                T s = std::sqrt(T{1} - w * w);
                if (s < T{1e-10}) {
                    // No rotation, arbitrary axis
                    ax = T{1};
                    ay = T{0};
                    az = T{0};
                } else {
                    ax = x / s;
                    ay = y / s;
                    az = z / s;
                }
            }

            // ===== COMPOUND ASSIGNMENT - QUATERNION =====

            constexpr Quaternion &operator+=(const Quaternion &other) noexcept {
                w += other.w;
                x += other.x;
                y += other.y;
                z += other.z;
                return *this;
            }

            constexpr Quaternion &operator-=(const Quaternion &other) noexcept {
                w -= other.w;
                x -= other.x;
                y -= other.y;
                z -= other.z;
                return *this;
            }

            // Hamilton product (non-commutative!)
            constexpr Quaternion &operator*=(const Quaternion &other) noexcept {
                T nw = w * other.w - x * other.x - y * other.y - z * other.z;
                T nx = w * other.x + x * other.w + y * other.z - z * other.y;
                T ny = w * other.y - x * other.z + y * other.w + z * other.x;
                T nz = w * other.z + x * other.y - y * other.x + z * other.w;
                w = nw;
                x = nx;
                y = ny;
                z = nz;
                return *this;
            }

            inline Quaternion &operator/=(const Quaternion &other) noexcept {
                *this *= other.inverse();
                return *this;
            }

            // ===== COMPOUND ASSIGNMENT - SCALAR =====

            constexpr Quaternion &operator*=(T s) noexcept {
                w *= s;
                x *= s;
                y *= s;
                z *= s;
                return *this;
            }

            constexpr Quaternion &operator/=(T s) noexcept {
                w /= s;
                x /= s;
                y /= s;
                z /= s;
                return *this;
            }

            // ===== UNARY OPERATORS =====

            constexpr Quaternion operator-() const noexcept { return Quaternion{-w, -x, -y, -z}; }
            constexpr Quaternion operator+() const noexcept { return *this; }

            // ===== COMPARISON =====

            constexpr bool operator==(const Quaternion &other) const noexcept {
                return w == other.w && x == other.x && y == other.y && z == other.z;
            }

            constexpr bool operator!=(const Quaternion &other) const noexcept { return !(*this == other); }
        };

        // ===== BINARY OPERATORS - QUATERNION-QUATERNION =====

        template <typename T>
        constexpr Quaternion<T> operator+(const Quaternion<T> &a, const Quaternion<T> &b) noexcept {
            return Quaternion<T>{a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z};
        }

        template <typename T>
        constexpr Quaternion<T> operator-(const Quaternion<T> &a, const Quaternion<T> &b) noexcept {
            return Quaternion<T>{a.w - b.w, a.x - b.x, a.y - b.y, a.z - b.z};
        }

        // Hamilton product
        template <typename T>
        constexpr Quaternion<T> operator*(const Quaternion<T> &a, const Quaternion<T> &b) noexcept {
            return Quaternion<T>{
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
        }

        template <typename T> inline Quaternion<T> operator/(const Quaternion<T> &a, const Quaternion<T> &b) noexcept {
            return a * b.inverse();
        }

        // ===== BINARY OPERATORS - QUATERNION-SCALAR =====

        template <typename T> constexpr Quaternion<T> operator*(const Quaternion<T> &q, T s) noexcept {
            return Quaternion<T>{q.w * s, q.x * s, q.y * s, q.z * s};
        }

        template <typename T> constexpr Quaternion<T> operator*(T s, const Quaternion<T> &q) noexcept { return q * s; }

        template <typename T> constexpr Quaternion<T> operator/(const Quaternion<T> &q, T s) noexcept {
            return Quaternion<T>{q.w / s, q.x / s, q.y / s, q.z / s};
        }

        // ===== DOT PRODUCT =====

        template <typename T> constexpr T dot(const Quaternion<T> &a, const Quaternion<T> &b) noexcept {
            return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
        }

        // ===== INTERPOLATION =====

        // Linear interpolation (not normalized - use for blending, then normalize)
        template <typename T>
        constexpr Quaternion<T> lerp(const Quaternion<T> &a, const Quaternion<T> &b, T t) noexcept {
            return Quaternion<T>{a.w + t * (b.w - a.w), a.x + t * (b.x - a.x), a.y + t * (b.y - a.y),
                                 a.z + t * (b.z - a.z)};
        }

        // Normalized linear interpolation (fast approximation to slerp)
        template <typename T> inline Quaternion<T> nlerp(const Quaternion<T> &a, const Quaternion<T> &b, T t) noexcept {
            // Handle Quaternion double-cover (q and -q represent same rotation)
            Quaternion<T> b2 = dot(a, b) < T{0} ? -b : b;
            return lerp(a, b2, t).normalized();
        }

        // Spherical linear interpolation (constant angular velocity)
        template <typename T> inline Quaternion<T> slerp(const Quaternion<T> &a, const Quaternion<T> &b, T t) noexcept {
            // Handle Quaternion double-cover
            T d = dot(a, b);
            Quaternion<T> b2 = d < T{0} ? -b : b;
            d = std::abs(d);

            // If quaternions are very close, use nlerp
            if (d > T{0.9995}) {
                return nlerp(a, b2, t);
            }

            T theta = std::acos(d);
            T sin_theta = std::sin(theta);
            T wa = std::sin((T{1} - t) * theta) / sin_theta;
            T wb = std::sin(t * theta) / sin_theta;

            return Quaternion<T>{wa * a.w + wb * b2.w, wa * a.x + wb * b2.x, wa * a.y + wb * b2.y,
                                 wa * a.z + wb * b2.z};
        }

        // ===== EXPONENTIAL AND LOGARITHM =====

        // Exponential: exp(q) where q = (0, v) is a pure Quaternion
        // Result is a unit Quaternion representing rotation
        template <typename T> inline Quaternion<T> exp(const Quaternion<T> &q) noexcept {
            T vnorm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
            T ew = std::exp(q.w);

            if (vnorm < T{1e-10}) {
                return Quaternion<T>{ew, T{0}, T{0}, T{0}};
            }

            T s = ew * std::sin(vnorm) / vnorm;
            return Quaternion<T>{ew * std::cos(vnorm), s * q.x, s * q.y, s * q.z};
        }

        // Logarithm: log(q) - result is a pure Quaternion (w ≈ 0) for unit quaternions
        template <typename T> inline Quaternion<T> log(const Quaternion<T> &q) noexcept {
            T n = q.norm();
            T vnorm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);

            if (vnorm < T{1e-10}) {
                return Quaternion<T>{std::log(n), T{0}, T{0}, T{0}};
            }

            T s = std::acos(q.w / n) / vnorm;
            return Quaternion<T>{std::log(n), s * q.x, s * q.y, s * q.z};
        }

        // Power: q^t (useful for interpolation)
        template <typename T> inline Quaternion<T> pow(const Quaternion<T> &q, T t) noexcept { return exp(log(q) * t); }

        // ===== TYPE TRAITS =====

        template <typename T> struct is_quaternion : std::false_type {};
        template <typename T> struct is_quaternion<Quaternion<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_quaternion_v = is_quaternion<T>::value;

        // ===== TYPE ALIASES =====

        using quaternionf = Quaternion<float>;
        using quaterniond = Quaternion<double>;

    } // namespace mat
} // namespace datapod
