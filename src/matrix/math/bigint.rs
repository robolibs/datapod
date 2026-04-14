use std::cmp::Ordering;
use std::ops::{
    Add, AddAssign, BitAnd, BitAndAssign, BitOr, BitOrAssign, BitXor, BitXorAssign, Mul, MulAssign,
    Not, Shl, ShlAssign, Shr, ShrAssign, Sub, SubAssign,
};

const LIMBS: usize = 4;
const LIMB_BITS: usize = 64;
pub const TOTAL_BITS: usize = LIMBS * LIMB_BITS;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct BigInt {
    pub limbs: [u64; LIMBS],
}

impl BigInt {
    pub const NUM_LIMBS: usize = LIMBS;
    pub const TOTAL_BITS: usize = TOTAL_BITS;

    pub const fn new() -> Self {
        Self { limbs: [0; LIMBS] }
    }

    pub const fn from_u64(value: u64) -> Self {
        let mut limbs = [0u64; LIMBS];
        limbs[0] = value;
        Self { limbs }
    }

    pub const fn from_limbs(limbs: [u64; LIMBS]) -> Self {
        Self { limbs }
    }

    pub fn to_u64(&self) -> u64 {
        self.limbs[0]
    }

    pub fn fits_u64(&self) -> bool {
        (1..LIMBS).all(|i| self.limbs[i] == 0)
    }

    pub fn is_zero(&self) -> bool {
        self.limbs.iter().all(|&l| l == 0)
    }

    pub fn is_one(&self) -> bool {
        self.limbs[0] == 1 && (1..LIMBS).all(|i| self.limbs[i] == 0)
    }

    pub fn is_set(&self) -> bool {
        !self.is_zero()
    }

    pub fn get_bit(&self, pos: usize) -> bool {
        if pos >= TOTAL_BITS {
            return false;
        }
        let limb_idx = pos / LIMB_BITS;
        let bit_idx = pos % LIMB_BITS;
        (self.limbs[limb_idx] >> bit_idx) & 1 == 1
    }

    pub fn set_bit(&mut self, pos: usize, value: bool) {
        if pos >= TOTAL_BITS {
            return;
        }
        let limb_idx = pos / LIMB_BITS;
        let bit_idx = pos % LIMB_BITS;
        if value {
            self.limbs[limb_idx] |= 1u64 << bit_idx;
        } else {
            self.limbs[limb_idx] &= !(1u64 << bit_idx);
        }
    }

    pub fn leading_zeros(&self) -> usize {
        for i in (0..LIMBS).rev() {
            if self.limbs[i] != 0 {
                return (LIMBS - 1 - i) * LIMB_BITS + self.limbs[i].leading_zeros() as usize;
            }
        }
        TOTAL_BITS
    }

    pub fn bit_width(&self) -> usize {
        TOTAL_BITS - self.leading_zeros()
    }
}

impl AddAssign for BigInt {
    fn add_assign(&mut self, rhs: Self) {
        let mut carry: u64 = 0;
        for i in 0..LIMBS {
            let (s1, c1) = self.limbs[i].overflowing_add(rhs.limbs[i]);
            let (s2, c2) = s1.overflowing_add(carry);
            self.limbs[i] = s2;
            carry = (c1 as u64) + (c2 as u64);
        }
    }
}

impl SubAssign for BigInt {
    fn sub_assign(&mut self, rhs: Self) {
        let mut borrow: u64 = 0;
        for i in 0..LIMBS {
            let (d1, b1) = self.limbs[i].overflowing_sub(rhs.limbs[i]);
            let (d2, b2) = d1.overflowing_sub(borrow);
            self.limbs[i] = d2;
            borrow = (b1 as u64) + (b2 as u64);
        }
    }
}

impl Add for BigInt {
    type Output = Self;
    fn add(mut self, rhs: Self) -> Self {
        self += rhs;
        self
    }
}

impl Sub for BigInt {
    type Output = Self;
    fn sub(mut self, rhs: Self) -> Self {
        self -= rhs;
        self
    }
}

