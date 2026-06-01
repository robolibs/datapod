#[datapod::datapod]
#[derive(Default)]
pub struct Acceleration {
    pub ax: f64,
    pub ay: f64,
    pub az: f64,
}

impl Acceleration {
    pub fn magnitude(&self) -> f64 {
        (self.ax * self.ax + self.ay * self.ay + self.az * self.az).sqrt()
    }

    pub fn magnitude_2d(&self) -> f64 {
        (self.ax * self.ax + self.ay * self.ay).sqrt()
    }

    pub fn magnitude_squared(&self) -> f64 {
        self.ax * self.ax + self.ay * self.ay + self.az * self.az
    }

    pub fn is_set(&self) -> bool {
        self.ax != 0.0 || self.ay != 0.0 || self.az != 0.0
    }

    pub fn to_mat(&self) -> [f64; 3] {
        [self.ax, self.ay, self.az]
    }

    pub fn from_mat(v: [f64; 3]) -> Self {
        Self {
            ax: v[0],
            ay: v[1],
            az: v[2],
        }
    }
}

impl std::ops::Add for Acceleration {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            ax: self.ax + rhs.ax,
            ay: self.ay + rhs.ay,
            az: self.az + rhs.az,
        }
    }
}

impl std::ops::Sub for Acceleration {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            ax: self.ax - rhs.ax,
            ay: self.ay - rhs.ay,
            az: self.az - rhs.az,
        }
    }
}

impl std::ops::Mul<f64> for Acceleration {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self {
            ax: self.ax * rhs,
            ay: self.ay * rhs,
            az: self.az * rhs,
        }
    }
}

impl std::ops::Div<f64> for Acceleration {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self {
            ax: self.ax / rhs,
            ay: self.ay / rhs,
            az: self.az / rhs,
        }
    }
}
