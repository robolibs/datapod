//! LIFO stack Pod — holds any `T: bytemuck::Pod`.
//!
//! Same wire shape as [`crate::seq::Vector`]; API restricted to LIFO ops.

use crate::{DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Stack {
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Stack {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    /// Fallible constructor that rejects element types too wide for the
    /// stable u32 wire header before truncating their size.
    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let stack = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            _pad: 0,
            data: Vec::new(),
        };
        stack.validate_owned()?;
        Ok(stack)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical element count for callers handling potentially
    /// malformed owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let element_size = self.try_element_size()?;
        if element_size == 0 {
            Ok(0)
        } else {
            Ok(self.data.len() / element_size)
        }
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    /// Fallible emptiness check for callers handling potentially malformed
    /// owned buffers.
    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    pub fn push<T: bytemuck::Pod>(&mut self, value: T) {
        let _ = self.try_push(value);
    }

    pub fn try_push<T: bytemuck::Pod>(&mut self, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let bytes = bytemuck::bytes_of(&value);
        self.data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("stack payload allocation failed: {err}"))
        })?;
        self.data.extend_from_slice(bytemuck::bytes_of(&value));
        Ok(())
    }

    pub fn pop<T: bytemuck::Pod>(&mut self) -> Option<T> {
        self.try_pop().unwrap_or(None)
    }

    pub fn try_pop<T: bytemuck::Pod>(&mut self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let es = self.try_element_size()?;
        if self.data.len() < es {
            return Ok(None);
        }
        let start = self.data.len().checked_sub(es).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("top element offset underflowed")
        })?;
        let bytes = self.data.get(start..).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("top element range is out of bounds")
        })?;
        let value: T = bytemuck::pod_read_unaligned(bytes);
        self.data.truncate(start);
        Ok(Some(value))
    }

    pub fn top<T: bytemuck::Pod>(&self) -> Option<T> {
        self.try_top().unwrap_or(None)
    }

    pub fn try_top<T: bytemuck::Pod>(&self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let es = self.try_element_size()?;
        if self.data.len() < es {
            return Ok(None);
        }
        let start = self.data.len().checked_sub(es).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("top element offset underflowed")
        })?;
        let bytes = self.data.get(start..).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("top element range is out of bounds")
        })?;
        Ok(Some(bytemuck::pod_read_unaligned(bytes)))
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &StackHeader {
                element_size: self.element_size,
                _pad: self._pad,
            },
            &self.data,
        )
    }

    fn try_element_size(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.element_size, "element_size")
    }
}

fn check_element_size<P: 'static, T>(stored: u32) -> Result<(), WireError> {
    let actual = std::mem::size_of::<T>();
    if actual == 0 {
        return Err(crate::wire::invalid_header::<P>(
            "zero-sized Pod elements cannot be represented in byte-counted datapod containers",
        ));
    }
    let stored = super::checked_u32_to_usize::<P>(stored, "element_size")?;
    if actual != stored {
        return Err(crate::wire::invalid_header::<P>(format!(
            "element size mismatch: T is {actual}, container expects {stored}"
        )));
    }
    Ok(())
}
