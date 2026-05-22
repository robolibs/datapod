use crate::motion::State;

/// Time-stamped sequence of motion states. Owns the buffer.
#[datapod::datapod]
#[derive(Default)]
pub struct Trajectory {
    #[dp(bytes)]
    pub states: Vec<State>,
}

impl Trajectory {
    pub fn new(states: Vec<State>) -> Self {
        Self { states }
    }

    pub fn size(&self) -> usize {
        self.states.len()
    }

    pub fn is_empty(&self) -> bool {
        self.states.is_empty()
    }
}
