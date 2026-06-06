use super::INVALID_ID;
use crate::WireError;

/// Aggregate header for a robot model. The actual `Link` / `Joint` /
/// `Transmission` records are shipped as separate messages and tracked
/// application-side by their `u32` IDs; this header carries the counts
/// and the root pointer.
///
/// Pre-computed graph adjacency (parent_of, joint_from_parent, children)
/// is also application-side — those tables can be rebuilt from the joint
/// parent/child IDs without a Pod-compatible representation.
#[datapod::datapod]
pub struct Model {
    pub link_count: u32,
    pub joint_count: u32,
    pub transmission_count: u32,
    pub root_link_id: u32,
}

impl Default for Model {
    fn default() -> Self {
        Self {
            link_count: 0,
            joint_count: 0,
            transmission_count: 0,
            root_link_id: INVALID_ID,
        }
    }
}

impl Model {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn num_links(&self) -> usize {
        self.try_num_links().unwrap_or(0)
    }

    pub fn try_num_links(&self) -> Result<usize, WireError> {
        u32_to_usize::<Self>(self.link_count, "link_count")
    }

    pub fn num_joints(&self) -> usize {
        self.try_num_joints().unwrap_or(0)
    }

    pub fn try_num_joints(&self) -> Result<usize, WireError> {
        u32_to_usize::<Self>(self.joint_count, "joint_count")
    }

    pub fn is_valid_link(&self, id: u32) -> bool {
        id != INVALID_ID && id < self.link_count
    }

    pub fn is_valid_joint(&self, id: u32) -> bool {
        id != INVALID_ID && id < self.joint_count
    }

    pub fn is_root(&self, link_id: u32) -> bool {
        link_id == self.root_link_id
    }
}

fn u32_to_usize<T: 'static>(value: u32, field: &'static str) -> Result<usize, WireError> {
    usize::try_from(value)
        .map_err(|_| crate::wire::invalid_header::<T>(format!("{field} exceeds usize")))
}
