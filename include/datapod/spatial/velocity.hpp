#pragma once

#include <cmath>
#include <tuple>

namespace datapod {

    /**
     * @brief 3D velocity with double precision components (POD)
     *
     * Represents linear velocity in 3D space (vx, vy, vz).
     * Pure aggregate struct with velocity-specific utility methods.
     * Use aggregate initialization: Velocity{1.5, 0.0, 0.3}
     * Fully serializable and reflectable.
     */
    struct Velocity {
        double vx = 0.0; // Velocity in x direction (m/s)
        double vy = 0.0; // Velocity in y direction (m/s)
        double vz = 0.0; // Velocity in z direction (m/s)

        auto members() noexcept { return std::tie(vx, vy, vz); }
        auto members() const noexcept { return std::tie(vx, vy, vz); }

        // Speed and magnitude
        inline double speed() const noexcept { return std::sqrt(vx * vx + vy * vy + vz * vz); }

        inline double speed_2d() const noexcept { return std::sqrt(vx * vx + vy * vy); }

        // Kinetic energy (requires mass externally: KE = 0.5 * m * v²)
        inline double speed_squared() const noexcept { return vx * vx + vy * vy + vz * vz; }

        // Utility
        inline bool is_set() const noexcept { return vx != 0.0 || vy != 0.0 || vz != 0.0; }

        // Operators - velocity arithmetic
        inline Velocity operator+(const Velocity &other) const noexcept {
            return Velocity{vx + other.vx, vy + other.vy, vz + other.vz};
        }

        inline Velocity operator-(const Velocity &other) const noexcept {
            return Velocity{vx - other.vx, vy - other.vy, vz - other.vz};
        }

        inline Velocity operator*(double scale) const noexcept { return Velocity{vx * scale, vy * scale, vz * scale}; }

        inline Velocity operator/(double scale) const noexcept { return Velocity{vx / scale, vy / scale, vz / scale}; }

        inline bool operator==(const Velocity &other) const noexcept {
            return vx == other.vx && vy == other.vy && vz == other.vz;
        }

        inline bool operator!=(const Velocity &other) const noexcept { return !(*this == other); }
    };

} // namespace datapod
