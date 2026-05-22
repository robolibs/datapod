use crate::id::STRING_NONE;

use super::{INVALID_ID, Inertial, kv::KV};

/// Max number of visual IDs per link.
pub const LINK_VISUAL_CAP: usize = 8;
/// Max number of collision IDs per link.
pub const LINK_COLLISION_CAP: usize = 8;
/// Max number of `(key, value)` URDF property pairs per link.
pub const LINK_PROP_CAP: usize = 8;

/// Link record. Visuals, collisions, and sensor are referenced by ID
/// (`INVALID_ID` for unused slots). `inertial_present` / `sensor_present`
/// act as `Option<…>` flags for their inline sub-records.
#[datapod::datapod]
pub struct Link {
    pub name_id: u32,
    pub inertial_present: u32,
    pub sensor_id: u32,
    pub _pad: u32,
    pub inertial: Inertial,
    pub visual_ids: [u32; LINK_VISUAL_CAP],
    pub collision_ids: [u32; LINK_COLLISION_CAP],
    pub props: [KV; LINK_PROP_CAP],
}

impl Default for Link {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            inertial_present: 0,
            sensor_id: INVALID_ID,
            _pad: 0,
            inertial: Inertial::default(),
            visual_ids: [INVALID_ID; LINK_VISUAL_CAP],
            collision_ids: [INVALID_ID; LINK_COLLISION_CAP],
            props: [KV::default(); LINK_PROP_CAP],
        }
    }
}

impl Link {
    pub fn new(name_id: u32) -> Self {
        Self { name_id, ..Self::default() }
    }

    pub fn with_inertial(name_id: u32, inertial: Inertial) -> Self {
        Self {
            name_id,
            inertial_present: 1,
            inertial,
            ..Self::default()
        }
    }

    pub fn has_inertial(&self) -> bool {
        self.inertial_present != 0
    }

    pub fn has_visuals(&self) -> bool {
        self.visual_ids.iter().any(|&id| id != INVALID_ID)
    }

    pub fn has_collisions(&self) -> bool {
        self.collision_ids.iter().any(|&id| id != INVALID_ID)
    }

    pub fn has_sensor(&self) -> bool {
        self.sensor_id != INVALID_ID
    }
}
