#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

// Realistic message structures (flatsim-like)

struct Vector3 {
    double x;
    double y;
    double z;
};

struct TestPose {
    Vector3 position;
    Vector3 orientation;
};

struct WheelState {
    double angle;
    double angular_velocity;
    double torque;
};

struct VehicleState {
    TestPose pose;
    Vector3 velocity;
    Array<WheelState, 4> wheels;
    double timestamp;
};

struct SensorData {
    String sensor_id;
    Vector<double> readings;
    double timestamp;
    Optional<String> error_message;
};

struct ControlCommand {
    int command_id;
    String command_type;
    Map<String, double> parameters;
    Optional<double> timeout;
};

struct SimulationMessage {
    int message_id;
    String message_type;
    VehicleState vehicle;
    Vector<SensorData> sensors;
    Optional<ControlCommand> command;
    double timestamp;
};

// Integration tests

TEST_CASE("integration - realistic vehicle state") {
    VehicleState state;
    state.pose.position = {1.0, 2.0, 3.0};
    state.pose.orientation = {0.0, 0.0, 1.57};
    state.velocity = {10.0, 0.0, 0.0};
    state.wheels[0] = {0.1, 2.0, 100.0};
    state.wheels[1] = {0.1, 2.0, 100.0};
    state.wheels[2] = {0.1, 2.0, 100.0};
    state.wheels[3] = {0.1, 2.0, 100.0};
    state.timestamp = 123.456;

    auto buf = serialize(state);
    auto result = deserialize<Mode::NONE, VehicleState>(buf);

    CHECK(result.pose.position.x == doctest::Approx(1.0));
    CHECK(result.pose.position.y == doctest::Approx(2.0));
    CHECK(result.pose.position.z == doctest::Approx(3.0));
    CHECK(result.velocity.x == doctest::Approx(10.0));
    CHECK(result.wheels[0].angle == doctest::Approx(0.1));
    CHECK(result.wheels[0].torque == doctest::Approx(100.0));
    CHECK(result.timestamp == doctest::Approx(123.456));
}

TEST_CASE("integration - sensor data with optionals") {
    SensorData sensor;
    sensor.sensor_id = String("IMU_01");
    sensor.readings.push_back(1.5);
    sensor.readings.push_back(2.5);
    sensor.readings.push_back(3.5);
    sensor.timestamp = 100.0;
    sensor.error_message = String("calibration needed");

    auto buf = serialize(sensor);
    auto result = deserialize<Mode::NONE, SensorData>(buf);

    CHECK(result.sensor_id == "IMU_01");
    REQUIRE(result.readings.size() == 3);
    CHECK(result.readings[0] == doctest::Approx(1.5));
    CHECK(result.readings[1] == doctest::Approx(2.5));
    CHECK(result.readings[2] == doctest::Approx(3.5));
    CHECK(result.timestamp == doctest::Approx(100.0));
    REQUIRE(result.error_message.has_value());
    CHECK(*result.error_message == "calibration needed");
}

TEST_CASE("integration - control command with hashmap") {
    ControlCommand cmd;
    cmd.command_id = 42;
    cmd.command_type = String("set_velocity");
    cmd.parameters.insert({String("speed"), 50.0});
    cmd.parameters.insert({String("direction"), 90.0});
    cmd.timeout = 5.0;

    auto buf = serialize(cmd);
    auto result = deserialize<Mode::NONE, ControlCommand>(buf);

    CHECK(result.command_id == 42);
    CHECK(result.command_type == "set_velocity");
    REQUIRE(result.parameters.size() == 2);
    CHECK(result.parameters[String("speed")] == doctest::Approx(50.0));
    CHECK(result.parameters[String("direction")] == doctest::Approx(90.0));
    REQUIRE(result.timeout.has_value());
    CHECK(*result.timeout == doctest::Approx(5.0));
}

