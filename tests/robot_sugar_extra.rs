//! Robot-type / sugar helper coverage after Phase 6d (all-Pod robot
//! subtree). Strings are referenced by `u32` ID; lists are fixed-cap
//! ID arrays; `Option<T>` becomes `*_present: u32` + inline `T`.

use datapod::{
    Actuator, Collision, Geometry, INVALID_ID, IP, Identity, Inertial, Joint, JointCalibration,
    JointDynamics, JointLimits, JointMimic, JointSafetyController, JointType, KV, Link, MacAddr,
    Material, Model, Pose, Robot, Size, Transmission, TransmissionJoint, UUID, Visual, WireError,
};

#[test]
fn material_named_and_color_helpers_work() {
    let named = Material::named(7, [1.0, 0.0, 0.0, 1.0]);
    assert_eq!(named.name_id, 7);
    let _color = Material::color([0.0, 1.0, 0.0, 1.0]);
    let textured = Material::textured(42);
    assert!(textured.has_texture());
}

#[test]
fn visual_new_and_with_origin_work() {
    let geom = Geometry::box_shape(Size::new(1.0, 2.0, 3.0));
    let visual = Visual::new(geom);
    assert_eq!(visual.geom, geom);
    let pose = Pose::default();
    let visual = Visual::with_origin(pose, geom);
    assert_eq!(visual.origin, pose);
}

#[test]
fn collision_new_and_with_origin_work() {
    let geom = Geometry::sphere(1.0);
    let collision = Collision::new(geom);
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
    let mesh = Geometry::mesh(99, [2.0, 2.0, 2.0]);
    assert!(mesh.is_mesh());
    assert_eq!(mesh.as_mesh().unwrap().scale, [2.0, 2.0, 2.0]);
    assert_eq!(mesh.as_mesh().unwrap().uri_id, 99);
}

#[test]
fn joint_helper_structs_default_to_expected_values() {
    assert_eq!(JointMimic::default().multiplier, 1.0);
    assert_eq!(JointLimits::default().velocity, 0.0);
    assert_eq!(JointDynamics::default().damping, 0.0);
    assert_eq!(JointSafetyController::default().k_velocity, 0.0);
    assert_eq!(JointCalibration::default().rising_present, 0);
}

#[test]
fn joint_default_and_type_helpers_work() {
    let joint = Joint::default();
    assert_eq!(joint.parent_id, INVALID_ID);
    assert!(joint.is_fixed());
    assert_eq!(joint.joint_type, JointType::Fixed);
}

#[test]
fn link_with_inertial_and_new_helpers_work() {
    let link = Link::new(5);
    assert_eq!(link.name_id, 5);
    let inertial = Inertial {
        mass: 1.0,
        ..Inertial::default()
    };
    let link = Link::with_inertial(6, inertial);
    assert!(link.has_inertial());
    assert_eq!(link.inertial, inertial);
}

#[test]
fn model_validity_queries_handle_unknown_ids() {
    let model = Model::default();
    assert!(!model.is_valid_link(0));
    assert!(!model.is_valid_joint(0));

    let populated = Model {
        link_count: 2,
        joint_count: 1,
        ..Model::default()
    };
    assert_eq!(populated.try_num_links().unwrap(), 2);
    assert_eq!(populated.try_num_joints().unwrap(), 1);
    assert_eq!(populated.num_links(), 2);
    assert_eq!(populated.num_joints(), 1);

    const MODEL_RS: &str = include_str!("../src/robot/model.rs");
    for needle in [
        "pub fn try_num_links(&self) -> Result<usize, WireError>",
        "pub fn try_num_joints(&self) -> Result<usize, WireError>",
        "usize::try_from(value)",
    ] {
        assert!(
            MODEL_RS.contains(needle),
            "Model count helpers should keep checked conversion path {needle:?}"
        );
    }
    for forbidden in ["self.link_count as usize", "self.joint_count as usize"] {
        assert!(
            !MODEL_RS.contains(forbidden),
            "Model count helpers should not retain unchecked count cast {forbidden:?}"
        );
    }
}

#[test]
fn robot_identity_and_transmission_fields_are_plainly_accessible() {
    let mut robot = Robot {
        id: Identity::default(),
        ..Robot::default()
    };
    robot.id.name_id = 1;

    let mut transmission = Transmission {
        name_id: 10,
        type_id: 11,
        ..Default::default()
    };
    transmission.joints[0] = TransmissionJoint {
        name_id: 20,
        reduction_present: 1,
        offset_present: 0,
        _pad: 0,
        mechanical_reduction: 2.0,
        offset: 0.0,
    };
    transmission.actuators[0] = Actuator {
        name_id: 21,
        reduction_present: 1,
        mechanical_reduction: 3.0,
    };
    assert_eq!(robot.id.name_id, 1);
    assert_eq!(transmission.actuators[0].name_id, 21);
    robot.model.transmission_count = 1;
    assert_eq!(robot.model.transmission_count, 1);
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

#[test]
fn kv_string_helpers_have_fallible_utf8_accessors() {
    let mut kv = KV::default();
    kv.set_key("robot.name");
    kv.set_value("arm");

    assert_eq!(kv.try_key_str().unwrap(), "robot.name");
    assert_eq!(kv.try_value_str().unwrap(), "arm");
    assert_eq!(kv.key_str(), "robot.name");
    assert_eq!(kv.value_str(), "arm");

    kv.key = [0; 32];
    kv.key[0] = 0xff;
    assert!(matches!(
        kv.try_key_str(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(kv.key_str(), "");

    kv.value = [0; 64];
    kv.value[0] = 0xfe;
    assert!(matches!(
        kv.try_value_str(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(kv.value_str(), "");

    kv.key = [0; 32];
    kv.key[0] = b'a';
    kv.key[1] = 0;
    kv.key[2] = 0xff;
    assert_eq!(kv.try_key_str().unwrap(), "a");
}
