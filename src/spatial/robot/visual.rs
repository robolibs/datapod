use crate::spatial::Pose;

use super::Geometry;

#[derive(Debug, Clone, PartialEq)]
pub struct Material {
    pub name: String,
    pub rgba: [f64; 4],
    pub texture: String,
}

impl Default for Material {
    fn default() -> Self {
        Self { name: String::new(), rgba: [1.0, 1.0, 1.0, 1.0], texture: String::new() }
    }
}

impl Material {
    pub fn named(name: impl Into<String>, rgba: [f64; 4]) -> Self {
        Self { name: name.into(), rgba, texture: String::new() }
    }

    pub fn color(rgba: [f64; 4]) -> Self {
        Self { name: String::new(), rgba, texture: String::new() }
    }

    pub fn textured(texture: impl Into<String>) -> Self {
        Self { name: String::new(), rgba: [1.0, 1.0, 1.0, 1.0], texture: texture.into() }
    }

    pub fn has_texture(&self) -> bool {
        !self.texture.is_empty()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Visual {
    pub name: String,
    pub origin: Pose,
    pub geom: Geometry,
    pub material: Option<Material>,
}

impl Visual {
    pub fn new(geom: Geometry) -> Self {
        Self { name: String::new(), origin: Pose::default(), geom, material: None }
    }

    pub fn with_origin(origin: Pose, geom: Geometry) -> Self {
        Self { name: String::new(), origin, geom, material: None }
    }

    pub fn with_material(geom: Geometry, material: Material) -> Self {
        Self { name: String::new(), origin: Pose::default(), geom, material: Some(material) }
    }

    pub fn named(
        name: impl Into<String>,
        origin: Pose,
        geom: Geometry,
        material: Option<Material>,
    ) -> Self {
        Self { name: name.into(), origin, geom, material }
    }

    pub fn is_set(&self) -> bool {
        self.origin.is_set() || !self.name.is_empty()
    }
}
