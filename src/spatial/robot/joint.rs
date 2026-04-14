use crate::associative::Map;
use crate::spatial::Pose;

pub const INVALID_ID: u32 = u32::MAX;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum JointType {
    #[default]
    Fixed,
    Revolute,
    Continuous,
    Prismatic,
    Floating,
    Planar,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointLimits {
    pub lower: f64,
    pub upper: f64,
    pub effort: f64,
    pub velocity: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointDynamics {
    pub damping: f64,
    pub friction: f64,
}

#[derive(Debug, Clone, PartialEq)]
pub struct JointMimic {
    pub joint: String,
    pub multiplier: f64,
    pub offset: f64,
}

impl Default for JointMimic {
    fn default() -> Self {
        Self { joint: String::new(), multiplier: 1.0, offset: 0.0 }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointSafetyController {
    pub soft_lower_limit: f64,
    pub soft_upper_limit: f64,
    pub k_position: f64,
    pub k_velocity: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointCalibration {
    pub rising: Option<f64>,
    pub falling: Option<f64>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct Joint {
    pub name: String,
    pub r#type: JointType,
    pub origin: Pose,
    pub axis: [f64; 3],
    pub limits: Option<JointLimits>,
    pub dynamics: Option<JointDynamics>,
    pub mimic: Option<JointMimic>,
    pub safety_controller: Option<JointSafetyController>,
    pub calibration: Option<JointCalibration>,
    pub parent: u32,
    pub child: u32,
    pub props: Map<String, String>,
}

impl Default for Joint {
    fn default() -> Self {
        Self {
            name: String::new(),
            r#type: JointType::Fixed,
            origin: Pose::default(),
            axis: [1.0, 0.0, 0.0],
            limits: None,
            dynamics: None,
            mimic: None,
            safety_controller: None,
            calibration: None,
            parent: INVALID_ID,
            child: INVALID_ID,
            props: Map::default(),
        }
    }
}

impl Joint {
    pub fn fixed(name: impl Into<String>, origin: Pose) -> Self {
        Self { name: name.into(), origin, ..Self::default() }
    }

    pub fn revolute(
        name: impl Into<String>,
        axis: [f64; 3],
        limits: JointLimits,
        origin: Pose,
    ) -> Self {
        Self {
            name: name.into(),
            r#type: JointType::Revolute,
            origin,
            axis,
            limits: Some(limits),
            ..Self::default()
        }
    }

    pub fn continuous(name: impl Into<String>, axis: [f64; 3], origin: Pose) -> Self {
        Self {
            name: name.into(),
            r#type: JointType::Continuous,
            origin,
            axis,
            ..Self::default()
        }
    }

    pub fn prismatic(
        name: impl Into<String>,
        axis: [f64; 3],
        limits: JointLimits,
        origin: Pose,
    ) -> Self {
        Self {
            name: name.into(),
            r#type: JointType::Prismatic,
            origin,
            axis,
            limits: Some(limits),
            ..Self::default()
        }
    }

    pub fn is_fixed(&self) -> bool { self.r#type == JointType::Fixed }
    pub fn is_revolute(&self) -> bool { self.r#type == JointType::Revolute }
    pub fn is_continuous(&self) -> bool { self.r#type == JointType::Continuous }
    pub fn is_prismatic(&self) -> bool { self.r#type == JointType::Prismatic }
    pub fn is_floating(&self) -> bool { self.r#type == JointType::Floating }
    pub fn is_planar(&self) -> bool { self.r#type == JointType::Planar }
}
