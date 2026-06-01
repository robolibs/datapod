use crate::id::STRING_NONE;
use crate::motion::Pose;

use super::kv::KV;

/// Max number of `(key, value)` URDF property pairs per sensor.
pub const SENSOR_PROP_CAP: usize = 8;

#[datapod::datapod]
pub struct Sensor {
    pub name_id: u32,
    pub type_id: u32,
    pub origin: Pose,
    pub props: [KV; SENSOR_PROP_CAP],
}

impl Default for Sensor {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            type_id: STRING_NONE,
            origin: Pose::default(),
            props: [KV::default(); SENSOR_PROP_CAP],
        }
    }
}

impl Sensor {
    pub fn new(name_id: u32, type_id: u32, origin: Pose) -> Self {
        Self {
            name_id,
            type_id,
            origin,
            props: [KV::default(); SENSOR_PROP_CAP],
        }
    }
}
