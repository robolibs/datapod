use super::{Identity, Model, kv::KV};

/// Max number of `(key, value)` URDF property pairs on a robot.
pub const ROBOT_PROP_CAP: usize = 16;

#[datapod::datapod]
pub struct Robot {
    pub id: Identity,
    pub model: Model,
    pub props: [KV; ROBOT_PROP_CAP],
}

impl Default for Robot {
    fn default() -> Self {
        Self {
            id: Identity::default(),
            model: Model::default(),
            props: [KV::default(); ROBOT_PROP_CAP],
        }
    }
}

impl Robot {
    pub fn new(id: Identity, model: Model) -> Self {
        Self {
            id,
            model,
            props: [KV::default(); ROBOT_PROP_CAP],
        }
    }
}
