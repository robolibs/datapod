//! Smoke test that the public surface of `datapod` is reachable.
//!
//! After the Phase-6 cleanup, `datapod` is just the wire contract plus
//! spatial. The pre-cleanup C++-parity modules (matrix, sequential,
//! adapters, associative, lockfree, memory, temporal, trees, the lowercase
//! primitive aliases, top-level sugar) are gone.

use datapod::{
    Aabb, Accel, Acceleration, BoundingSphere, Box, Circle, Encoding, Envelope, Euler, GaussianBox,
    GaussianCircle, GaussianPoint, GaussianRectangle, Geo, Grid, INVALID_ID, Identity, Inertial,
    Ip, Joint, JointCalibration, JointDynamics, JointLimits, JointMimic, JointSafetyController,
    Imu, Layer, Line, Linestring, Link, Loc, MacAddr, Material, MeshShape, Model, MultiPoint, Obb,
    Odom, Path, Point, PointKey, Polygon, Pose, Quaternion, Rectangle, Ring, Robot, Segment,
    Sensor, Size, SphereShape, Square, State, Trajectory, Transform, Triangle, TurnRadius, Twist,
    Utm, Uuid, Velocity, Visual, WheelEncoder, WheelEncoders, Wrench,
};

#[test]
fn wire_contract_types_are_reachable() {
    let _envelope = Envelope::default();
    let _encoding = Encoding::default();
}

#[test]
fn spatial_pod_surface_compiles() {
    let _aabb = Aabb::default();
    let _acceleration = Acceleration::default();
    let _bounding_sphere = BoundingSphere::default();
    let _box = Box::default();
    let _euler = Euler::default();
    let _geo = Geo::default();
    let _loc = Loc::default();
    let _obb = Obb::default();
    let point = Point::new(1.0, 2.0, 3.0);
    let _point_key: PointKey = point.into();
    let _pose = Pose::default();
    let _quat = Quaternion::default();
    let _segment = Segment::new(point, Point::default());
    let _size = Size::default();
    let _state = State::default();
    let _transform = Transform::default();
    let _utm = Utm::default();
    let _velocity = Velocity::default();
    let _ip = Ip::default();
    let _mac = MacAddr::default();
    let _uuid = Uuid::default();
}

#[test]
fn spatial_subgroup_pod_surface_compiles() {
    let _circle = Circle::default();
    let _line = Line::default();
    let _rectangle = Rectangle::default();
    let _square = Square::default();
    let _triangle = Triangle::default();
    let _gb = GaussianBox::default();
    let _gc = GaussianCircle::default();
    let _gp = GaussianPoint::default();
    let _gr = GaussianRectangle::default();
}

#[test]
fn spatial_complex_heap_headers_compile() {
    let _linestring = Linestring::default();
    let _polygon = Polygon::default();
    let _ring = Ring::default();
    let _mp = MultiPoint::default();
    let _grid = Grid::default();
    let _layer = Layer::default();
    let _path = Path::default();
    let _trajectory = Trajectory::default();
}

#[test]
fn robot_surface_compiles() {
    let _identity = Identity::default();
    let _inertial = Inertial::default();
    let _joint = Joint::default();
    let _joint_limits = JointLimits::default();
    let _joint_dynamics = JointDynamics::default();
    let _joint_safety = JointSafetyController::default();
    let _joint_calibration = JointCalibration::default();
    let _joint_mimic = JointMimic::default();
    let _link = Link::default();
    let _model = Model::default();
    let _accel = Accel::default();
    let _twist = Twist::default();
    let _wrench = Wrench::default();
    let _odom = Odom::default();
    let _imu = Imu::default();
    let _turn_radius = TurnRadius::default();
    let _wheel_encoder = WheelEncoder::default();
    let _wheel_encoders = WheelEncoders::default();
    let _sensor = Sensor::default();
    let _visual = Visual::default();
    let _material = Material::default();
    let _mesh = MeshShape::default();
    let _sphere = SphereShape::default();
    let _robot = Robot::default();
    let _invalid: u32 = INVALID_ID;
}
