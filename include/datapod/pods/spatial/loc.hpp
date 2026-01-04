#pragma once

#include <cmath>
#include <tuple>

#include "datapod/pods/spatial/geo.hpp"
#include "datapod/pods/spatial/point.hpp"

namespace datapod {

    /**
     * @brief Loc - Local coordinate with WGS84 reference (POD)
     *
     * Represents a local position (ENU - East-North-Up or similar) relative to
     * a WGS84 reference point. Used when converting global GPS coordinates to
     * a local Cartesian coordinate system for navigation, path planning, etc.
     *
     * Pure aggregate struct for local coordinate data with its reference origin.
     * Use aggregate initialization: Loc{Point{x, y, z}, Geo{lat, lon, alt}}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - local: Local Cartesian coordinates (typically ENU: x=East, y=North, z=Up)
     * - origin: WGS84 reference point that defines the local frame origin
     *
     * Use cases:
     * - Robot navigation in local coordinates with GPS reference
     * - Converting between WGS84 and local ENU frames
     * - Path planning with global reference tracking
     * - Multi-robot coordination with common reference
     * - Storing waypoints relative to a base station
     *
     * Reference frames:
     * - ENU (East-North-Up): Common in robotics
     * - NED (North-East-Down): Common in aviation
     */
    struct Loc {
        Point local; // Local Cartesian coordinates [m] (e.g., ENU frame)
        Geo origin;  // WGS84 reference point defining local frame origin

        auto members() noexcept { return std::tie(local, origin); }
        auto members() const noexcept { return std::tie(local, origin); }

        // Utility
        inline bool is_set() const noexcept { return local.is_set() || origin.is_set(); }

        // Check if the reference origin is valid
        inline bool has_valid_origin() const noexcept { return origin.is_valid(); }

        // Get distance from local origin (magnitude of local coordinates)
        inline double distance_from_origin() const noexcept { return local.magnitude(); }

        // Get 2D distance from local origin (ignoring z)
        inline double distance_from_origin_2d() const noexcept {
            return std::sqrt(local.x * local.x + local.y * local.y);
        }

        // Calculate distance to another Loc point (in local coordinates)
        // Note: Only valid if both have the same origin
        inline double distance_to(const Loc &other) const noexcept { return local.distance_to(other.local); }

        // Calculate 2D distance to another Loc point
        inline double distance_to_2d(const Loc &other) const noexcept { return local.distance_to_2d(other.local); }

        // Check if two Loc points share the same origin (approximately)
        inline bool same_origin(const Loc &other, double tolerance = 1e-9) const noexcept {
            return std::abs(origin.latitude - other.origin.latitude) < tolerance &&
                   std::abs(origin.longitude - other.origin.longitude) < tolerance &&
                   std::abs(origin.altitude - other.origin.altitude) < tolerance;
        }

        // Comparison
        inline bool operator==(const Loc &other) const noexcept {
            return local == other.local && origin == other.origin;
        }

        inline bool operator!=(const Loc &other) const noexcept { return !(*this == other); }

        // Arithmetic on local coordinates (preserves origin)
        inline Loc operator+(const Point &offset) const noexcept { return Loc{local + offset, origin}; }

        inline Loc operator-(const Point &offset) const noexcept { return Loc{local - offset, origin}; }
    };

} // namespace datapod
