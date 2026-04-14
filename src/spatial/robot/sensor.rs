use crate::associative::Map;
use crate::spatial::Pose;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Sensor {
    pub name: String,
    pub r#type: String,
    pub origin: Pose,
    pub props: Map<String, String>,
}

impl Sensor {
    pub fn new(name: impl Into<String>, r#type: impl Into<String>, origin: Pose) -> Self {
        Self {
            name: name.into(),
            r#type: r#type.into(),
            origin,
            props: Map::default(),
        }
    }
}
