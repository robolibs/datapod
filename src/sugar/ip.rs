use std::fmt;
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
use std::str::FromStr;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Ip {
    V4([u8; 4]),
    V6([u8; 16]),
}

impl Default for Ip {
    fn default() -> Self {
        Self::V4([0, 0, 0, 0])
    }
}

impl Ip {
    pub const fn from_v4_bytes(bytes: [u8; 4]) -> Self {
        Self::V4(bytes)
    }

    pub const fn from_v6_bytes(bytes: [u8; 16]) -> Self {
        Self::V6(bytes)
    }

    pub fn is_v4(&self) -> bool {
        matches!(self, Self::V4(_))
    }

    pub fn is_v6(&self) -> bool {
        matches!(self, Self::V6(_))
    }

    pub fn v4_bytes(&self) -> Option<&[u8; 4]> {
        match self {
            Self::V4(bytes) => Some(bytes),
            Self::V6(_) => None,
        }
    }

    pub fn v6_bytes(&self) -> Option<&[u8; 16]> {
        match self {
            Self::V4(_) => None,
            Self::V6(bytes) => Some(bytes),
        }
    }

    pub fn to_ip_addr(self) -> IpAddr {
        match self {
            Self::V4(bytes) => IpAddr::V4(Ipv4Addr::from(bytes)),
            Self::V6(bytes) => IpAddr::V6(Ipv6Addr::from(bytes)),
        }
    }

    pub fn from_string(input: &str) -> Result<Self, String> {
        input.parse()
    }
}
impl fmt::Display for Ip {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::V4(bytes) => write!(f, "{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]),
            Self::V6(bytes) => {
                for group in 0..8 {
                    if group > 0 {
                        f.write_str(":")?;
                    }
                    let word = u16::from_be_bytes([bytes[group * 2], bytes[group * 2 + 1]]);
                    write!(f, "{word:04x}")?;
                }
                Ok(())
            }
        }
    }
}

impl From<IpAddr> for Ip {
    fn from(value: IpAddr) -> Self {
        match value {
            IpAddr::V4(v4) => Self::V4(v4.octets()),
            IpAddr::V6(v6) => Self::V6(v6.octets()),
        }
    }
}

impl From<Ip> for IpAddr {
    fn from(value: Ip) -> Self {
        value.to_ip_addr()
    }
}

impl FromStr for Ip {
    type Err = String;

    fn from_str(input: &str) -> Result<Self, Self::Err> {
        let trimmed = input.trim();
        if let Ok(v4) = Ipv4Addr::from_str(trimmed) {
            return Ok(Self::V4(v4.octets()));
        }

        if trimmed.contains("::") {
            return Err("IP parse error: IPv6 compression is not supported".into());
        }

        Ipv6Addr::from_str(trimmed)
            .map(|v6| Self::V6(v6.octets()))
            .map_err(|_| "IP parse error: invalid IPv4/IPv6 address".to_string())
    }
}
