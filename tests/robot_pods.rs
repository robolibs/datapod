use datapod::{
    Actuator, Collision, Geometry, INVALID_ID, Identity, Inertial, Joint, JointLimits, Link,
    Material, Model, Odom, Point, Pose, Quaternion, Robot, Sensor, Size, Transmission,
    TransmissionJoint, Twist, Velocity, Visual, Wrench,
};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

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

#[test]
fn robot_geometry_visual_and_collision_match_urdf_style_layout() {
    let mut sphere = Geometry::sphere(0.5);
    assert!(sphere.is_sphere());
    assert_eq!(sphere.as_sphere().unwrap().radius, 0.5);
    sphere.as_sphere_mut().unwrap().radius = 0.75;
    assert_eq!(sphere.as_sphere().unwrap().radius, 0.75);

    let visual = Visual {
        name: "mesh".into(),
        ..Visual::with_material(
            Geometry::box_shape(Size::new(1.0, 2.0, 3.0)),
            Material::textured("package://robot/mesh.png"),
        )
    };
    assert!(visual.is_set());
    assert!(visual.material.as_ref().unwrap().has_texture());

    let collision = Collision::new(sphere);
    assert!(!collision.is_set());
    let collision = Collision {
        name: "hitbox".into(),
        ..collision
    };
    assert!(collision.is_set());
}

#[test]
fn robot_joint_link_and_model_follow_cpp_structure() {
    let mut model = Model::default();
    let base_id = model.add_link(Link::new("base"));
    let tool_id = model.add_link(Link {
        name: Link::new("tool").name,
        visuals: datapod::Vector::from([Visual {
            name: "tool_vis".into(),
            origin: Pose::default(),
            geom: Geometry::box_shape(Size::new(0.1, 0.2, 0.3)),
            material: None,
        }]),
        collisions: datapod::Vector::from([Collision {
            name: "tool_col".into(),
            origin: Pose::default(),
            geom: Geometry::sphere(0.2),
        }]),
        sensor: Some(Sensor {
            name: "camera".into(),
            r#type: "rgb".into(),
            origin: Pose::default(),
            props: Default::default(),
        }),
        ..Link::default()
    });

    let joint_id = model.add_joint(Joint::revolute(
        "joint0",
        [0.0, 0.0, 1.0],
        JointLimits {
            lower: -1.0,
            upper: 1.0,
            effort: 10.0,
            velocity: 5.0,
        },
        Pose::default(),
    ));
    model.connect(base_id, tool_id, joint_id);

    assert_eq!(model.root, base_id);
    assert_eq!(model.num_links(), 2);
    assert_eq!(model.num_joints(), 1);
    assert!(model.is_valid_link(tool_id));
    assert!(model.is_valid_joint(joint_id));
    assert_eq!(model.get_parent(tool_id), base_id);
    assert_eq!(model.get_parent_joint(tool_id), joint_id);
    assert_eq!(model.get_children(base_id), &[tool_id]);
    assert!(model.is_root(base_id));
    assert!(model.is_leaf(tool_id));
    assert!(model.joints[0].is_revolute());
    assert_eq!(model.joints[0].parent, base_id);
    assert_eq!(model.joints[0].child, tool_id);
    assert_eq!(model.joints[0].parent, 0);
    assert_ne!(INVALID_ID, tool_id);
    assert!(model.links[1].has_visuals());
    assert!(model.links[1].has_collisions());
    assert!(model.links[1].sensor.is_some());
    assert!(Joint::fixed("fixed", Pose::default()).is_fixed());
    assert!(Joint::continuous("wheel", [1.0, 0.0, 0.0], Pose::default()).is_continuous());
    assert!(
        Joint::prismatic(
            "slide",
            [1.0, 0.0, 0.0],
            JointLimits::default(),
            Pose::default()
        )
        .is_prismatic()
    );
}

#[test]
fn robot_identity_transmission_and_wrapper_are_present() {
    let transmission = Transmission {
        name: "drive".into(),
        r#type: "SimpleTransmission".into(),
        joints: datapod::Vector::from([TransmissionJoint {
            name: "joint0".into(),
            mechanical_reduction: Some(2.0),
            offset: Some(0.1),
        }]),
        actuators: datapod::Vector::from([Actuator {
            name: "motor0".into(),
            mechanical_reduction: Some(50.0),
        }]),
    };
    assert_eq!(transmission.joints[0].offset, Some(0.1));

    let robot = Robot {
        id: Identity {
            name: "demo".into(),
            ..Identity::default()
        },
        model: Model {
            transmissions: datapod::Vector::from([transmission]),
            ..Model::default()
        },
        ..Robot::default()
    };
    assert_eq!(robot.id.name, "demo");
    assert_eq!(robot.model.transmissions.len(), 1);
}
