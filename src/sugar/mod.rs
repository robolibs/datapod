use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Ip {
    V4(Ipv4Addr),
    V6(Ipv6Addr),
}

impl Default for Ip {
    fn default() -> Self {
        Self::V4(Ipv4Addr::UNSPECIFIED)
    }
}

impl From<IpAddr> for Ip {
    fn from(value: IpAddr) -> Self {
        match value {
            IpAddr::V4(v4) => Self::V4(v4),
            IpAddr::V6(v6) => Self::V6(v6),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct MacAddr {
    pub bytes: [u8; 6],
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Uuid {
    pub bytes: [u8; 16],
}
