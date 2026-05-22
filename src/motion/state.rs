
use crate::geom::Point;
use super::{Pose, Quaternion, Velocity};

#[datapod::datapod]
#[derive(Default)]
pub struct State {
    pub pose: Pose,
    pub linear_velocity: Velocity,
    pub angular_velocity: Velocity,
}

impl State {
    pub fn is_set(&self) -> bool {
        self.pose.is_set() || self.linear_velocity.is_set() || self.angular_velocity.is_set()
    }

    pub fn to_mat(&self) -> [f64; 13] {
        [
            self.pose.point.x,
            self.pose.point.y,
            self.pose.point.z,
            self.pose.rotation.w,
            self.pose.rotation.x,
            self.pose.rotation.y,
            self.pose.rotation.z,
            self.linear_velocity.vx,
            self.linear_velocity.vy,
            self.linear_velocity.vz,
            self.angular_velocity.vx,
            self.angular_velocity.vy,
            self.angular_velocity.vz,
        ]    }

    pub fn from_mat(v: [f64; 13]) -> Self {
        Self {
            pose: Pose {
                point: Point::new(v[0], v[1], v[2]),
                rotation: Quaternion::new(v[3], v[4], v[5], v[6]),
            },
            linear_velocity: Velocity {
                vx: v[7],
                vy: v[8],
                vz: v[9],
            },
            angular_velocity: Velocity {
                vx: v[10],
                vy: v[11],
                vz: v[12],
            },
        }
    }
}
