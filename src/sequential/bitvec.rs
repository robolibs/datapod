//! Bit vector matching `datapod::Bitvec` (template `BasicBitvec`).
//!
//! The C++ type is a block-storage bit vector providing test/set/flip,
//! popcount, next-set-bit, and bitwise operations. The Rust surface keeps
//! the alias `BitVec = Vec<bool>` so that existing code continues to work,
//! and adds the C++-style API via an extension trait that operates on
//! `Vec<bool>` directly.

pub type BitVec = Vec<bool>;

/// C++-style bit-vector API for `Vec<bool>`.
pub trait BitVecExt {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn test(&self, i: usize) -> bool;
    fn set_bit(&mut self, i: usize, value: bool);
    fn flip(&mut self, i: usize);
    fn flip_all(&mut self);
    fn count(&self) -> usize;
    fn any(&self) -> bool;
    fn none(&self) -> bool;
    fn push_back_bit(&mut self, value: bool);
    fn pop_back_bit(&mut self);
    fn resize_bits(&mut self, new_size: usize);
    fn zero_out(&mut self);
    fn one_out(&mut self);
    fn next_set_bit(&self, start: usize) -> Option<usize>;
    fn for_each_set_bit<F: FnMut(usize)>(&self, f: F);
    fn str(&self) -> String;
    fn set_from_str(&mut self, s: &str);
    fn bit_and_assign(&mut self, other: &Self);
    fn bit_or_assign(&mut self, other: &Self);
    fn bit_xor_assign(&mut self, other: &Self);
    fn bit_not(&self) -> BitVec;
}

impl BitVecExt for Vec<bool> {
    #[inline]
    fn size(&self) -> usize {
        self.len()
    }

    #[inline]
    fn empty(&self) -> bool {
        self.is_empty()
    }

    #[inline]
    fn test(&self, i: usize) -> bool {
        if i >= self.len() {
            return false;
        }
        self[i]
    }

    #[inline]
    fn set_bit(&mut self, i: usize, value: bool) {
        if i < self.len() {
            self[i] = value;
        }
    }

    #[inline]
    fn flip(&mut self, i: usize) {
        if i < self.len() {
            let v = self[i];
            self[i] = !v;
        }
    }

    fn flip_all(&mut self) {
        for b in self.iter_mut() {
            *b = !*b;
        }
    }

    fn count(&self) -> usize {
        self.iter().filter(|b| **b).count()
    }

    fn any(&self) -> bool {
        self.iter().any(|b| *b)
    }

    fn none(&self) -> bool {
        !self.any()
    }

    fn push_back_bit(&mut self, value: bool) {
        self.push(value);
    }

    fn pop_back_bit(&mut self) {
        self.pop();
    }

    fn resize_bits(&mut self, new_size: usize) {
        self.resize(new_size, false);
    }

    fn zero_out(&mut self) {
        for b in self.iter_mut() {
            *b = false;
        }
    }

    fn one_out(&mut self) {
        for b in self.iter_mut() {
            *b = true;
        }
    }

    fn next_set_bit(&self, start: usize) -> Option<usize> {
        if start >= self.len() {
            return None;
        }
        for (offset, bit) in self[start..].iter().enumerate() {
            if *bit {
                return Some(start + offset);
            }
        }
        None
    }

    fn for_each_set_bit<F: FnMut(usize)>(&self, mut f: F) {
        for (i, bit) in self.iter().enumerate() {
            if *bit {
                f(i);
            }
        }
    }

    fn str(&self) -> String {
        // Mirror the C++ `str()` output where bit 0 is on the RIGHT.
        let mut s = String::with_capacity(self.len());
        for i in 0..self.len() {
            let bit = self[self.len() - 1 - i];
            s.push(if bit { '1' } else { '0' });
        }
        s
    }

    fn set_from_str(&mut self, s: &str) {
        debug_assert!(s.chars().all(|c| c == '0' || c == '1'));
        self.clear();
        self.resize(s.len(), false);
        for (i, ch) in s.chars().enumerate() {
            // Bit 0 is the rightmost character in the C++ format.
            let bit_index = s.len() - 1 - i;
            self[bit_index] = ch == '1';
        }
    }

    fn bit_and_assign(&mut self, other: &Self) {
        debug_assert!(self.len() == other.len());
        let n = self.len().min(other.len());
        for i in 0..n {
            self[i] &= other[i];
        }
    }

    fn bit_or_assign(&mut self, other: &Self) {
        debug_assert!(self.len() == other.len());
        let n = self.len().min(other.len());
        for i in 0..n {
            self[i] |= other[i];
        }
    }

    fn bit_xor_assign(&mut self, other: &Self) {
        debug_assert!(self.len() == other.len());
        let n = self.len().min(other.len());
        for i in 0..n {
            self[i] ^= other[i];
        }
    }

    fn bit_not(&self) -> BitVec {
        self.iter().map(|b| !*b).collect()
    }
}
