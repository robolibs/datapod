use crate::Vector;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Actuator {
    pub name: String,
    pub mechanical_reduction: Option<f64>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct TransmissionJoint {
    pub name: String,
    pub mechanical_reduction: Option<f64>,
    pub offset: Option<f64>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Transmission {
    pub name: String,
    pub r#type: String,
    pub joints: Vector<TransmissionJoint>,
    pub actuators: Vector<Actuator>,
}
