#include <iostream>
#include "datapod/datapod.hpp"

using namespace datapod;

// Test message types
struct RobotPose {
    f64 x, y, z;
};

struct RobotHeading {
    u32 degrees;
};

struct SensorReading {
    u8 sensor_id;
    f32 value;
};

int main() {
    std::cout << "=== Match API Test ===\n\n";

    bool pose_handled = false;
    bool heading_handled = false;
    bool sensor_handled = false;

    // Test 1: Match RobotPose
    std::cout << "Test 1: Send RobotPose, match it\n";
    {
        RobotPose pose = {1.0, 2.0, 3.0};
        auto buf = serialize<Mode::WITH_VERSION>(pose);

        pose_handled = false;
        Result<void, Error> result = match(buf)
            .on<RobotPose>([&](RobotPose const& p) {
                std::cout << "  Matched RobotPose: [" << p.x << ", " << p.y << ", " << p.z << "]\n";
                pose_handled = true;
            })
            .on<RobotHeading>([&](RobotHeading const& h) {
                std::cout << "  Matched RobotHeading: " << h.degrees << "\n";
                heading_handled = true;
            });

        std::cout << "  Result: " << (result ? "OK" : "ERROR") << "\n";
        std::cout << "  pose_handled: " << (pose_handled ? "yes" : "no") << "\n";
    }

    // Test 2: Match RobotHeading
    std::cout << "\nTest 2: Send RobotHeading, match it\n";
    {
        RobotHeading heading = {180};
        auto buf = serialize<Mode::WITH_VERSION>(heading);

        heading_handled = false;
        Result<void, Error> result = match(buf)
            .on<RobotPose>([&](RobotPose const& p) {
                pose_handled = true;
            })
            .on<RobotHeading>([&](RobotHeading const& h) {
                std::cout << "  Matched RobotHeading: " << h.degrees << " degrees\n";
                heading_handled = true;
            });

        std::cout << "  Result: " << (result ? "OK" : "ERROR") << "\n";
        std::cout << "  heading_handled: " << (heading_handled ? "yes" : "no") << "\n";
    }

    // Test 3: No match - should return Error
    std::cout << "\nTest 3: Send SensorReading, no handler for it\n";
    {
        SensorReading sensor = {42, 3.14f};
        auto buf = serialize<Mode::WITH_VERSION>(sensor);

        Result<void, Error> result = match(buf)
            .on<RobotPose>([](RobotPose const&) {
                std::cout << "  Matched RobotPose (unexpected!)\n";
            })
            .on<RobotHeading>([](RobotHeading const&) {
                std::cout << "  Matched RobotHeading (unexpected!)\n";
            });

        std::cout << "  Result: " << (result ? "OK (unexpected!)" : "ERROR (expected)") << "\n";
        if (!result) {
            std::cout << "  Error message: " << result.error().message.c_str() << "\n";
        }
    }

    // Test 4: Three handlers, match the third
    std::cout << "\nTest 4: Three handlers, match the third\n";
    {
        SensorReading sensor = {7, 98.6f};
        auto buf = serialize<Mode::WITH_VERSION>(sensor);

        sensor_handled = false;
        Result<void, Error> result = match(buf)
            .on<RobotPose>([](RobotPose const&) { })
            .on<RobotHeading>([](RobotHeading const&) { })
            .on<SensorReading>([&](SensorReading const& s) {
                std::cout << "  Matched SensorReading: id=" << (int)s.sensor_id << ", value=" << s.value << "\n";
                sensor_handled = true;
            });

        std::cout << "  Result: " << (result ? "OK" : "ERROR") << "\n";
        std::cout << "  sensor_handled: " << (sensor_handled ? "yes" : "no") << "\n";
    }

    std::cout << "\n=== All tests completed! ===\n";
    return 0;
}
