use crate::sugar::{Ip, Uuid};

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Identity {
    pub name: String,
    pub uuid: Uuid,
    pub ip: Ip,
    pub rci: u8,
}

impl Identity {
    pub fn new(name: impl Into<String>, uuid: Uuid) -> Self {
        Self { name: name.into(), uuid, ip: Ip::default(), rci: 0 }
    }

    pub fn with_ip(name: impl Into<String>, uuid: Uuid, ip: Ip) -> Self {
        Self { name: name.into(), uuid, ip, rci: 0 }
    }
}