TEST_CASE("integration - full simulation message") {
    SimulationMessage msg;
    msg.message_id = 1001;
    msg.message_type = String("state_update");

    // Vehicle state
    msg.vehicle.pose.position = {10.0, 20.0, 0.0};
    msg.vehicle.pose.orientation = {0.0, 0.0, 0.0};
    msg.vehicle.velocity = {5.0, 0.0, 0.0};
    msg.vehicle.wheels[0] = {0.2, 3.0, 150.0};
    msg.vehicle.wheels[1] = {0.2, 3.0, 150.0};
    msg.vehicle.wheels[2] = {0.2, 3.0, 150.0};
    msg.vehicle.wheels[3] = {0.2, 3.0, 150.0};
    msg.vehicle.timestamp = 200.0;

    // Sensors
    SensorData sensor1;
    sensor1.sensor_id = String("GPS");
    sensor1.readings.push_back(10.0);
    sensor1.readings.push_back(20.0);
    sensor1.timestamp = 200.0;
    msg.sensors.push_back(sensor1);

    SensorData sensor2;
    sensor2.sensor_id = String("LIDAR");
    sensor2.readings.push_back(1.0);
    sensor2.readings.push_back(2.0);
    sensor2.readings.push_back(3.0);
    sensor2.timestamp = 200.0;
    msg.sensors.push_back(sensor2);

    // Command
    ControlCommand cmd;
    cmd.command_id = 99;
    cmd.command_type = String("brake");
    cmd.parameters.insert({String("force"), 80.0});
    msg.command = cmd;

    msg.timestamp = 200.0;

    auto buf = serialize(msg);
    auto result = deserialize<Mode::NONE, SimulationMessage>(buf);

    CHECK(result.message_id == 1001);
    CHECK(result.message_type == "state_update");
    CHECK(result.vehicle.pose.position.x == doctest::Approx(10.0));
    CHECK(result.vehicle.velocity.x == doctest::Approx(5.0));
    REQUIRE(result.sensors.size() == 2);
    CHECK(result.sensors[0].sensor_id == "GPS");
    CHECK(result.sensors[1].sensor_id == "LIDAR");
    CHECK(result.sensors[1].readings.size() == 3);
    REQUIRE(result.command.has_value());
    CHECK(result.command->command_id == 99);
    CHECK(result.command->command_type == "brake");
    CHECK(result.timestamp == doctest::Approx(200.0));
}

// Test combined modes

TEST_CASE("integration - version tracking with complex message") {
    SimulationMessage msg;
    msg.message_id = 2001;
    msg.message_type = String("telemetry");
    msg.vehicle.pose.position = {1.0, 2.0, 3.0};
    msg.vehicle.velocity = {0.0, 0.0, 0.0};
    msg.vehicle.timestamp = 300.0;
    msg.timestamp = 300.0;

    auto buf = serialize<Mode::WITH_VERSION>(msg);
    auto result = deserialize<Mode::WITH_VERSION, SimulationMessage>(buf);

    CHECK(result.message_id == 2001);
    CHECK(result.message_type == "telemetry");
}

TEST_CASE("integration - big endian with complex message") {
    VehicleState state;
    state.pose.position = {100.0, 200.0, 300.0};
    state.velocity = {50.0, 0.0, 0.0};
    state.wheels[0] = {0.5, 5.0, 200.0};
    state.wheels[1] = {0.5, 5.0, 200.0};
    state.wheels[2] = {0.5, 5.0, 200.0};
    state.wheels[3] = {0.5, 5.0, 200.0};
    state.timestamp = 400.0;

    auto buf = serialize<Mode::SERIALIZE_BIG_ENDIAN>(state);
    auto result = deserialize<Mode::SERIALIZE_BIG_ENDIAN, VehicleState>(buf);

    CHECK(result.pose.position.x == doctest::Approx(100.0));
    CHECK(result.pose.position.y == doctest::Approx(200.0));
    CHECK(result.velocity.x == doctest::Approx(50.0));
    CHECK(result.timestamp == doctest::Approx(400.0));
}

TEST_CASE("integration - version + big endian combined") {
    ControlCommand cmd;
    cmd.command_id = 0x12345678;
    cmd.command_type = String("test");
    cmd.parameters.insert({String("value"), 999.0});

    auto buf = serialize<Mode::WITH_VERSION | Mode::SERIALIZE_BIG_ENDIAN>(cmd);
    auto result = deserialize<Mode::WITH_VERSION | Mode::SERIALIZE_BIG_ENDIAN, ControlCommand>(buf);

    CHECK(result.command_id == 0x12345678);
    CHECK(result.command_type == "test");
    CHECK(result.parameters[String("value")] == doctest::Approx(999.0));
}

// Test unaligned deserialization with complex types

