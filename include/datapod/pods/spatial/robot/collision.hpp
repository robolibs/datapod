#pragma once

#include <tuple>

#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/spatial/pose.hpp"
#include "geometry.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Collision - Collision geometry of a robot link (POD)
         *
         * Represents a collision element with origin transform and geometry.
         * Used in URDF/SDF-style robot definitions for collision detection.
         */
        struct Collision {
            String name; // Optional name for round-tripping
            Pose origin;
            Geometry geom;

            auto members() noexcept { return std::tie(name, origin, geom); }
            auto members() const noexcept { return std::tie(name, origin, geom); }

            inline bool is_set() const noexcept { return origin.is_set() || !name.empty(); }

            inline bool operator==(const Collision &other) const noexcept {
                return name == other.name && origin == other.origin && geom == other.geom;
            }
            inline bool operator!=(const Collision &other) const noexcept { return !(*this == other); }
        };

        namespace collision {
            inline Collision make(const Geometry &geom) noexcept { return Collision{String{}, pose::identity(), geom}; }
            inline Collision make(const Pose &origin, const Geometry &geom) noexcept {
                return Collision{String{}, origin, geom};
            }
            inline Collision make(const String &name, const Pose &origin, const Geometry &geom) noexcept {
                return Collision{name, origin, geom};
            }
        } // namespace collision

    } // namespace robot
} // namespace datapod
