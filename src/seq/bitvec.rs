//! Bit-packed boolean vector Pod.
//!
//! `bool` is NOT `bytemuck::Pod` (invalid bit patterns exist), so the wire
//! payload is `Vec<u8>` with one bit per logical entry. The exact bit count
//! rides in the header — bytes alone aren't enough because the last byte
//! has trailing-bit slack.

use crate::{DataPodAccess, DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct BitVec {
    /// Logical bit length. Bytes are `ceil(bits / 8)`.
    pub bits: u64,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl BitVec {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_len(bits: usize) -> Self {
        Self::try_with_len(bits).unwrap_or_else(|_| Self::new())
    }

    pub fn try_with_len(bits: usize) -> Result<Self, WireError> {
        let bits_u64 = u64::try_from(bits)
            .map_err(|_| crate::wire::invalid_payload::<Self>("bit length exceeds u64"))?;
        let bytes = bit_payload_len::<Self>(bits_u64)?;
        let mut data = Vec::new();
        data.try_reserve_exact(bytes).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("bit payload allocation failed: {err}"))
        })?;
        data.resize(bytes, 0);
        let bitvec = Self {
            bits: bits_u64,
            data,
        };
        bitvec.validate_owned()?;
        Ok(bitvec)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical bit count for callers handling potentially malformed
    /// owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        usize::try_from(self.bits)
            .map_err(|_| crate::wire::invalid_payload::<Self>("bit length exceeds usize"))
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    /// Fallible emptiness check for callers handling potentially malformed
    /// owned buffers.
    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    pub fn test(&self, i: usize) -> bool {
        self.try_test(i).unwrap_or(false)
    }

    pub fn try_test(&self, i: usize) -> Result<bool, WireError> {
        self.validate_owned()?;
        let len = self.try_size()?;
        if i >= len {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "bit index out of bounds: {i} for len {}",
                self.bits
            )));
        }
        let byte = self.data.get(i >> 3).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bit byte range is out of bounds")
        })?;
        Ok((*byte >> (i & 7)) & 1 != 0)
    }

    pub fn set_bit(&mut self, i: usize, value: bool) {
        let _ = self.try_set_bit(i, value);
    }

    pub fn try_set_bit(&mut self, i: usize, value: bool) -> Result<(), WireError> {
        self.validate_owned()?;
        let len = self.try_size()?;
        if i >= len {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "bit index out of bounds: {i} for len {}",
                self.bits
            )));
        }
        let mask = 1u8 << (i & 7);
        let byte = self.data.get_mut(i >> 3).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bit byte range is out of bounds")
        })?;
        if value {
            *byte |= mask;
        } else {
            *byte &= !mask;
        }
        Ok(())
    }

    pub fn flip(&mut self, i: usize) {
        let _ = self.try_flip(i);
    }

    pub fn try_flip(&mut self, i: usize) -> Result<(), WireError> {
        self.validate_owned()?;
        let len = self.try_size()?;
        if i >= len {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "bit index out of bounds: {i} for len {}",
                self.bits
            )));
        }
        let byte = self.data.get_mut(i >> 3).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bit byte range is out of bounds")
        })?;
        *byte ^= 1u8 << (i & 7);
        Ok(())
    }

    pub fn push(&mut self, value: bool) {
        let _ = self.try_push(value);
    }

    pub fn try_push(&mut self, value: bool) -> Result<(), WireError> {
        self.validate_owned()?;
        let bit_idx = self.try_size()?;
        if bit_idx % 8 == 0 {
            self.data.try_reserve_exact(1).map_err(|err| {
                crate::wire::invalid_payload::<Self>(format!(
                    "bit payload allocation failed: {err}"
                ))
            })?;
            self.data.push(0);
        }
        self.bits = self
            .bits
            .checked_add(1)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("bit length overflowed u64"))?;
        self.try_set_bit(bit_idx, value)
    }

    pub fn pop(&mut self) -> Option<bool> {
        self.try_pop().unwrap_or(None)
    }

    pub fn try_pop(&mut self) -> Result<Option<bool>, WireError> {
        self.validate_owned()?;
        if self.bits == 0 {
            return Ok(None);
        }
        let last = self.try_size()?.checked_sub(1).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bit length underflowed before pop")
        })?;
        let v = self.try_test(last)?;
        self.bits = self.bits.checked_sub(1).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bit length underflowed during pop")
        })?;
        let new_bits = usize::try_from(self.bits)
            .map_err(|_| crate::wire::invalid_payload::<Self>("bit length exceeds usize"))?;
        // Clear the now-dead bit so the byte representation is stable.
        let mask = 1u8 << (new_bits & 7);
        let byte = self.data.get_mut(new_bits >> 3).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bit byte range is out of bounds")
        })?;
        *byte &= !mask;
        if new_bits % 8 == 0 {
            self.data.pop();
        }
        Ok(Some(v))
    }

    pub fn count_ones(&self) -> usize {
        self.try_count_ones().unwrap_or(0)
    }

    pub fn try_count_ones(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let len = self.try_size()?;
        let full_bytes = len / 8;
        let full = self.data.get(..full_bytes).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bit byte range is out of bounds")
        })?;
        let mut total = count_bit_ones::<Self>(full)?;
        let tail_bits = len % 8;
        if tail_bits > 0 {
            let mask = (1u8 << tail_bits) - 1;
            let tail = self.data.get(full_bytes).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("bit tail byte range is out of bounds")
            })?;
            total = total
                .checked_add(byte_count_ones::<Self>(*tail & mask)?)
                .ok_or_else(|| crate::wire::invalid_payload::<Self>("bit count overflowed"))?;
        }
        Ok(total)
    }

    pub fn any(&self) -> bool {
        self.count_ones() > 0
    }

    pub fn none(&self) -> bool {
        self.count_ones() == 0
    }

    pub fn zero_out(&mut self) {
        if self.validate_owned().is_err() {
            return;
        }
        for b in self.data.iter_mut() {
            *b = 0;
        }
    }

    pub fn one_out(&mut self) {
        let _ = self.try_one_out();
    }

    pub fn try_one_out(&mut self) -> Result<(), WireError> {
        self.validate_owned()?;
        for b in self.data.iter_mut() {
            *b = 0xFF;
        }
        // Mask trailing slack bits in the last byte so the canonical
        // representation matches `count_ones() == bits`.
        let len = self.try_size()?;
        let tail_bits = len % 8;
        if tail_bits > 0 {
            let last = self.data.len().checked_sub(1).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("bit tail index underflowed")
            })?;
            let byte = self.data.get_mut(last).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("bit byte range is out of bounds")
            })?;
            *byte &= (1u8 << tail_bits) - 1;
        }
        Ok(())
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &BitVecHeader { bits: self.bits },
            &self.data,
        )
    }
}

