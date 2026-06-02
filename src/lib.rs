//! Datapod — the wire contract for the stack.
//!
//! Everything that travels between processes or hosts implements [`DataPod`].
//! Fixed-Pod types are their own header (`type Header = Self`). Heap-bearing
//! types own a `Vec<...>` inside, and the macro generates a sibling
//! `<T>Header` Pod struct that rides the wire.
//!
//! Top-level domains:
//! - [`geom`]   — geometric types (points, polygons, shapes, rings, ...)
//! - [`motion`] — pose, transforms, motion state
//! - [`world`]  — earth-frame / geodetic (Geo, Loc, Utm)
//! - [`raster`] — gridded raster data (Grid, Layer)
//! - [`robot`]  — robotics / URDF (Joint, Link, Robot, ...)
//! - [`id`]     — identifier primitives (Uuid, Ip, MacAddr, DpString)
//! - [`wire`]   — the [`DataPod`] trait + transport envelope

// The Rust port intentionally preserves several C++ datapod naming and
// module-shape conventions while downstream crates migrate. Keep clippy's
// strict CI gate useful without forcing those public compatibility names to
// churn in this milestone.
#![allow(
    clippy::derivable_impls,
    clippy::manual_div_ceil,
    clippy::manual_is_multiple_of,
    clippy::module_inception,
    clippy::needless_range_loop,
    clippy::new_ret_no_self,
    clippy::should_implement_trait,
    clippy::suspicious_arithmetic_impl,
    clippy::too_many_arguments
)]

// `#[datapod]` and `#[derive(DataPod)]` expansions emit absolute paths
// like `::datapod::DataPod`. Make those paths resolve when the macro is
// invoked from inside this crate itself.
extern crate self as datapod;

pub use datapod_macros::{DataPod, ZeroCopySend, datapod};
pub use wire::ZeroCopySend;

// Re-exported so the `#[datapod]` / `#[derive(DataPod)]` expansions can
// reference `::datapod::bytemuck::{Pod, Zeroable, ...}` instead of a bare
// `::bytemuck`. That means a downstream crate using the macro only needs
// `datapod` as a dependency — not `bytemuck` too.
pub use bytemuck;

pub mod wire;
pub use wire::{
    DataPod, DataPodDecode, Encoding, Envelope, WireError, WireMessage, decode_payload_vec,
    from_wire_message, to_wire_message,
};

pub mod bind;
pub mod ffi;
#[cfg(feature = "python")]
pub mod python;

pub mod assoc;
pub mod geom;
pub mod id;
pub mod motion;
pub mod raster;
pub mod registry;
pub mod robot;
pub mod seq;
pub mod world;

// ---------------------------------------------------------------------------
// Top-level re-exports — common types accessible as `datapod::X`.
// ---------------------------------------------------------------------------

pub use geom::shapes::{
    Aabb, BoundingSphere, Box, Bs, Circle, GaussianBox, GaussianCircle, GaussianPoint,
    GaussianRectangle, Line, Obb, Rectangle, Size, Square, Triangle,
};
pub use geom::{
    Linestring, LinestringHeader, MultiPoint, MultiPointHeader, Path, PathHeader, Point, PointKey,
    PointMap, PointSet, Polygon, PolygonHeader, Ring, RingHeader, Segment, Trajectory,
    TrajectoryHeader,
};

pub use motion::{Acceleration, Euler, Pose, Quaternion, State, Transform, Velocity};

pub use world::{Geo, Loc, Utm};

pub use raster::{Grid, GridHeader, Layer, LayerHeader};

pub use seq::{
    BitVec, Bytes, Deque, DpStr, Fifo, ForwardList, Heap, HeapOrder, IndexedHeap, List, Matrix,
    MatrixHeader, MaxHeap, MinHeap, PagedVecvec, PriorityQueue, Queue, Stack, Tensor, Vector,
    Vecvec,
};

pub use assoc::{Map, MapEntry, OMap, OSet, Set, SetEntry};

pub use id::{DpString, IP, Ip, MacAddr, STRING_NONE, UUID, Uuid};

pub use robot::{
    Accel, Actuator, BoxShape, Collision, CylinderShape, Geometry, GeometryKind, INVALID_ID,
    Identity, Inertial, Joint, JointCalibration, JointDynamics, JointLimits, JointMimic,
    JointSafetyController, JointType, KV, Link, Material, MeshShape, Model, Odom, Robot, Sensor,
    SphereShape, Transmission, TransmissionJoint, Twist, Visual, Wrench,
};
