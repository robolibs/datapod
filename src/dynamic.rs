//! Zero-copy dynamic views over registered datapod wire messages.
//!
//! This module does not decode owned datapod values. It borrows the incoming
//! header/payload bytes and reads fields by schema offset.

use crate::schema::{FieldRole, FieldType, ScalarType, SchemaDescriptor, SchemaField};
use crate::{WireError, WireFrame, split_wire_parts, validate_registered_wire_frame};

#[derive(Debug, Clone, PartialEq)]
pub enum DynamicValue<'a> {
    U8(u8),
    U16(u16),
    U32(u32),
    U64(u64),
    U128(u128),
    I8(i8),
    I16(i16),
    I32(i32),
    I64(i64),
    I128(i128),
    F32(f32),
    F64(f64),
    Bool(bool),
    Bytes(&'a [u8]),
    ArrayBytes {
        element: ScalarType,
        len: usize,
        bytes: &'a [u8],
    },
    Nested(DynamicView<'a>),
}

impl DynamicValue<'_> {
    pub fn as_u32(&self) -> Result<u32, WireError> {
        match self {
            Self::U32(value) => Ok(*value),
            other => Err(invalid_dynamic(format!("expected u32, got {other:?}"))),
        }
    }

    pub fn as_f64(&self) -> Result<f64, WireError> {
        match self {
            Self::F64(value) => Ok(*value),
            other => Err(invalid_dynamic(format!("expected f64, got {other:?}"))),
        }
    }

    pub fn as_bool(&self) -> Result<bool, WireError> {
        match self {
            Self::Bool(value) => Ok(*value),
            other => Err(invalid_dynamic(format!("expected bool, got {other:?}"))),
        }
    }

    pub fn as_bytes(&self) -> Result<&[u8], WireError> {
        match self {
            Self::Bytes(value) => Ok(value),
            Self::ArrayBytes { bytes, .. } => Ok(bytes),
            other => Err(invalid_dynamic(format!("expected bytes, got {other:?}"))),
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct DynamicView<'a> {
    schema: SchemaDescriptor,
    header: &'a [u8],
    payload: &'a [u8],
}

impl<'a> DynamicView<'a> {
    pub fn schema(&self) -> &SchemaDescriptor {
        &self.schema
    }

    pub fn type_hash(&self) -> u64 {
        self.schema.type_hash
    }

    pub fn header(&self) -> &'a [u8] {
        self.header
    }

    pub fn payload(&self) -> &'a [u8] {
        self.payload
    }

    pub fn field(&self, name: &str) -> Result<DynamicValue<'a>, WireError> {
        let field = self
            .schema
            .fields
            .iter()
            .find(|field| field.name == name)
            .ok_or_else(|| invalid_dynamic(format!("unknown datapod field {name:?}")))?;
        self.field_value(field)
    }

    pub fn get_u32(&self, name: &str) -> Result<u32, WireError> {
        self.field(name)?.as_u32()
    }

    pub fn get_f64(&self, name: &str) -> Result<f64, WireError> {
        self.field(name)?.as_f64()
    }

    fn field_value(&self, field: &SchemaField) -> Result<DynamicValue<'a>, WireError> {
        match field.role {
            FieldRole::Payload => return Ok(DynamicValue::Bytes(self.payload)),
            FieldRole::Header => {}
        }

        match field.ty {
            FieldType::Scalar(scalar) => {
                let bytes = self.header_field(field.offset, scalar.wire_size())?;
                scalar_value(scalar, bytes)
            }
            FieldType::Array { element, len } => {
                let size = element.wire_size().checked_mul(len).ok_or_else(|| {
                    invalid_dynamic(format!("array field {} byte length overflows", field.name))
                })?;
                let bytes = self.header_field(field.offset, size)?;
                Ok(DynamicValue::ArrayBytes {
                    element,
                    len,
                    bytes,
                })
            }
            FieldType::NestedArray { type_hash, len } => {
                let Some(schema) = crate::registry::find_schema(type_hash) else {
                    return Err(WireError::UnknownTypeHash { type_hash });
                };
                let size = schema.header_size.checked_mul(len).ok_or_else(|| {
                    invalid_dynamic(format!(
                        "nested array field {} byte length overflows",
                        field.name
                    ))
                })?;
                let bytes = self.header_field(field.offset, size)?;
                Ok(DynamicValue::ArrayBytes {
                    element: ScalarType::U8,
                    len: bytes.len(),
                    bytes,
                })
            }
            FieldType::Nested { type_hash } => {
                let Some(schema) = crate::registry::find_schema(type_hash) else {
                    return Err(WireError::UnknownTypeHash { type_hash });
                };
                let bytes = self.header_field(field.offset, schema.header_size)?;
                Ok(DynamicValue::Nested(DynamicView {
                    schema,
                    header: bytes,
                    payload: &[],
                }))
            }
            FieldType::PayloadSection => {
                let bytes = self.header_field(field.offset, 8)?;
                Ok(DynamicValue::ArrayBytes {
                    element: ScalarType::U32,
                    len: 2,
                    bytes,
                })
            }
            FieldType::Opaque { wire_size } => {
                let bytes = self.header_field(field.offset, wire_size)?;
                Ok(DynamicValue::Bytes(bytes))
            }
            FieldType::Bytes => Ok(DynamicValue::Bytes(self.payload)),
        }
    }

    fn header_field(&self, offset: usize, len: usize) -> Result<&'a [u8], WireError> {
        let end = offset
            .checked_add(len)
            .ok_or_else(|| invalid_dynamic("dynamic field offset overflows"))?;
        self.header.get(offset..end).ok_or(WireError::ShortHeader {
            type_name: "dynamic datapod view",
            needed: end,
            got: self.header.len(),
        })
    }
}

