use std::fmt;
use std::str::FromStr;

use crate::{DataPodAccess, DataPodValidate, FixedView, WireError};

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Eq, Hash, Default)]
pub struct MacAddr {
    pub bytes: [u8; 6],
    pub _pad: [u8; 2],
}

impl MacAddr {
    pub const fn new(bytes: [u8; 6]) -> Self {
        Self {
            bytes,
            _pad: [0; 2],
        }
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

        let mut bytes = [0_u8; 6];
        let mut count = 0usize;
        for part in trimmed.split(separator) {
            if count >= bytes.len() {
                return Err("MacAddr parse error: expected 6 octets".into());
            }
            if part.len() != 2 {
                return Err("MacAddr parse error: each octet must have 2 hex digits".into());
            }
            bytes[count] = u8::from_str_radix(part, 16)
                .map_err(|_| "MacAddr parse error: invalid hex digit".to_string())?;
            count += 1;
        }
        if count != bytes.len() {
            return Err("MacAddr parse error: expected 6 octets".into());
        }

        Ok(Self {
            bytes,
            _pad: [0; 2],
        })
    }
}

impl DataPodValidate for MacAddr {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if !payload.is_empty() {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "fixed datapod payload must be empty, got {}",
                payload.len()
            )));
        }
        if header._pad != [0; 2] {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        Ok(())
    }
}

impl DataPodAccess for MacAddr {
    type View<'a> = FixedView<Self>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(FixedView { value: header })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        _payload: &'a [u8],
    ) -> Self::View<'a> {
        FixedView { value: header }
    }
}
