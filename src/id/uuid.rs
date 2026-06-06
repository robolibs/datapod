use std::fmt;
use std::io::{self, Read};
use std::str::FromStr;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{SystemTime, UNIX_EPOCH};

static UUID_FALLBACK_COUNTER: AtomicU64 = AtomicU64::new(1);

#[datapod::datapod]
#[derive(Eq, Hash, Default)]
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

    pub fn try_generate_v4() -> io::Result<Self> {
        let mut bytes = [0_u8; 16];
        fill_os_random(&mut bytes)?;
        Ok(Self::with_v4_bits(bytes))
    }

    pub fn generate_v4() -> Self {
        Self::try_generate_v4().unwrap_or_else(|_| Self::fallback_v4())
    }

    fn with_v4_bits(mut bytes: [u8; 16]) -> Self {
        bytes[6] = (bytes[6] & 0x0f) | 0x40;
        bytes[8] = (bytes[8] & 0x3f) | 0x80;
        Self { bytes }
    }

    fn fallback_v4() -> Self {
        let counter = UUID_FALLBACK_COUNTER.fetch_add(1, Ordering::Relaxed);
        let nanos = match SystemTime::now().duration_since(UNIX_EPOCH) {
            Ok(duration) => duration.as_nanos(),
            Err(error) => error.duration().as_nanos() ^ u128::MAX,
        };
        let process_id = u128::from(std::process::id());
        let state = Self::mix_fallback_state(nanos, counter, process_id);
        Self::v4_from_state(state)
    }

    fn mix_fallback_state(nanos: u128, counter: u64, process_id: u128) -> u128 {
        let counter = u128::from(counter);
        let mut state = nanos
            ^ (counter << 64)
            ^ (process_id << 32)
            ^ 0xa5a5_5a5a_d3c4_b2e1_1234_5678_9abc_def0u128;
        state ^= state.rotate_left(29);
        state = state.wrapping_mul(0x9e37_79b9_7f4a_7c15_d1b5_4a32_d192_ed03u128);
        state ^= counter.wrapping_mul(0xbf58_476d_1ce4_e5b9_94d0_49bb_1331_11ebu128);
        state ^= process_id.rotate_left(61);
        state ^ state.rotate_right(33)
    }

    fn v4_from_state(mut state: u128) -> Self {
        let mut bytes = [0_u8; 16];
        for byte in &mut bytes {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            *byte = state as u8;
        }
        Self::with_v4_bits(bytes)
    }
}

#[cfg(unix)]
fn fill_os_random(bytes: &mut [u8]) -> io::Result<()> {
    std::fs::File::open("/dev/urandom")?.read_exact(bytes)
}

#[cfg(not(unix))]
fn fill_os_random(_bytes: &mut [u8]) -> io::Result<()> {
    Err(io::Error::new(
        io::ErrorKind::Unsupported,
        "OS random UUID generation is not wired for this platform",
    ))
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

#[cfg(test)]
mod tests {
    use super::Uuid;

    #[test]
    fn v4_from_state_sets_required_version_and_variant_bits() {
        let uuid = Uuid::v4_from_state(0x1234_5678_9abc_def0_0fed_cba9_8765_4321);

        assert_eq!(uuid.bytes[6] & 0xf0, 0x40);
        assert_eq!(uuid.bytes[8] & 0xc0, 0x80);
    }

    #[test]
    fn fallback_state_mixer_uses_each_input() {
        let base = Uuid::mix_fallback_state(1, 2, 3);

        assert_ne!(base, Uuid::mix_fallback_state(4, 2, 3));
        assert_ne!(base, Uuid::mix_fallback_state(1, 4, 3));
        assert_ne!(base, Uuid::mix_fallback_state(1, 2, 4));
    }

    #[test]
    fn fallback_v4_changes_between_calls() {
        let first = Uuid::fallback_v4();
        let second = Uuid::fallback_v4();

        assert_ne!(first.bytes, second.bytes);
        assert_eq!(first.bytes[6] & 0xf0, 0x40);
        assert_eq!(second.bytes[6] & 0xf0, 0x40);
        assert_eq!(first.bytes[8] & 0xc0, 0x80);
        assert_eq!(second.bytes[8] & 0xc0, 0x80);
    }
}
