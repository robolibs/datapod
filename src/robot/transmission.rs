use crate::id::STRING_NONE;

/// Max number of joints attached to a single transmission.
pub const TRANSMISSION_JOINT_CAP: usize = 8;
/// Max number of actuators attached to a single transmission.
pub const TRANSMISSION_ACTUATOR_CAP: usize = 8;

#[datapod::datapod]
pub struct Actuator {
    pub name_id: u32,
    pub reduction_present: u32,
    pub mechanical_reduction: f64,
}

impl Default for Actuator {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            reduction_present: 0,
            mechanical_reduction: 1.0,
        }
    }
}

#[datapod::datapod]
pub struct TransmissionJoint {
    pub name_id: u32,
    pub reduction_present: u32,
    pub offset_present: u32,
    pub _pad: u32,
    pub mechanical_reduction: f64,
    pub offset: f64,
}

impl Default for TransmissionJoint {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            reduction_present: 0,
            offset_present: 0,
            _pad: 0,
            mechanical_reduction: 1.0,
            offset: 0.0,
        }
    }
}

#[datapod::datapod]
pub struct Transmission {
    pub name_id: u32,
    pub type_id: u32,
    pub joints: [TransmissionJoint; TRANSMISSION_JOINT_CAP],
    pub actuators: [Actuator; TRANSMISSION_ACTUATOR_CAP],
}

impl Default for Transmission {
    fn default() -> Self {
        Self {
            name_id: STRING_NONE,
            type_id: STRING_NONE,
            joints: [TransmissionJoint::default(); TRANSMISSION_JOINT_CAP],
            actuators: [Actuator::default(); TRANSMISSION_ACTUATOR_CAP],
        }
    }
}