impl Mul for BigInt {
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        let mut result = BigInt::new();
        for i in 0..LIMBS {
            let mut carry: u128 = 0;
            for j in 0..(LIMBS - i) {
                let prod = (self.limbs[i] as u128) * (rhs.limbs[j] as u128)
                    + (result.limbs[i + j] as u128)
                    + carry;
                result.limbs[i + j] = prod as u64;
                carry = prod >> 64;
            }
        }
        result
    }
}

impl MulAssign for BigInt {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl BitAnd for BigInt {
    type Output = Self;
    fn bitand(mut self, rhs: Self) -> Self {
        self &= rhs;
        self
    }
}

impl BitAndAssign for BigInt {
    fn bitand_assign(&mut self, rhs: Self) {
        for i in 0..LIMBS {
            self.limbs[i] &= rhs.limbs[i];
        }
    }
}

impl BitOr for BigInt {
    type Output = Self;
    fn bitor(mut self, rhs: Self) -> Self {
        self |= rhs;
        self
    }
}

impl BitOrAssign for BigInt {
    fn bitor_assign(&mut self, rhs: Self) {
        for i in 0..LIMBS {
            self.limbs[i] |= rhs.limbs[i];
        }
    }
}

impl BitXor for BigInt {
    type Output = Self;
    fn bitxor(mut self, rhs: Self) -> Self {
        self ^= rhs;
        self
    }
}

impl BitXorAssign for BigInt {
    fn bitxor_assign(&mut self, rhs: Self) {
        for i in 0..LIMBS {
            self.limbs[i] ^= rhs.limbs[i];
        }
    }
}

impl Not for BigInt {
    type Output = Self;
    fn not(self) -> Self {
        let mut out = BigInt::new();
        for i in 0..LIMBS {
            out.limbs[i] = !self.limbs[i];
        }
        out
    }
}

impl ShlAssign<usize> for BigInt {
    fn shl_assign(&mut self, shift: usize) {
        if shift >= TOTAL_BITS {
            self.limbs = [0; LIMBS];
            return;
        }
        let limb_shift = shift / LIMB_BITS;
        let bit_shift = shift % LIMB_BITS;
        if limb_shift > 0 {
            for i in (limb_shift..LIMBS).rev() {
                self.limbs[i] = self.limbs[i - limb_shift];
            }
            for l in self.limbs.iter_mut().take(limb_shift) {
                *l = 0;
            }
        }
        if bit_shift > 0 {
            let mut carry: u64 = 0;
            for l in self.limbs.iter_mut() {
                let new_carry = *l >> (LIMB_BITS - bit_shift);
                *l = (*l << bit_shift) | carry;
                carry = new_carry;
            }
        }
    }
}

impl ShrAssign<usize> for BigInt {
    fn shr_assign(&mut self, shift: usize) {
        if shift >= TOTAL_BITS {
            self.limbs = [0; LIMBS];
            return;
        }
        let limb_shift = shift / LIMB_BITS;
        let bit_shift = shift % LIMB_BITS;
        if limb_shift > 0 {
            for i in 0..(LIMBS - limb_shift) {
                self.limbs[i] = self.limbs[i + limb_shift];
            }
            for i in (LIMBS - limb_shift)..LIMBS {
                self.limbs[i] = 0;
            }
        }
        if bit_shift > 0 {
            let mut carry: u64 = 0;
            for i in (0..LIMBS).rev() {
                let new_carry = self.limbs[i] << (LIMB_BITS - bit_shift);
                self.limbs[i] = (self.limbs[i] >> bit_shift) | carry;
                carry = new_carry;
            }
        }
    }
}

impl Shl<usize> for BigInt {
    type Output = Self;
    fn shl(mut self, shift: usize) -> Self {
        self <<= shift;
        self
    }
}

impl Shr<usize> for BigInt {
    type Output = Self;
    fn shr(mut self, shift: usize) -> Self {
        self >>= shift;
        self
    }
}

impl Ord for BigInt {
    fn cmp(&self, other: &Self) -> Ordering {
        for i in (0..LIMBS).rev() {
            match self.limbs[i].cmp(&other.limbs[i]) {
                Ordering::Equal => continue,
                non_eq => return non_eq,
            }
        }
        Ordering::Equal
    }
}

impl PartialOrd for BigInt {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl From<u64> for BigInt {
    fn from(v: u64) -> Self {
        Self::from_u64(v)
    }
}
