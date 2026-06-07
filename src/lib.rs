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

// Keep clippy's strict CI gate focused on real safety/ABI problems instead of
// style churn in public robotics data-model names.
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
    ArchiveFrame, Archived, BytePayloadView, DataPod, DataPodAccess, DataPodDecode,
    DataPodValidate, Encoding, Envelope, FixedView, LeWireHeader, OwnedWireMessage,
    SegmentedArchiveFrame, SegmentedArchived, WireError, WireFrame, WireMessage, WireParts,
    WireSegmentedFrame, access_wire, access_wire_bytes, access_wire_bytes_unchecked,
    access_wire_bytes_v1, access_wire_frame, access_wire_frame_unchecked, access_wire_frame_v1,
    access_wire_unchecked, archive, decode_payload_vec, from_archive, from_wire_frame,
    from_wire_frame_v1, from_wire_message, from_wire_message_v1, read_le_field, segmented_archive,
    split_wire_frame, split_wire_parts, to_wire_message, to_wire_message_v1,
    to_wire_message_v1_named, try_to_wire_message, try_to_wire_message_v1_named,
    try_wire_frame_to_message, try_wire_segmented_frame_to_message, validate_registered_wire,
    validate_registered_wire_frame, validate_registered_wire_frame_v1, validate_registered_wire_v1,
    validate_wire, validate_wire_bytes, validate_wire_bytes_v1, validate_wire_frame,
    validate_wire_frame_v1, view_archive, wire_frame_to_message, wire_segmented_frame_to_message,
    with_archive, with_segmented_wire_frame, with_segmented_wire_frame_named, with_wire_frame,
    with_wire_frame_named, with_wire_frame_slices, with_wire_segmented_frame_slices,
};

pub mod bind;
pub mod dynamic;
pub mod ffi;
#[cfg(feature = "python")]
pub mod python;

pub mod assoc;
pub mod geom;
pub mod id;
pub mod layout;
pub mod motion;
pub mod raster;
pub mod registry;
pub mod robot;
pub mod schema;
pub mod seq;
pub mod world;

// ---------------------------------------------------------------------------
// Top-level re-exports — common types accessible as `datapod::X`.
// ---------------------------------------------------------------------------

pub use geom::shapes::{
    Aabb, BoundingSphere, Box, Circle, GaussianBox, GaussianCircle, GaussianPoint,
    GaussianRectangle, Line, Obb, Rectangle, Size, Square, Triangle,
};
pub use geom::{
    Linestring, LinestringHeader, LinestringView, MultiPoint, MultiPointHeader, MultiPointView,
    Path, PathHeader, PathView, Point, PointKey, PointMap, PointSet, Polygon, PolygonHeader,
    PolygonView, Ring, RingHeader, RingView, Segment, Trajectory, TrajectoryHeader, TrajectoryView,
};

pub use motion::{Acceleration, Euler, Pose, Quaternion, State, Transform, Velocity};

pub use world::{Geo, Loc, Utm};

pub use raster::{Grid, GridHeader, GridView, Layer, LayerHeader, LayerView};

pub use seq::{
    BitVec, BitVecHeader, BitVecView, Bytes, BytesHeader, BytesView, Deque, DequeHeader, DequeView,
    DpStr, DpStrHeader, DpStrView, Fifo, ForwardList, ForwardListHeader, ForwardListView, Heap,
    HeapHeader, HeapOrder, HeapView, IndexedHeap, IndexedHeapHeader, IndexedHeapView, List,
    ListHeader, ListView, Matrix, MatrixHeader, MatrixView, MaxHeap, MinHeap, PagedVecvec,
    PagedVecvecHeader, PagedVecvecView, PriorityQueue, Queue, QueueHeader, QueueView, Stack,
    StackHeader, StackView, Tensor, TensorHeader, TensorView, Vector, VectorHeader, VectorView,
    Vecvec, VecvecHeader, VecvecView,
};

pub use assoc::{Map, MapEntry, MapHeader, MapView, OMap, OSet, Set, SetEntry, SetHeader, SetView};

pub use id::{DpString, DpStringHeader, DpStringView, IP, Ip, MacAddr, STRING_NONE, UUID, Uuid};

pub use layout::{
    PayloadLayoutBuilder, PayloadSection, SectionValidation, section_bytes, validate_sections,
    validate_sections_with_policy,
};

pub use robot::{
    Accel, Actuator, BoxShape, Collision, CylinderShape, Geometry, GeometryKind, INVALID_ID,
    Identity, Inertial, Joint, JointCalibration, JointDynamics, JointLimits, JointMimic,
    JointSafetyController, JointType, KV, Link, Material, MeshShape, Model, Odom, Robot, Sensor,
    SphereShape, Transmission, TransmissionJoint, Twist, Visual, Wrench,
};
