#pragma once

#include <cmath>
#include <tuple>

namespace datapod {

    /**
     * @brief Nav - GPS Navigation coordinates (POD)
     *
     * Represents a global position using WGS84 geodetic coordinates.
     * Simplified version of ROS sensor_msgs/NavSatFix containing only position data.
     *
     * Pure aggregate struct for GPS/GNSS position data.
     * Use aggregate initialization: Nav{lat, lon, alt}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - latitude: Latitude in decimal degrees (positive = north of equator, negative = south)
     * - longitude: Longitude in decimal degrees (positive = east of prime meridian, negative = west)
     * - altitude: Altitude in meters above WGS84 ellipsoid (NaN if unavailable)
     *
     * Valid ranges:
     * - Latitude: [-90.0, 90.0] degrees
     * - Longitude: [-180.0, 180.0] degrees
     * - Altitude: any value or NaN
     *
     * Use cases:
     * - Outdoor mobile robot localization
     * - Drone navigation and waypoints
     * - Agricultural/construction robots
     * - Fleet management and tracking
     * - Geographic coordinate storage
     *
     * Reference: WGS84 (World Geodetic System 1984) coordinate system
     */
    struct Geo {
        double latitude = 0.0;  // Latitude [degrees] (+north, -south)
        double longitude = 0.0; // Longitude [degrees] (+east, -west)
        double altitude = 0.0;  // Altitude [m] above WGS84 ellipsoid

        auto members() noexcept { return std::tie(latitude, longitude, altitude); }
        auto members() const noexcept { return std::tie(latitude, longitude, altitude); }

        // Utility
        inline bool is_set() const noexcept { return latitude != 0.0 || longitude != 0.0 || altitude != 0.0; }

        // Check if altitude is available (not NaN)
        inline bool has_altitude() const noexcept { return !std::isnan(altitude); }

        // Check if coordinates are within valid ranges
        inline bool is_valid() const noexcept {
            return latitude >= -90.0 && latitude <= 90.0 && longitude >= -180.0 && longitude <= 180.0;
        }

        // Calculate approximate 2D distance to another Geo point (meters)
        // Uses Haversine formula for great circle distance
        inline double distance_to(const Geo &other) const noexcept {
            constexpr double EARTH_RADIUS = 6371000.0; // meters
            const double lat1_rad = latitude * M_PI / 180.0;
            const double lat2_rad = other.latitude * M_PI / 180.0;
            const double dlat = (other.latitude - latitude) * M_PI / 180.0;
            const double dlon = (other.longitude - longitude) * M_PI / 180.0;

            const double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
                             std::cos(lat1_rad) * std::cos(lat2_rad) * std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
            const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

            return EARTH_RADIUS * c;
        }

        // Calculate approximate bearing to another Geo point (radians)
        // Returns bearing in range [0, 2π) where 0 = north, π/2 = east
        inline double bearing_to(const Geo &other) const noexcept {
            const double lat1_rad = latitude * M_PI / 180.0;
            const double lat2_rad = other.latitude * M_PI / 180.0;
            const double dlon = (other.longitude - longitude) * M_PI / 180.0;

            const double y = std::sin(dlon) * std::cos(lat2_rad);
            const double x =
                std::cos(lat1_rad) * std::sin(lat2_rad) - std::sin(lat1_rad) * std::cos(lat2_rad) * std::cos(dlon);

            double bearing = std::atan2(y, x);
            if (bearing < 0.0) {
                bearing += 2.0 * M_PI;
            }
            return bearing;
        }

        // Comparison
        inline bool operator==(const Geo &other) const noexcept {
            return latitude == other.latitude && longitude == other.longitude && altitude == other.altitude;
        }

        inline bool operator!=(const Geo &other) const noexcept { return !(*this == other); }
    };

} // namespace datapod
