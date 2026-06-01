use super::Quaternion;
use crate::geom::Point;

#[datapod::datapod]
#[derive(Default)]
pub struct Pose {
    pub point: Point,
    pub rotation: Quaternion,
}

impl Pose {
    pub fn is_set(&self) -> bool {
        self.point.is_set() || self.rotation.is_set()
    }

    pub fn transform_point(&self, local_point: Point) -> Point {
        let q = self.rotation;
        let p = Quaternion::new(0.0, local_point.x, local_point.y, local_point.z);
        let rotated = q * p * q.conjugate();
        Point::new(
            self.point.x + rotated.x,
            self.point.y + rotated.y,
            self.point.z + rotated.z,
        )
    }

    pub fn inverse_transform_point(&self, world_point: Point) -> Point {
        let translated = Point::new(
            world_point.x - self.point.x,
            world_point.y - self.point.y,
            world_point.z - self.point.z,
        );
        let p = Quaternion::new(0.0, translated.x, translated.y, translated.z);
        let rotated = self.rotation.conjugate() * p * self.rotation;
        Point::new(rotated.x, rotated.y, rotated.z)
    }

    pub fn inverse(&self) -> Self {
        let q_inv = self.rotation.conjugate();
        let neg_pos = Quaternion::new(0.0, -self.point.x, -self.point.y, -self.point.z);
        let rotated = q_inv * neg_pos * self.rotation;
        Self {
            point: Point::new(rotated.x, rotated.y, rotated.z),
            rotation: q_inv,
        }
    }

    pub fn to_mat(&self) -> [f64; 7] {
        [
            self.point.x,
            self.point.y,
            self.point.z,
            self.rotation.w,
            self.rotation.x,
            self.rotation.y,
            self.rotation.z,
        ]
    }

    pub fn from_mat(v: [f64; 7]) -> Self {
        Self {
            point: Point::new(v[0], v[1], v[2]),
            rotation: Quaternion::new(v[3], v[4], v[5], v[6]),
        }
    }
}

impl std::ops::Mul for Pose {
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        Self {
            point: self.transform_point(rhs.point),
            rotation: self.rotation * rhs.rotation,
        }
    }
}
