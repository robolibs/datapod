use crate::motion::{Acceleration, Quaternion, Velocity};

/// A machine's internal IMU: how fast it's rotating, how hard it's
/// accelerating, and which way it's facing.
#[datapod::datapod]
#[derive(Default)]
pub struct Imu {
    pub angular_velocity: Velocity,
    pub linear_acceleration: Acceleration,
    pub orientation: Quaternion,
}

impl Imu {
    pub fn new(
        angular_velocity: Velocity,
        linear_acceleration: Acceleration,
        orientation: Quaternion,
    ) -> Self {
        Self {
            angular_velocity,
            linear_acceleration,
            orientation,
        }
    }

    pub fn is_set(&self) -> bool {
        self.angular_velocity.is_set() || self.linear_acceleration.is_set()
    }
}
