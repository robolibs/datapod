use crate::motion::Pose;

/// Sequence of waypoint poses. Owns the buffer.
#[datapod::datapod]
#[derive(Default)]
pub struct Path {
    #[dp(bytes)]
    pub waypoints: Vec<Pose>,
}

impl Path {
    pub fn new(waypoints: Vec<Pose>) -> Self {
        Self { waypoints }
    }

    pub fn size(&self) -> usize {
        self.waypoints.len()
    }

    pub fn is_empty(&self) -> bool {
        self.waypoints.is_empty()
    }
}
