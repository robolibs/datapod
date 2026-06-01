use crate::id::STRING_NONE;
use crate::motion::Pose;

use super::kv::KV;

pub const INVALID_ID: u32 = u32::MAX;

/// Max number of `(key, value)` URDF property pairs per joint.
pub const JOINT_PROP_CAP: usize = 8;

#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum JointType {
    Fixed = 0,
    Revolute = 1,
    Continuous = 2,
    Prismatic = 3,
    Floating = 4,
    Planar = 5,
}

impl Default for JointType {
    fn default() -> Self {
        Self::Fixed
    }
}

// Manual Pod impls — bytemuck can't derive Pod on enums.
// Safety: #[repr(u32)] gives a fixed 4-byte layout; every declared variant
// is a valid u32 bit pattern, and the all-zero pattern (= Fixed) is valid.
unsafe impl bytemuck::Zeroable for JointType {}
unsafe impl bytemuck::Pod for JointType {}
unsafe impl crate::ZeroCopySend for JointType {}
impl crate::DataPod for JointType {
    type Header = JointType;
    type Payload = ();
    fn header(&self) -> JointType {
        *self
    }
    fn payload_bytes(&self) -> &[u8] {
        &[]
    }
}

#[datapod::datapod]
#[derive(Default)]
pub struct JointLimits {
    pub lower: f64,
    pub upper: f64,
    pub effort: f64,
    pub velocity: f64,
}

#[datapod::datapod]
#[derive(Default)]
pub struct JointDynamics {
    pub damping: f64,
    pub friction: f64,
}

/// Mimic relation — this joint follows `mimicked_id` with `multiplier`/`offset`.
#[datapod::datapod]
pub struct JointMimic {
    /// ID of the mimicked joint (`INVALID_ID` if unset).
    pub mimicked_id: u32,
    pub _pad: u32,
    pub multiplier: f64,
    pub offset: f64,
}

impl Default for JointMimic {
    fn default() -> Self {
        Self {
            mimicked_id: INVALID_ID,
            _pad: 0,
            multiplier: 1.0,
            offset: 0.0,
        }
    }
}

#[datapod::datapod]
#[derive(Default)]
pub struct JointSafetyController {
    pub soft_lower_limit: f64,
    pub soft_upper_limit: f64,
    pub k_position: f64,
    pub k_velocity: f64,
}

/// Calibration thresholds. The `*_present` flags act as `Option<f64>`.
#[datapod::datapod]
#[derive(Default)]
pub struct JointCalibration {
    pub rising_present: u32,
    pub falling_present: u32,
    pub rising: f64,
    pub falling: f64,
}

#[datapod::datapod]
pub struct Joint {
    /// Name as a [`DpString`](crate::spatial::sugar::DpString) ID.
    pub name_id: u32,
    /// Joint type tag.
    pub joint_type: JointType,
    /// Parent link ID (`INVALID_ID` if root).
    pub parent_id: u32,
    /// Child link ID (`INVALID_ID` if no child).
    pub child_id: u32,
    /// Presence flags for the optional sub-records.
    pub limits_present: u32,
    pub dynamics_present: u32,
    pub mimic_present: u32,
    pub safety_present: u32,
    pub calibration_present: u32,
    pub _pad: u32,
    pub origin: Pose,
    pub axis: [f64; 3],
    pub limits: JointLimits,
    pub dynamics: JointDynamics,
    pub mimic: JointMimic,
    pub safety_controller: JointSafetyController,
    pub calibration: JointCalibration,
    pub props: [KV; JOINT_PROP_CAP],
}

impl Default for Joint {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            joint_type: JointType::Fixed,
            parent_id: INVALID_ID,
            child_id: INVALID_ID,
            limits_present: 0,
            dynamics_present: 0,
            mimic_present: 0,
            safety_present: 0,
            calibration_present: 0,
            _pad: 0,
            origin: Pose::default(),
            axis: [1.0, 0.0, 0.0],
            limits: JointLimits::default(),
            dynamics: JointDynamics::default(),
            mimic: JointMimic::default(),
            safety_controller: JointSafetyController::default(),
            calibration: JointCalibration::default(),
            props: [KV::default(); JOINT_PROP_CAP],
        }
    }
}

impl Joint {
    pub fn fixed(name_id: u32, origin: Pose) -> Self {
        Self {
            name_id,
            origin,
            ..Self::default()
        }
    }

    pub fn revolute(name_id: u32, axis: [f64; 3], limits: JointLimits, origin: Pose) -> Self {
        Self {
            name_id,
            joint_type: JointType::Revolute,
            origin,
            axis,
            limits_present: 1,
            limits,
            ..Self::default()
        }
    }

    pub fn continuous(name_id: u32, axis: [f64; 3], origin: Pose) -> Self {
        Self {
            name_id,
            joint_type: JointType::Continuous,
            origin,
            axis,
            ..Self::default()
        }
    }

    pub fn prismatic(name_id: u32, axis: [f64; 3], limits: JointLimits, origin: Pose) -> Self {
        Self {
            name_id,
            joint_type: JointType::Prismatic,
            origin,
            axis,
            limits_present: 1,
            limits,
            ..Self::default()
        }
    }

    pub fn is_fixed(&self) -> bool {
        self.joint_type == JointType::Fixed
    }
    pub fn is_revolute(&self) -> bool {
        self.joint_type == JointType::Revolute
    }
    pub fn is_continuous(&self) -> bool {
        self.joint_type == JointType::Continuous
    }
    pub fn is_prismatic(&self) -> bool {
        self.joint_type == JointType::Prismatic
    }
    pub fn is_floating(&self) -> bool {
        self.joint_type == JointType::Floating
    }
    pub fn is_planar(&self) -> bool {
        self.joint_type == JointType::Planar
    }

    pub fn has_limits(&self) -> bool {
        self.limits_present != 0
    }
    pub fn has_dynamics(&self) -> bool {
        self.dynamics_present != 0
    }
    pub fn has_mimic(&self) -> bool {
        self.mimic_present != 0
    }
    pub fn has_safety_controller(&self) -> bool {
        self.safety_present != 0
    }
    pub fn has_calibration(&self) -> bool {
        self.calibration_present != 0
    }
}
