//! Compile-time verification of the wire contract.
//!
//! Two kinds of types implement `DataPod`:
//!
//! - **Fixed Pod** (`type Header = Self`, `type Payload = ()`): the type
//!   itself is Pod-castable. Verified via `assert_fixed_pod::<T>()`.
//! - **Heap-bearing** (`type Header = <T>Header`, `type Payload = [u8]`):
//!   the user struct owns a `Vec<...>` inside; the generated `<T>Header`
//!   is what travels on the wire's metadata slot. Verified via
//!   `assert_heap_datapod::<T>()`.

use bytemuck::Pod;
use datapod::{
    Aabb, Accel, Acceleration, Actuator, BoundingSphere, BoxShape, Bs, Circle, Collision,
    CylinderShape, DataPod, Encoding, Envelope, Euler, GaussianBox, GaussianCircle, GaussianPoint,
    GaussianRectangle, Geo, Geometry, GeometryKind, Grid, Identity, Inertial, Ip, Joint,
    JointCalibration, JointDynamics, JointLimits, JointMimic, JointSafetyController, JointType, KV,
    Layer, Line, Linestring, Link, Loc, MacAddr, Material, MeshShape, Model, MultiPoint, Obb, Odom,
    Path, Point, PointKey, Polygon, Pose, Quaternion, Rectangle, Ring, Robot, Segment, Sensor,
    Size, SphereShape, Square, State, Trajectory, Transform, Transmission, TransmissionJoint,
    Triangle, Twist, Utm, Uuid, Velocity, Visual, Wrench,
};

fn assert_fixed_pod<T: DataPod<Header = T, Payload = ()> + Pod>() {}
fn assert_heap_datapod<T: DataPod<Payload = [u8]>>() {}

#[test]
fn fixed_pod_types_are_their_own_header() {
    assert_fixed_pod::<Envelope>();
    assert_fixed_pod::<Encoding>();

    assert_fixed_pod::<Aabb>();
    assert_fixed_pod::<Acceleration>();
    assert_fixed_pod::<BoundingSphere>();
    assert_fixed_pod::<Bs>();
    assert_fixed_pod::<Euler>();
    assert_fixed_pod::<Geo>();
    assert_fixed_pod::<Loc>();
    assert_fixed_pod::<Obb>();
    assert_fixed_pod::<Point>();
    assert_fixed_pod::<PointKey>();
    assert_fixed_pod::<Pose>();
    assert_fixed_pod::<Quaternion>();
    assert_fixed_pod::<Segment>();
    assert_fixed_pod::<Size>();
    assert_fixed_pod::<State>();
    assert_fixed_pod::<Transform>();
    assert_fixed_pod::<Utm>();
    assert_fixed_pod::<Velocity>();

    assert_fixed_pod::<Ip>();
    assert_fixed_pod::<MacAddr>();
    assert_fixed_pod::<Uuid>();
    assert_fixed_pod::<KV>();

    assert_fixed_pod::<Circle>();
    assert_fixed_pod::<datapod::Line>();
    assert_fixed_pod::<Rectangle>();
    assert_fixed_pod::<Square>();
    assert_fixed_pod::<Triangle>();
    assert_fixed_pod::<GaussianBox>();
    assert_fixed_pod::<GaussianCircle>();
    assert_fixed_pod::<GaussianPoint>();
    assert_fixed_pod::<GaussianRectangle>();

    assert_fixed_pod::<Accel>();
    assert_fixed_pod::<Actuator>();
    assert_fixed_pod::<BoxShape>();
    assert_fixed_pod::<Collision>();
    assert_fixed_pod::<CylinderShape>();
    assert_fixed_pod::<Geometry>();
    assert_fixed_pod::<GeometryKind>();
    assert_fixed_pod::<Identity>();
    assert_fixed_pod::<Inertial>();
    assert_fixed_pod::<Joint>();
    assert_fixed_pod::<JointCalibration>();
    assert_fixed_pod::<JointDynamics>();
    assert_fixed_pod::<JointLimits>();
    assert_fixed_pod::<JointMimic>();
    assert_fixed_pod::<JointSafetyController>();
    assert_fixed_pod::<JointType>();
    assert_fixed_pod::<Link>();
    assert_fixed_pod::<Material>();
    assert_fixed_pod::<MeshShape>();
    assert_fixed_pod::<Model>();
    assert_fixed_pod::<Odom>();
    assert_fixed_pod::<Robot>();
    assert_fixed_pod::<Sensor>();
    assert_fixed_pod::<SphereShape>();
    assert_fixed_pod::<Transmission>();
    assert_fixed_pod::<TransmissionJoint>();
    assert_fixed_pod::<Twist>();
    assert_fixed_pod::<Visual>();
    assert_fixed_pod::<Wrench>();
}

#[test]
fn heap_bearing_types_implement_datapod_with_byte_payload() {
    assert_heap_datapod::<Linestring>();
    assert_heap_datapod::<Ring>();
    assert_heap_datapod::<MultiPoint>();
    assert_heap_datapod::<Polygon>();
    assert_heap_datapod::<Path>();
    assert_heap_datapod::<Trajectory>();
    assert_heap_datapod::<Grid>();
    assert_heap_datapod::<Layer>();
    assert_heap_datapod::<datapod::DpString>();
}

#[test]
fn polygon_carries_vertices_inside() {
    let poly = Polygon::new(vec![
        Point::new(0.0, 0.0, 0.0),
        Point::new(1.0, 0.0, 0.0),
        Point::new(1.0, 1.0, 0.0),
        Point::new(0.0, 1.0, 0.0),
    ]);
    // Data inside — no separate Vec to pass:
    assert_eq!(poly.num_vertices(), 4);
    assert_eq!(poly.area(), 1.0);
    assert_eq!(poly.perimeter(), 4.0);
    assert!(poly.contains(Point::new(0.5, 0.5, 0.0)));

    // Wire shipping: header is a separate Pod struct, payload is the bytes.
    let header = poly.header();
    let payload = poly.payload_bytes();
    assert_eq!(payload.len(), 4 * core::mem::size_of::<Point>());
    let _ = header; // PolygonHeader has no fields — just a Pod marker

    // bytes_of works on the header.
    let _bytes = bytemuck::bytes_of(&header);
}

#[test]
fn grid_carries_data_inside_plus_header_fields() {
    let g = Grid::new(
        4,
        4,
        Encoding::U8,
        1.0,
        true,
        Pose::default(),
        vec![0u8; 16],
    );
    assert_eq!(g.size(), 16);
    assert_eq!(g.data.len(), 16);
    assert!(g.is_valid(1));

    // Wire shipping:
    let h = g.header();
    assert_eq!(h.rows, 4);
    assert_eq!(h.cols, 4);
    assert_eq!(h.encoding, Encoding::U8);
    let _bytes = bytemuck::bytes_of(&h); // header IS Pod
}
