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
         * A phasor represents a sinusoidal signal as a rotating vector in the complex plane.
         * Commonly used in electrical engineering for AC circuit analysis, and in signal
         * processing for representing amplitude and phase of harmonic signals.
         *
         * Stored in polar form (magnitude, phase) for intuitive manipulation.
         * Phase is in radians.
         * Fully serializable via members().
         *
         * Examples:
         *   phasor<double> v{120.0, 0.0};           // 120V at 0° phase
         *   phasor<double> i{10.0, -0.5236};        // 10A lagging by 30°
         *   auto z = v / i;                         // Impedance
         *   auto p = v.real_power(i);               // Real power
         */
        template <typename T> struct phasor {
            static_assert(std::is_floating_point_v<T>, "phasor<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T mag{};   // Magnitude (amplitude)
            T phase{}; // Phase angle in radians

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(mag, phase); }
            auto members() const noexcept { return std::tie(mag, phase); }

            // Construction
            constexpr phasor() noexcept = default;
            constexpr phasor(T magnitude) noexcept : mag(magnitude), phase{} {}
            constexpr phasor(T magnitude, T phase_rad) noexcept : mag(magnitude), phase(phase_rad) {}

            // From rectangular (real + imag)
            static inline phasor from_rectangular(T real, T imag) noexcept {
                return phasor{std::sqrt(real * real + imag * imag), std::atan2(imag, real)};
            }

            // From degrees
            static constexpr phasor from_degrees(T magnitude, T phase_deg) noexcept {
                constexpr T deg_to_rad = T{3.14159265358979323846} / T{180};
                return phasor{magnitude, phase_deg * deg_to_rad};
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
            inline phasor normalized_phase() const noexcept {
                constexpr T pi = T{3.14159265358979323846};
                constexpr T two_pi = T{2} * pi;
                T p = phase;
                while (p > pi)
                    p -= two_pi;
                while (p < -pi)
                    p += two_pi;
                return phasor{mag, p};
            }

            // Conjugate (negate phase)
            constexpr phasor conjugate() const noexcept { return phasor{mag, -phase}; }

            // Compound assignment
            constexpr phasor &operator*=(const phasor &other) noexcept {
                mag *= other.mag;
                phase += other.phase;
                return *this;
            }

            constexpr phasor &operator/=(const phasor &other) noexcept {
                mag /= other.mag;
                phase -= other.phase;
                return *this;
            }

            constexpr phasor &operator*=(T s) noexcept {
                mag *= s;
                return *this;
            }

            constexpr phasor &operator/=(T s) noexcept {
                mag /= s;
                return *this;
            }

            // Addition requires conversion to rectangular
            inline phasor &operator+=(const phasor &other) noexcept {
                T r = real() + other.real();
                T i = imag() + other.imag();
                mag = std::sqrt(r * r + i * i);
                phase = std::atan2(i, r);
                return *this;
            }

            inline phasor &operator-=(const phasor &other) noexcept {
                T r = real() - other.real();
                T i = imag() - other.imag();
                mag = std::sqrt(r * r + i * i);
                phase = std::atan2(i, r);
                return *this;
            }

            // Unary operators
            constexpr phasor operator-() const noexcept {
                constexpr T pi = T{3.14159265358979323846};
                return phasor{mag, phase + pi};
            }
            constexpr phasor operator+() const noexcept { return *this; }

            // Power calculations for AC circuits

            // Complex power S = V * I* (voltage phasor times current conjugate)
            inline phasor complex_power(const phasor &current) const noexcept {
                return phasor{mag * current.mag, phase - current.phase};
            }

            // Real power P = |V||I|cos(θ) where θ is phase difference
            inline T real_power(const phasor &current) const noexcept {
                return mag * current.mag * std::cos(phase - current.phase);
            }

            // Reactive power Q = |V||I|sin(θ)
            inline T reactive_power(const phasor &current) const noexcept {
                return mag * current.mag * std::sin(phase - current.phase);
            }

            // Apparent power |S| = |V||I|
            constexpr T apparent_power(const phasor &current) const noexcept { return mag * current.mag; }

            // Power factor = cos(θ)
            inline T power_factor(const phasor &current) const noexcept { return std::cos(phase - current.phase); }

            // Comparison
            constexpr bool operator==(const phasor &other) const noexcept {
                return mag == other.mag && phase == other.phase;
            }
            constexpr bool operator!=(const phasor &other) const noexcept { return !(*this == other); }
        };

        // Binary operators
        template <typename T> constexpr phasor<T> operator*(const phasor<T> &a, const phasor<T> &b) noexcept {
            return phasor<T>{a.mag * b.mag, a.phase + b.phase};
        }

        template <typename T> constexpr phasor<T> operator/(const phasor<T> &a, const phasor<T> &b) noexcept {
            return phasor<T>{a.mag / b.mag, a.phase - b.phase};
        }

        template <typename T> inline phasor<T> operator+(const phasor<T> &a, const phasor<T> &b) noexcept {
            T r = a.real() + b.real();
            T i = a.imag() + b.imag();
            return phasor<T>{std::sqrt(r * r + i * i), std::atan2(i, r)};
        }

        template <typename T> inline phasor<T> operator-(const phasor<T> &a, const phasor<T> &b) noexcept {
            T r = a.real() - b.real();
            T i = a.imag() - b.imag();
            return phasor<T>{std::sqrt(r * r + i * i), std::atan2(i, r)};
        }

        // Scalar multiplication
        template <typename T> constexpr phasor<T> operator*(const phasor<T> &p, T s) noexcept {
            return phasor<T>{p.mag * s, p.phase};
        }

        template <typename T> constexpr phasor<T> operator*(T s, const phasor<T> &p) noexcept {
            return phasor<T>{s * p.mag, p.phase};
        }

        template <typename T> constexpr phasor<T> operator/(const phasor<T> &p, T s) noexcept {
            return phasor<T>{p.mag / s, p.phase};
        }

        // Power of phasor: p^n
        template <typename T> constexpr phasor<T> pow(const phasor<T> &p, T n) noexcept {
            return phasor<T>{std::pow(p.mag, n), p.phase * n};
        }

        // Square root
        template <typename T> inline phasor<T> sqrt(const phasor<T> &p) noexcept {
            return phasor<T>{std::sqrt(p.mag), p.phase / T{2}};
        }

        // Type traits
        template <typename T> struct is_phasor : std::false_type {};
        template <typename T> struct is_phasor<phasor<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_phasor_v = is_phasor<T>::value;

        // Type aliases
        using phasorf = phasor<float>;
        using phasord = phasor<double>;

    } // namespace mat
} // namespace datapod
