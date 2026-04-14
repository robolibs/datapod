use crate::Vector;
use crate::associative::Map;

use super::{Collision, Inertial, Sensor, Visual};

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Link {
    pub name: String,
    pub inertial: Option<Inertial>,
    pub visuals: Vector<Visual>,
    pub collisions: Vector<Collision>,
    pub props: Map<String, String>,
    pub sensor: Option<Sensor>,
}

impl Link {
    pub fn new(name: impl Into<String>) -> Self {
        Self { name: name.into(), ..Self::default() }
    }

    pub fn with_inertial(name: impl Into<String>, inertial: Inertial) -> Self {
        Self { name: name.into(), inertial: Some(inertial), ..Self::default() }
    }

    pub fn has_inertial(&self) -> bool {
        self.inertial.is_some()
    }

    pub fn has_visuals(&self) -> bool {
        !self.visuals.is_empty()
    }

    pub fn has_collisions(&self) -> bool {
        !self.collisions.is_empty()
    }
}
