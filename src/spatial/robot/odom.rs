use crate::matrix::mat;
use crate::spatial::{Point, Pose, Quaternion, Velocity};

use super::Twist;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Odom {
    pub pose: Pose,
    pub twist: Twist,
}

impl Odom {
    pub fn new(pose: Pose, twist: Twist) -> Self {
        Self { pose, twist }
    }

    pub fn from_pose(pose: Pose) -> Self {
        Self {
            pose,
            twist: Twist::default(),
        }
    }

    pub fn at_rest() -> Self {
        Self {
            pose: Pose {
                point: Point::new(0.0, 0.0, 0.0),
                rotation: Quaternion::new(1.0, 0.0, 0.0, 0.0),
            },
            twist: Twist::default(),
        }
    }

    pub fn is_set(&self) -> bool {
        self.pose.is_set() || self.twist.is_set()
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 13> {
        mat::Vector::from([
            self.pose.point.x,
            self.pose.point.y,
            self.pose.point.z,
            self.pose.rotation.w,
            self.pose.rotation.x,
            self.pose.rotation.y,
            self.pose.rotation.z,
            self.twist.linear.vx,
            self.twist.linear.vy,
            self.twist.linear.vz,
            self.twist.angular.vx,
            self.twist.angular.vy,
            self.twist.angular.vz,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 13>) -> Self {
        Self {
            pose: Pose {
                point: Point::new(v[0], v[1], v[2]),
                rotation: Quaternion::new(v[3], v[4], v[5], v[6]),
            },
            twist: Twist {
                linear: Velocity {
                    vx: v[7],
                    vy: v[8],
                    vz: v[9],
                },
                angular: Velocity {
                    vx: v[10],
                    vy: v[11],
                    vz: v[12],
                },
            },
        }
    }
}
