//! Shared binding helpers for the C ABI and Python module.

use crate::DataPod;

/// Stable-ish datapod type discriminator used by language bindings.
///
/// The hash is derived from the Rust type path with FNV-1a. It is deliberately
/// centralized here so C and Python expose the same value for the same Rust
/// implementation type.
pub fn type_hash<T: 'static>() -> u64 {
    type_hash_name(core::any::type_name::<T>())
}

/// FNV-1a discriminator for an explicit canonical type name.
///
/// Built-in Rust types currently use [`type_hash`] over the Rust type path for
/// 0.3 compatibility. User-defined C/Python schemas should use this helper on a
/// canonical string such as `acme.depth_image.v1` and then register the schema
/// metadata with the runtime registry.
pub fn type_hash_name(name: &str) -> u64 {
    const OFFSET: u64 = 0xcbf29ce484222325;
    const PRIME: u64 = 0x00000100000001b3;

    let mut hash = OFFSET;
    for byte in name.as_bytes() {
        hash ^= *byte as u64;
        hash = hash.wrapping_mul(PRIME);
    }
    hash
}

/// Header byte width for a datapod type.
pub fn header_size<T: DataPod>() -> usize {
    core::mem::size_of::<T::Header>()
}

/// Copy a fixed datapod header into `out`.
pub fn write_header<T: DataPod>(value: &T, out: &mut [u8]) -> Result<(), String> {
    let header = value.header();
    let bytes = bytemuck::bytes_of(&header);
    if out.len() < bytes.len() {
        return Err(format!(
            "header output too small: got {}, need {}",
            out.len(),
            bytes.len()
        ));
    }
    out[..bytes.len()].copy_from_slice(bytes);
    Ok(())
}

/// Read a fixed POD value from its header bytes.
pub fn read_fixed_header<T>(bytes: &[u8]) -> Result<T, String>
where
    T: DataPod<Header = T> + bytemuck::Pod + Copy,
{
    let need = core::mem::size_of::<T>();
    if bytes.len() != need {
        return Err(format!(
            "header input has wrong size: got {}, need {}",
            bytes.len(),
            need
        ));
    }
    bytemuck::try_pod_read_unaligned(bytes).map_err(|_| "invalid fixed datapod header".to_string())
}
