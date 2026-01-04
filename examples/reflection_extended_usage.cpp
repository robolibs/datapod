/**
 * @file reflection_extended_usage.cpp
 * @brief Demonstrates extended automatic reflection for structs with >10 fields
 *
 * Datapod now supports automatic reflection for structs with up to 64 fields!
 * No need to manually write members() functions anymore.
 */

#include <datapod/reflection/arity.hpp>
#include <datapod/reflection/for_each_field.hpp>
#include <cstdint>
#include <datapod/reflection/to_tuple.hpp>

#include <iostream>
#include <iomanip>

using namespace datapod;

// ============================================================================
// Example 1: Large struct with 20 fields - NO members() needed!
// ============================================================================

struct SensorData {
    // Temperature sensors (5)
    double temp1, temp2, temp3, temp4, temp5;
    
    // Pressure sensors (5)
    double pressure1, pressure2, pressure3, pressure4, pressure5;
    
    // Humidity sensors (5)
    double humidity1, humidity2, humidity3, humidity4, humidity5;
    
    // Metadata (5)
    int64_t timestamp;
    int sensor_id;
    int status;
    float battery_voltage;
    int signal_strength;
};

void example_sensor_data() {
    std::cout << "=== Example 1: Automatic Reflection for 20-field Struct ===\n";
    
    SensorData data{
        // Temps
        23.5, 24.1, 22.8, 25.0, 23.2,
        // Pressures
        1013.25, 1012.5, 1014.0, 1013.0, 1012.8,
        // Humidity
        65.5, 64.2, 66.1, 65.0, 64.8,
        // Metadata
        1234567890, 42, 1, 3.7f, 85
    };
    
    // Automatic arity detection
    std::cout << "Struct has " << arity_v<SensorData> << " fields\n";
    
    // Iterate over all fields automatically
    std::cout << "\nAll sensor values:\n";
    int field_num = 0;
    for_each_field(data, [&field_num](auto& field) {
        std::cout << "  Field " << std::setw(2) << field_num++ << ": " << field << "\n";
    });
    
    // Access via tuple
    auto tuple = to_tuple(data);
    std::cout << "\nFirst temperature: " << std::get<0>(tuple) << "°C\n";
    std::cout << "Timestamp: " << std::get<15>(tuple) << "\n";
    
    std::cout << "\n";
}

// ============================================================================
// Example 2: Even larger struct with 30+ fields
// ============================================================================

struct RobotState {
    // Joint positions (12)
    double joint1, joint2, joint3, joint4, joint5, joint6;
    double joint7, joint8, joint9, joint10, joint11, joint12;
    
    // Joint velocities (12)
    double vel1, vel2, vel3, vel4, vel5, vel6;
    double vel7, vel8, vel9, vel10, vel11, vel12;
    
    // Joint torques (12)
    double torque1, torque2, torque3, torque4, torque5, torque6;
    double torque7, torque8, torque9, torque10, torque11, torque12;
    
    // Metadata
    int64_t timestamp;
    int robot_id;
    int mode;
};

void example_robot_state() {
    std::cout << "=== Example 2: 39-field Robot State (No members() needed!) ===\n";
    
    RobotState state{};
    
    // Initialize all fields to specific values
    for_each_field_indexed(state, [](auto& field, auto index) {
        field = index.value * 0.1;
    });
    
    std::cout << "Struct has " << arity_v<RobotState> << " fields\n";
    
    // Calculate statistics
    double sum = 0.0;
    int count = 0;
    for_each_field(state, [&](auto& field) {
        sum += field;
        count++;
    });
    
    std::cout << "Average field value: " << (sum / count) << "\n";
    
    // Access specific joints
    auto tuple = to_tuple(state);
    std::cout << "Joint 1 position: " << std::get<0>(tuple) << "\n";
    std::cout << "Joint 12 position: " << std::get<11>(tuple) << "\n";
    std::cout << "Joint 1 velocity: " << std::get<12>(tuple) << "\n";
    
    std::cout << "\n";
}

// ============================================================================
// Example 3: Comparison - Old way vs New way
// ============================================================================

// OLD WAY: Manual members() function (still works!)
struct OldStyleStruct {
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o;
    
    // Had to write this manually before
    auto members() { return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); }
    auto members() const { return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); }
};

// NEW WAY: Just define the struct - reflection is automatic!
struct NewStyleStruct {
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o;
    // That's it! No members() needed!
};

