use datapod::{
    Actuator, Collision, Geometry, INVALID_ID, IP, Identity, Inertial, Joint, JointCalibration,
    JointDynamics, JointLimits, JointMimic, JointSafetyController, JointType, Link, MacAddr,
    Material, Model, Pose, Robot, Size, Transmission, TransmissionJoint, UUID, Visual,
};

#[test]
fn material_named_and_color_helpers_work() {
    let named = Material::named("red", [1.0, 0.0, 0.0, 1.0]);
    assert_eq!(named.name, "red");
    let color = Material::color([0.0, 1.0, 0.0, 1.0]);
    assert_eq!(color.texture, "");
}

#[test]
fn visual_new_and_with_origin_work() {
    let geom = Geometry::box_shape(Size::new(1.0, 2.0, 3.0));
    let visual = Visual::new(geom.clone());
    assert_eq!(visual.geom, geom);
    let pose = Pose::default();
    let visual = Visual::with_origin(pose, geom);
    assert_eq!(visual.origin, pose);
}

#[test]
fn collision_new_and_with_origin_work() {
    let geom = Geometry::sphere(1.0);
    let collision = Collision::new(geom.clone());
    assert_eq!(collision.geom, geom);
    let pose = Pose::default();
    let collision = Collision::with_origin(pose, geom);
    assert_eq!(collision.origin, pose);
}

#[test]
fn geometry_box_cylinder_mesh_helpers_work() {
    let bx = Geometry::box_shape(Size::new(1.0, 2.0, 3.0));
    assert!(bx.is_box());
    let cylinder = Geometry::cylinder(1.0, 2.0);
    assert!(cylinder.is_cylinder());
    let mesh = Geometry::mesh("file://mesh.obj", [2.0, 2.0, 2.0]);
    assert!(mesh.is_mesh());
    assert_eq!(mesh.as_mesh().unwrap().scale, [2.0, 2.0, 2.0]);
}

#[test]
fn joint_helper_structs_default_to_expected_values() {
    assert_eq!(JointMimic::default().multiplier, 1.0);
    assert_eq!(JointLimits::default().velocity, 0.0);
    assert_eq!(JointDynamics::default().damping, 0.0);
    assert_eq!(JointSafetyController::default().k_velocity, 0.0);
    assert_eq!(JointCalibration::default().rising, None);
}

#[test]
fn joint_default_and_type_helpers_work() {
    let joint = Joint::default();
    assert_eq!(joint.parent, INVALID_ID);
    assert!(joint.is_fixed());
    assert_eq!(joint.r#type, JointType::Fixed);
}

#[test]
fn link_with_inertial_and_new_helpers_work() {
    let link = Link::new("base");
    assert_eq!(link.name, "base");
    let inertial = Inertial {
        mass: 1.0,
        ..Inertial::default()
    };
    let link = Link::with_inertial("body", inertial.clone());
    assert!(link.has_inertial());
    assert_eq!(link.inertial, Some(inertial));
}

#[test]
fn model_parent_queries_return_invalid_for_unknown_ids() {
    let model = Model::default();
    assert_eq!(model.get_parent(99), INVALID_ID);
    assert_eq!(model.get_parent_joint(99), INVALID_ID);
    assert_eq!(model.get_children(99), &[]);
    assert!(!model.is_valid_link(0));
    assert!(!model.is_valid_joint(0));
}

#[test]
fn robot_identity_and_transmission_fields_are_plainly_accessible() {
    let robot = Robot {
        id: Identity {
            name: "r".into(),
            ..Identity::default()
        },
        model: Model {
            transmissions: datapod::Vector::from([Transmission {
                name: "tx".into(),
                r#type: "simple".into(),
                joints: datapod::Vector::from([TransmissionJoint {
                    name: "j".into(),
                    mechanical_reduction: Some(2.0),
                    offset: None,
                }]),
                actuators: datapod::Vector::from([Actuator {
                    name: "a".into(),
                    mechanical_reduction: Some(3.0),
                }]),
            }]),
            ..Model::default()
        },
        ..Robot::default()
    };
    assert_eq!(robot.id.name, "r");
    assert_eq!(robot.model.transmissions[0].actuators[0].name, "a");
}

#[test]
fn sugar_type_aliases_are_usable() {
    let _ip: IP = Default::default();
    let _uuid: UUID = Default::default();
    let _mac = MacAddr::new([1, 2, 3, 4, 5, 6]);
}

#[test]
fn sugar_parsers_reject_bad_inputs() {
    assert!(datapod::Uuid::from_string("not-a-uuid").is_err());
    assert!(datapod::MacAddr::from_string("00:11:22:33:44").is_err());
    assert!(datapod::Ip::from_string("999.1.1.1").is_err());
}
