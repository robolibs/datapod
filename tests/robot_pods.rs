//! Pod-shape verification for the `spatial::robot::*` types post-Phase-6d.
//!
//! All robot types are now Pod: strings are referenced by `u32` ID
//! (`STRING_NONE` for unset), lists of related entities are referenced by
//! `u32` ID arrays (`INVALID_ID` for unset slots), and `Option<T>` is
//! flattened to a `*_present: u32` flag with the inline `T`.

use datapod::{
    Actuator, Collision, Geometry, GeometryKind, INVALID_ID, Identity, Inertial, Joint,
    JointLimits, Link, Material, Model, Odom, Point, Pose, Quaternion, Robot, Sensor, Size,
    Transmission, TransmissionJoint, Twist, Velocity, Visual, Wrench,
};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

// ---------------------------------------------------------------------------
// Already-Pod types — preserved behavior
// ---------------------------------------------------------------------------

#[test]
fn twist_round_trips_through_mat() {
    let twist = Twist {
        linear: Velocity {
            vx: 1.0,
            vy: 2.0,
            vz: 3.0,
        },
        angular: Velocity {
            vx: 0.1,
            vy: 0.2,
            vz: 0.3,
        },
    };
    assert_eq!(Twist::from_mat(twist.to_mat()), twist);
    assert!(twist.is_set());
}

#[test]
fn wrench_supports_math_and_round_trip() {
    let wrench = Wrench {
        force: Point::new(3.0, 4.0, 0.0),
        torque: Point::new(0.0, 0.0, 2.0),
    };
    approx_eq(wrench.force_magnitude(), 5.0, 1e-9);
    approx_eq(wrench.torque_magnitude(), 2.0, 1e-9);
    assert_eq!(Wrench::from_mat(wrench.to_mat()), wrench);
    assert_eq!((wrench * 2.0).force, Point::new(6.0, 8.0, 0.0));
}

#[test]
fn odom_round_trips_through_mat() {
    let odom = Odom {
        pose: Pose {
            point: Point::new(1.0, 2.0, 3.0),
            rotation: Quaternion::identity(),
        },
        twist: Twist {
            linear: Velocity {
                vx: 1.0,
                vy: 2.0,
                vz: 3.0,
            },
            angular: Velocity {
                vx: 0.1,
                vy: 0.2,
                vz: 0.3,
            },
        },
    };
    assert_eq!(Odom::from_mat(odom.to_mat()), odom);
    assert!(odom.is_set());
}

#[test]
fn inertial_reports_basic_properties() {
    let inertial = Inertial {
        origin: Pose::default(),
        mass: 2.0,
        ixx: 1.0,
        ixy: 0.0,
        ixz: 0.0,
        iyy: 2.0,
        iyz: 0.0,
        izz: 3.0,
    };
    assert!(inertial.is_set());
    approx_eq(inertial.trace(), 6.0, 1e-9);
    assert!(inertial.is_diagonal());
}

// ---------------------------------------------------------------------------
// Geometry / Material / Visual / Collision — Pod variants
// ---------------------------------------------------------------------------

#[test]
fn geometry_tag_and_variant_accessors_work() {
    let sphere = Geometry::sphere(0.5);
    assert_eq!(sphere.kind, GeometryKind::Sphere);
    assert!(sphere.is_sphere());
    assert_eq!(sphere.as_sphere().unwrap().radius, 0.5);

    let box_geom = Geometry::box_shape(Size::new(1.0, 2.0, 3.0));
    assert_eq!(box_geom.kind, GeometryKind::Box);
    assert_eq!(box_geom.as_box().unwrap().size, Size::new(1.0, 2.0, 3.0));

    let cyl = Geometry::cylinder(0.1, 1.0);
    assert!(cyl.is_cylinder());

    let mesh = Geometry::mesh(7, [1.0, 1.0, 1.0]);
    assert!(mesh.is_mesh());
    assert_eq!(mesh.as_mesh().unwrap().uri_id, 7);
}

