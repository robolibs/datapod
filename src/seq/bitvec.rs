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
        let bytes = (bits + 7) / 8;
        Self {
            bits: bits as u64,
            data: vec![0u8; bytes],
        }
    }

    pub fn size(&self) -> usize {
        self.bits as usize
    }

    pub fn empty(&self) -> bool {
        self.bits == 0
    }

    pub fn test(&self, i: usize) -> bool {
        debug_assert!(i < self.bits as usize, "BitVec::test out of range");
        (self.data[i >> 3] >> (i & 7)) & 1 != 0
    }

    pub fn set_bit(&mut self, i: usize, value: bool) {
        debug_assert!(i < self.bits as usize, "BitVec::set_bit out of range");
        let mask = 1u8 << (i & 7);
        if value {
            self.data[i >> 3] |= mask;
        } else {
            self.data[i >> 3] &= !mask;
        }
    }

    pub fn flip(&mut self, i: usize) {
        debug_assert!(i < self.bits as usize, "BitVec::flip out of range");
        self.data[i >> 3] ^= 1u8 << (i & 7);
    }

    pub fn push(&mut self, value: bool) {
        let bit_idx = self.bits as usize;
        if bit_idx % 8 == 0 {
            self.data.push(0);
        }
        self.bits += 1;
        self.set_bit(bit_idx, value);
    }

    pub fn pop(&mut self) -> Option<bool> {
        if self.bits == 0 {
            return None;
        }
        self.bits -= 1;
        let v = self.test(self.bits as usize);
        // Clear the now-dead bit so the byte representation is stable.
        let mask = 1u8 << (self.bits as usize & 7);
        self.data[(self.bits as usize) >> 3] &= !mask;
        if self.bits as usize % 8 == 0 {
            self.data.pop();
        }
        Some(v)
    }

    pub fn count_ones(&self) -> usize {
        let full_bytes = (self.bits / 8) as usize;
        let mut total: usize = self.data[..full_bytes]
            .iter()
            .map(|b| b.count_ones() as usize)
            .sum();
        let tail_bits = (self.bits % 8) as usize;
        if tail_bits > 0 {
            let mask = (1u8 << tail_bits) - 1;
            total += (self.data[full_bytes] & mask).count_ones() as usize;
        }
        total
    }

    pub fn any(&self) -> bool {
        self.count_ones() > 0
    }

    pub fn none(&self) -> bool {
        self.count_ones() == 0
    }

    pub fn zero_out(&mut self) {
        for b in self.data.iter_mut() {
            *b = 0;
        }
    }

    pub fn one_out(&mut self) {
        for b in self.data.iter_mut() {
            *b = 0xFF;
        }
        // Mask trailing slack bits in the last byte so the canonical
        // representation matches `count_ones() == bits`.
        let tail_bits = (self.bits % 8) as usize;
        if tail_bits > 0 {
            let last = (self.data.len()) - 1;
            self.data[last] &= (1u8 << tail_bits) - 1;
        }
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
        self.header.bits as usize
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn test(&self, index: usize) -> Result<bool, WireError> {
        if index >= self.header.bits as usize {
            return Err(crate::wire::invalid_header::<BitVec>(format!(
                "bit index out of bounds: {index} for len {}",
                self.header.bits
            )));
        }
        Ok((self.data[index >> 3] >> (index & 7)) & 1 != 0)
    }

    pub fn count_ones(&self) -> usize {
        self.data.iter().map(|b| b.count_ones() as usize).sum()
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
        let tail_bits = (header.bits % 8) as usize;
        if tail_bits != 0 && !payload.is_empty() {
            let slack_mask = !((1u8 << tail_bits) - 1);
            if payload[payload.len() - 1] & slack_mask != 0 {
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
