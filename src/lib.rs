//! Rust port of the `datapod` base library.
//!
//! The initial focus is the foundational type and geometry layers that
//! downstream crates such as `vectkit` depend on.

pub mod adapters;
pub mod associative;
pub mod lockfree;
pub mod matrix;
pub mod memory;
pub mod sequential;
pub mod spatial;
pub mod sugar;
pub mod temporal;
pub mod trees;
pub mod types;

pub use associative::{Map, MapExt, OMap, OMapExt, OSet, OSetExt, Set, SetExt};
pub use matrix::mat;
pub use sequential::Vector;
pub use spatial::{
    Aabb, Accel, Acceleration, Actuator, BoundingSphere, Box, BoxShape, Bs, Circle, Collision,
    CylinderShape, Euler, GaussianBox, GaussianCircle, GaussianPoint, GaussianRectangle, Geo,
    Geometry, Grid, INVALID_ID, Identity, Inertial, Joint, JointCalibration, JointDynamics,
    JointLimits, JointMimic, JointSafetyController, JointType, Layer, Linestring, Link, Loc,
    Material, MeshShape, Model, MultiLinestring, MultiPoint, MultiPolygon, Obb, Odom, Path, Point,
    PointKey, PointMap, PointSet, Polygon, Pose, Quadtree, Quaternion, RTree, Rectangle, Ring,
    Robot, Segment, Sensor, Size, SphereShape, Square, State, Trajectory, Transform, Transmission,
    TransmissionJoint, Triangle, Twist, Utm, Velocity, Visual, Wrench,
};
pub use sugar::{IP, Ip, MacAddr, UUID, Uuid};
pub use types::{boolean, byte, f32, f64, i8, i16, i32, i64, isize, u8, u16, u32, u64, usize};
