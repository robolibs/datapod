#pragma once

#include <cmath>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Phasor (magnitude ∠ phase) for AC circuit analysis and signal processing - POD
         *
         * A Phasor represents a sinusoidal signal as a rotating vector in the complex plane.
         * Commonly used in electrical engineering for AC circuit analysis, and in signal
         * processing for representing amplitude and phase of harmonic signals.
         *
         * Stored in polar form (magnitude, phase) for intuitive manipulation.
         * Phase is in radians.
         * Fully serializable via members().
         *
         * Examples:
         *   Phasor<double> v{120.0, 0.0};           // 120V at 0° phase
         *   Phasor<double> i{10.0, -0.5236};        // 10A lagging by 30°
         *   auto z = v / i;                         // Impedance
         *   auto p = v.real_power(i);               // Real power
         */
        template <typename T> struct Phasor {
            static_assert(std::is_floating_point_v<T>, "Phasor<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T mag{};   // Magnitude (amplitude)
            T phase{}; // Phase angle in radians

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(mag, phase); }
            auto members() const noexcept { return std::tie(mag, phase); }

            // Construction
            constexpr Phasor() noexcept = default;
            constexpr Phasor(T magnitude) noexcept : mag(magnitude), phase{} {}
            constexpr Phasor(T magnitude, T phase_rad) noexcept : mag(magnitude), phase(phase_rad) {}

            // From rectangular (real + imag)
            static inline Phasor from_rectangular(T real, T imag) noexcept {
                return Phasor{std::sqrt(real * real + imag * imag), std::atan2(imag, real)};
            }

            // From degrees
            static constexpr Phasor from_degrees(T magnitude, T phase_deg) noexcept {
                constexpr T deg_to_rad = T{3.14159265358979323846} / T{180};
                return Phasor{magnitude, phase_deg * deg_to_rad};
            }

            // Convert to rectangular form
            inline T real() const noexcept { return mag * std::cos(phase); }
            inline T imag() const noexcept { return mag * std::sin(phase); }

            // Phase in degrees
            inline T phase_degrees() const noexcept {
                constexpr T rad_to_deg = T{180} / T{3.14159265358979323846};
                return phase * rad_to_deg;
            }

            // RMS value (for sinusoidal signals, peak/sqrt(2))
            inline T rms() const noexcept { return mag / std::sqrt(T{2}); }

            // Peak value
            constexpr T peak() const noexcept { return mag; }

            // Peak-to-peak value
            constexpr T peak_to_peak() const noexcept { return mag * T{2}; }

            // Utility
            constexpr bool is_set() const noexcept { return mag != T{} || phase != T{}; }

            // Normalize phase to [-π, π]
            inline Phasor normalized_phase() const noexcept {
                constexpr T pi = T{3.14159265358979323846};
                constexpr T two_pi = T{2} * pi;
                T p = phase;
                while (p > pi)
                    p -= two_pi;
                while (p < -pi)
                    p += two_pi;
                return Phasor{mag, p};
            }

            // Conjugate (negate phase)
            constexpr Phasor conjugate() const noexcept { return Phasor{mag, -phase}; }

            // Compound assignment
            constexpr Phasor &operator*=(const Phasor &other) noexcept {
                mag *= other.mag;
                phase += other.phase;
                return *this;
            }

            constexpr Phasor &operator/=(const Phasor &other) noexcept {
                mag /= other.mag;
                phase -= other.phase;
                return *this;
            }

            constexpr Phasor &operator*=(T s) noexcept {
                mag *= s;
                return *this;
            }

            constexpr Phasor &operator/=(T s) noexcept {
                mag /= s;
                return *this;
            }

            // Addition requires conversion to rectangular
            inline Phasor &operator+=(const Phasor &other) noexcept {
                T r = real() + other.real();
                T i = imag() + other.imag();
                mag = std::sqrt(r * r + i * i);
                phase = std::atan2(i, r);
                return *this;
            }

            inline Phasor &operator-=(const Phasor &other) noexcept {
                T r = real() - other.real();
                T i = imag() - other.imag();
                mag = std::sqrt(r * r + i * i);
                phase = std::atan2(i, r);
                return *this;
            }

            // Unary operators
            constexpr Phasor operator-() const noexcept {
                constexpr T pi = T{3.14159265358979323846};
                return Phasor{mag, phase + pi};
            }
            constexpr Phasor operator+() const noexcept { return *this; }

            // Power calculations for AC circuits

            // Complex power S = V * I* (voltage Phasor times current conjugate)
            inline Phasor complex_power(const Phasor &current) const noexcept {
                return Phasor{mag * current.mag, phase - current.phase};
            }

            // Real power P = |V||I|cos(θ) where θ is phase difference
            inline T real_power(const Phasor &current) const noexcept {
                return mag * current.mag * std::cos(phase - current.phase);
            }

            // Reactive power Q = |V||I|sin(θ)
            inline T reactive_power(const Phasor &current) const noexcept {
                return mag * current.mag * std::sin(phase - current.phase);
            }

            // Apparent power |S| = |V||I|
            constexpr T apparent_power(const Phasor &current) const noexcept { return mag * current.mag; }

            // Power factor = cos(θ)
            inline T power_factor(const Phasor &current) const noexcept { return std::cos(phase - current.phase); }

            // Comparison
            constexpr bool operator==(const Phasor &other) const noexcept {
                return mag == other.mag && phase == other.phase;
            }
            constexpr bool operator!=(const Phasor &other) const noexcept { return !(*this == other); }
        };

        // Binary operators
        template <typename T> constexpr Phasor<T> operator*(const Phasor<T> &a, const Phasor<T> &b) noexcept {
            return Phasor<T>{a.mag * b.mag, a.phase + b.phase};
        }

        template <typename T> constexpr Phasor<T> operator/(const Phasor<T> &a, const Phasor<T> &b) noexcept {
            return Phasor<T>{a.mag / b.mag, a.phase - b.phase};
        }

        template <typename T> inline Phasor<T> operator+(const Phasor<T> &a, const Phasor<T> &b) noexcept {
            T r = a.real() + b.real();
            T i = a.imag() + b.imag();
            return Phasor<T>{std::sqrt(r * r + i * i), std::atan2(i, r)};
        }

        template <typename T> inline Phasor<T> operator-(const Phasor<T> &a, const Phasor<T> &b) noexcept {
            T r = a.real() - b.real();
            T i = a.imag() - b.imag();
            return Phasor<T>{std::sqrt(r * r + i * i), std::atan2(i, r)};
        }

        // Scalar multiplication
        template <typename T> constexpr Phasor<T> operator*(const Phasor<T> &p, T s) noexcept {
            return Phasor<T>{p.mag * s, p.phase};
        }

        template <typename T> constexpr Phasor<T> operator*(T s, const Phasor<T> &p) noexcept {
            return Phasor<T>{s * p.mag, p.phase};
        }

        template <typename T> constexpr Phasor<T> operator/(const Phasor<T> &p, T s) noexcept {
            return Phasor<T>{p.mag / s, p.phase};
        }

        // Power of Phasor: p^n
        template <typename T> constexpr Phasor<T> pow(const Phasor<T> &p, T n) noexcept {
            return Phasor<T>{std::pow(p.mag, n), p.phase * n};
        }

        // Square root
        template <typename T> inline Phasor<T> sqrt(const Phasor<T> &p) noexcept {
            return Phasor<T>{std::sqrt(p.mag), p.phase / T{2}};
        }

        // Type traits
        template <typename T> struct is_phasor : std::false_type {};
        template <typename T> struct is_phasor<Phasor<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_phasor_v = is_phasor<T>::value;

        // Type aliases
        using phasorf = Phasor<float>;
        using phasord = Phasor<double>;

    } // namespace mat

    namespace phasor {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace phasor

} // namespace datapod
