use crate::associative::{Map, Set};

use super::{Aabb, Geo, Obb, Point};

pub type PointMap<V> = Map<PointKey, V>;
pub type PointSet = Set<PointKey>;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct PointKey {
    pub x_bits: u64,
    pub y_bits: u64,
    pub z_bits: u64,
}

impl From<Point> for PointKey {
    fn from(value: Point) -> Self {
        Self {
            x_bits: value.x.to_bits(),
            y_bits: value.y.to_bits(),
            z_bits: value.z.to_bits(),
        }
    }
}

#[allow(dead_code)]
fn _keep_upstream_surface_visible(_geo: Geo, _aabb: Aabb, _obb: Obb) {}
