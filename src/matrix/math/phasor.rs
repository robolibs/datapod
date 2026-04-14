use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Phasor<T> {
    pub mag: T,
    pub phase: T,
}

impl<T> Phasor<T> {
    pub const fn new(mag: T, phase: T) -> Self {
        Self { mag, phase }
    }
}

impl Phasor<f64> {
    pub fn from_rectangular(real: f64, imag: f64) -> Self {
        Self {
            mag: (real * real + imag * imag).sqrt(),
            phase: imag.atan2(real),
        }
    }

    pub fn from_degrees(magnitude: f64, phase_deg: f64) -> Self {
        Self {
            mag: magnitude,
            phase: phase_deg.to_radians(),
        }
    }

    pub fn real(&self) -> f64 {
        self.mag * self.phase.cos()
    }

    pub fn imag(&self) -> f64 {
        self.mag * self.phase.sin()
    }

    pub fn phase_degrees(&self) -> f64 {
        self.phase.to_degrees()
    }

    pub fn rms(&self) -> f64 {
        self.mag / 2.0_f64.sqrt()
    }

    pub fn peak(&self) -> f64 {
        self.mag
    }

    pub fn peak_to_peak(&self) -> f64 {
        self.mag * 2.0
    }

    pub fn is_set(&self) -> bool {
        self.mag != 0.0 || self.phase != 0.0
    }

    pub fn normalized_phase(&self) -> Self {
        let pi = std::f64::consts::PI;
        let two_pi = 2.0 * pi;
        let mut p = self.phase;
        while p > pi {
            p -= two_pi;
        }
        while p < -pi {
            p += two_pi;
        }
        Self {
            mag: self.mag,
            phase: p,
        }
    }

    pub fn conjugate(&self) -> Self {
        Self {
            mag: self.mag,
            phase: -self.phase,
        }
    }

    pub fn complex_power(&self, current: &Self) -> Self {
        Self {
            mag: self.mag * current.mag,
            phase: self.phase - current.phase,
        }
    }

    pub fn real_power(&self, current: &Self) -> f64 {
        self.mag * current.mag * (self.phase - current.phase).cos()
    }

    pub fn reactive_power(&self, current: &Self) -> f64 {
        self.mag * current.mag * (self.phase - current.phase).sin()
    }

    pub fn apparent_power(&self, current: &Self) -> f64 {
        self.mag * current.mag
    }

    pub fn power_factor(&self, current: &Self) -> f64 {
        (self.phase - current.phase).cos()
    }

    pub fn powf(&self, n: f64) -> Self {
        Self {
            mag: self.mag.powf(n),
            phase: self.phase * n,
        }
    }

    pub fn sqrt(&self) -> Self {
        Self {
            mag: self.mag.sqrt(),
            phase: self.phase / 2.0,
        }
    }
}

impl Mul for Phasor<f64> {
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        Self::new(self.mag * rhs.mag, self.phase + rhs.phase)
    }
}

impl Div for Phasor<f64> {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        Self::new(self.mag / rhs.mag, self.phase - rhs.phase)
    }
}

impl Add for Phasor<f64> {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        let r = self.real() + rhs.real();
        let i = self.imag() + rhs.imag();
        Self::from_rectangular(r, i)
    }
}

impl Sub for Phasor<f64> {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        let r = self.real() - rhs.real();
        let i = self.imag() - rhs.imag();
        Self::from_rectangular(r, i)
    }
}

impl Mul<f64> for Phasor<f64> {
    type Output = Self;
    fn mul(self, s: f64) -> Self {
        Self::new(self.mag * s, self.phase)
    }
}

impl Div<f64> for Phasor<f64> {
    type Output = Self;
    fn div(self, s: f64) -> Self {
        Self::new(self.mag / s, self.phase)
    }
}

impl Neg for Phasor<f64> {
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(self.mag, self.phase + std::f64::consts::PI)
    }
}

impl AddAssign for Phasor<f64> {
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl SubAssign for Phasor<f64> {
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl MulAssign for Phasor<f64> {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl DivAssign for Phasor<f64> {
    fn div_assign(&mut self, rhs: Self) {
        *self = *self / rhs;
    }
}

pub type Phasorf = Phasor<f32>;
pub type Phasord = Phasor<f64>;
