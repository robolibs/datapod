use crate::motion::Acceleration;

#[datapod::datapod]
#[derive(Default)]
pub struct Accel {
    pub linear: Acceleration,
    pub angular: Acceleration,
}

impl Accel {
    pub fn new(linear: Acceleration, angular: Acceleration) -> Self {
        Self { linear, angular }
    }

    pub fn from_components(
        ax: f64,
        ay: f64,
        az: f64,
        alpha_x: f64,
        alpha_y: f64,
        alpha_z: f64,
    ) -> Self {
        Self {
            linear: Acceleration { ax, ay, az },
            angular: Acceleration {
                ax: alpha_x,
                ay: alpha_y,
                az: alpha_z,
            },
        }
    }

    pub fn linear_only(linear: Acceleration) -> Self {
        Self {
            linear,
            angular: Acceleration::default(),
        }
    }

    pub fn angular_only(angular: Acceleration) -> Self {
        Self {
            linear: Acceleration::default(),
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
            self.linear.ax,
            self.linear.ay,
            self.linear.az,
            self.angular.ax,
            self.angular.ay,
            self.angular.az,
        ]
    }

    pub fn from_mat(v: [f64; 6]) -> Self {
        Self {
            linear: Acceleration {
                ax: v[0],
                ay: v[1],
                az: v[2],
            },
            angular: Acceleration {
                ax: v[3],
                ay: v[4],
                az: v[5],
            },
        }
    }
}
