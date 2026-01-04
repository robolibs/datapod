#include <cmath>
#include <datapod/pods/spatial/geo.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Geo (GPS) Usage Example ===" << std::endl;

    // Create GPS coordinates for various locations
    // San Francisco, CA
    Geo sf{37.7749, -122.4194, 16.0};

    std::cout << "San Francisco:" << std::endl;
    std::cout << "  Latitude: " << sf.latitude << "°" << std::endl;
    std::cout << "  Longitude: " << sf.longitude << "°" << std::endl;
    std::cout << "  Altitude: " << sf.altitude << " m" << std::endl;
    std::cout << "  Valid: " << (sf.is_valid() ? "yes" : "no") << std::endl;

    // New York, NY
    Geo ny{40.7128, -74.0060, 10.0};

    std::cout << "\nNew York:" << std::endl;
    std::cout << "  Latitude: " << ny.latitude << "°" << std::endl;
    std::cout << "  Longitude: " << ny.longitude << "°" << std::endl;

    // Calculate distance between SF and NY
    double distance = sf.distance_to(ny);
    std::cout << "\nDistance from SF to NY: " << distance / 1000.0 << " km" << std::endl;

    // Calculate bearing from SF to NY
    double bearing_rad = sf.bearing_to(ny);
    double bearing_deg = bearing_rad * 180.0 / M_PI;
    std::cout << "Bearing from SF to NY: " << bearing_deg << "° (0° = North)" << std::endl;

    // Robot waypoint navigation example
    std::cout << "\n=== Robot Waypoint Geoigation ===" << std::endl;

    Geo waypoint1{37.4219, -122.0840, 0.0}; // Stanford University
    Geo waypoint2{37.4275, -122.1697, 0.0}; // Mountain View
    Geo waypoint3{37.3861, -122.0839, 0.0}; // Palo Alto

    std::cout << "Waypoint 1 (Stanford): " << waypoint1.latitude << ", " << waypoint1.longitude << std::endl;
    std::cout << "Waypoint 2 (Mountain View): " << waypoint2.latitude << ", " << waypoint2.longitude << std::endl;
    std::cout << "Waypoint 3 (Palo Alto): " << waypoint3.latitude << ", " << waypoint3.longitude << std::endl;

    double leg1_distance = waypoint1.distance_to(waypoint2);
    double leg2_distance = waypoint2.distance_to(waypoint3);
    double total_distance = leg1_distance + leg2_distance;

    std::cout << "\nRoute distances:" << std::endl;
    std::cout << "  Leg 1: " << leg1_distance << " m" << std::endl;
    std::cout << "  Leg 2: " << leg2_distance << " m" << std::endl;
    std::cout << "  Total: " << total_distance << " m" << std::endl;

    // Test with altitude
    Geo drone_pos1{37.4219, -122.0840, 100.0}; // 100m altitude
    Geo drone_pos2{37.4219, -122.0840, 50.0};  // 50m altitude (same lat/lon)

    std::cout << "\nDrone positions:" << std::endl;
    std::cout << "  Position 1 altitude: " << drone_pos1.altitude << " m" << std::endl;
    std::cout << "  Position 2 altitude: " << drone_pos2.altitude << " m" << std::endl;
    std::cout << "  Has altitude: " << (drone_pos1.has_altitude() ? "yes" : "no") << std::endl;

    // Test with NaN altitude (no altitude fix)
    Geo no_alt_fix{37.4219, -122.0840, NAN};
    std::cout << "\nNo altitude fix:" << std::endl;
    std::cout << "  Has altitude: " << (no_alt_fix.has_altitude() ? "yes" : "no") << std::endl;

    // Check validity
    Geo invalid{91.0, 200.0, 0.0}; // Invalid coordinates
    std::cout << "\nInvalid coordinates (91°, 200°):" << std::endl;
    std::cout << "  Valid: " << (invalid.is_valid() ? "yes" : "no") << std::endl;

    // Comparison
    Geo same_as_sf{37.7749, -122.4194, 16.0};
    std::cout << "\nSF == same_as_sf: " << (sf == same_as_sf ? "true" : "false") << std::endl;
    std::cout << "SF == NY: " << (sf == ny ? "true" : "false") << std::endl;

    return 0;
}
