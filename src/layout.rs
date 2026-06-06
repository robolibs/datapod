//! Forward-only helpers for future multi-section datapod payloads.
//!
//! Current datapod wire bodies are `header || payload`. Some future schemas
//! need several payload regions while staying C/Python friendly. The supported
//! shape is:
//!
//! ```text
//! header_with_sections || section_0 || section_1 || ... || section_n
//! ```
//!
//! Each header field that points at a section uses [`PayloadSection`], which is
//! an offset/length pair into the single payload blob. This copies rkyv's
//! resolver idea while avoiding pointer-relative archived data.

use bytemuck::{Pod, Zeroable};

use crate::{LeWireHeader, WireError};

/// Offset/length pair for one section inside a datapod payload blob.
///
/// Offsets are relative to the start of the payload blob, not the start of the
/// whole wire body. The fields are fixed-width so this can be embedded in C ABI
/// headers and future stable little-endian headers.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq, Pod, Zeroable)]
pub struct PayloadSection {
    pub offset: u32,
    pub len: u32,
}

unsafe impl crate::ZeroCopySend for PayloadSection {}

impl crate::LeWireHeader for PayloadSection {
    const LE_WIRE_SIZE: usize = <u32 as LeWireHeader>::LE_WIRE_SIZE * 2;

    fn write_le(&self, out: &mut Vec<u8>) {
        self.offset.write_le(out);
        self.len.write_le(out);
    }

    fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
        if bytes.len() != Self::LE_WIRE_SIZE {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "little-endian payload section has {} bytes, expected {}",
                bytes.len(),
                Self::LE_WIRE_SIZE
            )));
        }
        let offset = bytes.get(..4).ok_or_else(|| {
            crate::wire::invalid_header::<Self>("payload section offset range is out of bounds")
        })?;
        let len = bytes.get(4..8).ok_or_else(|| {
            crate::wire::invalid_header::<Self>("payload section length range is out of bounds")
        })?;
        Ok(Self {
            offset: <u32 as LeWireHeader>::read_le(offset)?,
            len: <u32 as LeWireHeader>::read_le(len)?,
        })
    }
}

impl PayloadSection {
    pub const fn new(offset: u32, len: u32) -> Self {
        Self { offset, len }
    }

    pub const fn is_empty(self) -> bool {
        self.len == 0
    }

    pub fn end(self) -> Result<u32, WireError> {
        self.offset
            .checked_add(self.len)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("section end offset overflowed"))
    }

    pub fn range(self) -> Result<std::ops::Range<usize>, WireError> {
        let end = self.end()?;
        let start = section_u32_to_usize(self.offset, "section offset")?;
        let end = section_u32_to_usize(end, "section end")?;
        Ok(start..end)
    }
}

fn section_u32_to_usize(value: u32, field: &'static str) -> Result<usize, WireError> {
    usize::try_from(value).map_err(|_| {
        crate::wire::invalid_payload::<PayloadSection>(format!("{field} exceeds usize"))
    })
}

/// Validation policy for a list of [`PayloadSection`] entries.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SectionValidation {
    pub require_sorted: bool,
    pub require_non_overlapping: bool,
}

impl Default for SectionValidation {
    fn default() -> Self {
        Self {
            require_sorted: true,
            require_non_overlapping: true,
        }
    }
}

/// Forward-only payload-section builder.
///
/// Sections are appended in payload order. The returned [`PayloadSection`]
/// values can be copied into a header, and the final payload blob can be
/// appended after that header.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct PayloadLayoutBuilder {
    sections: Vec<PayloadSection>,
    payload: Vec<u8>,
}

impl PayloadLayoutBuilder {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_capacity(section_capacity: usize, payload_capacity: usize) -> Self {
        match Self::try_with_capacity(section_capacity, payload_capacity) {
            Ok(builder) => builder,
            Err(_) => Self::new(),
        }
    }

