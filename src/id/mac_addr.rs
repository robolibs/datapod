use std::fmt;
use std::str::FromStr;

#[datapod::datapod]
#[derive(Eq, Hash, Default)]
pub struct MacAddr {
    pub bytes: [u8; 6],
    pub _pad: [u8; 2],
}

impl MacAddr {
    pub const fn new(bytes: [u8; 6]) -> Self {
        Self { bytes, _pad: [0; 2] }
    }

    pub fn to_colon_string(&self) -> String {
        format!(
            "{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
            self.bytes[0],
            self.bytes[1],
            self.bytes[2],
            self.bytes[3],
            self.bytes[4],
            self.bytes[5]
        )
    }

    pub fn from_string(input: &str) -> Result<Self, String> {
        input.parse()
    }
}

impl fmt::Display for MacAddr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.to_colon_string())
    }
}

impl FromStr for MacAddr {
    type Err = String;

    fn from_str(input: &str) -> Result<Self, Self::Err> {
        let trimmed = input.trim();
        let separator = if trimmed.contains(':') {
            ':'
        } else if trimmed.contains('-') {
            '-'
        } else {
            return Err("MacAddr parse error: expected ':' or '-' separator".into());
        };

        let parts: Vec<_> = trimmed.split(separator).collect();
        if parts.len() != 6 {
            return Err("MacAddr parse error: expected 6 octets".into());
        }

        let mut bytes = [0_u8; 6];
        for (index, part) in parts.into_iter().enumerate() {
            if part.len() != 2 {
                return Err("MacAddr parse error: each octet must have 2 hex digits".into());
            }
            bytes[index] = u8::from_str_radix(part, 16)
                .map_err(|_| "MacAddr parse error: invalid hex digit".to_string())?;
        }

        Ok(Self { bytes, _pad: [0; 2] })
    }
}