#[test]
fn visual_and_collision_carry_geometry_and_optional_material() {
    let visual = Visual::with_material(
        Geometry::box_shape(Size::new(1.0, 2.0, 3.0)),
        Material::textured(42),
    );
    assert!(visual.has_material());
    assert!(visual.material.has_texture());
    assert_eq!(visual.material.texture_id, 42);

    let collision = Collision::new(Geometry::sphere(0.5));
    assert!(!collision.is_set());
    let collision = Collision::named(99, Pose::default(), Geometry::sphere(0.5));
    assert_eq!(collision.name_id, 99);
}

// ---------------------------------------------------------------------------
// Joint, Link, Sensor — ID-based references
// ---------------------------------------------------------------------------

#[test]
fn joint_constructors_set_type_and_axis() {
    let revolute = Joint::revolute(
        7,
        [0.0, 0.0, 1.0],
        JointLimits {
            lower: -1.0,
            upper: 1.0,
            effort: 10.0,
            velocity: 5.0,
        },
        Pose::default(),
    );
    assert!(revolute.is_revolute());
    assert_eq!(revolute.name_id, 7);
    assert!(revolute.has_limits());
    assert_eq!(revolute.limits.upper, 1.0);

    assert!(Joint::fixed(0, Pose::default()).is_fixed());
    assert!(Joint::continuous(0, [1.0, 0.0, 0.0], Pose::default()).is_continuous());
    assert!(
        Joint::prismatic(0, [1.0, 0.0, 0.0], JointLimits::default(), Pose::default())
            .is_prismatic()
    );
}

#[test]
fn link_tracks_inertial_visuals_collisions_sensor_by_id() {
    let mut link = Link::new(5);
    assert_eq!(link.name_id, 5);
    assert!(!link.has_inertial());
    assert!(!link.has_visuals());
    assert!(!link.has_collisions());
    assert!(!link.has_sensor());

    link.inertial_present = 1;
    link.inertial.mass = 2.0;
    link.visual_ids[0] = 11;
    link.collision_ids[0] = 22;
    link.sensor_id = 33;

    assert!(link.has_inertial());
    assert!(link.has_visuals());
    assert!(link.has_collisions());
    assert!(link.has_sensor());
}

#[test]
fn sensor_carries_id_and_origin() {
    let s = Sensor::new(1, 2, Pose::default());
    assert_eq!(s.name_id, 1);
    assert_eq!(s.type_id, 2);
}

// ---------------------------------------------------------------------------
// Model & Robot — aggregate headers (records are application-side)
// ---------------------------------------------------------------------------

#[test]
fn model_tracks_counts_and_root() {
    let model = Model {
        link_count: 3,
        joint_count: 2,
        ..Default::default()
    };
    assert_eq!(model.num_links(), 3);
    assert_eq!(model.num_joints(), 2);
    assert!(model.is_valid_link(0));
    assert!(!model.is_valid_link(3));
    assert!(model.is_valid_joint(1));
    assert!(!model.is_valid_joint(2));
}

#[test]
fn transmission_holds_fixed_cap_joints_and_actuators() {
    let mut t = Transmission {
        name_id: 1,
        type_id: 2,
        ..Default::default()
    };
    t.joints[0] = TransmissionJoint {
        name_id: 7,
        reduction_present: 1,
        offset_present: 1,
        _pad: 0,
        mechanical_reduction: 2.0,
        offset: 0.1,
    };
    t.actuators[0] = Actuator {
        name_id: 8,
        reduction_present: 1,
        mechanical_reduction: 50.0,
    };
    assert_eq!(t.joints[0].offset, 0.1);
    assert_eq!(t.actuators[0].mechanical_reduction, 50.0);
}

#[test]
fn robot_aggregates_identity_and_model() {
    let mut robot = Robot::default();
    robot.id.name_id = 100;
    robot.model.link_count = 4;
    assert_eq!(robot.id.name_id, 100);
    assert_eq!(robot.model.num_links(), 4);

    // INVALID_ID is the sentinel for unset relationships.
    let _: u32 = INVALID_ID;
}
