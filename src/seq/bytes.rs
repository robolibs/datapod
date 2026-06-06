//! Raw byte buffer Pod.
//!
//! Mirrors the wire shape of `datapod::Bytes` from the C++ library, but in
//! POD form: the bytes themselves ride on the slice payload, no metadata.

use crate::{DataPodAccess, DataPodValidate, WireError};

/// Sentinel returned by `find` / `rfind` when no match is present.
pub const BYTES_NPOS: usize = usize::MAX;

/// Heap-bearing byte buffer. `data` is the wire payload, no header fields.
#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Bytes {
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Bytes {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn from_slice(slice: &[u8]) -> Self {
        Self::try_from_slice(slice).unwrap_or_default()
    }

    pub fn try_from_slice(slice: &[u8]) -> Result<Self, WireError> {
        let mut data = Vec::new();
        data.try_reserve_exact(slice.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {} bytes: {err}",
                slice.len()
            ))
        })?;
        data.extend_from_slice(slice);
        Ok(Self { data })
    }

    pub fn size(&self) -> usize {
        self.data.len()
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn as_slice(&self) -> &[u8] {
        &self.data
    }

    pub fn as_mut_slice(&mut self) -> &mut [u8] {
        &mut self.data
    }

    pub fn append(&mut self, slice: &[u8]) {
        let _ = self.try_append(slice);
    }

    pub fn try_append(&mut self, slice: &[u8]) -> Result<(), WireError> {
        self.data.try_reserve_exact(slice.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {} appended bytes: {err}",
                slice.len()
            ))
        })?;
        self.data.extend_from_slice(slice);
        Ok(())
    }

    pub fn fill_byte(&mut self, value: u8) {
        for b in self.data.iter_mut() {
            *b = value;
        }
    }

    pub fn zero(&mut self) {
        self.fill_byte(0);
    }

    pub fn find(&self, needle: u8, from: usize) -> usize {
        if from >= self.data.len() {
            return BYTES_NPOS;
        }
        let Some(hay) = self.data.get(from..) else {
            return BYTES_NPOS;
        };
        hay.iter()
            .position(|b| *b == needle)
            .map(|i| i + from)
            .unwrap_or(BYTES_NPOS)
    }

    pub fn rfind(&self, needle: u8, from: usize) -> usize {
        if self.data.is_empty() {
            return BYTES_NPOS;
        }
        let Some(last) = self.data.len().checked_sub(1) else {
            return BYTES_NPOS;
        };
        let upper = from.min(last);
        for i in (0..=upper).rev() {
            let Some(byte) = self.data.get(i) else {
                return BYTES_NPOS;
            };
            if *byte == needle {
                return i;
            }
        }
        BYTES_NPOS
    }

    pub fn contains(&self, needle: u8) -> bool {
        self.data.contains(&needle)
    }

    pub fn starts_with(&self, prefix: &[u8]) -> bool {
        self.data.starts_with(prefix)
    }

    pub fn ends_with(&self, suffix: &[u8]) -> bool {
        self.data.ends_with(suffix)
    }

    pub fn substr(&self, pos: usize, count: usize) -> Vec<u8> {
        self.try_substr(pos, count).unwrap_or_default()
    }

    pub fn try_substr(&self, pos: usize, count: usize) -> Result<Vec<u8>, WireError> {
        if pos >= self.data.len() {
            return Ok(Vec::new());
        }
        let end = pos
            .checked_add(count)
            .unwrap_or(self.data.len())
            .min(self.data.len());
        let slice = self.data.get(pos..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("substring range is out of bounds")
        })?;
        let mut out = Vec::new();
        out.try_reserve_exact(slice.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {} substring bytes: {err}",
                slice.len()
            ))
        })?;
        out.extend_from_slice(slice);
        Ok(out)
    }
}

/// Borrowed view over `Bytes` wire payload.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BytesView<'a> {
    pub data: &'a [u8],
}

impl<'a> BytesView<'a> {
    pub fn as_slice(&self) -> &'a [u8] {
        self.data
    }

    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }
}

impl DataPodValidate for Bytes {
    fn validate_wire_parts(_header: &Self::Header, _payload: &[u8]) -> Result<(), WireError> {
        Ok(())
    }
}

impl DataPodAccess for Bytes {
    type View<'a> = BytesView<'a>;

    fn access_wire_parts<'a>(
        _header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Ok(BytesView { data: payload })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        _header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        BytesView { data: payload }
    }
}
