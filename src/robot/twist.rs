use crate::motion::Velocity;

#[datapod::datapod]
#[derive(Default)]
pub struct Twist {
    pub linear: Velocity,
    pub angular: Velocity,
}

impl Twist {
    pub fn new(linear: Velocity, angular: Velocity) -> Self {
        Self { linear, angular }
    }

    pub fn from_components(vx: f64, vy: f64, vz: f64, wx: f64, wy: f64, wz: f64) -> Self {
        Self {
            linear: Velocity { vx, vy, vz },
            angular: Velocity {
                vx: wx,
                vy: wy,
                vz: wz,
            },
        }
    }

    pub fn linear_only(linear: Velocity) -> Self {
        Self {
            linear,
            angular: Velocity::default(),
        }
    }

    pub fn angular_only(angular: Velocity) -> Self {
        Self {
            linear: Velocity::default(),
            angular,
        }
    }

    pub fn zero() -> Self {
        Self::default()
    }

    pub fn is_set(&self) -> bool {
        self.linear.is_set() || self.angular.is_set()
    }

    pub fn to_mat(&self) -> [f64; 6] {
        [
            self.linear.vx,
            self.linear.vy,
            self.linear.vz,
            self.angular.vx,
            self.angular.vy,
            self.angular.vz,
        ]
    }

    pub fn from_mat(v: [f64; 6]) -> Self {
        Self {
            linear: Velocity {
                vx: v[0],
                vy: v[1],
                vz: v[2],
            },
            angular: Velocity {
                vx: v[3],
                vy: v[4],
                vz: v[5],
            },
        }
    }
}
