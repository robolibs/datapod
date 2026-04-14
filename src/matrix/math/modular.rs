use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Modular {
    pub val: u64,
    pub modulus: u64,
}

impl Modular {
    pub fn new(value: u64, modulus: u64) -> Self {
        assert!(modulus > 0, "modulus must be positive");
        Self {
            val: value % modulus,
            modulus,
        }
    }

    pub fn from_signed(value: i64, modulus: u64) -> Self {
        assert!(modulus > 0, "modulus must be positive");
        let m = modulus as i64;
        let v = ((value % m) + m) % m;
        Self {
            val: v as u64,
            modulus,
        }
    }

    pub fn value(&self) -> u64 {
        self.val
    }

    pub fn modulus(&self) -> u64 {
        self.modulus
    }

    pub fn is_zero(&self) -> bool {
        self.val == 0
    }

    pub fn is_one(&self) -> bool {
        self.val == 1
    }

    pub fn is_set(&self) -> bool {
        self.val != 0
    }

    pub fn inverse(&self) -> Self {
        let n = self.modulus as i128;
        let mut t: i128 = 0;
        let mut new_t: i128 = 1;
        let mut r: i128 = n;
        let mut new_r: i128 = self.val as i128;
        while new_r != 0 {
            let q = r / new_r;
            let tmp_t = new_t;
            new_t = t - q * new_t;
            t = tmp_t;
            let tmp_r = new_r;
            new_r = r - q * new_r;
            r = tmp_r;
        }
        if r > 1 {
            return Self {
                val: 0,
                modulus: self.modulus,
            };
        }
        if t < 0 {
            t += n;
        }
        Self {
            val: t as u64,
            modulus: self.modulus,
        }
    }

    pub fn pow(&self, mut exp: u64) -> Self {
        if exp == 0 {
            return Self {
                val: 1,
                modulus: self.modulus,
            };
        }
        let mut result = Self {
            val: 1,
            modulus: self.modulus,
        };
        let mut base = *self;
        while exp > 0 {
            if exp & 1 == 1 {
                result = result * base;
            }
            base = base * base;
            exp >>= 1;
        }
        result
    }

    fn check_same_modulus(&self, other: &Self) {
        debug_assert_eq!(self.modulus, other.modulus, "modulus mismatch");
    }
}

impl Add for Modular {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        self.check_same_modulus(&rhs);
        Self {
            val: (self.val + rhs.val) % self.modulus,
            modulus: self.modulus,
        }
    }
}

impl Sub for Modular {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        self.check_same_modulus(&rhs);
        Self {
            val: (self.val + self.modulus - rhs.val) % self.modulus,
            modulus: self.modulus,
        }
    }
}

impl Mul for Modular {
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        self.check_same_modulus(&rhs);
        let prod = (self.val as u128) * (rhs.val as u128);
        Self {
            val: (prod % self.modulus as u128) as u64,
            modulus: self.modulus,
        }
    }
}

impl Div for Modular {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        self * rhs.inverse()
    }
}

impl Neg for Modular {
    type Output = Self;
    fn neg(self) -> Self {
        Self {
            val: (self.modulus - self.val) % self.modulus,
            modulus: self.modulus,
        }
    }
}

impl AddAssign for Modular {
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl SubAssign for Modular {
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl MulAssign for Modular {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl DivAssign for Modular {
    fn div_assign(&mut self, rhs: Self) {
        *self = *self / rhs;
    }
}

impl PartialOrd for Modular {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.val.cmp(&other.val))
    }
}

impl Ord for Modular {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.val.cmp(&other.val)
    }
}
