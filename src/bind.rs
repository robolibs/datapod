//! Shared binding helpers for the C ABI and Python module.

use crate::{DataPod, DataPodValidate};

/// Stable datapod type discriminator used by language bindings.
///
/// Registered built-ins use their canonical schema name. Application-local
/// datapods can opt into a canonical schema name with
/// `#[datapod(name = "...")]`; unnamed custom datapods fall back to the Rust
/// type path.
pub fn type_hash<T: DataPod>() -> u64 {
    T::CANONICAL_NAME
        .or_else(|| crate::registry::canonical_name::<T>())
        .map_or_else(|| rust_type_hash::<T>(), type_hash_name)
}

/// FNV-1a hash of a Rust type path. This is for non-datapod POD helper records
/// and debugging; datapod wire identities use [`type_hash`].
pub fn rust_type_hash<T: 'static>() -> u64 {
    type_hash_name(core::any::type_name::<T>())
}

/// Stable datapod discriminator derived from the registered canonical type name.
pub fn type_hash_canonical<T: DataPod>() -> Option<u64> {
    T::CANONICAL_NAME
        .or_else(|| crate::registry::canonical_name::<T>())
        .map(type_hash_name)
}

/// True when `type_hash` names `T` using the current canonical-name hash.
pub fn is_type_hash_for<T: DataPod>(hash: u64) -> bool {
    hash == type_hash::<T>()
}

/// Hash currently emitted for a built-in datapod type.
///
/// In `datapod-wire-v1/le`, built-ins emit the canonical-name hash. Runtime
/// schemas also emit canonical-name hashes.
pub fn emitted_type_hash<T: DataPod>() -> u64 {
    type_hash::<T>()
}

/// FNV-1a discriminator for an explicit canonical type name.
///
/// User-defined C/Python schemas should use this helper on a canonical string
/// such as `acme.depth_image.v1` and then register the schema metadata with the
/// runtime registry.
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
    let Some(slot) = out.get_mut(..bytes.len()) else {
        return Err(format!(
            "header output range unavailable: got {}, need {}",
            out.len(),
            bytes.len()
        ));
    };
    slot.copy_from_slice(bytes);
    Ok(())
}

/// Read a fixed POD value from its header bytes.
pub fn read_fixed_header<T>(bytes: &[u8]) -> Result<T, String>
where
    T: DataPod<Header = T> + DataPodValidate + bytemuck::Pod + Copy,
{
    let need = core::mem::size_of::<T>();
    if bytes.len() != need {
        return Err(format!(
            "header input has wrong size: got {}, need {}",
            bytes.len(),
            need
        ));
    }
    let header = bytemuck::try_pod_read_unaligned(bytes)
        .map_err(|_| "invalid fixed datapod header".to_string())?;
    T::validate_wire_parts(&header, &[]).map_err(|error| error.to_string())?;
    Ok(header)
}
