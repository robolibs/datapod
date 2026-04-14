use crate::Vector;
use crate::spatial::Pose;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Path {
    pub waypoints: Vector<Pose>,
}

impl Path {
    pub fn size(&self) -> usize {
        self.waypoints.len()
    }

    pub fn is_empty(&self) -> bool {
        self.waypoints.is_empty()
    }
}
