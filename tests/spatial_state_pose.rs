use datapod::{Acceleration, Geo, Loc, Point, Pose, Quaternion, State, Transform, Velocity};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn velocity_and_acceleration_have_expected_math() {
    let velocity = Velocity {
        vx: 3.0,
        vy: 4.0,
        vz: 0.0,
    };
    approx_eq(velocity.speed(), 5.0, 1e-9);
    approx_eq(velocity.speed_2d(), 5.0, 1e-9);

    let acceleration = Acceleration {
        ax: 1.0,
        ay: 2.0,
        az: 2.0,
    };
    approx_eq(acceleration.magnitude(), 3.0, 1e-9);
}

#[test]
fn pose_can_transform_and_inverse_transform_points() {
    let pose = Pose {
        point: Point::new(1.0, 2.0, 3.0),
        rotation: Quaternion::identity(),
    };

    let world = pose.transform_point(Point::new(4.0, 5.0, 6.0));
    assert_eq!(world, Point::new(5.0, 7.0, 9.0));

    let local = pose.inverse_transform_point(world);
    assert_eq!(local, Point::new(4.0, 5.0, 6.0));
}

#[test]
fn pose_and_state_round_trip_through_mat_vectors() {
    let pose = Pose {
        point: Point::new(1.0, 2.0, 3.0),
        rotation: Quaternion::identity(),
    };
    assert_eq!(Pose::from_mat(pose.to_mat()), pose);

    let state = State {
        pose,
        linear_velocity: Velocity {
            vx: 1.0,
            vy: 2.0,
            vz: 3.0,
        },
        angular_velocity: Velocity {
            vx: 0.1,
            vy: 0.2,
            vz: 0.3,
        },
    };
    assert_eq!(State::from_mat(state.to_mat()), state);
}

#[test]
fn loc_preserves_origin_and_computes_distances() {
    let origin = Geo::new(52.0, 5.0, 0.0);
    let loc = Loc {
        local: Point::new(3.0, 4.0, 0.0),
        origin,
    };
    approx_eq(loc.distance_from_origin(), 5.0, 1e-9);
    assert!(loc.has_valid_origin());
    assert!(loc.same_origin(
        Loc {
            local: Point::default(),
            origin
        },
        1e-9
    ));
}

#[test]
fn transform_translation_and_apply_work() {
    let transform = Transform::from_translation(1.0, 2.0, 3.0);
    assert_eq!(transform.get_translation(), Point::new(1.0, 2.0, 3.0));

    let mut x = 4.0;
    let mut y = 5.0;
    let mut z = 6.0;
    transform.apply(&mut x, &mut y, &mut z);
    assert_eq!(Point::new(x, y, z), Point::new(5.0, 7.0, 9.0));
}
