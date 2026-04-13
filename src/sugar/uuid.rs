use std::fmt;
use std::str::FromStr;
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Uuid {
    pub bytes: [u8; 16],
}

impl Uuid {
    pub const fn nil() -> Self {
        Self { bytes: [0; 16] }
    }

    pub fn to_hyphenated_string(&self) -> String {
        format!(
            "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            self.bytes[0],
            self.bytes[1],
            self.bytes[2],
            self.bytes[3],
            self.bytes[4],
            self.bytes[5],
            self.bytes[6],
            self.bytes[7],
            self.bytes[8],
            self.bytes[9],
            self.bytes[10],
            self.bytes[11],
            self.bytes[12],
            self.bytes[13],
            self.bytes[14],
            self.bytes[15]
        )
    }

    pub fn from_string(input: &str) -> Result<Self, String> {
        input.parse()
    }

    pub fn generate_v4() -> Self {
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|duration| duration.as_nanos())
            .unwrap_or(0);
        let mut state = nanos ^ 0xa5a5_5a5a_d3c4_b2e1_1234_5678_9abc_def0u128;
        let mut bytes = [0_u8; 16];
        for byte in &mut bytes {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            *byte = state as u8;
        }
        bytes[6] = (bytes[6] & 0x0f) | 0x40;
        bytes[8] = (bytes[8] & 0x3f) | 0x80;
        Self { bytes }
    }
}

impl fmt::Display for Uuid {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.to_hyphenated_string())
    }
}

impl FromStr for Uuid {
    type Err = String;

    fn from_str(input: &str) -> Result<Self, Self::Err> {
        let trimmed = input.trim();
        let mut chars = trimmed.chars();
        let mut bytes = [0_u8; 16];
        let next_nibble = |chars: &mut std::str::Chars<'_>| -> Result<u8, String> {
            let ch = chars
                .next()
                .ok_or_else(|| "UUID parse error: unexpected end of input".to_string())?;
            ch.to_digit(16)
                .map(|digit| digit as u8)
                .ok_or_else(|| "UUID parse error: invalid hex digit".to_string())
        };

        for index in 0..16 {
            if matches!(index, 4 | 6 | 8 | 10) {
                match chars.next() {
                    Some('-') => {}
                    _ => return Err("UUID parse error: expected '-' separator".into()),
                }
            }
            let hi = next_nibble(&mut chars)?;
            let lo = next_nibble(&mut chars)?;
            bytes[index] = (hi << 4) | lo;
        }

        if chars.next().is_some() {
            return Err("UUID parse error: extra characters after UUID".into());
        }

        Ok(Self { bytes })
    }
}
