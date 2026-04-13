use super::{Euler, Point, Size};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
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
}
