//! Bit-packed boolean vector Pod.
//!
//! `bool` is NOT `bytemuck::Pod` (invalid bit patterns exist), so the wire
//! payload is `Vec<u8>` with one bit per logical entry. The exact bit count
//! rides in the header — bytes alone aren't enough because the last byte
//! has trailing-bit slack.

#[datapod::datapod]
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
