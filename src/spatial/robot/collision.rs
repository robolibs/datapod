use crate::spatial::Pose;

use super::Geometry;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Collision {
    pub name: String,
    pub origin: Pose,
    pub geom: Geometry,
}

impl Collision {
    pub fn new(geom: Geometry) -> Self {
        Self { name: String::new(), origin: Pose::default(), geom }
    }

    pub fn with_origin(origin: Pose, geom: Geometry) -> Self {
        Self { name: String::new(), origin, geom }
    }

    pub fn named(name: impl Into<String>, origin: Pose, geom: Geometry) -> Self {
        Self { name: name.into(), origin, geom }
    }

    pub fn is_set(&self) -> bool {
        self.origin.is_set() || !self.name.is_empty()
    }
}
