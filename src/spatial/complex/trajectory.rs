use crate::Vector;
use crate::spatial::State;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Trajectory {
    pub states: Vector<State>,
}

impl Trajectory {
    pub fn size(&self) -> usize {
        self.states.len()
    }

    pub fn is_empty(&self) -> bool {
        self.states.is_empty()
    }
}
