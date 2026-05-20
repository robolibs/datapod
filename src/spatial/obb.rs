use super::{Euler, Point, Size};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Obb {
    pub center: Point,
    pub half_extents: Size,
    pub rotation: Euler,
}

impl Obb {
    pub fn new(center: Point, half_extents: Size, rotation: Euler) -> Self {
        Self {
            center,
            half_extents,
            rotation,
        }
    }

    pub fn volume(&self) -> f64 {
        8.0 * self.half_extents.x * self.half_extents.y * self.half_extents.z
    }

    pub fn surface_area(&self) -> f64 {
        let width = 2.0 * self.half_extents.x;
        let height = 2.0 * self.half_extents.y;
        let depth = 2.0 * self.half_extents.z;
        2.0 * (width * height + height * depth + depth * width)
    }

    pub fn contains(&self, point: Point) -> bool {
        let dx = (point.x - self.center.x).abs();
        let dy = (point.y - self.center.y).abs();
        let dz = (point.z - self.center.z).abs();
        dx <= self.half_extents.x && dy <= self.half_extents.y && dz <= self.half_extents.z
    }

    pub fn full_size(&self) -> Size {
        Size::new(
            2.0 * self.half_extents.x,
            2.0 * self.half_extents.y,
            2.0 * self.half_extents.z,
        )
    }

    pub fn corners(&self) -> [Point; 8] {
        let hx = self.half_extents.x;
        let hy = self.half_extents.y;
        let hz = self.half_extents.z;
        [
            Point::new(self.center.x - hx, self.center.y - hy, self.center.z - hz),
            Point::new(self.center.x + hx, self.center.y - hy, self.center.z - hz),
            Point::new(self.center.x + hx, self.center.y + hy, self.center.z - hz),
            Point::new(self.center.x - hx, self.center.y + hy, self.center.z - hz),
            Point::new(self.center.x - hx, self.center.y - hy, self.center.z + hz),
            Point::new(self.center.x + hx, self.center.y - hy, self.center.z + hz),
            Point::new(self.center.x + hx, self.center.y + hy, self.center.z + hz),
            Point::new(self.center.x - hx, self.center.y + hy, self.center.z + hz),
        ]
    }

    pub fn to_mat(&self) -> crate::mat::Vector<f64, 9> {
        crate::mat::Vector::from([
            self.center.x,
            self.center.y,
            self.center.z,
            self.half_extents.x,
            self.half_extents.y,
            self.half_extents.z,
            self.rotation.roll,
            self.rotation.pitch,
            self.rotation.yaw,
        ])
    }

    pub fn from_mat(v: crate::mat::Vector<f64, 9>) -> Self {
        Self::new(
            Point::new(v[0], v[1], v[2]),
            Size::new(v[3], v[4], v[5]),
            Euler::new(v[6], v[7], v[8]),
        )
    }
}