TEST_CASE("integration - unaligned complex message") {
    VehicleState state;
    state.pose.position = {1.0, 2.0, 3.0};
    state.velocity = {10.0, 0.0, 0.0};
    state.wheels[0] = {0.1, 2.0, 100.0};
    state.wheels[1] = {0.1, 2.0, 100.0};
    state.wheels[2] = {0.1, 2.0, 100.0};
    state.wheels[3] = {0.1, 2.0, 100.0};
    state.timestamp = 500.0;

    auto buf = serialize(state);

    // Create unaligned buffer (offset by 1 byte)
    ByteBuf unaligned_buf(buf.size() + 1);
    unaligned_buf[0] = 0xFF; // Padding byte
    std::memcpy(unaligned_buf.data() + 1, buf.data(), buf.size());

    // Deserialize from unaligned view
    auto view = std::string_view(reinterpret_cast<char const *>(unaligned_buf.data() + 1), buf.size());
    auto result = copy_from_potentially_unaligned<Mode::NONE, VehicleState>(view);

    CHECK(result.pose.position.x == doctest::Approx(1.0));
    CHECK(result.velocity.x == doctest::Approx(10.0));
    CHECK(result.wheels[0].torque == doctest::Approx(100.0));
    CHECK(result.timestamp == doctest::Approx(500.0));
}

// Test large data structures

TEST_CASE("integration - large sensor array") {
    Vector<SensorData> sensors;

    for (int i = 0; i < 100; ++i) {
        SensorData sensor;
        auto id_str = std::string("SENSOR_") + std::to_string(i);
        sensor.sensor_id = String(id_str.c_str());
        for (int j = 0; j < 10; ++j) {
            sensor.readings.push_back(static_cast<double>(i * 10 + j));
        }
        sensor.timestamp = static_cast<double>(i);
        sensors.push_back(sensor);
    }

    auto buf = serialize(sensors);
    auto result = deserialize<Mode::NONE, Vector<SensorData>>(buf);

    REQUIRE(result.size() == 100);
    CHECK(result[0].sensor_id == "SENSOR_0");
    CHECK(result[99].sensor_id == "SENSOR_99");
    CHECK(result[50].readings.size() == 10);
    CHECK(result[50].readings[0] == doctest::Approx(500.0));
    CHECK(result[50].readings[9] == doctest::Approx(509.0));
}

// Test deeply nested structures

TEST_CASE("integration - nested containers") {
    Vector<Map<String, Vector<Optional<double>>>> nested;

    Map<String, Vector<Optional<double>>> map1;
    Vector<Optional<double>> vec1;
    vec1.push_back(1.0);
    vec1.push_back(Optional<double>());
    vec1.push_back(3.0);
    map1.insert({String("data1"), vec1});
    nested.push_back(map1);

    Map<String, Vector<Optional<double>>> map2;
    Vector<Optional<double>> vec2;
    vec2.push_back(Optional<double>());
    vec2.push_back(2.0);
    map2.insert({String("data2"), vec2});
    nested.push_back(map2);

    auto buf = serialize(nested);
    auto result = deserialize<Mode::NONE, Vector<Map<String, Vector<Optional<double>>>>>(buf);

    REQUIRE(result.size() == 2);
    REQUIRE(result[0].size() == 1);
    REQUIRE(result[0][String("data1")].size() == 3);
    CHECK(result[0][String("data1")][0].has_value());
    CHECK(*result[0][String("data1")][0] == doctest::Approx(1.0));
    CHECK(!result[0][String("data1")][1].has_value());
    CHECK(*result[0][String("data1")][2] == doctest::Approx(3.0));
}

// Test empty containers edge cases

TEST_CASE("integration - empty nested containers") {
    SimulationMessage msg;
    msg.message_id = 0;
    msg.message_type = String("");
    msg.vehicle.timestamp = 0.0;
    msg.timestamp = 0.0;
    // sensors vector is empty
    // command is nullopt

    auto buf = serialize(msg);
    auto result = deserialize<Mode::NONE, SimulationMessage>(buf);

    CHECK(result.message_id == 0);
    CHECK(result.message_type == "");
    CHECK(result.sensors.size() == 0);
    CHECK(!result.command.has_value());
}

// Performance baseline test

TEST_CASE("integration - serialization performance baseline") {
    // Create a realistic message
    SimulationMessage msg;
    msg.message_id = 1;
    msg.message_type = String("benchmark");
    msg.vehicle.pose.position = {1.0, 2.0, 3.0};
    msg.vehicle.velocity = {10.0, 0.0, 0.0};
    msg.vehicle.timestamp = 1.0;

    for (int i = 0; i < 10; ++i) {
        SensorData sensor;
        sensor.sensor_id = String("SENSOR");
        sensor.readings.push_back(1.0);
        sensor.readings.push_back(2.0);
        sensor.timestamp = 1.0;
        msg.sensors.push_back(sensor);
    }

    msg.timestamp = 1.0;

    // Serialize/deserialize multiple times
    for (int i = 0; i < 1000; ++i) {
        auto buf = serialize(msg);
        auto result = deserialize<Mode::NONE, SimulationMessage>(buf);
        CHECK(result.message_id == 1);
    }
}
