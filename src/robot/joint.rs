use crate::id::STRING_NONE;
use crate::motion::Pose;

use super::kv::KV;

pub const INVALID_ID: u32 = u32::MAX;

/// Max number of `(key, value)` URDF property pairs per joint.
pub const JOINT_PROP_CAP: usize = 8;

#[repr(transparent)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct JointType(pub u32);

#[allow(non_upper_case_globals)]
impl JointType {
    pub const Fixed: Self = Self(0);
    pub const Revolute: Self = Self(1);
    pub const Continuous: Self = Self(2);
    pub const Prismatic: Self = Self(3);
    pub const Floating: Self = Self(4);
    pub const Planar: Self = Self(5);

    pub fn is_valid(self) -> bool {
        self.0 <= Self::Planar.0
    }
}

// Manual Pod impls — bytemuck can't derive Pod on enums.
// Safety: #[repr(u32)] gives a fixed 4-byte layout; every declared variant
// is a valid u32 bit pattern, and the all-zero pattern (= Fixed) is valid.
unsafe impl bytemuck::Zeroable for JointType {}
unsafe impl bytemuck::Pod for JointType {}
unsafe impl crate::ZeroCopySend for JointType {}
impl crate::LeWireHeader for JointType {
    const LE_WIRE_SIZE: usize = <u32 as crate::LeWireHeader>::LE_WIRE_SIZE;

    fn write_le(&self, out: &mut Vec<u8>) {
        <u32 as crate::LeWireHeader>::write_le(&self.0, out);
    }

    fn read_le(bytes: &[u8]) -> Result<Self, crate::WireError> {
        Ok(Self(<u32 as crate::LeWireHeader>::read_le(bytes)?))
    }
}

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

impl crate::DataPodDecode for JointType {
    fn from_wire_parts(header: Self::Header, payload: Vec<u8>) -> Result<Self, crate::WireError> {
        <Self as crate::DataPodValidate>::validate_wire_parts(&header, &payload)?;
        Ok(header)
    }
}

impl crate::DataPodValidate for JointType {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), crate::WireError> {
        if !payload.is_empty() {
            return Err(crate::WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            });
        }
        if !header.is_valid() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "unknown joint type tag {}",
                header.0
            )));
        }
        Ok(())
    }
}

impl crate::DataPodAccess for JointType {
    type View<'a> = crate::FixedView<Self>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, crate::WireError> {
        <Self as crate::DataPodValidate>::validate_wire_parts(&header, payload)?;
        Ok(crate::FixedView { value: header })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        _payload: &'a [u8],
    ) -> Self::View<'a> {
        crate::FixedView { value: header }
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
#[dp(manual_access)]
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

impl crate::DataPodValidate for Joint {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), crate::WireError> {
        if !payload.is_empty() {
            return Err(crate::WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            });
        }
        if !header.joint_type.is_valid() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "unknown joint type tag {}",
                header.joint_type.0
            )));
        }
        for (name, value) in [
            ("limits_present", header.limits_present),
            ("dynamics_present", header.dynamics_present),
            ("mimic_present", header.mimic_present),
            ("safety_present", header.safety_present),
            ("calibration_present", header.calibration_present),
        ] {
            if value > 1 {
                return Err(crate::wire::invalid_header::<Self>(format!(
                    "{name} must be 0 or 1"
                )));
            }
        }
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        if header.axis.iter().any(|value| !value.is_finite()) {
            return Err(crate::wire::invalid_header::<Self>(
                "axis must contain finite values",
            ));
        }
        if header.mimic._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "mimic reserved _pad field must be zero",
            ));
        }
        for (name, value) in [
            ("limits.lower", header.limits.lower),
            ("limits.upper", header.limits.upper),
            ("limits.effort", header.limits.effort),
            ("limits.velocity", header.limits.velocity),
            ("dynamics.damping", header.dynamics.damping),
            ("dynamics.friction", header.dynamics.friction),
            ("mimic.multiplier", header.mimic.multiplier),
            ("mimic.offset", header.mimic.offset),
            (
                "safety.soft_lower_limit",
                header.safety_controller.soft_lower_limit,
            ),
            (
                "safety.soft_upper_limit",
                header.safety_controller.soft_upper_limit,
            ),
            ("safety.k_position", header.safety_controller.k_position),
            ("safety.k_velocity", header.safety_controller.k_velocity),
            ("calibration.rising", header.calibration.rising),
            ("calibration.falling", header.calibration.falling),
        ] {
            if !value.is_finite() {
                return Err(crate::wire::invalid_header::<Self>(format!(
                    "{name} must be finite"
                )));
            }
        }
        Ok(())
    }
}

impl crate::DataPodAccess for Joint {
    type View<'a> = crate::FixedView<Self>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, crate::WireError> {
        <Self as crate::DataPodValidate>::validate_wire_parts(&header, payload)?;
        Ok(crate::FixedView { value: header })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        _payload: &'a [u8],
    ) -> Self::View<'a> {
        crate::FixedView { value: header }
    }
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
