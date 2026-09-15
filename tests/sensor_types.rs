use datapod::{
    Acceleration, Imu, Quaternion, TurnRadius, Velocity, WheelEncoder, WheelEncoders,
    from_wire_message, to_wire_message,
};

#[test]
fn wheel_encoders_round_trip_over_the_wire() {
    let readings = WheelEncoders::new(vec![
        WheelEncoder::new(0, 1.5, 0.4),
        WheelEncoder::new(1, 1.6, 0.42),
        WheelEncoder::new(2, 1.4, 0.38),
        WheelEncoder::new(3, 1.55, 0.41),
    ]);

    let msg = to_wire_message(&readings);
    let decoded: WheelEncoders = from_wire_message(&msg).expect("wheel encoders decode");

    assert_eq!(decoded.num_wheels(), 4);
    for (a, b) in readings.wheels.iter().zip(decoded.wheels.iter()) {
        assert_eq!(a.wheel_id, b.wheel_id);
        assert_eq!(a.angle_rad, b.angle_rad);
        assert_eq!(a.velocity_rad_s, b.velocity_rad_s);
    }
}

#[test]
fn empty_wheel_encoders_round_trip() {
    let readings = WheelEncoders::default();
    let msg = to_wire_message(&readings);
    let decoded: WheelEncoders = from_wire_message(&msg).expect("empty wheel encoders decode");
    assert_eq!(decoded.num_wheels(), 0);
}

#[test]
fn imu_round_trip_over_the_wire() {
    let imu = Imu::new(
        Velocity {
            vx: 0.0,
            vy: 0.0,
            vz: 0.1,
        },
        Acceleration {
            ax: 0.2,
            ay: 0.0,
            az: 9.81,
        },
        Quaternion::new(1.0, 0.0, 0.0, 0.0),
    );

    let msg = to_wire_message(&imu);
    let decoded: Imu = from_wire_message(&msg).expect("imu decode");

    assert_eq!(decoded.angular_velocity.vz, 0.1);
    assert_eq!(decoded.linear_acceleration.az, 9.81);
    assert_eq!(decoded.orientation.w, 1.0);
    assert!(decoded.is_set());
}

#[test]
fn turn_radius_default_is_straight() {
    let straight = TurnRadius::default();
    assert!(straight.is_straight());

    let curving = TurnRadius::new(5.5);
    assert!(!curving.is_straight());

    let msg = to_wire_message(&curving);
    let decoded: TurnRadius = from_wire_message(&msg).expect("turn radius decode");
    assert_eq!(decoded.radius_m, 5.5);
}
