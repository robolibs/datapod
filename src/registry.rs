//! Central metadata registry for datapod language bindings.

use std::collections::BTreeMap;
use std::fmt;
use std::sync::{OnceLock, RwLock};

use crate::DataPod;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TypeInfo {
    /// Stable canonical-name schema hash.
    pub type_hash: u64,
    /// Alias retained for schema introspection; equal to `type_hash`.
    pub canonical_type_hash: u64,
    pub rust_type_name: String,
    pub canonical_name: String,
    pub header_size: usize,
    pub payload_kind: PayloadKind,
    pub format_version: u32,
    pub wire_format: WireFormat,
    pub endian: Endian,
    pub alignment: AlignmentPolicy,
    pub validator: ValidatorKind,
    pub emitted_hash: u64,
    pub emitted_hash_kind: HashKind,
    pub has_archive: bool,
    pub has_view: bool,
    pub has_owned_decode: bool,
    pub archive_shape: ArchiveShape,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PayloadKind {
    Fixed,
    Bytes,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Endian {
    Little,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WireFormat {
    DatapodWireV1Little,
}

impl WireFormat {
    pub fn name(self) -> &'static str {
        match self {
            Self::DatapodWireV1Little => "datapod-wire-v1/le",
        }
    }

    pub fn from_name(name: &str) -> Option<Self> {
        match name {
            "datapod-wire-v1/le" | "v1" | "le" | "little" => Some(Self::DatapodWireV1Little),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HashKind {
    CanonicalName,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AlignmentPolicy {
    UnalignedWire,
    AlignedPayload { alignment: usize },
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ValidatorKind {
    RegistryOnly,
    BuiltIn,
    RuntimeSchema,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ArchiveShape {
    Fixed,
    SinglePayload,
    SegmentedPayload,
    RuntimeSchema,
}

impl ArchiveShape {
    pub fn name(self) -> &'static str {
        match self {
            Self::Fixed => "fixed",
            Self::SinglePayload => "single-payload",
            Self::SegmentedPayload => "segmented-payload",
            Self::RuntimeSchema => "runtime-schema",
        }
    }
}

pub const CURRENT_WIRE_FORMAT: WireFormat = WireFormat::DatapodWireV1Little;
pub const BUILTIN_EMITTED_HASH_KIND: HashKind = HashKind::CanonicalName;
pub const BUILTIN_HASH_POLICY: &str =
    "built-in emission uses canonical-name hashes in datapod-wire-v1/le";

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum RegistryError {
    EmptyCanonicalName,
    InvalidCanonicalName {
        canonical_name: String,
        reason: &'static str,
    },
    InvalidTypeHash,
    HeaderSizeTooLarge {
        header_size: usize,
    },
    TypeHashCanonicalNameMismatch {
        canonical_name: String,
        expected_hash: u64,
        provided_hash: u64,
    },
    HashConflict {
        type_hash: u64,
        existing_name: String,
        new_name: String,
    },
    NameConflict {
        canonical_name: String,
        existing_hash: u64,
        new_hash: u64,
    },
    MetadataConflict {
        type_hash: u64,
        existing: Box<TypeInfo>,
        new: Box<TypeInfo>,
    },
    RegistryLockPoisoned,
}

impl fmt::Display for RegistryError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::EmptyCanonicalName => write!(f, "datapod canonical name is empty"),
            Self::InvalidCanonicalName {
                canonical_name,
                reason,
            } => write!(
                f,
                "datapod canonical name {canonical_name:?} is invalid: {reason}"
            ),
            Self::InvalidTypeHash => write!(f, "datapod type hash must be non-zero"),
            Self::HeaderSizeTooLarge { header_size } => write!(
                f,
                "datapod header size {header_size} exceeds maximum supported slice length"
            ),
            Self::TypeHashCanonicalNameMismatch {
                canonical_name,
                expected_hash,
                provided_hash,
            } => write!(
                f,
                "datapod canonical name {canonical_name} hashes to {expected_hash}, not provided hash {provided_hash}"
            ),
            Self::HashConflict {
                type_hash,
                existing_name,
                new_name,
            } => write!(
                f,
                "datapod type hash {type_hash} is already registered as {existing_name}, not {new_name}"
            ),
            Self::NameConflict {
                canonical_name,
                existing_hash,
                new_hash,
            } => write!(
                f,
                "datapod canonical name {canonical_name} is already registered as {existing_hash}, not {new_hash}"
            ),
            Self::MetadataConflict {
                type_hash,
                existing,
                new,
            } => write!(
                f,
                "datapod type hash {type_hash} metadata conflict: existing={existing:?}, new={new:?}"
            ),
            Self::RegistryLockPoisoned => write!(f, "datapod custom registry lock is poisoned"),
        }
    }
}

impl std::error::Error for RegistryError {}

macro_rules! datapod_types {
    ($macro:ident) => {
        $macro! {
            ("datapod.envelope.v1", crate::Envelope),
            ("datapod.encoding.v1", crate::Encoding),
            ("datapod.point.v1", crate::Point),
            ("datapod.point_key.v1", crate::PointKey),
            ("datapod.geo.v1", crate::Geo),
            ("datapod.loc.v1", crate::Loc),
            ("datapod.utm.v1", crate::Utm),
            ("datapod.segment.v1", crate::Segment),
            ("datapod.linestring.v1", crate::Linestring),
            ("datapod.multi_point.v1", crate::MultiPoint),
            ("datapod.ring.v1", crate::Ring),
            ("datapod.polygon.v1", crate::Polygon),
            ("datapod.path.v1", crate::Path),
            ("datapod.trajectory.v1", crate::Trajectory),
            ("datapod.line.v1", crate::Line),
            ("datapod.size.v1", crate::Size),
            ("datapod.rectangle.v1", crate::Rectangle),
            ("datapod.square.v1", crate::Square),
            ("datapod.aabb.v1", crate::Aabb),
            ("datapod.obb.v1", crate::Obb),
            ("datapod.box.v1", crate::Box),
            ("datapod.bounding_sphere.v1", crate::BoundingSphere),
            ("datapod.circle.v1", crate::Circle),
            ("datapod.triangle.v1", crate::Triangle),
            ("datapod.gaussian_point.v1", crate::GaussianPoint),
            ("datapod.gaussian_circle.v1", crate::GaussianCircle),
            ("datapod.gaussian_rectangle.v1", crate::GaussianRectangle),
            ("datapod.gaussian_box.v1", crate::GaussianBox),
            ("datapod.euler.v1", crate::Euler),
            ("datapod.quaternion.v1", crate::Quaternion),
            ("datapod.pose.v1", crate::Pose),
            ("datapod.transform.v1", crate::Transform),
            ("datapod.velocity.v1", crate::Velocity),
            ("datapod.acceleration.v1", crate::Acceleration),
            ("datapod.state.v1", crate::State),
            ("datapod.grid.v1", crate::Grid),
            ("datapod.layer.v1", crate::Layer),
            ("datapod.bytes.v1", crate::Bytes),
            ("datapod.dpstr.v1", crate::DpStr),
            ("datapod.dpstring.v1", crate::DpString),
            ("datapod.bitvec.v1", crate::BitVec),
            ("datapod.deque.v1", crate::Deque),
            ("datapod.queue.v1", crate::Queue),
            ("datapod.stack.v1", crate::Stack),
            ("datapod.list.v1", crate::List),
            ("datapod.forward_list.v1", crate::ForwardList),
            ("datapod.heap.v1", crate::Heap),
            ("datapod.indexed_heap.v1", crate::IndexedHeap),
            ("datapod.vector.v1", crate::Vector),
            ("datapod.matrix.v1", crate::Matrix),
            ("datapod.tensor.v1", crate::Tensor),
            ("datapod.vecvec.v1", crate::Vecvec),
            ("datapod.paged_vecvec.v1", crate::PagedVecvec),
            ("datapod.map.v1", crate::Map),
            ("datapod.set.v1", crate::Set),
            ("datapod.uuid.v1", crate::Uuid),
            ("datapod.ip.v1", crate::Ip),
            ("datapod.mac_addr.v1", crate::MacAddr),
            ("datapod.twist.v1", crate::Twist),
            ("datapod.wrench.v1", crate::Wrench),
            ("datapod.odom.v1", crate::Odom),
            ("datapod.inertial.v1", crate::Inertial),
            ("datapod.joint_limits.v1", crate::JointLimits),
            ("datapod.accel.v1", crate::Accel),
            ("datapod.joint_dynamics.v1", crate::JointDynamics),
            ("datapod.joint_mimic.v1", crate::JointMimic),
            ("datapod.joint_safety_controller.v1", crate::JointSafetyController),
            ("datapod.joint_calibration.v1", crate::JointCalibration),
            ("datapod.kv.v1", crate::KV),
            ("datapod.box_shape.v1", crate::BoxShape),
            ("datapod.sphere_shape.v1", crate::SphereShape),
            ("datapod.cylinder_shape.v1", crate::CylinderShape),
            ("datapod.mesh_shape.v1", crate::MeshShape),
            ("datapod.geometry_kind.v1", crate::GeometryKind),
            ("datapod.geometry.v1", crate::Geometry),
            ("datapod.identity.v1", crate::Identity),
            ("datapod.material.v1", crate::Material),
            ("datapod.visual.v1", crate::Visual),
            ("datapod.collision.v1", crate::Collision),
            ("datapod.joint_type.v1", crate::JointType),
            ("datapod.joint.v1", crate::Joint),
            ("datapod.link.v1", crate::Link),
            ("datapod.sensor.v1", crate::Sensor),
            ("datapod.model.v1", crate::Model),
            ("datapod.robot.v1", crate::Robot),
            ("datapod.actuator.v1", crate::Actuator),
            ("datapod.transmission_joint.v1", crate::TransmissionJoint),
            ("datapod.transmission.v1", crate::Transmission)
        }
    };
}

macro_rules! count_types {
    ($(($canonical:literal, $ty:ty)),* $(,)?) => {
        [ $(($canonical, core::any::type_name::<$ty>())),* ].len()
    };
}

pub fn type_count() -> usize {
    datapod_types!(count_types) + custom_registry().read().map_or(0, |r| r.by_hash.len())
}

pub fn all_type_infos() -> Vec<TypeInfo> {
    macro_rules! build {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {
            {
                let mut infos = Vec::new();
                let builtin_count = [$(($canonical, core::any::type_name::<$ty>())),*].len();
                if infos.try_reserve_exact(builtin_count).is_err() {
                    return Vec::new();
                }
                $(
                    infos.push(type_info::<$ty>($canonical));
                )*
                infos
            }
        };
    }
    let mut infos = datapod_types!(build);
    if let Ok(custom) = custom_registry().read() {
        if infos.try_reserve_exact(custom.by_hash.len()).is_err() {
            return infos;
        }
        infos.extend(custom.by_hash.values().cloned());
    }
    infos
}

pub fn find_type_info(type_hash: u64) -> Option<TypeInfo> {
    try_find_type_info(type_hash).unwrap_or(None)
}

pub fn try_find_type_info(type_hash: u64) -> Result<Option<TypeInfo>, RegistryError> {
    macro_rules! find {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {{
            $(
                if crate::bind::type_hash::<$ty>() == type_hash {
                    return Ok(Some(type_info::<$ty>($canonical)));
                }
            )*
            None
        }};
    }
    if let Some(info) = datapod_types!(find) {
        return Ok(Some(info));
    }
    let registry = custom_registry()
        .read()
        .map_err(|_| RegistryError::RegistryLockPoisoned)?;
    Ok(registry.by_hash.get(&type_hash).cloned())
}

pub fn type_exists(type_hash: u64) -> bool {
    find_type_info(type_hash).is_some()
}

pub fn v1_header_size(type_hash: u64) -> Option<usize> {
    macro_rules! find {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {{
            $(
                if crate::bind::type_hash::<$ty>() == type_hash {
                    return Some(<<$ty as crate::DataPod>::Header as crate::LeWireHeader>::LE_WIRE_SIZE);
                }
            )*
            None
        }};
    }
    datapod_types!(find).or_else(|| find_type_info(type_hash).map(|info| info.header_size))
}

pub fn validate_registered_wire_v1(type_hash: u64, bytes: &[u8]) -> Result<(), crate::WireError> {
    macro_rules! validate {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {{
            $(
                if crate::bind::type_hash_name($canonical) == type_hash {
                    return crate::validate_wire_bytes_v1::<$ty>(type_hash, bytes);
                }
            )*
            None
        }};
    }
    if let Some(()) = datapod_types!(validate) {
        return Ok(());
    }

    let Some(info) = find_type_info(type_hash) else {
        return Err(crate::WireError::UnknownTypeHash { type_hash });
    };
    if bytes.len() < info.header_size {
        return Err(crate::WireError::ShortHeader {
            type_name: "registered datapod type",
            needed: info.header_size,
            got: bytes.len(),
        });
    }
    if info.payload_kind == PayloadKind::Fixed && bytes.len() != info.header_size {
        let payload_bytes = bytes.len().checked_sub(info.header_size).ok_or_else(|| {
            crate::WireError::InvalidPayloadSize {
                type_name: "registered datapod type",
                message: "fixed payload byte count underflowed".to_string(),
            }
        })?;
        return Err(crate::WireError::InvalidPayloadSize {
            type_name: "registered datapod type",
            message: format!(
                "fixed-size registered datapod has {} payload bytes",
                payload_bytes
            ),
        });
    }
    Ok(())
}

pub fn validate_registered_wire_frame_v1(
    frame: crate::WireFrame<'_>,
) -> Result<(), crate::WireError> {
    macro_rules! validate {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {{
            $(
                if crate::bind::type_hash::<$ty>() == frame.type_hash {
                    return crate::validate_wire_frame_v1::<$ty>(frame);
                }
            )*
        }};
    }
    datapod_types!(validate);

    let Some(info) = find_type_info(frame.type_hash) else {
        return Err(crate::WireError::UnknownTypeHash {
            type_hash: frame.type_hash,
        });
    };
    if frame.header.len() != info.header_size {
        return Err(crate::WireError::ShortHeader {
            type_name: "registered datapod type",
            needed: info.header_size,
            got: frame.header.len(),
        });
    }
    if info.payload_kind == PayloadKind::Fixed && !frame.payload.is_empty() {
        return Err(crate::WireError::InvalidPayloadSize {
            type_name: "registered datapod type",
            message: format!(
                "fixed-size registered datapod archive has {} payload bytes",
                frame.payload.len()
            ),
        });
    }
    Ok(())
}

pub fn find_type_info_by_name(canonical_name: &str) -> Option<TypeInfo> {
    try_find_type_info_by_name(canonical_name).unwrap_or(None)
}

pub fn try_find_type_info_by_name(canonical_name: &str) -> Result<Option<TypeInfo>, RegistryError> {
    if let Some(info) = find_static_type_by_name(canonical_name) {
        return Ok(Some(info));
    }
    let registry = custom_registry()
        .read()
        .map_err(|_| RegistryError::RegistryLockPoisoned)?;
    Ok(registry
        .by_name
        .get(canonical_name)
        .and_then(|hash| registry.by_hash.get(hash))
        .cloned())
}

pub fn canonical_name<T: DataPod>() -> Option<&'static str> {
    if let Some(name) = T::CANONICAL_NAME {
        return Some(name);
    }

    macro_rules! find {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {{
            $(
                if core::any::TypeId::of::<T>() == core::any::TypeId::of::<$ty>() {
                    return Some($canonical);
                }
            )*
            None
        }};
    }
    datapod_types!(find)
}

/// Register runtime metadata for a user-defined C/Python datapod type.
///
/// This does not create Rust `DataPod` implementations. It makes the generic
/// registry-backed C/Python helpers understand how to split and validate
/// `type_hash + header || payload` messages for a schema defined outside the
/// Rust built-in type list.
pub fn register_type(
    type_hash: u64,
    canonical_name: impl Into<String>,
    header_size: usize,
    payload_kind: PayloadKind,
) -> Result<(), RegistryError> {
    let canonical_name = canonical_name.into();
    register_type_info(TypeInfo {
        type_hash,
        canonical_type_hash: type_hash,
        rust_type_name: "<runtime>".to_string(),
        canonical_name,
        header_size,
        payload_kind,
        format_version: 1,
        wire_format: CURRENT_WIRE_FORMAT,
        endian: Endian::Little,
        alignment: AlignmentPolicy::UnalignedWire,
        validator: ValidatorKind::RuntimeSchema,
        emitted_hash: type_hash,
        emitted_hash_kind: HashKind::CanonicalName,
        has_archive: true,
        has_view: true,
        has_owned_decode: false,
        archive_shape: ArchiveShape::RuntimeSchema,
    })
}

/// Register metadata for a Rust custom datapod schema.
///
/// `#[datapod(name = "...")]` uses this through its generated
/// `register_schema()` helper. Registration is only needed by generic
/// registry-backed helpers such as [`crate::split_wire_frame`]; typed Rust
/// helpers already know their header and payload layout from `T`.
pub fn register_datapod_type<T>(
    canonical_name: &'static str,
    payload_kind: PayloadKind,
    archive_shape: ArchiveShape,
) -> Result<(), RegistryError>
where
    T: DataPod,
    T::Header: crate::LeWireHeader,
{
    let type_hash = crate::bind::type_hash_name(canonical_name);
    register_type_info(TypeInfo {
        type_hash,
        canonical_type_hash: type_hash,
        rust_type_name: core::any::type_name::<T>().to_string(),
        canonical_name: canonical_name.to_string(),
        header_size: <T::Header as crate::LeWireHeader>::LE_WIRE_SIZE,
        payload_kind,
        format_version: 1,
        wire_format: CURRENT_WIRE_FORMAT,
        endian: Endian::Little,
        alignment: AlignmentPolicy::UnalignedWire,
        validator: ValidatorKind::RegistryOnly,
        emitted_hash: type_hash,
        emitted_hash_kind: HashKind::CanonicalName,
        has_archive: true,
        has_view: true,
        has_owned_decode: true,
        archive_shape,
    })
}

fn register_type_info(new: TypeInfo) -> Result<(), RegistryError> {
    let type_hash = new.type_hash;
    let canonical_name = new.canonical_name.clone();
    validate_canonical_name(&canonical_name)?;
    if type_hash == 0 {
        return Err(RegistryError::InvalidTypeHash);
    }
    if new.header_size > isize::MAX as usize {
        return Err(RegistryError::HeaderSizeTooLarge {
            header_size: new.header_size,
        });
    }
    let expected_hash = type_hash_name(&canonical_name);
    if type_hash != expected_hash {
        return Err(RegistryError::TypeHashCanonicalNameMismatch {
            canonical_name,
            expected_hash,
            provided_hash: type_hash,
        });
    }

    if let Some(existing) = find_static_type_info(type_hash) {
        if existing.canonical_name != canonical_name {
            return Err(RegistryError::HashConflict {
                type_hash,
                existing_name: existing.canonical_name,
                new_name: canonical_name,
            });
        }
        if existing.header_size != new.header_size || existing.payload_kind != new.payload_kind {
            return Err(RegistryError::MetadataConflict {
                type_hash,
                existing: Box::new(existing),
                new: Box::new(new),
            });
        }
        return Ok(());
    }

    if let Some(existing) = find_static_type_by_name(&canonical_name) {
        if existing.type_hash != type_hash {
            return Err(RegistryError::NameConflict {
                canonical_name,
                existing_hash: existing.type_hash,
                new_hash: type_hash,
            });
        }
        return Ok(());
    }

    let mut registry = custom_registry()
        .write()
        .map_err(|_| RegistryError::RegistryLockPoisoned)?;
    if let Some(existing) = registry.by_hash.get(&type_hash) {
        if existing == &new {
            return Ok(());
        }
        return Err(RegistryError::MetadataConflict {
            type_hash,
            existing: Box::new(existing.clone()),
            new: Box::new(new),
        });
    }
    if let Some(existing_hash) = registry.by_name.get(&canonical_name)
        && *existing_hash != type_hash
    {
        return Err(RegistryError::NameConflict {
            canonical_name,
            existing_hash: *existing_hash,
            new_hash: type_hash,
        });
    }

    registry.by_name.insert(canonical_name, type_hash);
    registry.by_hash.insert(type_hash, new);
    Ok(())
}

pub fn type_hash_name(canonical_name: &str) -> u64 {
    crate::bind::type_hash_name(canonical_name)
}

pub fn validate_canonical_name(canonical_name: &str) -> Result<(), RegistryError> {
    if canonical_name.is_empty() {
        return Err(RegistryError::EmptyCanonicalName);
    }

    let mut previous_was_dot = true;
    for byte in canonical_name.bytes() {
        match byte {
            0 => {
                return Err(RegistryError::InvalidCanonicalName {
                    canonical_name: canonical_name.to_string(),
                    reason: "must not contain NUL bytes",
                });
            }
            b'.' if previous_was_dot => {
                return Err(RegistryError::InvalidCanonicalName {
                    canonical_name: canonical_name.to_string(),
                    reason: "must not contain empty dot-separated segments",
                });
            }
            b'.' => {
                previous_was_dot = true;
            }
            b'a'..=b'z' | b'0'..=b'9' | b'_' => {
                previous_was_dot = false;
            }
            _ => {
                return Err(RegistryError::InvalidCanonicalName {
                    canonical_name: canonical_name.to_string(),
                    reason: "must contain only ASCII lowercase letters, digits, underscores, and dots",
                });
            }
        }
    }

    if previous_was_dot {
        return Err(RegistryError::InvalidCanonicalName {
            canonical_name: canonical_name.to_string(),
            reason: "must not contain empty dot-separated segments",
        });
    }

    Ok(())
}

pub fn current_wire_format_name() -> &'static str {
    CURRENT_WIRE_FORMAT.name()
}

pub fn builtin_hash_policy() -> &'static str {
    BUILTIN_HASH_POLICY
}

fn find_static_type_info(type_hash: u64) -> Option<TypeInfo> {
    macro_rules! find {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {{
            $(
                if crate::bind::type_hash::<$ty>() == type_hash {
                    return Some(type_info::<$ty>($canonical));
                }
            )*
            None
        }};
    }
    datapod_types!(find)
}

fn find_static_type_by_name(canonical_name: &str) -> Option<TypeInfo> {
    macro_rules! find {
        ($(($canonical:literal, $ty:ty)),* $(,)?) => {{
            $(
                if $canonical == canonical_name {
                    return Some(type_info::<$ty>($canonical));
                }
            )*
            None
        }};
    }
    datapod_types!(find)
}

#[derive(Default)]
struct CustomRegistry {
    by_hash: BTreeMap<u64, TypeInfo>,
    by_name: BTreeMap<String, u64>,
}

fn custom_registry() -> &'static RwLock<CustomRegistry> {
    static REGISTRY: OnceLock<RwLock<CustomRegistry>> = OnceLock::new();
    REGISTRY.get_or_init(|| RwLock::new(CustomRegistry::default()))
}

