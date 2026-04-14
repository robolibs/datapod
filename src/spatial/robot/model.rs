use crate::Vector;
use crate::associative::Map;

use super::{INVALID_ID, Joint, Link, Transmission};

#[derive(Debug, Clone, PartialEq)]
pub struct Model {
    pub links: Vector<Link>,
    pub joints: Vector<Joint>,
    pub transmissions: Vector<Transmission>,
    pub props: Map<String, String>,
    pub root: u32,
    pub parent_of: Vector<u32>,
    pub joint_from_parent: Vector<u32>,
    pub children_of: Vector<Vector<u32>>,
}

impl Default for Model {
    fn default() -> Self {
        Self {
            links: Vector::default(),
            joints: Vector::default(),
            transmissions: Vector::default(),
            props: Map::default(),
            root: INVALID_ID,
            parent_of: Vector::default(),
            joint_from_parent: Vector::default(),
            children_of: Vector::default(),
        }
    }
}

impl Model {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn add_link(&mut self, link: Link) -> u32 {
        let id = self.links.len() as u32;
        self.links.push(link);
        self.parent_of.push(INVALID_ID);
        self.joint_from_parent.push(INVALID_ID);
        self.children_of.push(Vector::default());
        if self.root == INVALID_ID {
            self.root = id;
        }
        id
    }

    pub fn add_joint(&mut self, joint: Joint) -> u32 {
        let id = self.joints.len() as u32;
        self.joints.push(joint);
        id
    }

    pub fn connect(&mut self, parent: u32, child: u32, joint_id: u32) {
        self.joints[joint_id as usize].parent = parent;
        self.joints[joint_id as usize].child = child;
        self.parent_of[child as usize] = parent;
        self.joint_from_parent[child as usize] = joint_id;
        self.children_of[parent as usize].push(child);
    }

    pub fn num_links(&self) -> usize {
        self.links.len()
    }

    pub fn num_joints(&self) -> usize {
        self.joints.len()
    }

    pub fn is_valid_link(&self, id: u32) -> bool {
        (id as usize) < self.links.len()
    }

    pub fn is_valid_joint(&self, id: u32) -> bool {
        (id as usize) < self.joints.len()
    }

    pub fn get_parent(&self, link_id: u32) -> u32 {
        self.parent_of
            .get(link_id as usize)
            .copied()
            .unwrap_or(INVALID_ID)
    }

    pub fn get_parent_joint(&self, link_id: u32) -> u32 {
        self.joint_from_parent
            .get(link_id as usize)
            .copied()
            .unwrap_or(INVALID_ID)
    }

    pub fn get_children(&self, link_id: u32) -> &[u32] {
        self.children_of
            .get(link_id as usize)
            .map(|c| c.as_slice())
            .unwrap_or(&[])
    }

    pub fn is_leaf(&self, link_id: u32) -> bool {
        self.children_of
            .get(link_id as usize)
            .is_some_and(Vector::is_empty)
    }

    pub fn is_root(&self, link_id: u32) -> bool {
        link_id == self.root
    }
}
