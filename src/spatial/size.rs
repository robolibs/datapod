#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Size {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Size {
    pub fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    pub fn volume(&self) -> f64 {
        self.x * self.y * self.z
    }

    pub fn area_xy(&self) -> f64 {
        self.x * self.y
    }

    pub fn area_xz(&self) -> f64 {
        self.x * self.z
    }

    pub fn area_yz(&self) -> f64 {
        self.y * self.z
    }

    pub fn diagonal(&self) -> f64 {
        (self.x * self.x + self.y * self.y + self.z * self.z).sqrt()
    }

    pub fn diagonal_2d(&self) -> f64 {
        (self.x * self.x + self.y * self.y).sqrt()
    }

    pub fn is_set(&self) -> bool {
        self.x != 0.0 || self.y != 0.0 || self.z != 0.0
    }

    pub fn abs(&self) -> Self {
        Self::new(self.x.abs(), self.y.abs(), self.z.abs())
    }

    pub fn max(&self, other: Size) -> Self {
        Self::new(
            self.x.max(other.x),
            self.y.max(other.y),
            self.z.max(other.z),
        )
    }

    pub fn min(&self, other: Size) -> Self {
        Self::new(
            self.x.min(other.x),
            self.y.min(other.y),
            self.z.min(other.z),
        )
    }

    pub fn to_mat(&self) -> crate::mat::Vector<f64, 3> {
        crate::mat::Vector::from([self.x, self.y, self.z])
    }

    pub fn from_mat(v: crate::mat::Vector<f64, 3>) -> Self {
        Self::new(v[0], v[1], v[2])
    }
}

impl std::ops::Add for Size {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self::new(self.x + rhs.x, self.y + rhs.y, self.z + rhs.z)
    }
}

impl std::ops::Sub for Size {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self::new(self.x - rhs.x, self.y - rhs.y, self.z - rhs.z)
    }
}

impl std::ops::Mul<f64> for Size {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self::new(self.x * rhs, self.y * rhs, self.z * rhs)
    }
}

impl std::ops::Div<f64> for Size {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self::new(self.x / rhs, self.y / rhs, self.z / rhs)
    }
}

impl std::ops::Mul for Size {
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        Self::new(self.x * rhs.x, self.y * rhs.y, self.z * rhs.z)
    }
}