    pub fn try_with_capacity(
        section_capacity: usize,
        payload_capacity: usize,
    ) -> Result<Self, WireError> {
        let mut sections = Vec::new();
        sections
            .try_reserve_exact(section_capacity)
            .map_err(|error| {
                crate::wire::invalid_payload::<PayloadSection>(format!(
                    "failed to reserve {section_capacity} payload sections: {error}"
                ))
            })?;

        let mut payload = Vec::new();
        payload
            .try_reserve_exact(payload_capacity)
            .map_err(|error| {
                crate::wire::invalid_payload::<PayloadSection>(format!(
                    "failed to reserve {payload_capacity} payload bytes: {error}"
                ))
            })?;

        Ok(Self { sections, payload })
    }

    pub fn push_section(&mut self, bytes: &[u8]) -> Result<PayloadSection, WireError> {
        let offset = u32::try_from(self.payload.len())
            .map_err(|_| crate::wire::invalid_payload::<PayloadSection>("payload exceeds u32"))?;
        let len = u32::try_from(bytes.len())
            .map_err(|_| crate::wire::invalid_payload::<PayloadSection>("section exceeds u32"))?;
        let section = PayloadSection { offset, len };
        section.end()?;
        self.payload
            .try_reserve_exact(bytes.len())
            .map_err(|error| {
                crate::wire::invalid_payload::<PayloadSection>(format!(
                    "failed to reserve {} section payload bytes: {error}",
                    bytes.len()
                ))
            })?;
        self.sections.try_reserve_exact(1).map_err(|error| {
            crate::wire::invalid_payload::<PayloadSection>(format!(
                "failed to reserve one payload section: {error}"
            ))
        })?;
        self.payload.extend_from_slice(bytes);
        self.sections.push(section);
        Ok(section)
    }

    pub fn sections(&self) -> &[PayloadSection] {
        &self.sections
    }

    pub fn payload(&self) -> &[u8] {
        &self.payload
    }

    pub fn finish(self) -> (Vec<PayloadSection>, Vec<u8>) {
        (self.sections, self.payload)
    }
}

/// Validate sections with the default sorted/non-overlapping policy.
pub fn validate_sections(payload_len: usize, sections: &[PayloadSection]) -> Result<(), WireError> {
    validate_sections_with_policy(payload_len, sections, SectionValidation::default())
}

/// Validate that sections are in-bounds and satisfy `policy`.
pub fn validate_sections_with_policy(
    payload_len: usize,
    sections: &[PayloadSection],
    policy: SectionValidation,
) -> Result<(), WireError> {
    let mut previous_offset = 0usize;
    let mut previous_end = 0usize;
    for (index, section) in sections.iter().copied().enumerate() {
        let range = section.range()?;
        if range.end > payload_len {
            return Err(crate::wire::invalid_payload::<PayloadSection>(format!(
                "section {index} end {} exceeds payload length {payload_len}",
                range.end
            )));
        }
        if policy.require_sorted && range.start < previous_offset {
            return Err(crate::wire::invalid_payload::<PayloadSection>(
                "sections must be sorted by offset",
            ));
        }
        if policy.require_non_overlapping && policy.require_sorted && range.start < previous_end {
            return Err(crate::wire::invalid_payload::<PayloadSection>(
                "sections must not overlap",
            ));
        }
        previous_offset = range.start;
        previous_end = range.end;
    }

    if policy.require_non_overlapping && !policy.require_sorted {
        for (left_index, left) in sections.iter().copied().enumerate() {
            let left = left.range()?;
            for (right_index, right) in sections.iter().copied().enumerate().skip(left_index + 1) {
                let right = right.range()?;
                if left.start < right.end && right.start < left.end {
                    return Err(crate::wire::invalid_payload::<PayloadSection>(format!(
                        "sections {left_index} and {right_index} overlap"
                    )));
                }
            }
        }
    }

    Ok(())
}

/// Borrow one section from a payload blob after validating its bounds.
pub fn section_bytes(payload: &[u8], section: PayloadSection) -> Result<&[u8], WireError> {
    let range = section.range()?;
    payload.get(range).ok_or_else(|| {
        crate::wire::invalid_payload::<PayloadSection>("section is out of payload bounds")
    })
}
