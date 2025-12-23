#include <datapod/spatial/pose.hpp>
#include <datapod/spatial/robot/odom.hpp>
#include <datapod/spatial/robot/twist.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Odom Usage Example ===" << std::endl;

    // Create odometry data for a mobile robot
    Odom odom1{
        Pose{Point{5.0, 3.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, // Position at (5,3,0), no rotation
        Twist{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.2}}     // Moving forward at 0.5 m/s, turning at 0.2 rad/s
    };

    std::cout << "Robot Odometry:" << std::endl;
    std::cout << "  Position: (" << odom1.pose.point.x << ", " << odom1.pose.point.y << ", " << odom1.pose.point.z
              << ")" << std::endl;
    std::cout << "  Linear velocity: " << odom1.twist.linear.vx << " m/s" << std::endl;
    std::cout << "  Angular velocity: " << odom1.twist.angular.vz << " rad/s" << std::endl;

    // Create another odometry reading
    Odom odom2{
        Pose{Point{10.0, 5.0, 0.0}, Quaternion{0.9239, 0.0, 0.0, 0.3827}}, // ~45 degree rotation
        Twist{Velocity{1.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.0}}            // Moving faster, no turning
    };

    std::cout << "\nUpdated Odometry:" << std::endl;
    std::cout << "  Position: (" << odom2.pose.point.x << ", " << odom2.pose.point.y << ", " << odom2.pose.point.z
              << ")" << std::endl;
    std::cout << "  Orientation (qw): " << odom2.pose.rotation.w << std::endl;
    std::cout << "  Linear velocity: " << odom2.twist.linear.vx << " m/s" << std::endl;

    // Check if odometry is set
    Odom empty_odom;
    std::cout << "\nEmpty odom is_set: " << (empty_odom.is_set() ? "true" : "false") << std::endl;
    std::cout << "Odom1 is_set: " << (odom1.is_set() ? "true" : "false") << std::endl;

    // Comparison
    std::cout << "\nOdom1 == Odom2: " << (odom1 == odom2 ? "true" : "false") << std::endl;
    std::cout << "Odom1 == Odom1: " << (odom1 == odom1 ? "true" : "false") << std::endl;

    // Use case: Dead reckoning simulation
    std::cout << "\n=== Dead Reckoning Simulation ===" << std::endl;
    Odom robot_state{Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}},
                     Twist{Velocity{0.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.0}}};

    const double dt = 0.1; // 100ms timestep
    robot_state.twist.linear.vx = 2.0;
    robot_state.twist.angular.vz = 0.5;

    std::cout << "Initial state: x=" << robot_state.pose.point.x << ", y=" << robot_state.pose.point.y << std::endl;

    // Simple integration (for demonstration)
    for (int i = 0; i < 10; ++i) {
        robot_state.pose.point.x += robot_state.twist.linear.vx * dt;
        robot_state.pose.point.y += robot_state.twist.linear.vy * dt;
    }

    std::cout << "After 1 second: x=" << robot_state.pose.point.x << ", y=" << robot_state.pose.point.y << std::endl;

    return 0;
}
