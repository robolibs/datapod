use crate::motion::Pose;
use crate::id::STRING_NONE;

use super::Geometry;

#[datapod::datapod]
pub struct Collision {
    pub name_id: u32,
    pub _pad: u32,
    pub origin: Pose,
    pub geom: Geometry,
}

impl Default for Collision {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            _pad: 0,
            origin: Pose::default(),
            geom: Geometry::default(),
        }
    }
}

impl Collision {
    pub fn new(geom: Geometry) -> Self {
        Self { geom, ..Self::default() }
    }

    pub fn with_origin(origin: Pose, geom: Geometry) -> Self {
        Self { origin, geom, ..Self::default() }
    }

    pub fn named(name_id: u32, origin: Pose, geom: Geometry) -> Self {
        Self { name_id, origin, geom, ..Self::default() }
    }

    pub fn is_set(&self) -> bool {
        self.origin.is_set() || self.name_id != STRING_NONE
    }
}