pub fn view_message(type_hash: u64, bytes: &[u8]) -> Result<DynamicView<'_>, WireError> {
    let parts = split_wire_parts(type_hash, bytes)?;
    view_frame(WireFrame {
        type_hash: parts.type_hash,
        header: parts.header,
        payload: parts.payload,
    })
}

pub fn view_frame(frame: WireFrame<'_>) -> Result<DynamicView<'_>, WireError> {
    validate_registered_wire_frame(frame)?;
    let Some(schema) = crate::registry::find_schema(frame.type_hash) else {
        return Err(WireError::UnknownTypeHash {
            type_hash: frame.type_hash,
        });
    };
    Ok(DynamicView {
        schema,
        header: frame.header,
        payload: frame.payload,
    })
}

fn scalar_value(scalar: ScalarType, bytes: &[u8]) -> Result<DynamicValue<'_>, WireError> {
    match scalar {
        ScalarType::U8 => Ok(DynamicValue::U8(read_u8(bytes)?)),
        ScalarType::U16 => Ok(DynamicValue::U16(read_int(bytes, u16::from_le_bytes)?)),
        ScalarType::U32 => Ok(DynamicValue::U32(read_int(bytes, u32::from_le_bytes)?)),
        ScalarType::U64 => Ok(DynamicValue::U64(read_int(bytes, u64::from_le_bytes)?)),
        ScalarType::U128 => Ok(DynamicValue::U128(read_int(bytes, u128::from_le_bytes)?)),
        ScalarType::I8 => Ok(DynamicValue::I8(read_u8(bytes)? as i8)),
        ScalarType::I16 => Ok(DynamicValue::I16(read_int(bytes, i16::from_le_bytes)?)),
        ScalarType::I32 => Ok(DynamicValue::I32(read_int(bytes, i32::from_le_bytes)?)),
        ScalarType::I64 => Ok(DynamicValue::I64(read_int(bytes, i64::from_le_bytes)?)),
        ScalarType::I128 => Ok(DynamicValue::I128(read_int(bytes, i128::from_le_bytes)?)),
        ScalarType::F32 => Ok(DynamicValue::F32(f32::from_bits(read_int(
            bytes,
            u32::from_le_bytes,
        )?))),
        ScalarType::F64 => Ok(DynamicValue::F64(f64::from_bits(read_int(
            bytes,
            u64::from_le_bytes,
        )?))),
        ScalarType::Bool => match read_u8(bytes)? {
            0 => Ok(DynamicValue::Bool(false)),
            1 => Ok(DynamicValue::Bool(true)),
            other => Err(invalid_dynamic(format!("invalid bool wire value {other}"))),
        },
    }
}

fn read_u8(bytes: &[u8]) -> Result<u8, WireError> {
    bytes
        .first()
        .copied()
        .ok_or_else(|| invalid_dynamic("short scalar field"))
}

fn read_int<const N: usize, T>(
    bytes: &[u8],
    from_le_bytes: impl FnOnce([u8; N]) -> T,
) -> Result<T, WireError> {
    let array: [u8; N] = bytes.try_into().map_err(|_| {
        invalid_dynamic(format!(
            "scalar field has {} bytes, expected {N}",
            bytes.len()
        ))
    })?;
    Ok(from_le_bytes(array))
}

fn invalid_dynamic(message: impl Into<String>) -> WireError {
    WireError::InvalidHeader {
        type_name: "dynamic datapod view",
        message: message.into(),
    }
}