fn type_info<T>(canonical_name: &'static str) -> TypeInfo
where
    T: DataPod,
    T::Header: crate::LeWireHeader,
{
    let type_hash = crate::bind::type_hash::<T>();
    TypeInfo {
        type_hash,
        canonical_type_hash: type_hash,
        rust_type_name: core::any::type_name::<T>().to_string(),
        canonical_name: canonical_name.to_string(),
        header_size: <T::Header as crate::LeWireHeader>::LE_WIRE_SIZE,
        payload_kind: payload_kind::<T>(),
        format_version: 1,
        wire_format: CURRENT_WIRE_FORMAT,
        endian: Endian::Little,
        alignment: AlignmentPolicy::UnalignedWire,
        validator: ValidatorKind::BuiltIn,
        emitted_hash: type_hash,
        emitted_hash_kind: BUILTIN_EMITTED_HASH_KIND,
        has_archive: true,
        has_view: true,
        has_owned_decode: true,
        archive_shape: archive_shape::<T>(),
    }
}

fn payload_kind<T: DataPod>() -> PayloadKind {
    match core::any::type_name::<T::Payload>() {
        "()" => PayloadKind::Fixed,
        _ => PayloadKind::Bytes,
    }
}

fn archive_shape<T: DataPod>() -> ArchiveShape {
    match payload_kind::<T>() {
        PayloadKind::Fixed => ArchiveShape::Fixed,
        PayloadKind::Bytes => ArchiveShape::SinglePayload,
    }
}
