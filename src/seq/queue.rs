//! FIFO queue Pod — holds any `T: bytemuck::Pod`.
//!
//! Stored as a contiguous element buffer + a logical `front` index that
//! tracks how many leading slots have been popped. The buffer compacts
//! lazily when the front pointer crosses the half-way mark.

use crate::{DataPodValidate, WireError};

/// FIFO ordering alias.
pub type Fifo = Queue;

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Queue {
    pub element_size: u32,
    /// Number of leading elements that have been logically popped.
    pub front: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Queue {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    /// Fallible constructor that rejects element types too wide for the
    /// stable u32 wire header before truncating their size.
    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let queue = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            front: 0,
            data: Vec::new(),
        };
        queue.validate_owned()?;
        Ok(queue)
    }

    fn try_element_size(&self) -> Result<usize, WireError> {
        u32_to_usize::<Self>(self.element_size, "element_size")
    }

    fn try_front_usize(&self) -> Result<usize, WireError> {
        u32_to_usize::<Self>(self.front, "front")
    }

    fn try_raw_len(&self) -> Result<usize, WireError> {
        let es = self.try_element_size()?;
        if es == 0 {
            Ok(0)
        } else {
            Ok(self.data.len() / es)
        }
    }

    fn raw_len(&self) -> usize {
        self.try_raw_len().unwrap_or(0)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical element count for callers handling potentially
    /// malformed owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let raw = self.try_raw_len()?;
        raw.checked_sub(self.try_front_usize()?)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("queue front exceeds raw length"))
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
        self.front = 0;
    }

    pub fn push_back<T: bytemuck::Pod>(&mut self, value: T) {
        let _ = self.try_push_back(value);
    }

    pub fn try_push_back<T: bytemuck::Pod>(&mut self, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let bytes = bytemuck::bytes_of(&value);
        self.data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("queue payload allocation failed: {err}"))
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
        let byte_start = self
            .try_front_usize()?
            .checked_mul(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("front byte offset overflowed"))?;
        let byte_end = byte_start
            .checked_add(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("front byte end overflowed"))?;
        let bytes = self.data.get(byte_start..byte_end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("front element range is out of bounds")
        })?;
        let value: T = bytemuck::pod_read_unaligned(bytes);
        self.front = self
            .front
            .checked_add(1)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("front index overflowed u32"))?;
        self.compact_if_needed();
        Ok(Some(value))
    }

    pub fn front_elem<T: bytemuck::Pod>(&self) -> Option<T> {
        self.try_front_elem().unwrap_or(None)
    }

    pub fn try_front_elem<T: bytemuck::Pod>(&self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        if self.try_empty()? {
            return Ok(None);
        }
        let es = self.try_element_size()?;
        let byte_start = self
            .try_front_usize()?
            .checked_mul(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("front byte offset overflowed"))?;
        let byte_end = byte_start
            .checked_add(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("front byte end overflowed"))?;
        let bytes = self.data.get(byte_start..byte_end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("front element range is out of bounds")
        })?;
        Ok(Some(bytemuck::pod_read_unaligned(bytes)))
    }

    pub fn back_elem<T: bytemuck::Pod>(&self) -> Option<T> {
        self.try_back_elem().unwrap_or(None)
    }

    pub fn try_back_elem<T: bytemuck::Pod>(&self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        if self.try_empty()? {
            return Ok(None);
        }
        let es = self.try_element_size()?;
        let byte_end = self.data.len();
        let byte_start = byte_end.checked_sub(es).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("back element start underflowed")
        })?;
        let bytes = self.data.get(byte_start..byte_end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("back element range is out of bounds")
        })?;
        Ok(Some(bytemuck::pod_read_unaligned(bytes)))
    }

    fn compact_if_needed(&mut self) {
        let raw = self.raw_len();
        let Ok(front) = self.try_front_usize() else {
            return;
        };
        let Some(front_twice) = front.checked_mul(2) else {
            return;
        };
        if raw > 0 && front_twice >= raw {
            let Ok(es) = self.try_element_size() else {
                return;
            };
            let Some(drop_bytes) = front.checked_mul(es) else {
                return;
            };
            if drop_bytes > self.data.len() {
                return;
            }
            self.data.drain(..drop_bytes);
            self.front = 0;
        }
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &QueueHeader {
                element_size: self.element_size,
                front: self.front,
            },
            &self.data,
        )
    }
}

fn check_element_size<P: 'static, T>(stored: u32) -> Result<(), WireError> {
    let actual = std::mem::size_of::<T>();
    if actual == 0 {
        return Err(crate::wire::invalid_header::<P>(
            "zero-sized Pod elements cannot be represented in byte-counted datapod containers",
        ));
    }
    let stored = u32_to_usize::<P>(stored, "element_size")?;
    if actual != stored {
        return Err(crate::wire::invalid_header::<P>(format!(
            "element size mismatch: T is {actual}, container expects {stored}"
        )));
    }
    Ok(())
}

fn u32_to_usize<P: 'static>(value: u32, field: &'static str) -> Result<usize, WireError> {
    usize::try_from(value)
        .map_err(|_| crate::wire::invalid_header::<P>(format!("{field} does not fit in usize")))
}
