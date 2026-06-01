use std::fmt;
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr};
use std::str::FromStr;

/// Pod-form IP address. Always 16 bytes for the address with a family tag.
/// IPv4 addresses occupy the first 4 bytes; the remaining 12 are zero.
#[datapod::datapod]
#[derive(Eq, Hash)]
pub struct Ip {
    /// `4` for IPv4, `6` for IPv6, `0` for unset.
    pub family: u32,
    pub _pad: u32,
    /// Address bytes. For IPv4, only `bytes[..4]` is meaningful.
    pub bytes: [u8; 16],
}

impl Default for Ip {
    fn default() -> Self {
        Self {
            family: 4,
            _pad: 0,
            bytes: [0u8; 16],
        }
    }
}

impl Ip {
    pub const fn from_v4_bytes(bytes: [u8; 4]) -> Self {
        let mut storage = [0u8; 16];
        storage[0] = bytes[0];
        storage[1] = bytes[1];
        storage[2] = bytes[2];
        storage[3] = bytes[3];
        Self {
            family: 4,
            _pad: 0,
            bytes: storage,
        }
    }

    pub const fn from_v6_bytes(bytes: [u8; 16]) -> Self {
        Self {
            family: 6,
            _pad: 0,
            bytes,
        }
    }

    pub fn is_v4(&self) -> bool {
        self.family == 4
    }

    pub fn is_v6(&self) -> bool {
        self.family == 6
    }

    pub fn v4_bytes(&self) -> Option<[u8; 4]> {
        if self.is_v4() {
            Some([self.bytes[0], self.bytes[1], self.bytes[2], self.bytes[3]])
        } else {
            None
        }
    }

    pub fn v6_bytes(&self) -> Option<[u8; 16]> {
        if self.is_v6() { Some(self.bytes) } else { None }
    }

    pub fn to_ip_addr(self) -> IpAddr {
        if self.is_v4() {
            IpAddr::V4(Ipv4Addr::from(self.v4_bytes().unwrap()))
        } else {
            IpAddr::V6(Ipv6Addr::from(self.bytes))
        }
    }

    pub fn from_string(input: &str) -> Result<Self, String> {
        input.parse()
    }
}

impl fmt::Display for Ip {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.is_v4() {
            write!(
                f,
                "{}.{}.{}.{}",
                self.bytes[0], self.bytes[1], self.bytes[2], self.bytes[3]
            )
        } else {
            for group in 0..8 {
                if group > 0 {
                    f.write_str(":")?;
                }
                let word = u16::from_be_bytes([self.bytes[group * 2], self.bytes[group * 2 + 1]]);
                write!(f, "{word:04x}")?;
            }
            Ok(())
        }
    }
}

impl From<IpAddr> for Ip {
    fn from(value: IpAddr) -> Self {
        match value {
            IpAddr::V4(v4) => Self::from_v4_bytes(v4.octets()),
            IpAddr::V6(v6) => Self::from_v6_bytes(v6.octets()),
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
            return Ok(Self::from_v4_bytes(v4.octets()));
        }

        if trimmed.contains("::") {
            return Err("IP parse error: IPv6 compression is not supported".into());
        }

        Ipv6Addr::from_str(trimmed)
            .map(|v6| Self::from_v6_bytes(v6.octets()))
            .map_err(|_| "IP parse error: invalid IPv4/IPv6 address".to_string())
    }
}
