use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Fraction {
    pub num: i64,
    pub den: i64,
}

impl Default for Fraction {
    fn default() -> Self {
        Self { num: 0, den: 1 }
    }
}

impl Fraction {
    pub fn new(num: i64, den: i64) -> Self {
        let mut f = Self { num, den };
        f.normalize();
        f
    }

    pub const fn raw(num: i64, den: i64) -> Self {
        Self { num, den }
    }

    pub fn from_int(n: i64) -> Self {
        Self { num: n, den: 1 }
    }

    pub fn from_f64(value: f64, max_denom: i64) -> Self {
        if value == 0.0 {
            return Self { num: 0, den: 1 };
        }
        let negative = value < 0.0;
        let mut x = if negative { -value } else { value };
        let (mut n0, mut d0) = (0_i64, 1_i64);
        let (mut n1, mut d1) = (1_i64, 0_i64);
        while d1 <= max_denom {
            let a = x as i64;
            let n2 = a * n1 + n0;
            let d2 = a * d1 + d0;
            if d2 > max_denom {
                break;
            }
            n0 = n1;
            d0 = d1;
            n1 = n2;
            d1 = d2;
            if x == a as f64 {
                break;
            }
            x = 1.0 / (x - a as f64);
        }
        Self {
            num: if negative { -n1 } else { n1 },
            den: d1,
        }
    }

    pub fn normalize(&mut self) {
        if self.den == 0 {
            self.den = 1;
            self.num = 0;
            return;
        }
        if self.den < 0 {
            self.num = -self.num;
            self.den = -self.den;
        }
        if self.num == 0 {
            self.den = 1;
            return;
        }
        let g = gcd(self.num.unsigned_abs() as i64, self.den);
        if g > 1 {
            self.num /= g;
            self.den /= g;
        }
    }

    pub fn to_f64(&self) -> f64 {
        self.num as f64 / self.den as f64
    }

    pub fn to_f32(&self) -> f32 {
        self.num as f32 / self.den as f32
    }

    pub fn is_zero(&self) -> bool {
        self.num == 0
    }

    pub fn is_positive(&self) -> bool {
        self.num > 0
    }

    pub fn is_negative(&self) -> bool {
        self.num < 0
    }

    pub fn is_integer(&self) -> bool {
        self.den == 1
    }

    pub fn is_set(&self) -> bool {
        self.num != 0
    }

    pub fn abs(&self) -> Self {
        Self {
            num: self.num.abs(),
            den: self.den,
        }
    }

    pub fn reciprocal(&self) -> Self {
        Self::new(self.den, self.num)
    }

    pub fn floor(&self) -> i64 {
        if self.num >= 0 {
            self.num / self.den
        } else {
            (self.num - self.den + 1) / self.den
        }
    }

    pub fn ceil(&self) -> i64 {
        if self.num >= 0 {
            (self.num + self.den - 1) / self.den
        } else {
            self.num / self.den
        }
    }

    pub fn pow(&self, exp: i32) -> Self {
        if exp == 0 {
            return Self::from_int(1);
        }
        if exp < 0 {
            return self.reciprocal().pow(-exp);
        }
        let mut result = Self::from_int(1);
        let mut base = *self;
        let mut e = exp;
        while e > 0 {
            if e & 1 == 1 {
                result = result * base;
            }
            base = base * base;
            e >>= 1;
        }
        result
    }
}

fn gcd(mut a: i64, mut b: i64) -> i64 {
    while b != 0 {
        let t = b;
        b = a % b;
        a = t;
    }
    a
}

impl Add for Fraction {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self::new(self.num * rhs.den + rhs.num * self.den, self.den * rhs.den)
    }
}

impl Sub for Fraction {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self::new(self.num * rhs.den - rhs.num * self.den, self.den * rhs.den)
    }
}

impl Mul for Fraction {
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        Self::new(self.num * rhs.num, self.den * rhs.den)
    }
}

impl Div for Fraction {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        Self::new(self.num * rhs.den, self.den * rhs.num)
    }
}

impl Mul<i64> for Fraction {
    type Output = Self;
    fn mul(self, s: i64) -> Self {
        Self::new(self.num * s, self.den)
    }
}

impl Div<i64> for Fraction {
    type Output = Self;
    fn div(self, s: i64) -> Self {
        Self::new(self.num, self.den * s)
    }
}

impl Neg for Fraction {
    type Output = Self;
    fn neg(self) -> Self {
        Self {
            num: -self.num,
            den: self.den,
        }
    }
}

impl AddAssign for Fraction {
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl SubAssign for Fraction {
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl MulAssign for Fraction {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl DivAssign for Fraction {
    fn div_assign(&mut self, rhs: Self) {
        *self = *self / rhs;
    }
}

impl PartialOrd for Fraction {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for Fraction {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        (self.num * other.den).cmp(&(other.num * self.den))
    }
}