void example_comparison() {
    std::cout << "=== Example 3: Old Way vs New Way ===\n";
    
    OldStyleStruct old_s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    NewStyleStruct new_s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    std::cout << "Old style (manual members()): " << arity_v<OldStyleStruct> << " fields\n";
    std::cout << "New style (automatic):        " << arity_v<NewStyleStruct> << " fields\n";
    
    // Both work the same way!
    auto old_tuple = to_tuple(old_s);
    auto new_tuple = to_tuple(new_s);
    
    std::cout << "\nBoth produce identical results:\n";
    std::cout << "  Old: " << std::get<0>(old_tuple) << ", " << std::get<14>(old_tuple) << "\n";
    std::cout << "  New: " << std::get<0>(new_tuple) << ", " << std::get<14>(new_tuple) << "\n";
    
    std::cout << "\n";
}

// ============================================================================
// Example 4: Serialization-ready structs
// ============================================================================

struct NetworkPacket {
    // Header (10 fields)
    uint32_t magic;
    uint32_t version;
    uint32_t packet_id;
    uint32_t sequence;
    uint32_t timestamp_hi;
    uint32_t timestamp_lo;
    uint16_t payload_size;
    uint16_t checksum;
    uint8_t flags;
    uint8_t reserved;
    
    // Payload metadata (5 fields)
    uint32_t source_addr;
    uint32_t dest_addr;
    uint16_t source_port;
    uint16_t dest_port;
    uint8_t protocol;
    
    // Stats (5 fields)
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t errors;
};

void example_serialization() {
    std::cout << "=== Example 4: Serialization-Ready Struct (20 fields) ===\n";
    
    NetworkPacket packet{
        0xDEADBEEF, 1, 12345, 1, 0, 1234567890,
        1024, 0xABCD, 0x01, 0,
        0xC0A80001, 0xC0A80002, 8080, 80, 6,
        1024000, 512000, 1000, 500, 0
    };
    
    std::cout << "Packet has " << arity_v<NetworkPacket> << " fields\n";
    
    // Calculate checksum over all fields
    uint32_t checksum = 0;
    for_each_field(packet, [&checksum](auto& field) {
        checksum ^= static_cast<uint32_t>(field);
    });
    
    std::cout << "Calculated checksum: 0x" << std::hex << checksum << std::dec << "\n";
    
    // Access specific fields
    auto tuple = to_tuple(packet);
    std::cout << "Magic: 0x" << std::hex << std::get<0>(tuple) << std::dec << "\n";
    std::cout << "Payload size: " << std::get<6>(tuple) << " bytes\n";
    
    std::cout << "\n";
}

// ============================================================================
// Example 5: Maximum supported size (64 fields)
// ============================================================================

struct MaxSizeStruct {
    int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10;
    int f11, f12, f13, f14, f15, f16, f17, f18, f19, f20;
    int f21, f22, f23, f24, f25, f26, f27, f28, f29, f30;
    int f31, f32, f33, f34, f35, f36, f37, f38, f39, f40;
    int f41, f42, f43, f44, f45, f46, f47, f48, f49, f50;
    int f51, f52, f53, f54, f55, f56, f57, f58, f59, f60;
    int f61, f62, f63, f64;
};

void example_max_size() {
    std::cout << "=== Example 5: Maximum Supported Size (64 fields) ===\n";
    
    MaxSizeStruct large{};
    
    // Initialize all fields
    for_each_field_indexed(large, [](auto& field, auto index) {
        field = index.value + 1;
    });
    
    std::cout << "Struct has " << arity_v<MaxSizeStruct> << " fields\n";
    
    // Verify all fields
    auto tuple = to_tuple(large);
    std::cout << "First field: " << std::get<0>(tuple) << "\n";
    std::cout << "Last field: " << std::get<63>(tuple) << "\n";
    
    // Calculate sum
    int sum = 0;
    for_each_field(large, [&sum](auto& field) {
        sum += field;
    });
    std::cout << "Sum of all fields: " << sum << " (expected: " << (64 * 65 / 2) << ")\n";
    
    std::cout << "\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Extended Automatic Reflection - datapod library         ║\n";
    std::cout << "║   Now supports structs with up to 64 fields!              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    example_sensor_data();
    example_robot_state();
    example_comparison();
    example_serialization();
    example_max_size();
    
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Key Benefits:                                             ║\n";
    std::cout << "║  ✓ No manual members() functions needed                    ║\n";
    std::cout << "║  ✓ Supports up to 64 fields automatically                  ║\n";
    std::cout << "║  ✓ Works with serialization, iteration, tuple conversion  ║\n";
    std::cout << "║  ✓ Backwards compatible with manual members()             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    return 0;
}
