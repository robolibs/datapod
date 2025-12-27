#pragma once

#include <cmath>
#include <tuple>

namespace datapod {

    /**
     * @brief Utm - Universal Transverse Mercator coordinate (POD)
     *
     * Represents a position in the UTM coordinate system, which divides Earth
     * into 60 zones for accurate local mapping with minimal distortion.
     *
     * Pure aggregate struct for UTM coordinate data.
     * Use aggregate initialization: Utm{32, 'U', 500000.0, 5500000.0, 100.0}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - zone: UTM zone number [1-60]
     * - band: UTM latitude band letter [C-X, excluding I and O]
     * - easting: Easting in meters from zone's central meridian (+ 500000m false easting)
     * - northing: Northing in meters from equator (+ 10000000m false northing in southern hemisphere)
     * - altitude: Altitude in meters above WGS84 ellipsoid (optional, NaN if unavailable)
     *
     * Valid ranges:
     * - Zone: [1, 60]
     * - Band: C-X (excluding I, O)
     * - Easting: typically [166000, 834000] meters
     * - Northing: [0, 10000000] meters (with false northing applied)
     *
     * Use cases:
     * - Large-scale mapping and surveying
     * - Military grid reference system (MGRS) conversion
     * - Drone/vehicle navigation requiring planar coordinates
     * - GIS applications with metric coordinate system
     * - Construction and engineering projects
     *
     * Reference: WGS84 ellipsoid with Transverse Mercator projection
     */
    struct Utm {
        int zone = 0;          // UTM zone number [1-60]
        char band = 'N';       // UTM latitude band letter [C-X]
        double easting = 0.0;  // Easting [m] from zone central meridian
        double northing = 0.0; // Northing [m] from equator
        double altitude = 0.0; // Altitude [m] above WGS84 ellipsoid

        auto members() noexcept { return std::tie(zone, band, easting, northing, altitude); }
        auto members() const noexcept { return std::tie(zone, band, easting, northing, altitude); }

        // Utility
        inline bool is_set() const noexcept { return zone != 0 || easting != 0.0 || northing != 0.0; }

        // Check if altitude is available (not NaN)
        inline bool has_altitude() const noexcept { return !std::isnan(altitude); }

        // Check if zone number is valid
        inline bool is_valid_zone() const noexcept { return zone >= 1 && zone <= 60; }

        // Check if band letter is valid (C-X excluding I and O)
        inline bool is_valid_band() const noexcept {
            return (band >= 'C' && band <= 'X') && band != 'I' && band != 'O';
        }

        // Check if in northern hemisphere (band >= 'N')
        inline bool is_northern() const noexcept { return band >= 'N'; }

        // Check if coordinates are within typical UTM ranges
        inline bool is_valid() const noexcept {
            return is_valid_zone() && is_valid_band() && easting >= 100000.0 && easting <= 900000.0 &&
                   northing >= 0.0 && northing <= 10000000.0;
        }

        // Calculate 2D distance to another UTM point (meters)
        // Note: Only valid if both points are in the same zone
        inline double distance_to(const Utm &other) const noexcept {
            const double de = easting - other.easting;
            const double dn = northing - other.northing;
            return std::sqrt(de * de + dn * dn);
        }

        // Calculate 3D distance to another UTM point (meters)
        // Note: Only valid if both points are in the same zone
        inline double distance_to_3d(const Utm &other) const noexcept {
            const double de = easting - other.easting;
            const double dn = northing - other.northing;
            const double da = altitude - other.altitude;
            return std::sqrt(de * de + dn * dn + da * da);
        }

        // Check if two UTM points are in the same zone
        inline bool same_zone(const Utm &other) const noexcept { return zone == other.zone && band == other.band; }

        // Get central meridian longitude for this zone (degrees)
        inline double central_meridian() const noexcept { return (zone - 1) * 6.0 - 180.0 + 3.0; }

        // Comparison
        inline bool operator==(const Utm &other) const noexcept {
            return zone == other.zone && band == other.band && easting == other.easting && northing == other.northing &&
                   altitude == other.altitude;
        }

        inline bool operator!=(const Utm &other) const noexcept { return !(*this == other); }
    };

} // namespace datapod