/// Borrowed, validation-backed view over a bit-packed `BitVec` payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BitVecView<'a> {
    pub header: BitVecHeader,
    pub data: &'a [u8],
}

impl<'a> BitVecView<'a> {
    pub fn bits(&self) -> u64 {
        self.header.bits
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        BitVec::validate_wire_parts(&self.header, self.data)?;
        usize::try_from(self.header.bits)
            .map_err(|_| crate::wire::invalid_payload::<BitVec>("bit length exceeds usize"))
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn test(&self, index: usize) -> Result<bool, WireError> {
        BitVec::validate_wire_parts(&self.header, self.data)?;
        let len = self.try_size()?;
        if index >= len {
            return Err(crate::wire::invalid_header::<BitVec>(format!(
                "bit index out of bounds: {index} for len {}",
                self.header.bits
            )));
        }
        let byte = self.data.get(index >> 3).ok_or_else(|| {
            crate::wire::invalid_payload::<BitVec>("bit byte range is out of bounds")
        })?;
        Ok((*byte >> (index & 7)) & 1 != 0)
    }

    pub fn count_ones(&self) -> usize {
        self.try_count_ones().unwrap_or(0)
    }

    pub fn try_count_ones(&self) -> Result<usize, WireError> {
        BitVec::validate_wire_parts(&self.header, self.data)?;
        let len = self.try_size()?;
        let full_bytes = len / 8;
        let full = self.data.get(..full_bytes).ok_or_else(|| {
            crate::wire::invalid_payload::<BitVec>("bit byte range is out of bounds")
        })?;
        let mut total = count_bit_ones::<BitVec>(full)?;
        let tail_bits = len % 8;
        if tail_bits > 0 {
            let mask = (1u8 << tail_bits) - 1;
            let tail = self.data.get(full_bytes).ok_or_else(|| {
                crate::wire::invalid_payload::<BitVec>("bit tail byte range is out of bounds")
            })?;
            total = total
                .checked_add(byte_count_ones::<BitVec>(*tail & mask)?)
                .ok_or_else(|| crate::wire::invalid_payload::<BitVec>("bit count overflowed"))?;
        }
        Ok(total)
    }
}

impl DataPodValidate for BitVec {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        let expected = bit_payload_len::<Self>(header.bits)?;
        if payload.len() != expected {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "got {} bytes, expected {expected}",
                payload.len()
            )));
        }
        let tail_bits = bit_tail_bits::<Self>(header.bits)?;
        if tail_bits != 0 && !payload.is_empty() {
            let slack_mask = !((1u8 << tail_bits) - 1);
            let tail_index = payload.len().checked_sub(1).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("bit tail index underflowed")
            })?;
            let tail = payload.get(tail_index).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("bit tail byte range is out of bounds")
            })?;
            if *tail & slack_mask != 0 {
                return Err(crate::wire::invalid_payload::<Self>(
                    "trailing slack bits must be zero",
                ));
            }
        }
        Ok(())
    }
}

impl DataPodAccess for BitVec {
    type View<'a> = BitVecView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(BitVecView {
            header,
            data: payload,
        })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        BitVecView {
            header,
            data: payload,
        }
    }
}

fn bit_payload_len<T: 'static>(bits: u64) -> Result<usize, WireError> {
    let bytes = bits
        .checked_add(7)
        .ok_or_else(|| crate::wire::invalid_payload::<T>("bit length overflowed"))?
        / 8;
    usize::try_from(bytes)
        .map_err(|_| crate::wire::invalid_payload::<T>("bit payload length exceeds usize"))
}

fn bit_tail_bits<T: 'static>(bits: u64) -> Result<usize, WireError> {
    usize::try_from(bits % 8)
        .map_err(|_| crate::wire::invalid_payload::<T>("bit tail width exceeds usize"))
}

fn byte_count_ones<T: 'static>(byte: u8) -> Result<usize, WireError> {
    usize::try_from(byte.count_ones())
        .map_err(|_| crate::wire::invalid_payload::<T>("bit count exceeds usize"))
}

fn count_bit_ones<T: 'static>(bytes: &[u8]) -> Result<usize, WireError> {
    bytes.iter().try_fold(0usize, |total, byte| {
        total
            .checked_add(byte_count_ones::<T>(*byte)?)
            .ok_or_else(|| crate::wire::invalid_payload::<T>("bit count overflowed"))
    })
}
