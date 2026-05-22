use crate::motion::Pose;
use crate::id::STRING_NONE;

use super::Geometry;

/// Material record. `name_id` and `texture_id` reference [`DpString`]s
/// shipped separately. `STRING_NONE` marks "unset".
#[datapod::datapod]
pub struct Material {
    pub name_id: u32,
    pub texture_id: u32,
    pub rgba: [f64; 4],
}

impl Default for Material {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            texture_id: STRING_NONE,
            rgba: [1.0, 1.0, 1.0, 1.0],
        }
    }
}

impl Material {
    pub fn named(name_id: u32, rgba: [f64; 4]) -> Self {
        Self { name_id, texture_id: STRING_NONE, rgba }
    }

    pub fn color(rgba: [f64; 4]) -> Self {
        Self { name_id: STRING_NONE, texture_id: STRING_NONE, rgba }
    }

    pub fn textured(texture_id: u32) -> Self {
        Self { name_id: STRING_NONE, texture_id, rgba: [1.0, 1.0, 1.0, 1.0] }
    }

    pub fn has_texture(&self) -> bool {
        self.texture_id != STRING_NONE
    }
}

/// Visual record. `material_present` + `material` act as `Option<Material>`.
#[datapod::datapod]
pub struct Visual {
    pub name_id: u32,
    pub material_present: u32,
    pub origin: Pose,
    pub geom: Geometry,
    pub material: Material,
}

impl Default for Visual {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            material_present: 0,
            origin: Pose::default(),
            geom: Geometry::default(),
            material: Material::default(),
        }
    }
}

impl Visual {
    pub fn new(geom: Geometry) -> Self {
        Self { geom, ..Self::default() }
    }

    pub fn with_origin(origin: Pose, geom: Geometry) -> Self {
        Self { origin, geom, ..Self::default() }
    }

    pub fn with_material(geom: Geometry, material: Material) -> Self {
        Self {
            material_present: 1,
            geom,
            material,
            ..Self::default()
        }
    }

    pub fn named(name_id: u32, origin: Pose, geom: Geometry, material: Option<Material>) -> Self {
        let (material_present, material) = match material {
            Some(m) => (1, m),
            None => (0, Material::default()),
        };
        Self { name_id, material_present, origin, geom, material }
    }

    pub fn has_material(&self) -> bool {
        self.material_present != 0
    }

    pub fn is_set(&self) -> bool {
        self.origin.is_set() || self.name_id != STRING_NONE
    }
}
