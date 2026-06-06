//! Double-ended queue Pod — holds any `T: bytemuck::Pod`.
//!
//! Layout: one byte buffer split at `split_byte` into a front half
//! (stored in reverse element order so `push_front` is an O(1) prepend
//! before that point) and a back half (natural order).

use crate::{DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Deque {
    pub element_size: u32,
    /// Byte offset where the back half starts. Bytes `[..split_byte]` are
    /// the front half in reverse element order; `[split_byte..]` are the
    /// back half in natural order.
    pub split_byte: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Deque {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    /// Fallible constructor that rejects element types too wide for the
    /// stable u32 wire header before truncating their size.
    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let deque = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            split_byte: 0,
            data: Vec::new(),
        };
        deque.validate_owned()?;
        Ok(deque)
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
        self.split_byte = 0;
    }

    pub fn push_front<T: bytemuck::Pod>(&mut self, value: T) {
        let _ = self.try_push_front(value);
    }

    pub fn try_push_front<T: bytemuck::Pod>(&mut self, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let new_split = self
            .split_byte
            .checked_add(self.element_size)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("split_byte overflowed u32"))?;
        let bytes = bytemuck::bytes_of(&value);
        self.data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("deque payload allocation failed: {err}"))
        })?;
        let insert_at = self.try_split_byte()?;
        self.data
            .splice(insert_at..insert_at, bytes.iter().copied());
        self.split_byte = new_split;
        Ok(())
    }

    pub fn push_back<T: bytemuck::Pod>(&mut self, value: T) {
        let _ = self.try_push_back(value);
    }

    pub fn try_push_back<T: bytemuck::Pod>(&mut self, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let bytes = bytemuck::bytes_of(&value);
        self.data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("deque payload allocation failed: {err}"))
        })?;
        self.data.extend_from_slice(bytemuck::bytes_of(&value));
        Ok(())
    }

    pub fn pop_front<T: bytemuck::Pod>(&mut self) -> Option<T> {
        self.try_pop_front().unwrap_or(None)
    }

    pub fn try_pop_front<T: bytemuck::Pod>(&mut self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        if self.try_empty()? {
            return Ok(None);
        }
        let es = self.try_element_size()?;
        let split = self.try_split_byte()?;
        if split >= es {
            let end = split;
            let start = end.checked_sub(es).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("front element start underflowed")
            })?;
            let bytes = self.data.get(start..end).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("front element range is out of bounds")
            })?;
            let value: T = bytemuck::pod_read_unaligned(bytes);
            self.data.drain(start..end);
            self.split_byte = self
                .split_byte
                .checked_sub(self.element_size)
                .ok_or_else(|| {
                    crate::wire::invalid_header::<Self>("split_byte underflowed after front pop")
                })?;
            Ok(Some(value))
        } else {
            let start = split;
            let end = start.checked_add(es).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("front element end overflowed")
            })?;
            let bytes = self.data.get(start..end).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("front element range is out of bounds")
            })?;
            let value: T = bytemuck::pod_read_unaligned(bytes);
            self.data.drain(start..end);
            Ok(Some(value))
        }
    }

    pub fn pop_back<T: bytemuck::Pod>(&mut self) -> Option<T> {
        self.try_pop_back().unwrap_or(None)
    }

    pub fn try_pop_back<T: bytemuck::Pod>(&mut self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        if self.try_empty()? {
            return Ok(None);
        }
        let es = self.try_element_size()?;
        let split = self.try_split_byte()?;
        if self.data.len() > split {
            let end = self.data.len();
            let start = end.checked_sub(es).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("back element start underflowed")
            })?;
            let bytes = self.data.get(start..end).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("back element range is out of bounds")
            })?;
            let value: T = bytemuck::pod_read_unaligned(bytes);
            self.data.truncate(start);
            Ok(Some(value))
        } else {
            let bytes = self.data.get(..es).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("back element range is out of bounds")
            })?;
            let value: T = bytemuck::pod_read_unaligned(bytes);
            self.data.drain(..es);
            self.split_byte = self
                .split_byte
                .checked_sub(self.element_size)
                .ok_or_else(|| {
                    crate::wire::invalid_header::<Self>("split_byte underflowed after back pop")
                })?;
            Ok(Some(value))
        }
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &DequeHeader {
                element_size: self.element_size,
                split_byte: self.split_byte,
            },
            &self.data,
        )
    }

    fn try_element_size(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.element_size, "element_size")
    }

    fn try_split_byte(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.split_byte, "split_byte")
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
