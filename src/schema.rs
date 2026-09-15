//! Built-in datapod schema descriptors.
//!
//! Schemas are side metadata: they are registered once per process and are not
//! written into every datapod packet. The packet hot path remains
//! `type_hash + header || payload`.

use crate::{DataPod, LeWireHeader, registry};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FieldRole {
    Header,
    Payload,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ScalarType {
    U8,
    U16,
    U32,
    U64,
    U128,
    I8,
    I16,
    I32,
    I64,
    I128,
    F32,
    F64,
    Bool,
}

impl ScalarType {
    pub const fn name(self) -> &'static str {
        match self {
            Self::U8 => "u8",
            Self::U16 => "u16",
            Self::U32 => "u32",
            Self::U64 => "u64",
            Self::U128 => "u128",
            Self::I8 => "i8",
            Self::I16 => "i16",
            Self::I32 => "i32",
            Self::I64 => "i64",
            Self::I128 => "i128",
            Self::F32 => "f32",
            Self::F64 => "f64",
            Self::Bool => "bool",
        }
    }

    pub const fn wire_size(self) -> usize {
        match self {
            Self::U8 | Self::I8 | Self::Bool => 1,
            Self::U16 | Self::I16 => 2,
            Self::U32 | Self::I32 | Self::F32 => 4,
            Self::U64 | Self::I64 | Self::F64 => 8,
            Self::U128 | Self::I128 => 16,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FieldType {
    Scalar(ScalarType),
    Array { element: ScalarType, len: usize },
    NestedArray { type_hash: u64, len: usize },
    Nested { type_hash: u64 },
    Opaque { wire_size: usize },
    PayloadSection,
    Bytes,
    /// A heap-bearing `Vec<T>` payload whose element `T` is itself a
    /// registered datapod type. `type_hash` is zero when `T` is a bare
    /// scalar, so readers fall back to raw bytes.
    BytesElements { type_hash: u64 },
}

impl FieldType {
    pub fn wire_size(self) -> Option<usize> {
        match self {
            Self::Scalar(scalar) => Some(scalar.wire_size()),
            Self::Array { element, len } => element.wire_size().checked_mul(len),
            Self::NestedArray { type_hash, len } => crate::registry::find_schema(type_hash)
                .and_then(|schema| schema.header_size.checked_mul(len)),
            Self::Nested { .. } | Self::PayloadSection => None,
            Self::Opaque { wire_size } => Some(wire_size),
            Self::Bytes | Self::BytesElements { .. } => None,
        }
    }
}

/// Extracts the element type hash a `Vec<T>` payload field carries, given
/// `T`'s own [`SchemaFieldType::field_type`]. Non-`DataPod` elements (bare
/// scalars) yield zero, meaning "no element schema, treat as raw bytes".
pub fn bytes_element_type_hash(element: FieldType) -> u64 {
    match element {
        FieldType::Nested { type_hash } => type_hash,
        _ => 0,
    }
}

pub trait SchemaFieldType: 'static {
    fn field_type() -> FieldType;
}

macro_rules! impl_schema_scalar {
    ($($ty:ty => $scalar:ident),* $(,)?) => {
        $(
            impl SchemaFieldType for $ty {
                fn field_type() -> FieldType {
                    FieldType::Scalar(ScalarType::$scalar)
                }
            }
        )*
    };
}

impl_schema_scalar!(
    u8 => U8,
    u16 => U16,
    u32 => U32,
    u64 => U64,
    u128 => U128,
    i8 => I8,
    i16 => I16,
    i32 => I32,
    i64 => I64,
    i128 => I128,
    f32 => F32,
    f64 => F64,
    bool => Bool,
);

impl<T, const N: usize> SchemaFieldType for [T; N]
where
    T: SchemaFieldType,
{
    fn field_type() -> FieldType {
        match T::field_type() {
            FieldType::Scalar(element) => FieldType::Array { element, len: N },
            FieldType::Nested { type_hash } => FieldType::NestedArray { type_hash, len: N },
            other => FieldType::Opaque {
                wire_size: other.wire_size().unwrap_or(0).saturating_mul(N),
            },
        }
    }
}

impl<T> SchemaFieldType for T
where
    T: DataPod,
{
    fn field_type() -> FieldType {
        FieldType::Nested {
            type_hash: crate::bind::type_hash::<T>(),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SchemaField {
    pub name: &'static str,
    pub role: FieldRole,
    /// Stable v1 little-endian header byte offset. Payload fields use zero.
    pub offset: usize,
    pub ty: FieldType,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SchemaDescriptor {
    pub canonical_name: String,
    pub type_hash: u64,
    pub schema_hash: u64,
    pub header_size: usize,
    pub payload_kind: registry::PayloadKind,
    pub fields: Vec<SchemaField>,
}

pub trait DataPodSchema: DataPod {
    fn schema_fields() -> Vec<SchemaField>;
}

pub fn describe<T>(canonical_name: &'static str) -> SchemaDescriptor
where
    T: DataPod + DataPodSchema,
    T::Header: LeWireHeader,
{
    let type_hash = crate::bind::type_hash::<T>();
    let fields = T::schema_fields();
    SchemaDescriptor {
        canonical_name: canonical_name.to_string(),
        type_hash,
        schema_hash: schema_hash(
            canonical_name,
            type_hash,
            <T::Header as LeWireHeader>::LE_WIRE_SIZE,
            &fields,
        ),
        header_size: <T::Header as LeWireHeader>::LE_WIRE_SIZE,
        payload_kind: payload_kind::<T>(),
        fields,
    }
}

pub fn schema_hash(
    canonical_name: &str,
    type_hash: u64,
    header_size: usize,
    fields: &[SchemaField],
) -> u64 {
    let mut hash = crate::registry::type_hash_name(canonical_name);
    hash = mix(hash, type_hash);
    hash = mix(hash, header_size as u64);
    for field in fields {
        hash = mix_bytes(hash, field.name.as_bytes());
        hash = mix(hash, field.offset as u64);
        hash = mix(hash, field.role as u64);
        hash = mix(hash, field_type_hash(field.ty));
    }
    hash
}

fn mix(mut hash: u64, value: u64) -> u64 {
    for byte in value.to_le_bytes() {
        hash ^= u64::from(byte);
        hash = hash.wrapping_mul(0x00000100000001b3);
    }
    hash
}

fn mix_bytes(mut hash: u64, bytes: &[u8]) -> u64 {
    for byte in bytes {
        hash ^= u64::from(*byte);
        hash = hash.wrapping_mul(0x00000100000001b3);
    }
    hash
}

fn field_type_hash(ty: FieldType) -> u64 {
    match ty {
        FieldType::Scalar(scalar) => scalar as u64,
        FieldType::Array { element, len } => 100 + element as u64 + ((len as u64) << 16),
        FieldType::NestedArray { type_hash, len } => 150 ^ type_hash ^ ((len as u64) << 16),
        FieldType::Nested { type_hash } => 200 ^ type_hash,
        FieldType::Opaque { wire_size } => 250 ^ wire_size as u64,
        FieldType::PayloadSection => 300,
        FieldType::Bytes => 400,
        FieldType::BytesElements { type_hash } => 500 ^ type_hash,
    }
}

fn payload_kind<T: DataPod>() -> registry::PayloadKind {
    match core::any::type_name::<T::Payload>() {
        "()" => registry::PayloadKind::Fixed,
        _ => registry::PayloadKind::Bytes,
    }
}
