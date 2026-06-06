//! Inline fixed-cap key/value pair for URDF-style properties.
//!
//! Used by [`Link::props`] and [`Joint::props`]. Both key and value are
//! null-padded byte arrays; helpers convert to/from `&str`. An unused slot
//! is identified by `key[0] == 0`.

use crate::WireError;

#[datapod::datapod]
#[derive(Eq, Hash)]
pub struct KV {
    pub key: [u8; 32],
    pub value: [u8; 64],
}

impl Default for KV {
    fn default() -> Self {
        Self {
            key: [0; 32],
            value: [0; 64],
        }
    }
}

impl KV {
    pub fn is_set(&self) -> bool {
        self.key[0] != 0
    }

    pub fn key_str(&self) -> &str {
        self.try_key_str().unwrap_or("")
    }

    pub fn try_key_str(&self) -> Result<&str, WireError> {
        let end = self
            .key
            .iter()
            .position(|&b| b == 0)
            .unwrap_or(self.key.len());
        let bytes = self.key.get(..end).ok_or_else(|| {
            crate::wire::invalid_header::<Self>("key byte range is out of bounds")
        })?;
        std::str::from_utf8(bytes).map_err(|error| {
            crate::wire::invalid_header::<Self>(format!("key is not UTF-8 before NUL: {error}"))
        })
    }

    pub fn value_str(&self) -> &str {
        self.try_value_str().unwrap_or("")
    }

    pub fn try_value_str(&self) -> Result<&str, WireError> {
        let end = self
            .value
            .iter()
            .position(|&b| b == 0)
            .unwrap_or(self.value.len());
        let bytes = self.value.get(..end).ok_or_else(|| {
            crate::wire::invalid_header::<Self>("value byte range is out of bounds")
        })?;
        std::str::from_utf8(bytes).map_err(|error| {
            crate::wire::invalid_header::<Self>(format!("value is not UTF-8 before NUL: {error}"))
        })
    }

    pub fn set_key(&mut self, s: &str) {
        copy_str_into(&mut self.key, s);
    }

    pub fn set_value(&mut self, s: &str) {
        copy_str_into(&mut self.value, s);
    }
}

fn copy_str_into<const N: usize>(dst: &mut [u8; N], s: &str) {
    *dst = [0; N];
    let bytes = s.as_bytes();
    let n = bytes.len().min(N);
    let Some(out) = dst.get_mut(..n) else {
        return;
    };
    let Some(input) = bytes.get(..n) else {
        return;
    };
    out.copy_from_slice(input);
}
