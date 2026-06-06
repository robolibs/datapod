//! C ABI for datapod.
//!
//! Conventions: opaque Box-backed handles (free with the matching
//! `*_free`); fallible calls return bool/int with the reason in the
//! thread-local [`datapod_last_error_message`]; byte views borrow memory
//! owned by the handle they came from.
//!
//! `include/datapod.h` is generated from this file by cbindgen.

// extern "C" fns take raw pointers from C and deref them by design.
#![allow(clippy::not_unsafe_ptr_arg_deref)]

use std::cell::RefCell;
use std::ffi::{CStr, CString, c_char};
use std::mem::ManuallyDrop;
use std::ptr;

use crate::wire::{Encoding, Envelope};
use crate::{
    Aabb, Accel, Acceleration, Actuator, BitVec, BoundingSphere, BoxShape, Bytes, Circle,
    Collision, CylinderShape, DataPod, DataPodValidate, Deque, DpStr, DpString, Euler, ForwardList,
    GaussianBox, GaussianCircle, GaussianPoint, GaussianRectangle, Geo, Geometry, GeometryKind,
    Grid, Heap, Identity, IndexedHeap, Inertial, Ip, Joint, JointCalibration, JointDynamics,
    JointLimits, JointMimic, JointSafetyController, JointType, KV, Layer, Line, Linestring, Link,
    List, Loc, MacAddr, Map, MapEntry, Material, Matrix, MeshShape, Model, MultiPoint, Obb, Odom,
    PagedVecvec, Path, Point, PointKey, Polygon, Pose, Quaternion, Queue, Rectangle, Ring, Robot,
    Segment, Sensor, Set, SetEntry, Size, SphereShape, Square, Stack, State, Tensor, Trajectory,
    Transform, Transmission, TransmissionJoint, Triangle, Twist, Utm, Uuid, Vector, Vecvec,
    Velocity, Visual, Wrench,
};

thread_local! {
    static LAST_ERROR: RefCell<Option<CString>> = const { RefCell::new(None) };
    static TYPE_NAME_RESULT: RefCell<Option<CString>> = const { RefCell::new(None) };
}

fn clear_last_error() {
    LAST_ERROR.with(|slot| *slot.borrow_mut() = None);
}

fn set_last_error(message: impl Into<String>) {
    let message = message.into().replace('\0', " ");
    LAST_ERROR.with(|slot| {
        *slot.borrow_mut() = Some(c_string_without_nul(message));
    });
}

fn metadata_type_info(type_hash: u64) -> Option<crate::registry::TypeInfo> {
    clear_last_error();
    let Some(info) = crate::registry::find_type_info(type_hash) else {
        set_last_error(format!("unknown datapod type hash: {type_hash}"));
        return None;
    };
    Some(info)
}

fn c_string_without_nul(message: String) -> CString {
    let raw = message.into_bytes();
    if !raw.contains(&0) {
        // SAFETY: the branch above proved there are no interior NUL bytes.
        return unsafe { CString::from_vec_unchecked(raw) };
    }

    let mut bytes = Vec::new();
    if bytes.try_reserve_exact(raw.len()).is_err() {
        // Best effort: avoid panicking while recording an error. An empty
        // CString is still a valid sentinel for C callers.
        return CString::default();
    }
    bytes.extend(raw.into_iter().filter(|b| *b != 0));
    // SAFETY: every interior NUL byte was removed above.
    unsafe { CString::from_vec_unchecked(bytes) }
}

fn canonical_type_name_in<'a>(name: *const c_char) -> Result<&'a str, ()> {
    if name.is_null() {
        set_last_error("null datapod canonical type name");
        return Err(());
    }
    let Ok(name) = (unsafe { CStr::from_ptr(name) }).to_str() else {
        set_last_error("datapod canonical type name is not utf-8");
        return Err(());
    };
    if name.is_empty() {
        set_last_error("datapod canonical type name is empty");
        return Err(());
    }
    if let Err(error) = crate::registry::validate_canonical_name(name) {
        set_last_error(error.to_string());
        return Err(());
    }
    Ok(name)
}

fn canonical_type_name_bytes_in<'a>(name: *const u8, name_len: usize) -> Result<&'a str, ()> {
    // SAFETY: `bytes_in` validates null/length/range shape before borrowing.
    let bytes = unsafe { bytes_in(name, name_len) }?;
    if bytes.is_empty() {
        set_last_error("datapod canonical type name is empty");
        return Err(());
    }
    if bytes.contains(&0) {
        set_last_error("datapod canonical type name must not contain NUL bytes");
        return Err(());
    }
    let Ok(name) = std::str::from_utf8(bytes) else {
        set_last_error("datapod canonical type name is not utf-8");
        return Err(());
    };
    if let Err(error) = crate::registry::validate_canonical_name(name) {
        set_last_error(error.to_string());
        return Err(());
    }
    Ok(name)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_last_error_message() -> *const c_char {
    LAST_ERROR.with(|slot| {
        slot.borrow()
            .as_ref()
            .map(|m| m.as_ptr())
            .unwrap_or(ptr::null())
    })
}

/// A borrowed byte view.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct DatapodBytes {
    pub ptr: *const u8,
    pub len: usize,
}

impl DatapodBytes {
    fn empty() -> Self {
        Self {
            ptr: ptr::null(),
            len: 0,
        }
    }
}

/// Generic borrowed datapod wire message: `data = header || payload`.
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct DatapodWireMessage {
    pub type_hash: u64,
    pub data: *const u8,
    pub len: usize,
}

/// Generic borrowed zero-copy datapod wire frame.
///
/// This is the C ABI fast path: `header` and `payload` are separate borrowed
/// buffers and are not joined or copied by datapod.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default)]
pub struct DatapodWireFrame {
    pub type_hash: u64,
    pub header: *const u8,
    pub header_len: usize,
    pub payload: *const u8,
    pub payload_len: usize,
}

/// Archive/View/Owned terminology alias for the generic borrowed C ABI frame.
///
/// `DatapodWireFrame` remains available for compatibility. New C code can use
/// `DatapodArchiveFrame` to make the zero-copy archive role explicit.
pub type DatapodArchiveFrame = DatapodWireFrame;

impl DatapodWireFrame {
    fn empty() -> Self {
        Self {
            type_hash: 0,
            header: ptr::null(),
            header_len: 0,
            payload: ptr::null(),
            payload_len: 0,
        }
    }
}

/// Owned bytes returned by Rust to C. Free with `datapod_owned_bytes_free`.
#[repr(C)]
#[derive(Debug)]
pub struct DatapodOwnedBytes {
    pub ptr: *mut u8,
    pub len: usize,
    pub capacity: usize,
}

impl DatapodOwnedBytes {
    fn empty() -> Self {
        Self {
            ptr: ptr::null_mut(),
            len: 0,
            capacity: 0,
        }
    }
}

fn owned_bytes(bytes: Vec<u8>) -> DatapodOwnedBytes {
    let mut bytes = ManuallyDrop::new(bytes);
    DatapodOwnedBytes {
        ptr: bytes.as_mut_ptr(),
        len: bytes.len(),
        capacity: bytes.capacity(),
    }
}

fn prepare_owned_bytes_output(out: *mut DatapodOwnedBytes) -> Result<(), ()> {
    if out.is_null() {
        set_last_error("null owned-byte output");
        return Err(());
    }
    // Clear the caller-visible output before validation/allocation. This
    // prevents stale pointers from being accidentally freed or inspected after
    // a failed C ABI call.
    unsafe {
        *out = DatapodOwnedBytes::empty();
    }
    Ok(())
}

fn prepare_wire_frame_output(out: *mut DatapodWireFrame, label: &'static str) -> Result<(), ()> {
    if out.is_null() {
        set_last_error(format!("null {label} output"));
        return Err(());
    }
    // Clear the caller-visible frame before validation. A failed split/archive
    // call must not leave stale header/payload pointers that C code could
    // accidentally inspect as a fresh borrowed frame.
    unsafe {
        *out = DatapodWireFrame::empty();
    }
    Ok(())
}

fn prepare_default_output<T: Default>(out: *mut T, label: &'static str) -> Result<(), ()> {
    if out.is_null() {
        set_last_error(format!("null {label} output"));
        return Err(());
    }
    // Clear typed view/value outputs before validation. C callers often reuse
    // stack-allocated output structs; failed calls must not leave stale
    // borrowed pointers or stale scalar metadata behind.
    unsafe {
        *out = T::default();
    }
    Ok(())
}

fn free_boxed_handle<T>(handle: *mut T) {
    clear_last_error();
    if !handle.is_null() {
        // SAFETY: public `*_free` entry points only accept pointers returned by
        // their matching `*_new`/`*_from_*` constructors. Null is a no-op.
        unsafe { drop(Box::from_raw(handle)) };
    }
}

fn drop_owned_bytes_preserving_last_error(bytes: DatapodOwnedBytes) {
    if bytes.ptr.is_null() {
        return;
    }
    if bytes.len == 0 && bytes.capacity == 0 {
        return;
    }
    if bytes.len > bytes.capacity
        || bytes.len > isize::MAX as usize
        || bytes.capacity > isize::MAX as usize
        || pointer_range_overflows(bytes.ptr.cast_const(), bytes.capacity)
    {
        // This is an internal cleanup path for buffers created by this module.
        // If an impossible shape appears, avoid UB and preserve the caller's
        // more relevant decode/validation error instead of overwriting it.
        return;
    }
    unsafe {
        drop(Vec::from_raw_parts(bytes.ptr, bytes.len, bytes.capacity));
    }
}

fn try_join_wire_bytes(header: &[u8], payload: &[u8]) -> Result<Vec<u8>, ()> {
    let Some(total_len) = header.len().checked_add(payload.len()) else {
        set_last_error("wire header + payload length overflows addressable memory");
        return Err(());
    };
    let mut bytes = Vec::new();
    if let Err(error) = bytes.try_reserve_exact(total_len) {
        set_last_error(format!(
            "failed to reserve {total_len} wire message bytes: {error}"
        ));
        return Err(());
    }
    bytes.extend_from_slice(header);
    bytes.extend_from_slice(payload);
    Ok(bytes)
}

fn read_u32_le_ffi(bytes: &[u8], offset: usize, context: &'static str) -> Result<u32, ()> {
    let end = match offset.checked_add(4) {
        Some(end) => end,
        None => {
            set_last_error(format!("{context} u32 offset overflowed"));
            return Err(());
        }
    };
    let Some(raw) = bytes.get(offset..end) else {
        set_last_error(format!(
            "{context} needs 4 bytes at offset {offset}, payload has {} bytes",
            bytes.len()
        ));
        return Err(());
    };
    let mut array = [0u8; 4];
    array.copy_from_slice(raw);
    Ok(u32::from_le_bytes(array))
}

fn assoc_payload_ranges<'a, P: 'static>(
    payload: &'a [u8],
    count: usize,
    entry_size: usize,
    label: &'static str,
) -> Result<(&'a [u8], &'a [u8]), ()> {
    let entries_len = count.checked_mul(entry_size).ok_or_else(|| {
        set_last_error(
            crate::wire::invalid_payload::<P>(format!("{label} entry table length overflowed"))
                .to_string(),
        );
    })?;
    let blob_offset = 4usize.checked_add(entries_len).ok_or_else(|| {
        set_last_error(
            crate::wire::invalid_payload::<P>(format!("{label} blob offset overflowed"))
                .to_string(),
        );
    })?;
    let Some(entries) = payload.get(4..blob_offset) else {
        set_last_error(
            crate::wire::invalid_payload::<P>(format!(
                "{label} entry table outside payload: entries end {blob_offset}, payload len {}",
                payload.len()
            ))
            .to_string(),
        );
        return Err(());
    };
    let Some(blob) = payload.get(blob_offset..) else {
        set_last_error(
            crate::wire::invalid_payload::<P>(format!(
                "{label} blob outside payload: offset {blob_offset}, payload len {}",
                payload.len()
            ))
            .to_string(),
        );
        return Err(());
    };
    Ok((entries, blob))
}

fn deque_payload_ranges(payload: &[u8], split: usize) -> Result<(&[u8], &[u8]), ()> {
    let Some(front) = payload.get(..split) else {
        set_last_error(
            crate::wire::invalid_header::<Deque>(format!(
                "deque split_byte {split} outside payload len {}",
                payload.len()
            ))
            .to_string(),
        );
        return Err(());
    };
    let Some(back) = payload.get(split..) else {
        set_last_error(
            crate::wire::invalid_header::<Deque>(format!(
                "deque split_byte {split} outside payload len {}",
                payload.len()
            ))
            .to_string(),
        );
        return Err(());
    };
    Ok((front, back))
}

fn ffi_u32_to_usize<T: 'static>(value: u32, field: &'static str) -> Result<usize, ()> {
    usize::try_from(value).map_err(|_| {
        set_last_error(
            crate::wire::invalid_header::<T>(format!("{field} does not fit in usize")).to_string(),
        );
    })
}

fn ffi_usize_to_u32<T: 'static>(value: usize, field: &'static str) -> Result<u32, ()> {
    u32::try_from(value).map_err(|_| {
        set_last_error(
            crate::wire::invalid_payload::<T>(format!("{field} does not fit in u32")).to_string(),
        );
    })
}

fn ffi_element_count<T: 'static>(
    payload: &[u8],
    element_size: u32,
    _label: &'static str,
) -> Result<usize, ()> {
    if element_size == 0 {
        return Ok(0);
    }
    let element_size = ffi_u32_to_usize::<T>(element_size, "element_size")?;
    Ok(payload.len() / element_size)
}

fn ffi_node_layout<T: 'static>(
    payload: &[u8],
    element_size: u32,
    label: &'static str,
) -> Result<(usize, usize), ()> {
    if element_size == 0 {
        return Ok((0, 0));
    }
    let element_size = ffi_u32_to_usize::<T>(element_size, "element_size")?;
    let node_size = element_size.checked_add(8).ok_or_else(|| {
        set_last_error(
            crate::wire::invalid_header::<T>(format!("{label} node size overflowed")).to_string(),
        );
    })?;
    Ok((node_size, payload.len() / node_size))
}

fn ffi_indexed_heap_layout(payload: &[u8], priority_size: u32) -> Result<(usize, usize), ()> {
    if priority_size == 0 {
        return Ok((0, 0));
    }
    let priority_size = ffi_u32_to_usize::<IndexedHeap>(priority_size, "priority_size")?;
    let entry_size = 8usize.checked_add(priority_size).ok_or_else(|| {
        set_last_error(
            crate::wire::invalid_header::<IndexedHeap>("indexed heap entry size overflowed")
                .to_string(),
        );
    })?;
    Ok((entry_size, payload.len() / entry_size))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_owned_bytes_free(bytes: DatapodOwnedBytes) {
    clear_last_error();
    if bytes.ptr.is_null() {
        if bytes.len != 0 || bytes.capacity != 0 {
            set_last_error(format!(
                "null owned bytes pointer with len {} and capacity {}",
                bytes.len, bytes.capacity
            ));
        }
        return;
    }
    if bytes.len == 0 && bytes.capacity == 0 {
        return;
    }
    if bytes.len > bytes.capacity {
        set_last_error(format!(
            "owned bytes length {} exceeds capacity {}",
            bytes.len, bytes.capacity
        ));
        return;
    }
    if bytes.len > isize::MAX as usize || bytes.capacity > isize::MAX as usize {
        set_last_error("owned bytes length/capacity exceeds maximum supported allocation size");
        return;
    }
    if validate_pointer_range(
        bytes.ptr.cast_const(),
        bytes.capacity,
        "owned bytes allocation",
    )
    .is_err()
    {
        return;
    }
    unsafe {
        drop(Vec::from_raw_parts(bytes.ptr, bytes.len, bytes.capacity));
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_borrow(
    type_hash: u64,
    data: *const u8,
    len: usize,
) -> DatapodWireMessage {
    clear_last_error();
    DatapodWireMessage {
        type_hash,
        data,
        len,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_borrow(
    type_hash: u64,
    header: *const u8,
    header_len: usize,
    payload: *const u8,
    payload_len: usize,
) -> DatapodWireFrame {
    clear_last_error();
    DatapodWireFrame {
        type_hash,
        header,
        header_len,
        payload,
        payload_len,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_frame_borrow(
    type_hash: u64,
    header: *const u8,
    header_len: usize,
    payload: *const u8,
    payload_len: usize,
) -> DatapodArchiveFrame {
    clear_last_error();
    datapod_wire_frame_borrow(type_hash, header, header_len, payload, payload_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_from_message_v1(
    message: DatapodWireMessage,
    out: *mut DatapodWireFrame,
) -> bool {
    wire_frame_from_message_v1_impl(message, out, "wire-frame")
}

fn wire_frame_from_message_v1_impl(
    message: DatapodWireMessage,
    out: *mut DatapodWireFrame,
    output_label: &'static str,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, output_label).is_err() {
        return false;
    }
    if prevalidate_wire_message_shape(message).is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(message.data, message.len) }) else {
        return false;
    };
    if let Err(error) = crate::validate_registered_wire_v1(message.type_hash, bytes) {
        set_last_error(error.to_string());
        return false;
    }
    let frame = match crate::split_wire_frame(message.type_hash, bytes) {
        Ok(frame) => frame,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodWireFrame {
            type_hash: frame.type_hash,
            header: frame.header.as_ptr(),
            header_len: frame.header.len(),
            payload: frame.payload.as_ptr(),
            payload_len: frame.payload.len(),
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_from_message(
    message: DatapodWireMessage,
    out: *mut DatapodWireFrame,
) -> bool {
    datapod_wire_frame_from_message_v1(message, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_frame_from_message_v1(
    message: DatapodWireMessage,
    out: *mut DatapodArchiveFrame,
) -> bool {
    wire_frame_from_message_v1_impl(message, out, "archive-frame")
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_frame_from_message(
    message: DatapodWireMessage,
    out: *mut DatapodArchiveFrame,
) -> bool {
    datapod_archive_frame_from_message_v1(message, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_split_v1(
    type_hash: u64,
    bytes: *const u8,
    len: usize,
    out: *mut DatapodArchiveFrame,
) -> bool {
    datapod_archive_frame_from_message_v1(datapod_wire_message_borrow(type_hash, bytes, len), out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_split(
    type_hash: u64,
    bytes: *const u8,
    len: usize,
    out: *mut DatapodArchiveFrame,
) -> bool {
    datapod_archive_split_v1(type_hash, bytes, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_to_message_v1(
    frame: DatapodArchiveFrame,
    out: *mut DatapodOwnedBytes,
) -> bool {
    datapod_wire_message_join_v1(
        frame.type_hash,
        frame.header,
        frame.header_len,
        frame.payload,
        frame.payload_len,
        out,
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_to_message(
    frame: DatapodArchiveFrame,
    out: *mut DatapodOwnedBytes,
) -> bool {
    datapod_archive_to_message_v1(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_copy(
    message: DatapodWireMessage,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if !datapod_wire_message_validate_v1(message) {
        return false;
    }
    let Some(bytes) = clone_bytes(message.data, message.len) else {
        return false;
    };
    unsafe {
        *out = owned_bytes(bytes);
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_join(
    type_hash: u64,
    header: *const u8,
    header_len: usize,
    payload: *const u8,
    payload_len: usize,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if !crate::registry::type_exists(type_hash) {
        set_last_error("unknown datapod type hash");
        return false;
    }
    let expected_header_len = datapod_header_size(type_hash);
    if header_len != expected_header_len {
        set_last_error(format!(
            "wrong header length: got {header_len}, expected {expected_header_len}"
        ));
        return false;
    }
    if datapod_payload_kind(type_hash) == datapod_payload_kind_fixed() && payload_len != 0 {
        set_last_error(format!(
            "fixed-size datapod wire message cannot carry payload bytes: got {payload_len}"
        ));
        return false;
    }
    if prevalidate_joined_wire_len(header_len, payload_len).is_err() {
        return false;
    }
    let Ok(header) = (unsafe { bytes_in(header, header_len) }) else {
        return false;
    };
    let Ok(payload) = (unsafe { bytes_in(payload, payload_len) }) else {
        return false;
    };
    let Ok(bytes) = try_join_wire_bytes(header, payload) else {
        return false;
    };
    if let Err(error) = crate::validate_registered_wire_v1(type_hash, &bytes) {
        set_last_error(error.to_string());
        return false;
    }
    unsafe {
        *out = owned_bytes(bytes);
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_join_v1(
    type_hash: u64,
    header: *const u8,
    header_len: usize,
    payload: *const u8,
    payload_len: usize,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if !crate::registry::type_exists(type_hash) {
        set_last_error("unknown datapod type hash");
        return false;
    }
    let expected_header_len = datapod_header_size_v1(type_hash);
    if header_len != expected_header_len {
        set_last_error(format!(
            "wrong v1 header length: got {header_len}, expected {expected_header_len}"
        ));
        return false;
    }
    if datapod_payload_kind(type_hash) == datapod_payload_kind_fixed() && payload_len != 0 {
        set_last_error(format!(
            "fixed-size datapod wire message cannot carry payload bytes: got {payload_len}"
        ));
        return false;
    }
    if prevalidate_joined_wire_len(header_len, payload_len).is_err() {
        return false;
    }
    let Ok(header) = (unsafe { bytes_in(header, header_len) }) else {
        return false;
    };
    let Ok(payload) = (unsafe { bytes_in(payload, payload_len) }) else {
        return false;
    };
    let Ok(bytes) = try_join_wire_bytes(header, payload) else {
        return false;
    };
    if let Err(error) = crate::validate_registered_wire_v1(type_hash, &bytes) {
        set_last_error(error.to_string());
        return false;
    }
    unsafe {
        *out = owned_bytes(bytes);
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_is_valid(message: DatapodWireMessage) -> bool {
    datapod_wire_message_validate_v1(message)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_validate(message: DatapodWireMessage) -> bool {
    datapod_wire_message_validate_v1(message)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_validate_v1(message: DatapodWireMessage) -> bool {
    clear_last_error();
    if prevalidate_wire_message_shape(message).is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(message.data, message.len) }) else {
        return false;
    };
    match crate::validate_registered_wire_v1(message.type_hash, bytes) {
        Ok(()) => true,
        Err(error) => {
            set_last_error(error.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_is_valid_v1(message: DatapodWireMessage) -> bool {
    datapod_wire_message_validate_v1(message)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_validate_v1(frame: DatapodWireFrame) -> bool {
    clear_last_error();
    if prevalidate_wire_frame_shape(frame).is_err() {
        return false;
    }
    let Ok(header) = (unsafe { bytes_in(frame.header, frame.header_len) }) else {
        return false;
    };
    let Ok(payload) = (unsafe { bytes_in(frame.payload, frame.payload_len) }) else {
        return false;
    };
    match crate::validate_registered_wire_frame_v1(crate::WireFrame {
        type_hash: frame.type_hash,
        header,
        payload,
    }) {
        Ok(()) => true,
        Err(error) => {
            set_last_error(error.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_validate_as_v1(
    expected_hash: u64,
    frame: DatapodWireFrame,
) -> bool {
    clear_last_error();
    if frame.type_hash != expected_hash {
        set_last_error(format!(
            "wrong datapod frame type hash: got {}, expected {}",
            frame.type_hash, expected_hash
        ));
        return false;
    }
    datapod_wire_frame_validate_v1(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_validate_as(
    expected_hash: u64,
    frame: DatapodWireFrame,
) -> bool {
    datapod_wire_frame_validate_as_v1(expected_hash, frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_validate(frame: DatapodWireFrame) -> bool {
    datapod_wire_frame_validate_v1(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_validate_v1(frame: DatapodArchiveFrame) -> bool {
    datapod_wire_frame_validate_v1(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_validate(frame: DatapodArchiveFrame) -> bool {
    datapod_archive_validate_v1(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_validate_as_v1(
    expected_hash: u64,
    frame: DatapodArchiveFrame,
) -> bool {
    datapod_wire_frame_validate_as_v1(expected_hash, frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_validate_as(
    expected_hash: u64,
    frame: DatapodArchiveFrame,
) -> bool {
    datapod_archive_validate_as_v1(expected_hash, frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_is_valid_v1(frame: DatapodWireFrame) -> bool {
    datapod_wire_frame_validate_v1(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_frame_is_valid(frame: DatapodWireFrame) -> bool {
    datapod_wire_frame_validate_v1(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_is_valid_v1(frame: DatapodArchiveFrame) -> bool {
    datapod_archive_validate_v1(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_is_valid(frame: DatapodArchiveFrame) -> bool {
    datapod_archive_validate(frame)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_header(message: DatapodWireMessage) -> DatapodBytes {
    clear_last_error();
    if !datapod_wire_message_is_valid(message) {
        return DatapodBytes::empty();
    }
    DatapodBytes {
        ptr: message.data,
        len: datapod_header_size(message.type_hash),
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_payload(message: DatapodWireMessage) -> DatapodBytes {
    clear_last_error();
    if !datapod_wire_message_is_valid(message) {
        return DatapodBytes::empty();
    }
    let header_len = datapod_header_size(message.type_hash);
    let payload_ptr = if header_len == 0 {
        message.data
    } else {
        message.data.wrapping_add(header_len)
    };
    DatapodBytes {
        ptr: payload_ptr,
        len: message.len - header_len,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_header_v1(message: DatapodWireMessage) -> DatapodBytes {
    clear_last_error();
    if !datapod_wire_message_validate_v1(message) {
        return DatapodBytes::empty();
    }
    DatapodBytes {
        ptr: message.data,
        len: datapod_header_size_v1(message.type_hash),
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_message_payload_v1(message: DatapodWireMessage) -> DatapodBytes {
    clear_last_error();
    if !datapod_wire_message_validate_v1(message) {
        return DatapodBytes::empty();
    }
    let header_len = datapod_header_size_v1(message.type_hash);
    let payload_ptr = if header_len == 0 {
        message.data
    } else {
        message.data.wrapping_add(header_len)
    };
    DatapodBytes {
        ptr: payload_ptr,
        len: message.len - header_len,
    }
}

/// Copy any fixed-size datapod C value into canonical wire bytes.
///
/// `value` must point at the concrete C struct bytes for a registered fixed
/// datapod whose type hash is `type_hash`. The returned owned bytes are the
/// complete datapod wire body for fixed values: just the header bytes.
#[unsafe(no_mangle)]
pub extern "C" fn datapod_fixed_value_to_wire(
    type_hash: u64,
    value: *const u8,
    value_len: usize,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if fixed_type_header_size(type_hash).is_err() {
        return false;
    }
    datapod_wire_message_join(type_hash, value, value_len, ptr::null(), 0, out)
}

/// Borrow a fixed-size datapod C value as an archive.
///
/// The returned frame borrows `value` as its header and has no payload. The
/// caller must keep `value` alive while using the returned archive.
#[unsafe(no_mangle)]
pub extern "C" fn datapod_fixed_value_archive(
    type_hash: u64,
    value: *const u8,
    value_len: usize,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if fixed_type_header_size(type_hash).is_err() {
        return false;
    }
    let archive = datapod_archive_frame_borrow(type_hash, value, value_len, ptr::null(), 0);
    if !datapod_archive_validate_as(type_hash, archive) {
        return false;
    }
    unsafe {
        *out = archive;
    }
    true
}

/// Decode canonical fixed-size datapod wire bytes into a caller-owned C value.
///
/// `out` must point at writable storage for the concrete C struct identified by
/// `type_hash`; `out_len` must be at least that type's header size.
#[unsafe(no_mangle)]
pub extern "C" fn datapod_fixed_value_from_wire(
    type_hash: u64,
    data: *const u8,
    data_len: usize,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    let Ok(output) = (unsafe { bytes_out(out, out_len) }) else {
        return false;
    };
    output.fill(0);
    let Ok(header_len) = fixed_type_header_size(type_hash) else {
        return false;
    };
    let message = datapod_wire_message_borrow(type_hash, data, data_len);
    if !datapod_wire_message_is_valid(message) {
        return false;
    }
    if data_len != header_len {
        set_last_error(format!(
            "fixed-size wire message has payload bytes: got {data_len}, expected {header_len}"
        ));
        return false;
    }
    let Ok(input) = (unsafe { bytes_in(data, header_len) }) else {
        return false;
    };
    if output.len() < header_len {
        set_last_error(format!(
            "output buffer too small: need {header_len}, got {}",
            output.len()
        ));
        return false;
    }
    let Some(slot) = output.get_mut(..header_len) else {
        set_last_error(format!(
            "output range unavailable: need {header_len}, got {}",
            output.len()
        ));
        return false;
    };
    slot.copy_from_slice(input);
    true
}

/// Decode a fixed-size datapod archive into caller-owned C storage.
#[unsafe(no_mangle)]
pub extern "C" fn datapod_fixed_value_from_archive(
    type_hash: u64,
    frame: DatapodArchiveFrame,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    let Ok(output) = (unsafe { bytes_out(out, out_len) }) else {
        return false;
    };
    output.fill(0);
    let Ok(header_len) = fixed_type_header_size(type_hash) else {
        return false;
    };
    if frame.payload_len != 0 {
        set_last_error("fixed-size archive has payload bytes");
        return false;
    }
    if !datapod_archive_validate_as(type_hash, frame) {
        return false;
    }
    let Ok(input) = (unsafe { bytes_in(frame.header, frame.header_len) }) else {
        return false;
    };
    if output.len() < header_len {
        set_last_error(format!(
            "output buffer too small: need {header_len}, got {}",
            output.len()
        ));
        return false;
    }
    let Some(slot) = output.get_mut(..header_len) else {
        set_last_error(format!(
            "output range unavailable: need {header_len}, got {}",
            output.len()
        ));
        return false;
    };
    slot.copy_from_slice(input);
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_type_exists(type_hash: u64) -> bool {
    clear_last_error();
    crate::registry::type_exists(type_hash)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_type_hash_name(name: *const c_char) -> u64 {
    clear_last_error();
    let Ok(name) = canonical_type_name_in(name) else {
        return 0;
    };
    crate::registry::type_hash_name(name)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_type_hash_name_bytes(name: *const u8, name_len: usize) -> u64 {
    clear_last_error();
    let Ok(name) = canonical_type_name_bytes_in(name, name_len) else {
        return 0;
    };
    crate::registry::type_hash_name(name)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_canonical_type_hash(type_hash: u64) -> u64 {
    metadata_type_info(type_hash).map_or(0, |info| info.canonical_type_hash)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_emitted_type_hash(type_hash: u64) -> u64 {
    metadata_type_info(type_hash).map_or(0, |info| info.emitted_hash)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_hash_kind_canonical_name() -> u32 {
    clear_last_error();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_current_wire_format_name() -> *const c_char {
    clear_last_error();
    c"datapod-wire-v1/le".as_ptr()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_builtin_hash_policy() -> *const c_char {
    clear_last_error();
    c"built-in emission uses canonical-name hashes in datapod-wire-v1/le".as_ptr()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_payload_kind_fixed() -> u32 {
    clear_last_error();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_payload_kind_bytes() -> u32 {
    clear_last_error();
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_endian_little() -> u32 {
    clear_last_error();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_alignment_unaligned_wire() -> u32 {
    clear_last_error();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_validator_registry_only() -> u32 {
    clear_last_error();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_validator_builtin() -> u32 {
    clear_last_error();
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_validator_runtime_schema() -> u32 {
    clear_last_error();
    2
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_shape_fixed() -> u32 {
    clear_last_error();
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_shape_single_payload() -> u32 {
    clear_last_error();
    1
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_shape_segmented_payload() -> u32 {
    clear_last_error();
    2
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_shape_runtime_schema() -> u32 {
    clear_last_error();
    3
}

#[unsafe(no_mangle)]
/// Register runtime metadata for a C/Python schema.
///
/// `type_hash` must be the canonical-name hash returned by
/// `datapod_type_hash_name(canonical_name)`.
pub extern "C" fn datapod_register_type(
    type_hash: u64,
    canonical_name: *const c_char,
    header_size: usize,
    payload_kind: u32,
) -> bool {
    clear_last_error();
    let Ok(canonical_name) = canonical_type_name_in(canonical_name) else {
        return false;
    };
    register_type_metadata(type_hash, canonical_name, header_size, payload_kind)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_register_type_bytes(
    type_hash: u64,
    canonical_name: *const u8,
    canonical_name_len: usize,
    header_size: usize,
    payload_kind: u32,
) -> bool {
    clear_last_error();
    let Ok(canonical_name) = canonical_type_name_bytes_in(canonical_name, canonical_name_len)
    else {
        return false;
    };
    register_type_metadata(type_hash, canonical_name, header_size, payload_kind)
}

fn register_type_metadata(
    type_hash: u64,
    canonical_name: &str,
    header_size: usize,
    payload_kind: u32,
) -> bool {
    let payload_kind = match payload_kind {
        0 => crate::registry::PayloadKind::Fixed,
        1 => crate::registry::PayloadKind::Bytes,
        _ => {
            set_last_error("invalid datapod payload kind; expected 0=fixed or 1=bytes");
            return false;
        }
    };
    match crate::registry::register_type(type_hash, canonical_name, header_size, payload_kind) {
        Ok(()) => true,
        Err(error) => {
            set_last_error(error.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_register_type_name(
    canonical_name: *const c_char,
    header_size: usize,
    payload_kind: u32,
) -> u64 {
    clear_last_error();
    let type_hash = datapod_type_hash_name(canonical_name);
    if type_hash == 0 {
        return 0;
    }
    if datapod_register_type(type_hash, canonical_name, header_size, payload_kind) {
        type_hash
    } else {
        0
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_register_type_name_bytes(
    canonical_name: *const u8,
    canonical_name_len: usize,
    header_size: usize,
    payload_kind: u32,
) -> u64 {
    clear_last_error();
    let type_hash = datapod_type_hash_name_bytes(canonical_name, canonical_name_len);
    if type_hash == 0 {
        return 0;
    }
    if datapod_register_type_bytes(
        type_hash,
        canonical_name,
        canonical_name_len,
        header_size,
        payload_kind,
    ) {
        type_hash
    } else {
        0
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_type_name(type_hash: u64) -> *const c_char {
    let Some(info) = metadata_type_info(type_hash) else {
        return ptr::null();
    };
    TYPE_NAME_RESULT.with(|slot| {
        *slot.borrow_mut() = Some(c_string_without_nul(info.canonical_name));
        slot.borrow()
            .as_ref()
            .map_or(ptr::null(), |name| name.as_ptr())
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_type_exists_name(canonical_name: *const c_char) -> bool {
    clear_last_error();
    let Ok(canonical_name) = canonical_type_name_in(canonical_name) else {
        return false;
    };
    crate::registry::find_type_info_by_name(canonical_name).is_some()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_type_exists_name_bytes(
    canonical_name: *const u8,
    canonical_name_len: usize,
) -> bool {
    clear_last_error();
    let Ok(canonical_name) = canonical_type_name_bytes_in(canonical_name, canonical_name_len)
    else {
        return false;
    };
    crate::registry::find_type_info_by_name(canonical_name).is_some()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_header_size(type_hash: u64) -> usize {
    metadata_type_info(type_hash).map_or(0, |info| info.header_size)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_header_size_v1(type_hash: u64) -> usize {
    metadata_type_info(type_hash).map_or(0, |info| info.header_size)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_payload_kind(type_hash: u64) -> u32 {
    match metadata_type_info(type_hash).map(|info| info.payload_kind) {
        Some(crate::registry::PayloadKind::Fixed) => datapod_payload_kind_fixed(),
        Some(crate::registry::PayloadKind::Bytes) => datapod_payload_kind_bytes(),
        None => u32::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_format_version(type_hash: u64) -> u32 {
    metadata_type_info(type_hash).map_or(u32::MAX, |info| info.format_version)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wire_format(type_hash: u64) -> u32 {
    match metadata_type_info(type_hash).map(|info| info.wire_format) {
        Some(crate::registry::WireFormat::DatapodWireV1Little) => 1,
        None => u32::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_emitted_hash_kind(type_hash: u64) -> u32 {
    match metadata_type_info(type_hash).map(|info| info.emitted_hash_kind) {
        Some(crate::registry::HashKind::CanonicalName) => datapod_hash_kind_canonical_name(),
        None => u32::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_endian(type_hash: u64) -> u32 {
    match metadata_type_info(type_hash).map(|info| info.endian) {
        Some(crate::registry::Endian::Little) => datapod_endian_little(),
        None => u32::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_alignment_policy(type_hash: u64) -> u32 {
    match metadata_type_info(type_hash).map(|info| info.alignment) {
        Some(crate::registry::AlignmentPolicy::UnalignedWire) => datapod_alignment_unaligned_wire(),
        Some(crate::registry::AlignmentPolicy::AlignedPayload { .. }) => 1,
        None => u32::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_validator_kind(type_hash: u64) -> u32 {
    match metadata_type_info(type_hash).map(|info| info.validator) {
        Some(crate::registry::ValidatorKind::RegistryOnly) => datapod_validator_registry_only(),
        Some(crate::registry::ValidatorKind::BuiltIn) => datapod_validator_builtin(),
        Some(crate::registry::ValidatorKind::RuntimeSchema) => datapod_validator_runtime_schema(),
        None => u32::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_has_archive(type_hash: u64) -> bool {
    metadata_type_info(type_hash).is_some_and(|info| info.has_archive)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_has_view(type_hash: u64) -> bool {
    metadata_type_info(type_hash).is_some_and(|info| info.has_view)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_has_owned_decode(type_hash: u64) -> bool {
    metadata_type_info(type_hash).is_some_and(|info| info.has_owned_decode)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_archive_shape(type_hash: u64) -> u32 {
    match metadata_type_info(type_hash).map(|info| info.archive_shape) {
        Some(crate::registry::ArchiveShape::Fixed) => datapod_archive_shape_fixed(),
        Some(crate::registry::ArchiveShape::SinglePayload) => {
            datapod_archive_shape_single_payload()
        }
        Some(crate::registry::ArchiveShape::SegmentedPayload) => {
            datapod_archive_shape_segmented_payload()
        }
        Some(crate::registry::ArchiveShape::RuntimeSchema) => {
            datapod_archive_shape_runtime_schema()
        }
        None => u32::MAX,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_to_wire(value: DatapodPoint, out: *mut DatapodOwnedBytes) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    let message = crate::to_wire_message(&Point::from(value));
    unsafe {
        *out = owned_bytes(message.bytes);
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodPoint,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "output point").is_err() {
        return false;
    }
    let Some(bytes) = clone_bytes(ptr, len) else {
        return false;
    };
    let message = crate::WireMessage {
        type_hash: crate::bind::emitted_type_hash::<Point>(),
        bytes,
    };
    let value = match crate::from_wire_message::<Point>(&message) {
        Ok(value) => value,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = value.into();
    }
    true
}

fn heap_to_wire<T>(value: &T, out: *mut DatapodOwnedBytes) -> bool
where
    T: DataPod + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if let Err(error) = T::validate_wire_parts(&value.header(), value.payload_bytes()) {
        set_last_error(error.to_string());
        return false;
    }
    let message = crate::to_wire_message(value);
    unsafe {
        *out = owned_bytes(message.bytes);
    }
    true
}

fn heap_from_wire<T>(ptr: *const u8, len: usize) -> Option<T>
where
    T: DataPod + crate::DataPodDecode + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    let bytes = clone_bytes(ptr, len)?;
    let message = crate::WireMessage {
        type_hash: crate::bind::emitted_type_hash::<T>(),
        bytes,
    };
    match crate::from_wire_message::<T>(&message) {
        Ok(value) => Some(value),
        Err(error) => {
            set_last_error(error.to_string());
            None
        }
    }
}

fn validate_heap_inner<T>(inner: T) -> Option<T>
where
    T: DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    let header = inner.header();
    match T::validate_wire_parts(&header, inner.payload_bytes()) {
        Ok(()) => Some(inner),
        Err(error) => {
            set_last_error(error.to_string());
            None
        }
    }
}

fn heap_archive<T>(
    value: &T,
    archive_header: &RefCell<Vec<u8>>,
    out: *mut DatapodArchiveFrame,
) -> bool
where
    T: DataPod + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    let payload = value.payload_bytes();
    if payload.len() != value.payload_len() {
        set_last_error(format!(
            "datapod exposes {} contiguous payload bytes but reports {} payload bytes; \
             use a segmented archive API for multi-payload datapods",
            payload.len(),
            value.payload_len()
        ));
        return false;
    }
    if let Err(error) = T::validate_wire_parts(&value.header(), payload) {
        set_last_error(error.to_string());
        return false;
    }
    let mut header = archive_header.borrow_mut();
    header.clear();
    crate::LeWireHeader::write_le(&value.header(), &mut header);
    unsafe {
        *out = DatapodArchiveFrame {
            type_hash: crate::bind::emitted_type_hash::<T>(),
            header: header.as_ptr(),
            header_len: header.len(),
            payload: payload.as_ptr(),
            payload_len: payload.len(),
        };
    }
    true
}

fn heap_handle_from_archive<T, H>(
    frame: DatapodArchiveFrame,
    from_wire: extern "C" fn(*const u8, usize) -> *mut H,
) -> *mut H
where
    T: DataPod,
    T::Header: crate::LeWireHeader,
{
    let expected_hash = crate::bind::emitted_type_hash::<T>();
    if !datapod_archive_validate_as_v1(expected_hash, frame) {
        return ptr::null_mut();
    }
    let mut owned = DatapodOwnedBytes::empty();
    if !datapod_archive_to_message_v1(frame, &mut owned) {
        return ptr::null_mut();
    }
    let handle = from_wire(owned.ptr, owned.len);
    drop_owned_bytes_preserving_last_error(owned);
    handle
}

/// FFI-safe point value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodPoint {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl From<Point> for DatapodPoint {
    fn from(value: Point) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
        }
    }
}

impl From<DatapodPoint> for Point {
    fn from(value: DatapodPoint) -> Self {
        Self::new(value.x, value.y, value.z)
    }
}

/// FFI-safe geodetic value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodGeo {
    pub latitude: f64,
    pub longitude: f64,
    pub altitude: f64,
}

impl From<Geo> for DatapodGeo {
    fn from(value: Geo) -> Self {
        Self {
            latitude: value.latitude,
            longitude: value.longitude,
            altitude: value.altitude,
        }
    }
}

impl From<DatapodGeo> for Geo {
    fn from(value: DatapodGeo) -> Self {
        Self::new(value.latitude, value.longitude, value.altitude)
    }
}

/// FFI-safe segment value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodSegment {
    pub start: DatapodPoint,
    pub end: DatapodPoint,
}

impl From<Segment> for DatapodSegment {
    fn from(value: Segment) -> Self {
        Self {
            start: value.start.into(),
            end: value.end.into(),
        }
    }
}

impl From<DatapodSegment> for Segment {
    fn from(value: DatapodSegment) -> Self {
        Self::new(value.start.into(), value.end.into())
    }
}

/// FFI-safe Euler angle value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodEuler {
    pub roll: f64,
    pub pitch: f64,
    pub yaw: f64,
}

impl From<Euler> for DatapodEuler {
    fn from(value: Euler) -> Self {
        Self {
            roll: value.roll,
            pitch: value.pitch,
            yaw: value.yaw,
        }
    }
}

impl From<DatapodEuler> for Euler {
    fn from(value: DatapodEuler) -> Self {
        Self::new(value.roll, value.pitch, value.yaw)
    }
}

/// FFI-safe quaternion value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodQuaternion {
    pub w: f64,
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl From<Quaternion> for DatapodQuaternion {
    fn from(value: Quaternion) -> Self {
        Self {
            w: value.w,
            x: value.x,
            y: value.y,
            z: value.z,
        }
    }
}

impl From<DatapodQuaternion> for Quaternion {
    fn from(value: DatapodQuaternion) -> Self {
        Self::new(value.w, value.x, value.y, value.z)
    }
}

/// FFI-safe pose value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodPose {
    pub point: DatapodPoint,
    pub rotation: DatapodQuaternion,
}

impl From<Pose> for DatapodPose {
    fn from(value: Pose) -> Self {
        Self {
            point: value.point.into(),
            rotation: value.rotation.into(),
        }
    }
}

impl From<DatapodPose> for Pose {
    fn from(value: DatapodPose) -> Self {
        Self {
            point: value.point.into(),
            rotation: value.rotation.into(),
        }
    }
}

/// Borrowed C view over a validated Bytes wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodBytesView {
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated UTF-8 DpStr wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodDpStrView {
    pub utf8: DatapodBytes,
}

/// Borrowed C view over a validated DpString wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodDpStringView {
    pub bytes: DatapodBytes,
}

/// Borrowed C view over a validated point-sequence wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodLinestringView {
    pub point_count: usize,
    pub point_size: usize,
    pub points: DatapodBytes,
}

/// Borrowed C view over a validated multi-point wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodMultiPointView {
    pub point_count: usize,
    pub point_size: usize,
    pub points: DatapodBytes,
}

/// Borrowed C view over a validated ring wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodRingView {
    pub point_count: usize,
    pub point_size: usize,
    pub points: DatapodBytes,
}

/// Borrowed C view over a validated polygon wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodPolygonView {
    pub vertex_count: usize,
    pub point_size: usize,
    pub vertices: DatapodBytes,
}

/// Borrowed C view over a validated path wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodPathView {
    pub waypoint_count: usize,
    pub pose_size: usize,
    pub waypoints: DatapodBytes,
}

/// Borrowed C view over a validated trajectory wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodTrajectoryView {
    pub state_count: usize,
    pub state_size: usize,
    pub states: DatapodBytes,
}

/// Borrowed C view over a validated Matrix wire message.
///
/// `payload` points into the caller-owned wire bytes passed to
/// `datapod_matrix_view_from_wire`; keep those bytes alive while using the
/// view.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodMatrixView {
    pub rows: u32,
    pub cols: u32,
    pub element_size: u32,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Tensor wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodTensorView {
    pub rows: u32,
    pub cols: u32,
    pub layers: u32,
    pub element_size: u32,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Vector wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodVectorView {
    pub element_size: u32,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Stack wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodStackView {
    pub element_size: u32,
    pub element_count: usize,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Queue wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodQueueView {
    pub element_size: u32,
    pub front: u32,
    pub raw_count: usize,
    pub logical_count: usize,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Deque wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodDequeView {
    pub element_size: u32,
    pub split_byte: u32,
    pub element_count: usize,
    pub front: DatapodBytes,
    pub back: DatapodBytes,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Heap wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodHeapView {
    pub element_size: u32,
    pub order: u8,
    pub element_count: usize,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated IndexedHeap wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodIndexedHeapView {
    pub priority_size: u32,
    pub order: u8,
    pub entry_size: usize,
    pub entry_count: usize,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated bit-packed BitVec wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodBitVecView {
    pub bits: u64,
    pub data: DatapodBytes,
}

/// Borrowed C view over a validated ragged Vecvec wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodVecvecView {
    pub element_size: u32,
    pub bucket_count: u32,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated PagedVecvec wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodPagedVecvecView {
    pub element_size: u32,
    pub bucket_count: u32,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated List wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodListView {
    pub head: u32,
    pub tail: u32,
    pub free_head: u32,
    pub size: u32,
    pub element_size: u32,
    pub node_size: usize,
    pub slot_count: usize,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated ForwardList wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodForwardListView {
    pub head: u32,
    pub free_head: u32,
    pub size: u32,
    pub element_size: u32,
    pub node_size: usize,
    pub slot_count: usize,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Map wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodMapView {
    pub count: u32,
    pub entries: DatapodBytes,
    pub blob: DatapodBytes,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Set wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodSetView {
    pub count: u32,
    pub entries: DatapodBytes,
    pub blob: DatapodBytes,
    pub payload: DatapodBytes,
}

/// Borrowed C view over a validated Grid wire message.
///
/// `data` points into the caller-owned wire bytes passed to
/// `datapod_grid_view_from_wire`; keep those bytes alive while using the view.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodGridView {
    pub rows: u32,
    pub cols: u32,
    pub encoding: u32,
    pub centered: bool,
    pub resolution: f64,
    pub pose: DatapodPose,
    pub data: DatapodBytes,
}

/// Borrowed C view over a validated Layer wire message.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodLayerView {
    pub rows: u32,
    pub cols: u32,
    pub layers: u32,
    pub encoding: u32,
    pub centered: bool,
    pub resolution: f64,
    pub layer_height: f64,
    pub pose: DatapodPose,
    pub data: DatapodBytes,
}

/// FFI-safe velocity value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodVelocity {
    pub vx: f64,
    pub vy: f64,
    pub vz: f64,
}

impl From<Velocity> for DatapodVelocity {
    fn from(value: Velocity) -> Self {
        Self {
            vx: value.vx,
            vy: value.vy,
            vz: value.vz,
        }
    }
}

impl From<DatapodVelocity> for Velocity {
    fn from(value: DatapodVelocity) -> Self {
        Self {
            vx: value.vx,
            vy: value.vy,
            vz: value.vz,
        }
    }
}

/// FFI-safe acceleration value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodAcceleration {
    pub ax: f64,
    pub ay: f64,
    pub az: f64,
}

impl From<Acceleration> for DatapodAcceleration {
    fn from(value: Acceleration) -> Self {
        Self {
            ax: value.ax,
            ay: value.ay,
            az: value.az,
        }
    }
}

impl From<DatapodAcceleration> for Acceleration {
    fn from(value: DatapodAcceleration) -> Self {
        Self {
            ax: value.ax,
            ay: value.ay,
            az: value.az,
        }
    }
}

/// FFI-safe transform value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodTransform {
    pub rw: f64,
    pub rx: f64,
    pub ry: f64,
    pub rz: f64,
    pub dw: f64,
    pub dx: f64,
    pub dy: f64,
    pub dz: f64,
}

impl From<Transform> for DatapodTransform {
    fn from(value: Transform) -> Self {
        Self {
            rw: value.rw,
            rx: value.rx,
            ry: value.ry,
            rz: value.rz,
            dw: value.dw,
            dx: value.dx,
            dy: value.dy,
            dz: value.dz,
        }
    }
}

impl From<DatapodTransform> for Transform {
    fn from(value: DatapodTransform) -> Self {
        Self {
            rw: value.rw,
            rx: value.rx,
            ry: value.ry,
            rz: value.rz,
            dw: value.dw,
            dx: value.dx,
            dy: value.dy,
            dz: value.dz,
        }
    }
}

/// FFI-safe state value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodState {
    pub pose: DatapodPose,
    pub linear_velocity: DatapodVelocity,
    pub angular_velocity: DatapodVelocity,
}

impl From<State> for DatapodState {
    fn from(value: State) -> Self {
        Self {
            pose: value.pose.into(),
            linear_velocity: value.linear_velocity.into(),
            angular_velocity: value.angular_velocity.into(),
        }
    }
}

impl From<DatapodState> for State {
    fn from(value: DatapodState) -> Self {
        Self {
            pose: value.pose.into(),
            linear_velocity: value.linear_velocity.into(),
            angular_velocity: value.angular_velocity.into(),
        }
    }
}

/// FFI-safe local/world value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodLoc {
    pub local: DatapodPoint,
    pub origin: DatapodGeo,
}

impl From<Loc> for DatapodLoc {
    fn from(value: Loc) -> Self {
        Self {
            local: value.local.into(),
            origin: value.origin.into(),
        }
    }
}

impl From<DatapodLoc> for Loc {
    fn from(value: DatapodLoc) -> Self {
        Self {
            local: value.local.into(),
            origin: value.origin.into(),
        }
    }
}

/// FFI-safe UTM value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodUtm {
    pub zone: i32,
    pub band: u32,
    pub easting: f64,
    pub northing: f64,
    pub altitude: f64,
}

impl From<Utm> for DatapodUtm {
    fn from(value: Utm) -> Self {
        Self {
            zone: value.zone,
            band: value.band,
            easting: value.easting,
            northing: value.northing,
            altitude: value.altitude,
        }
    }
}

impl From<DatapodUtm> for Utm {
    fn from(value: DatapodUtm) -> Self {
        Self {
            zone: value.zone,
            band: value.band,
            easting: value.easting,
            northing: value.northing,
            altitude: value.altitude,
        }
    }
}

/// FFI-safe line value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodLine {
    pub origin: DatapodPoint,
    pub direction: DatapodPoint,
}

impl From<Line> for DatapodLine {
    fn from(value: Line) -> Self {
        Self {
            origin: value.origin.into(),
            direction: value.direction.into(),
        }
    }
}

impl From<DatapodLine> for Line {
    fn from(value: DatapodLine) -> Self {
        Self {
            origin: value.origin.into(),
            direction: value.direction.into(),
        }
    }
}

/// FFI-safe rectangle value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodRectangle {
    pub top_left: DatapodPoint,
    pub top_right: DatapodPoint,
    pub bottom_left: DatapodPoint,
    pub bottom_right: DatapodPoint,
}

impl From<Rectangle> for DatapodRectangle {
    fn from(value: Rectangle) -> Self {
        Self {
            top_left: value.top_left.into(),
            top_right: value.top_right.into(),
            bottom_left: value.bottom_left.into(),
            bottom_right: value.bottom_right.into(),
        }
    }
}

impl From<DatapodRectangle> for Rectangle {
    fn from(value: DatapodRectangle) -> Self {
        Self {
            top_left: value.top_left.into(),
            top_right: value.top_right.into(),
            bottom_left: value.bottom_left.into(),
            bottom_right: value.bottom_right.into(),
        }
    }
}

/// FFI-safe AABB value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodAabb {
    pub min_point: DatapodPoint,
    pub max_point: DatapodPoint,
}

impl From<Aabb> for DatapodAabb {
    fn from(value: Aabb) -> Self {
        Self {
            min_point: value.min_point.into(),
            max_point: value.max_point.into(),
        }
    }
}

impl From<DatapodAabb> for Aabb {
    fn from(value: DatapodAabb) -> Self {
        Self::new(value.min_point.into(), value.max_point.into())
    }
}

/// FFI-safe bounding sphere value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodBoundingSphere {
    pub center: DatapodPoint,
    pub radius: f64,
}

impl From<BoundingSphere> for DatapodBoundingSphere {
    fn from(value: BoundingSphere) -> Self {
        Self {
            center: value.center.into(),
            radius: value.radius,
        }
    }
}

impl From<DatapodBoundingSphere> for BoundingSphere {
    fn from(value: DatapodBoundingSphere) -> Self {
        Self {
            center: value.center.into(),
            radius: value.radius,
        }
    }
}

/// FFI-safe circle value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodCircle {
    pub center: DatapodPoint,
    pub radius: f64,
}

impl From<Circle> for DatapodCircle {
    fn from(value: Circle) -> Self {
        Self {
            center: value.center.into(),
            radius: value.radius,
        }
    }
}

impl From<DatapodCircle> for Circle {
    fn from(value: DatapodCircle) -> Self {
        Self {
            center: value.center.into(),
            radius: value.radius,
        }
    }
}

/// FFI-safe triangle value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodTriangle {
    pub a: DatapodPoint,
    pub b: DatapodPoint,
    pub c: DatapodPoint,
}

impl From<Triangle> for DatapodTriangle {
    fn from(value: Triangle) -> Self {
        Self {
            a: value.a.into(),
            b: value.b.into(),
            c: value.c.into(),
        }
    }
}

impl From<DatapodTriangle> for Triangle {
    fn from(value: DatapodTriangle) -> Self {
        Self {
            a: value.a.into(),
            b: value.b.into(),
            c: value.c.into(),
        }
    }
}

/// FFI-safe size value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodSize {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl From<Size> for DatapodSize {
    fn from(value: Size) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
        }
    }
}

impl From<DatapodSize> for Size {
    fn from(value: DatapodSize) -> Self {
        Size::new(value.x, value.y, value.z)
    }
}

/// FFI-safe oriented box value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodBox3 {
    pub pose: DatapodPose,
    pub size: DatapodSize,
}

impl From<crate::Box> for DatapodBox3 {
    fn from(value: crate::Box) -> Self {
        Self {
            pose: value.pose.into(),
            size: value.size.into(),
        }
    }
}

impl From<DatapodBox3> for crate::Box {
    fn from(value: DatapodBox3) -> Self {
        Self {
            pose: value.pose.into(),
            size: value.size.into(),
        }
    }
}

/// FFI-safe twist value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodTwist {
    pub linear: DatapodVelocity,
    pub angular: DatapodVelocity,
}

impl From<Twist> for DatapodTwist {
    fn from(value: Twist) -> Self {
        Self {
            linear: value.linear.into(),
            angular: value.angular.into(),
        }
    }
}

impl From<DatapodTwist> for Twist {
    fn from(value: DatapodTwist) -> Self {
        Self {
            linear: value.linear.into(),
            angular: value.angular.into(),
        }
    }
}

/// FFI-safe wrench value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodWrench {
    pub force: DatapodPoint,
    pub torque: DatapodPoint,
}

impl From<Wrench> for DatapodWrench {
    fn from(value: Wrench) -> Self {
        Self {
            force: value.force.into(),
            torque: value.torque.into(),
        }
    }
}

impl From<DatapodWrench> for Wrench {
    fn from(value: DatapodWrench) -> Self {
        Self {
            force: value.force.into(),
            torque: value.torque.into(),
        }
    }
}

/// FFI-safe odometry value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodOdom {
    pub pose: DatapodPose,
    pub twist: DatapodTwist,
}

impl From<Odom> for DatapodOdom {
    fn from(value: Odom) -> Self {
        Self {
            pose: value.pose.into(),
            twist: value.twist.into(),
        }
    }
}

impl From<DatapodOdom> for Odom {
    fn from(value: DatapodOdom) -> Self {
        Self {
            pose: value.pose.into(),
            twist: value.twist.into(),
        }
    }
}

/// FFI-safe joint limits value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodJointLimits {
    pub lower: f64,
    pub upper: f64,
    pub effort: f64,
    pub velocity: f64,
}

impl From<JointLimits> for DatapodJointLimits {
    fn from(value: JointLimits) -> Self {
        Self {
            lower: value.lower,
            upper: value.upper,
            effort: value.effort,
            velocity: value.velocity,
        }
    }
}

impl From<DatapodJointLimits> for JointLimits {
    fn from(value: DatapodJointLimits) -> Self {
        Self {
            lower: value.lower,
            upper: value.upper,
            effort: value.effort,
            velocity: value.velocity,
        }
    }
}

/// FFI-safe inertial value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct DatapodInertial {
    pub origin: DatapodPose,
    pub mass: f64,
    pub ixx: f64,
    pub ixy: f64,
    pub ixz: f64,
    pub iyy: f64,
    pub iyz: f64,
    pub izz: f64,
}

impl From<Inertial> for DatapodInertial {
    fn from(value: Inertial) -> Self {
        Self {
            origin: value.origin.into(),
            mass: value.mass,
            ixx: value.ixx,
            ixy: value.ixy,
            ixz: value.ixz,
            iyy: value.iyy,
            iyz: value.iyz,
            izz: value.izz,
        }
    }
}

impl From<DatapodInertial> for Inertial {
    fn from(value: DatapodInertial) -> Self {
        Self {
            origin: value.origin.into(),
            mass: value.mass,
            ixx: value.ixx,
            ixy: value.ixy,
            ixz: value.ixz,
            iyy: value.iyy,
            iyz: value.iyz,
            izz: value.izz,
        }
    }
}

/// FFI-safe UUID value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct DatapodUuid {
    pub bytes: [u8; 16],
}

impl From<Uuid> for DatapodUuid {
    fn from(value: Uuid) -> Self {
        Self { bytes: value.bytes }
    }
}

impl From<DatapodUuid> for Uuid {
    fn from(value: DatapodUuid) -> Self {
        Self { bytes: value.bytes }
    }
}

/// FFI-safe IP value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct DatapodIp {
    pub family: u32,
    pub _pad: u32,
    pub bytes: [u8; 16],
}

impl From<Ip> for DatapodIp {
    fn from(value: Ip) -> Self {
        Self {
            family: value.family,
            _pad: value._pad,
            bytes: value.bytes,
        }
    }
}

impl From<DatapodIp> for Ip {
    fn from(value: DatapodIp) -> Self {
        Self {
            family: value.family,
            _pad: value._pad,
            bytes: value.bytes,
        }
    }
}

/// FFI-safe MAC address value.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct DatapodMacAddr {
    pub bytes: [u8; 6],
    pub _pad: [u8; 2],
}

impl From<MacAddr> for DatapodMacAddr {
    fn from(value: MacAddr) -> Self {
        Self {
            bytes: value.bytes,
            _pad: value._pad,
        }
    }
}

impl From<DatapodMacAddr> for MacAddr {
    fn from(value: DatapodMacAddr) -> Self {
        Self {
            bytes: value.bytes,
            _pad: value._pad,
        }
    }
}

/// FFI-safe map entry value used inside the raw Map payload table.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct DatapodMapEntry {
    pub key_off: u32,
    pub key_len: u32,
    pub value_off: u32,
    pub value_len: u32,
}

impl From<MapEntry> for DatapodMapEntry {
    fn from(value: MapEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
            value_off: value.value_off,
            value_len: value.value_len,
        }
    }
}

impl From<DatapodMapEntry> for MapEntry {
    fn from(value: DatapodMapEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
            value_off: value.value_off,
            value_len: value.value_len,
        }
    }
}

/// FFI-safe set entry value used inside the raw Set payload table.
#[repr(C)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct DatapodSetEntry {
    pub key_off: u32,
    pub key_len: u32,
}

impl From<SetEntry> for DatapodSetEntry {
    fn from(value: SetEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
        }
    }
}

impl From<DatapodSetEntry> for SetEntry {
    fn from(value: DatapodSetEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
        }
    }
}

macro_rules! heap_handle {
    ($handle:ident, $inner:expr $(,)?) => {
        $handle {
            inner: $inner,
            archive_header: RefCell::new(Vec::new()),
        }
    };
}

/// Opaque polygon handle.
pub struct DatapodPolygon {
    inner: Polygon,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque byte-buffer handle.
pub struct DatapodBytesValue {
    inner: Bytes,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque UTF-8 datapod string handle (`seq::DpStr`).
pub struct DatapodDpStr {
    inner: DpStr,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque identifier string handle (`id::DpString`).
pub struct DatapodDpString {
    inner: DpString,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque linestring handle.
pub struct DatapodLinestring {
    inner: Linestring,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque multi-point handle.
pub struct DatapodMultiPoint {
    inner: MultiPoint,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque ring handle.
pub struct DatapodRing {
    inner: Ring,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque path handle.
pub struct DatapodPath {
    inner: Path,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque trajectory handle.
pub struct DatapodTrajectory {
    inner: Trajectory,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque raster grid handle.
pub struct DatapodGrid {
    inner: Grid,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque raster layer handle.
pub struct DatapodLayer {
    inner: Layer,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque map handle.
pub struct DatapodMap {
    inner: Map,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque set handle.
pub struct DatapodSet {
    inner: Set,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque vector handle.
pub struct DatapodVector {
    inner: Vector,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque matrix handle.
pub struct DatapodMatrix {
    inner: Matrix,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque tensor handle.
pub struct DatapodTensor {
    inner: Tensor,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque bit-vector handle.
pub struct DatapodBitVec {
    inner: BitVec,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque deque handle.
pub struct DatapodDeque {
    inner: Deque,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque queue handle.
pub struct DatapodQueue {
    inner: Queue,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque stack handle.
pub struct DatapodStack {
    inner: Stack,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque list handle.
pub struct DatapodList {
    inner: List,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque forward-list handle.
pub struct DatapodForwardList {
    inner: ForwardList,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque heap handle.
pub struct DatapodHeap {
    inner: Heap,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque indexed-heap handle.
pub struct DatapodIndexedHeap {
    inner: IndexedHeap,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque vec-of-vec handle.
pub struct DatapodVecvec {
    inner: Vecvec,
    archive_header: RefCell<Vec<u8>>,
}

/// Opaque paged vec-of-vec handle.
pub struct DatapodPagedVecvec {
    inner: PagedVecvec,
    archive_header: RefCell<Vec<u8>>,
}

unsafe fn typed_slice_in<'a, T>(ptr: *const T, len: usize, label: &str) -> Result<&'a [T], ()> {
    let byte_len = match len.checked_mul(std::mem::size_of::<T>()) {
        Some(byte_len) => byte_len,
        None => {
            set_last_error(format!(
                "{label} array length {len} overflows addressable memory"
            ));
            return Err(());
        }
    };
    if byte_len > isize::MAX as usize {
        set_last_error(format!(
            "{label} array byte length {byte_len} exceeds maximum supported slice length"
        ));
        return Err(());
    }
    if byte_len > 0 {
        validate_pointer_range(ptr.cast::<u8>(), byte_len, label)?;
    }
    if ptr.is_null() {
        if len == 0 {
            Ok(&[])
        } else {
            set_last_error(format!("null {label} array with non-zero length"));
            Err(())
        }
    } else {
        let align = std::mem::align_of::<T>();
        if (ptr as usize) % align != 0 {
            set_last_error(format!(
                "{label} array pointer is not aligned to {align} bytes"
            ));
            return Err(());
        }
        // SAFETY: caller promises `len` valid `T` values at `ptr`; we already
        // verified that the resulting byte range fits Rust's slice limits and
        // that the pointer satisfies Rust's alignment requirement for `T`.
        Ok(unsafe { std::slice::from_raw_parts(ptr, len) })
    }
}

unsafe fn points_in<'a>(ptr: *const DatapodPoint, len: usize) -> Result<&'a [DatapodPoint], ()> {
    unsafe { typed_slice_in(ptr, len, "point") }
}

unsafe fn bytes_out<'a>(ptr: *mut u8, len: usize) -> Result<&'a mut [u8], ()> {
    if len > isize::MAX as usize {
        set_last_error(format!(
            "output byte array length {len} exceeds maximum supported slice length"
        ));
        return Err(());
    }
    if ptr.is_null() {
        if len == 0 {
            Ok(&mut [])
        } else {
            set_last_error("null output byte array with non-zero length");
            Err(())
        }
    } else {
        if len > 0 {
            validate_pointer_range(ptr.cast_const(), len, "output byte array")?;
        }
        // SAFETY: caller promises `len` writable bytes at `ptr`.
        Ok(unsafe { std::slice::from_raw_parts_mut(ptr, len) })
    }
}

unsafe fn bytes_in<'a>(ptr: *const u8, len: usize) -> Result<&'a [u8], ()> {
    if len > isize::MAX as usize {
        set_last_error(format!(
            "input byte array length {len} exceeds maximum supported slice length"
        ));
        return Err(());
    }
    if ptr.is_null() {
        if len == 0 {
            Ok(&[])
        } else {
            set_last_error("null input byte array with non-zero length");
            Err(())
        }
    } else {
        if len > 0 {
            validate_pointer_range(ptr, len, "input byte array")?;
        }
        // SAFETY: caller promises `len` readable bytes at `ptr`.
        Ok(unsafe { std::slice::from_raw_parts(ptr, len) })
    }
}

fn pointer_range_overflows(ptr: *const u8, len: usize) -> bool {
    let start = ptr as usize;
    start.checked_add(len).is_none()
}

fn validate_pointer_range(ptr: *const u8, len: usize, label: &str) -> Result<(), ()> {
    if pointer_range_overflows(ptr, len) {
        let start = ptr as usize;
        set_last_error(format!(
            "{label} pointer range overflows address space: start=0x{start:x}, len={len}"
        ));
        return Err(());
    }
    Ok(())
}

fn wire_frame_in(frame: DatapodWireFrame) -> Result<crate::WireFrame<'static>, ()> {
    prevalidate_wire_frame_shape(frame)?;
    let header = unsafe { bytes_in(frame.header, frame.header_len) }?;
    let payload = unsafe { bytes_in(frame.payload, frame.payload_len) }?;
    Ok(crate::WireFrame {
        type_hash: frame.type_hash,
        header,
        payload,
    })
}

fn prevalidate_wire_frame_shape(frame: DatapodWireFrame) -> Result<(), ()> {
    let Some(info) = crate::registry::find_type_info(frame.type_hash) else {
        set_last_error("unknown datapod type hash");
        return Err(());
    };
    if frame.header_len != info.header_size {
        set_last_error(format!(
            "wrong frame header length: got {}, expected {}",
            frame.header_len, info.header_size
        ));
        return Err(());
    }
    if info.payload_kind == crate::registry::PayloadKind::Fixed && frame.payload_len != 0 {
        set_last_error(format!(
            "fixed-size datapod frame cannot carry payload bytes: got {}",
            frame.payload_len
        ));
        return Err(());
    }
    let Some(total_len) = frame.header_len.checked_add(frame.payload_len) else {
        set_last_error("frame header + payload length overflows addressable memory");
        return Err(());
    };
    if total_len > isize::MAX as usize {
        set_last_error(format!(
            "frame header + payload length {total_len} exceeds maximum supported slice length"
        ));
        return Err(());
    }
    Ok(())
}

fn prevalidate_wire_message_shape(message: DatapodWireMessage) -> Result<(), ()> {
    let Some(info) = crate::registry::find_type_info(message.type_hash) else {
        set_last_error("unknown datapod type hash");
        return Err(());
    };
    if message.len > isize::MAX as usize {
        set_last_error(format!(
            "wire message length {} exceeds maximum supported slice length",
            message.len
        ));
        return Err(());
    }
    if message.len < info.header_size {
        set_last_error(format!(
            "wire message too short: got {}, need at least {}",
            message.len, info.header_size
        ));
        return Err(());
    }
    if info.payload_kind == crate::registry::PayloadKind::Fixed && message.len != info.header_size {
        set_last_error(format!(
            "fixed-size datapod wire message cannot carry payload bytes: got {}",
            message.len - info.header_size
        ));
        return Err(());
    }
    Ok(())
}

fn prevalidate_joined_wire_len(header_len: usize, payload_len: usize) -> Result<(), ()> {
    let Some(total_len) = header_len.checked_add(payload_len) else {
        set_last_error("wire header + payload length overflows addressable memory");
        return Err(());
    };
    if total_len > isize::MAX as usize {
        set_last_error(format!(
            "wire header + payload length {total_len} exceeds maximum supported slice length"
        ));
        return Err(());
    }
    Ok(())
}

fn fixed_type_header_size(type_hash: u64) -> Result<usize, ()> {
    let Some(info) = crate::registry::find_type_info(type_hash) else {
        set_last_error("unknown datapod type hash");
        return Err(());
    };
    if info.payload_kind != crate::registry::PayloadKind::Fixed {
        set_last_error("datapod type is not fixed-size");
        return Err(());
    }
    Ok(info.header_size)
}

fn write_fixed_header<C, R>(value: C, out: *mut u8, out_len: usize) -> bool
where
    R: crate::DataPod + From<C>,
{
    clear_last_error();
    // SAFETY: pointer/length are validated before writing.
    let out = match unsafe { bytes_out(out, out_len) } {
        Ok(out) => out,
        Err(()) => return false,
    };
    out.fill(0);
    match crate::bind::write_header(&R::from(value), out) {
        Ok(()) => true,
        Err(error) => {
            set_last_error(error);
            false
        }
    }
}

fn read_fixed_header<C, R>(ptr: *const u8, len: usize, out: *mut C) -> bool
where
    C: From<R> + Default,
    R: crate::DataPod<Header = R> + crate::DataPodValidate + bytemuck::Pod + Copy,
{
    clear_last_error();
    if prepare_default_output(out, "output value").is_err() {
        return false;
    }
    // SAFETY: pointer/length are validated before reading.
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return false,
    };
    let value = match crate::bind::read_fixed_header::<R>(bytes) {
        Ok(value) => value,
        Err(error) => {
            set_last_error(error);
            return false;
        }
    };
    // SAFETY: validated non-null.
    unsafe {
        *out = C::from(value);
    }
    true
}

fn write_datapod_header<T: DataPod>(value: &T, out: *mut u8, out_len: usize) -> bool {
    clear_last_error();
    // SAFETY: pointer/length are validated before writing.
    let out = match unsafe { bytes_out(out, out_len) } {
        Ok(out) => out,
        Err(()) => return false,
    };
    out.fill(0);
    match crate::bind::write_header(value, out) {
        Ok(()) => true,
        Err(error) => {
            set_last_error(error);
            false
        }
    }
}

fn datapod_payload<T: DataPod>(value: &T) -> DatapodBytes {
    let payload = value.payload_bytes();
    DatapodBytes {
        ptr: payload.as_ptr(),
        len: payload.len(),
    }
}

fn ffi_byte_len_for_count(count: usize, element_size: usize, label: &str) -> Result<usize, ()> {
    let Some(byte_len) = count.checked_mul(element_size) else {
        set_last_error(format!(
            "{label} byte length overflows addressable memory: count={count}, element_size={element_size}"
        ));
        return Err(());
    };
    if byte_len > isize::MAX as usize {
        set_last_error(format!(
            "{label} byte length {byte_len} exceeds maximum supported slice length"
        ));
        return Err(());
    }
    Ok(byte_len)
}

fn clone_bytes(ptr: *const u8, len: usize) -> Option<Vec<u8>> {
    // SAFETY: pointer/length are validated before copying.
    match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => {
            let mut out = Vec::new();
            if let Err(error) = out.try_reserve_exact(bytes.len()) {
                set_last_error(format!(
                    "failed to reserve {} copied bytes: {error}",
                    bytes.len()
                ));
                return None;
            }
            out.extend_from_slice(bytes);
            Some(out)
        }
        Err(()) => None,
    }
}

fn ffi_type_hash<T: DataPod>() -> u64 {
    clear_last_error();
    crate::bind::type_hash::<T>()
}

fn ffi_rust_type_hash<T: 'static>() -> u64 {
    clear_last_error();
    crate::bind::rust_type_hash::<T>()
}

fn ffi_header_size<T: DataPod>() -> usize {
    clear_last_error();
    crate::bind::header_size::<T>()
}

fn ffi_pod_byte_size<T>() -> usize {
    clear_last_error();
    std::mem::size_of::<T>()
}

fn encoding_from_u32(value: u32) -> Option<Encoding> {
    match value {
        0 => Some(Encoding::U8),
        1 => Some(Encoding::U16),
        2 => Some(Encoding::U32),
        3 => Some(Encoding::U64),
        4 => Some(Encoding::I8),
        5 => Some(Encoding::I16),
        6 => Some(Encoding::I32),
        7 => Some(Encoding::I64),
        8 => Some(Encoding::F32),
        9 => Some(Encoding::F64),
        10 => Some(Encoding::Rgb8),
        11 => Some(Encoding::Rgba8),
        12 => Some(Encoding::Mono16),
        13 => Some(Encoding::Mono8),
        _ => None,
    }
}

macro_rules! fixed_wire_fns {
    (
        $type_hash_fn:ident,
        $header_size_fn:ident,
        $to_header_fn:ident,
        $from_header_fn:ident,
        $c_ty:ty,
        $rust_ty:ty
    ) => {
        #[unsafe(no_mangle)]
        pub extern "C" fn $type_hash_fn() -> u64 {
            ffi_type_hash::<$rust_ty>()
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn $header_size_fn() -> usize {
            ffi_header_size::<$rust_ty>()
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn $to_header_fn(value: $c_ty, out: *mut u8, out_len: usize) -> bool {
            write_fixed_header::<$c_ty, $rust_ty>(value, out, out_len)
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn $from_header_fn(ptr: *const u8, len: usize, out: *mut $c_ty) -> bool {
            read_fixed_header::<$c_ty, $rust_ty>(ptr, len, out)
        }
    };
}

fn write_pod_value<T: bytemuck::Pod>(value: &T, out: *mut u8, out_len: usize) -> bool {
    clear_last_error();
    let out = match unsafe { bytes_out(out, out_len) } {
        Ok(out) => out,
        Err(()) => return false,
    };
    out.fill(0);
    let bytes = bytemuck::bytes_of(value);
    if out.len() < bytes.len() {
        set_last_error(format!(
            "output buffer too small: need {}, got {}",
            bytes.len(),
            out.len()
        ));
        return false;
    }
    let Some(slot) = out.get_mut(..bytes.len()) else {
        set_last_error(format!(
            "output range unavailable: need {}, got {}",
            bytes.len(),
            out.len()
        ));
        return false;
    };
    slot.copy_from_slice(bytes);
    true
}

fn read_pod_value<C, R>(ptr: *const u8, len: usize, out: *mut C) -> bool
where
    C: From<R> + Default,
    R: bytemuck::Pod,
{
    clear_last_error();
    if prepare_default_output(out, "output value").is_err() {
        return false;
    }
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return false,
    };
    if bytes.len() != std::mem::size_of::<R>() {
        set_last_error(format!(
            "input buffer has wrong length: need exactly {}, got {}",
            std::mem::size_of::<R>(),
            bytes.len()
        ));
        return false;
    }
    let Some(input) = bytes.get(..std::mem::size_of::<R>()) else {
        set_last_error("input range unavailable");
        return false;
    };
    let value = bytemuck::pod_read_unaligned::<R>(input);
    unsafe {
        *out = C::from(value);
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_new(x: f64, y: f64, z: f64) -> DatapodPoint {
    clear_last_error();
    DatapodPoint { x, y, z }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_magnitude(point: DatapodPoint) -> f64 {
    clear_last_error();
    Point::from(point).magnitude()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_distance_to(a: DatapodPoint, b: DatapodPoint) -> f64 {
    clear_last_error();
    Point::from(a).distance_to(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_distance_to_2d(a: DatapodPoint, b: DatapodPoint) -> f64 {
    clear_last_error();
    Point::from(a).distance_to_2d(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_new(latitude: f64, longitude: f64, altitude: f64) -> DatapodGeo {
    clear_last_error();
    DatapodGeo {
        latitude,
        longitude,
        altitude,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_is_valid(geo: DatapodGeo) -> bool {
    clear_last_error();
    Geo::from(geo).is_valid()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_distance_to(a: DatapodGeo, b: DatapodGeo) -> f64 {
    clear_last_error();
    Geo::from(a).distance_to(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_bearing_to(a: DatapodGeo, b: DatapodGeo) -> f64 {
    clear_last_error();
    Geo::from(a).bearing_to(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_new(start: DatapodPoint, end: DatapodPoint) -> DatapodSegment {
    clear_last_error();
    DatapodSegment { start, end }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_length(segment: DatapodSegment) -> f64 {
    clear_last_error();
    Segment::from(segment).length()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_midpoint(segment: DatapodSegment) -> DatapodPoint {
    clear_last_error();
    Segment::from(segment).midpoint().into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_closest_point(
    segment: DatapodSegment,
    point: DatapodPoint,
) -> DatapodPoint {
    clear_last_error();
    Segment::from(segment).closest_point(point.into()).into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_distance_to(segment: DatapodSegment, point: DatapodPoint) -> f64 {
    clear_last_error();
    Segment::from(segment).distance_to(point.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_euler_new(roll: f64, pitch: f64, yaw: f64) -> DatapodEuler {
    clear_last_error();
    DatapodEuler { roll, pitch, yaw }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_quaternion_new(w: f64, x: f64, y: f64, z: f64) -> DatapodQuaternion {
    clear_last_error();
    DatapodQuaternion { w, x, y, z }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_quaternion_identity() -> DatapodQuaternion {
    clear_last_error();
    Quaternion::identity().into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_pose_new(
    point: DatapodPoint,
    rotation: DatapodQuaternion,
) -> DatapodPose {
    clear_last_error();
    DatapodPose { point, rotation }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_velocity_new(vx: f64, vy: f64, vz: f64) -> DatapodVelocity {
    clear_last_error();
    DatapodVelocity { vx, vy, vz }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_velocity_speed(velocity: DatapodVelocity) -> f64 {
    clear_last_error();
    Velocity::from(velocity).speed()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_acceleration_new(ax: f64, ay: f64, az: f64) -> DatapodAcceleration {
    clear_last_error();
    DatapodAcceleration { ax, ay, az }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_acceleration_magnitude(acceleration: DatapodAcceleration) -> f64 {
    clear_last_error();
    Acceleration::from(acceleration).magnitude()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transform_new(
    rw: f64,
    rx: f64,
    ry: f64,
    rz: f64,
    dw: f64,
    dx: f64,
    dy: f64,
    dz: f64,
) -> DatapodTransform {
    clear_last_error();
    DatapodTransform {
        rw,
        rx,
        ry,
        rz,
        dw,
        dx,
        dy,
        dz,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_state_new(
    pose: DatapodPose,
    linear_velocity: DatapodVelocity,
    angular_velocity: DatapodVelocity,
) -> DatapodState {
    clear_last_error();
    DatapodState {
        pose,
        linear_velocity,
        angular_velocity,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_loc_new(local: DatapodPoint, origin: DatapodGeo) -> DatapodLoc {
    clear_last_error();
    DatapodLoc { local, origin }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_utm_new(
    zone: i32,
    band: u32,
    easting: f64,
    northing: f64,
    altitude: f64,
) -> DatapodUtm {
    clear_last_error();
    DatapodUtm {
        zone,
        band,
        easting,
        northing,
        altitude,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_line_new(origin: DatapodPoint, direction: DatapodPoint) -> DatapodLine {
    clear_last_error();
    DatapodLine { origin, direction }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_rectangle_new(
    top_left: DatapodPoint,
    top_right: DatapodPoint,
    bottom_left: DatapodPoint,
    bottom_right: DatapodPoint,
) -> DatapodRectangle {
    clear_last_error();
    DatapodRectangle {
        top_left,
        top_right,
        bottom_left,
        bottom_right,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_rectangle_area(rectangle: DatapodRectangle) -> f64 {
    clear_last_error();
    Rectangle::from(rectangle).area()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_aabb_new(
    min_point: DatapodPoint,
    max_point: DatapodPoint,
) -> DatapodAabb {
    clear_last_error();
    DatapodAabb {
        min_point,
        max_point,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_aabb_center(aabb: DatapodAabb) -> DatapodPoint {
    clear_last_error();
    Aabb::from(aabb).center().into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bounding_sphere_new(
    center: DatapodPoint,
    radius: f64,
) -> DatapodBoundingSphere {
    clear_last_error();
    DatapodBoundingSphere { center, radius }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_circle_new(center: DatapodPoint, radius: f64) -> DatapodCircle {
    clear_last_error();
    DatapodCircle { center, radius }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_circle_area(circle: DatapodCircle) -> f64 {
    clear_last_error();
    Circle::from(circle).area()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_triangle_new(
    a: DatapodPoint,
    b: DatapodPoint,
    c: DatapodPoint,
) -> DatapodTriangle {
    clear_last_error();
    DatapodTriangle { a, b, c }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_triangle_area(triangle: DatapodTriangle) -> f64 {
    clear_last_error();
    Triangle::from(triangle).area()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_new(x: f64, y: f64, z: f64) -> DatapodSize {
    clear_last_error();
    DatapodSize { x, y, z }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box3_new(pose: DatapodPose, size: DatapodSize) -> DatapodBox3 {
    clear_last_error();
    DatapodBox3 { pose, size }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_value_type_hash() -> u64 {
    ffi_type_hash::<Size>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_value_header_size() -> usize {
    ffi_header_size::<Size>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_value_to_header_bytes(
    value: DatapodSize,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodSize, Size>(value, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_value_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodSize,
) -> bool {
    read_fixed_header::<DatapodSize, Size>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box3_type_hash() -> u64 {
    ffi_type_hash::<crate::Box>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box3_header_size() -> usize {
    ffi_header_size::<crate::Box>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box3_to_header_bytes(
    value: DatapodBox3,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodBox3, crate::Box>(value, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box3_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodBox3,
) -> bool {
    read_fixed_header::<DatapodBox3, crate::Box>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_twist_new(
    linear: DatapodVelocity,
    angular: DatapodVelocity,
) -> DatapodTwist {
    clear_last_error();
    DatapodTwist { linear, angular }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wrench_new(force: DatapodPoint, torque: DatapodPoint) -> DatapodWrench {
    clear_last_error();
    DatapodWrench { force, torque }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_odom_new(pose: DatapodPose, twist: DatapodTwist) -> DatapodOdom {
    clear_last_error();
    DatapodOdom { pose, twist }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_limits_new(
    lower: f64,
    upper: f64,
    effort: f64,
    velocity: f64,
) -> DatapodJointLimits {
    clear_last_error();
    DatapodJointLimits {
        lower,
        upper,
        effort,
        velocity,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_inertial_new(
    origin: DatapodPose,
    mass: f64,
    ixx: f64,
    ixy: f64,
    ixz: f64,
    iyy: f64,
    iyz: f64,
    izz: f64,
) -> DatapodInertial {
    clear_last_error();
    DatapodInertial {
        origin,
        mass,
        ixx,
        ixy,
        ixz,
        iyy,
        iyz,
        izz,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_uuid_nil() -> DatapodUuid {
    clear_last_error();
    Uuid::nil().into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ip_v4(a: u8, b: u8, c: u8, d: u8) -> DatapodIp {
    clear_last_error();
    Ip::from_v4_bytes([a, b, c, d]).into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mac_addr_new(bytes: *const u8, len: usize) -> DatapodMacAddr {
    clear_last_error();
    // SAFETY: pointer/length are validated before reading.
    let bytes = match unsafe { bytes_in(bytes, len) } {
        Ok(bytes) => bytes,
        Err(()) => return DatapodMacAddr::default(),
    };
    if bytes.len() != 6 {
        set_last_error("MAC address byte input must be exactly 6 bytes");
        return DatapodMacAddr::default();
    }
    let mut out = [0_u8; 6];
    out.copy_from_slice(bytes);
    MacAddr::new(out).into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_type_hash() -> u64 {
    ffi_type_hash::<Point>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_header_size() -> usize {
    ffi_header_size::<Point>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_to_header_bytes(
    value: DatapodPoint,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodPoint, Point>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodPoint,
) -> bool {
    read_fixed_header::<DatapodPoint, Point>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_type_hash() -> u64 {
    ffi_type_hash::<Geo>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_header_size() -> usize {
    ffi_header_size::<Geo>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_to_header_bytes(
    value: DatapodGeo,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodGeo, Geo>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodGeo,
) -> bool {
    read_fixed_header::<DatapodGeo, Geo>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_type_hash() -> u64 {
    ffi_type_hash::<Segment>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_header_size() -> usize {
    ffi_header_size::<Segment>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_to_header_bytes(
    value: DatapodSegment,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodSegment, Segment>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodSegment,
) -> bool {
    read_fixed_header::<DatapodSegment, Segment>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_euler_type_hash() -> u64 {
    ffi_type_hash::<Euler>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_euler_header_size() -> usize {
    ffi_header_size::<Euler>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_euler_to_header_bytes(
    value: DatapodEuler,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodEuler, Euler>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_euler_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodEuler,
) -> bool {
    read_fixed_header::<DatapodEuler, Euler>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_quaternion_type_hash() -> u64 {
    ffi_type_hash::<Quaternion>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_quaternion_header_size() -> usize {
    ffi_header_size::<Quaternion>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_quaternion_to_header_bytes(
    value: DatapodQuaternion,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodQuaternion, Quaternion>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_quaternion_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodQuaternion,
) -> bool {
    read_fixed_header::<DatapodQuaternion, Quaternion>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_pose_type_hash() -> u64 {
    ffi_type_hash::<Pose>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_pose_header_size() -> usize {
    ffi_header_size::<Pose>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_pose_to_header_bytes(
    value: DatapodPose,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodPose, Pose>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_pose_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodPose,
) -> bool {
    read_fixed_header::<DatapodPose, Pose>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_velocity_type_hash() -> u64 {
    ffi_type_hash::<Velocity>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_velocity_header_size() -> usize {
    ffi_header_size::<Velocity>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_velocity_to_header_bytes(
    value: DatapodVelocity,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodVelocity, Velocity>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_velocity_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodVelocity,
) -> bool {
    read_fixed_header::<DatapodVelocity, Velocity>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_acceleration_type_hash() -> u64 {
    ffi_type_hash::<Acceleration>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_acceleration_header_size() -> usize {
    ffi_header_size::<Acceleration>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_acceleration_to_header_bytes(
    value: DatapodAcceleration,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodAcceleration, Acceleration>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_acceleration_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodAcceleration,
) -> bool {
    read_fixed_header::<DatapodAcceleration, Acceleration>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transform_type_hash() -> u64 {
    ffi_type_hash::<Transform>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_transform_header_size() -> usize {
    ffi_header_size::<Transform>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_transform_to_header_bytes(
    value: DatapodTransform,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodTransform, Transform>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_transform_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodTransform,
) -> bool {
    read_fixed_header::<DatapodTransform, Transform>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_state_type_hash() -> u64 {
    ffi_type_hash::<State>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_state_header_size() -> usize {
    ffi_header_size::<State>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_state_to_header_bytes(
    value: DatapodState,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodState, State>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_state_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodState,
) -> bool {
    read_fixed_header::<DatapodState, State>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_loc_type_hash() -> u64 {
    ffi_type_hash::<Loc>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_loc_header_size() -> usize {
    ffi_header_size::<Loc>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_loc_to_header_bytes(
    value: DatapodLoc,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodLoc, Loc>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_loc_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodLoc,
) -> bool {
    read_fixed_header::<DatapodLoc, Loc>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_utm_type_hash() -> u64 {
    ffi_type_hash::<Utm>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_utm_header_size() -> usize {
    ffi_header_size::<Utm>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_utm_to_header_bytes(
    value: DatapodUtm,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodUtm, Utm>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_utm_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodUtm,
) -> bool {
    read_fixed_header::<DatapodUtm, Utm>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_line_type_hash() -> u64 {
    ffi_type_hash::<Line>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_line_header_size() -> usize {
    ffi_header_size::<Line>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_line_to_header_bytes(
    value: DatapodLine,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodLine, Line>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_line_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodLine,
) -> bool {
    read_fixed_header::<DatapodLine, Line>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_rectangle_type_hash() -> u64 {
    ffi_type_hash::<Rectangle>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_rectangle_header_size() -> usize {
    ffi_header_size::<Rectangle>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_rectangle_to_header_bytes(
    value: DatapodRectangle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodRectangle, Rectangle>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_rectangle_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodRectangle,
) -> bool {
    read_fixed_header::<DatapodRectangle, Rectangle>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_aabb_type_hash() -> u64 {
    ffi_type_hash::<Aabb>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_aabb_header_size() -> usize {
    ffi_header_size::<Aabb>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_aabb_to_header_bytes(
    value: DatapodAabb,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodAabb, Aabb>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_aabb_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodAabb,
) -> bool {
    read_fixed_header::<DatapodAabb, Aabb>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bounding_sphere_type_hash() -> u64 {
    ffi_type_hash::<BoundingSphere>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_bounding_sphere_header_size() -> usize {
    ffi_header_size::<BoundingSphere>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_bounding_sphere_to_header_bytes(
    value: DatapodBoundingSphere,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodBoundingSphere, BoundingSphere>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_bounding_sphere_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodBoundingSphere,
) -> bool {
    read_fixed_header::<DatapodBoundingSphere, BoundingSphere>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_circle_type_hash() -> u64 {
    ffi_type_hash::<Circle>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_circle_header_size() -> usize {
    ffi_header_size::<Circle>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_circle_to_header_bytes(
    value: DatapodCircle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodCircle, Circle>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_circle_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodCircle,
) -> bool {
    read_fixed_header::<DatapodCircle, Circle>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_triangle_type_hash() -> u64 {
    ffi_type_hash::<Triangle>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_triangle_header_size() -> usize {
    ffi_header_size::<Triangle>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_triangle_to_header_bytes(
    value: DatapodTriangle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodTriangle, Triangle>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_triangle_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodTriangle,
) -> bool {
    read_fixed_header::<DatapodTriangle, Triangle>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_twist_type_hash() -> u64 {
    ffi_type_hash::<Twist>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_twist_header_size() -> usize {
    ffi_header_size::<Twist>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_twist_to_header_bytes(
    value: DatapodTwist,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodTwist, Twist>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_twist_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodTwist,
) -> bool {
    read_fixed_header::<DatapodTwist, Twist>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_wrench_type_hash() -> u64 {
    ffi_type_hash::<Wrench>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_wrench_header_size() -> usize {
    ffi_header_size::<Wrench>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_wrench_to_header_bytes(
    value: DatapodWrench,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodWrench, Wrench>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_wrench_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodWrench,
) -> bool {
    read_fixed_header::<DatapodWrench, Wrench>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_odom_type_hash() -> u64 {
    ffi_type_hash::<Odom>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_odom_header_size() -> usize {
    ffi_header_size::<Odom>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_odom_to_header_bytes(
    value: DatapodOdom,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodOdom, Odom>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_odom_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodOdom,
) -> bool {
    read_fixed_header::<DatapodOdom, Odom>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_limits_type_hash() -> u64 {
    ffi_type_hash::<JointLimits>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_limits_header_size() -> usize {
    ffi_header_size::<JointLimits>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_limits_to_header_bytes(
    value: DatapodJointLimits,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodJointLimits, JointLimits>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_limits_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodJointLimits,
) -> bool {
    read_fixed_header::<DatapodJointLimits, JointLimits>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_inertial_type_hash() -> u64 {
    ffi_type_hash::<Inertial>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_inertial_header_size() -> usize {
    ffi_header_size::<Inertial>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_inertial_to_header_bytes(
    value: DatapodInertial,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodInertial, Inertial>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_inertial_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodInertial,
) -> bool {
    read_fixed_header::<DatapodInertial, Inertial>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_uuid_type_hash() -> u64 {
    ffi_type_hash::<Uuid>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_uuid_header_size() -> usize {
    ffi_header_size::<Uuid>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_uuid_to_header_bytes(
    value: DatapodUuid,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodUuid, Uuid>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_uuid_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodUuid,
) -> bool {
    read_fixed_header::<DatapodUuid, Uuid>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ip_type_hash() -> u64 {
    ffi_type_hash::<Ip>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_ip_header_size() -> usize {
    ffi_header_size::<Ip>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_ip_to_header_bytes(
    value: DatapodIp,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodIp, Ip>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_ip_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodIp,
) -> bool {
    read_fixed_header::<DatapodIp, Ip>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mac_addr_type_hash() -> u64 {
    ffi_type_hash::<MacAddr>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_mac_addr_header_size() -> usize {
    ffi_header_size::<MacAddr>()
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_mac_addr_to_header_bytes(
    value: DatapodMacAddr,
    out: *mut u8,
    out_len: usize,
) -> bool {
    write_fixed_header::<DatapodMacAddr, MacAddr>(value, out, out_len)
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_mac_addr_from_header_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodMacAddr,
) -> bool {
    read_fixed_header::<DatapodMacAddr, MacAddr>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_entry_new(
    key_off: u32,
    key_len: u32,
    value_off: u32,
    value_len: u32,
) -> DatapodMapEntry {
    clear_last_error();
    DatapodMapEntry {
        key_off,
        key_len,
        value_off,
        value_len,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_entry_type_hash() -> u64 {
    ffi_rust_type_hash::<MapEntry>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_entry_byte_size() -> usize {
    ffi_pod_byte_size::<MapEntry>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_entry_to_bytes(
    value: DatapodMapEntry,
    out: *mut u8,
    out_len: usize,
) -> bool {
    let value = MapEntry::from(value);
    write_pod_value(&value, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_entry_from_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodMapEntry,
) -> bool {
    read_pod_value::<DatapodMapEntry, MapEntry>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_entry_new(key_off: u32, key_len: u32) -> DatapodSetEntry {
    clear_last_error();
    DatapodSetEntry { key_off, key_len }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_entry_type_hash() -> u64 {
    ffi_rust_type_hash::<SetEntry>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_entry_byte_size() -> usize {
    ffi_pod_byte_size::<SetEntry>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_entry_to_bytes(
    value: DatapodSetEntry,
    out: *mut u8,
    out_len: usize,
) -> bool {
    let value = SetEntry::from(value);
    write_pod_value(&value, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_entry_from_bytes(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodSetEntry,
) -> bool {
    read_pod_value::<DatapodSetEntry, SetEntry>(ptr, len, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_new(
    vertices: *const DatapodPoint,
    len: usize,
) -> *mut DatapodPolygon {
    clear_last_error();
    // SAFETY: pointer/length are validated before conversion.
    let vertices = match unsafe { points_in(vertices, len) } {
        Ok(vertices) => match collect_ffi_values(vertices, "polygon vertices") {
            Ok(vertices) => vertices,
            Err(()) => return ptr::null_mut(),
        },
        Err(()) => return ptr::null_mut(),
    };
    Box::into_raw(Box::new(heap_handle!(
        DatapodPolygon,
        Polygon::new(vertices)
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_free(polygon: *mut DatapodPolygon) {
    free_boxed_handle(polygon);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_len(polygon: *const DatapodPolygon) -> usize {
    clear_last_error();
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return 0;
    }
    // SAFETY: validated non-null.
    unsafe { &*polygon }.inner.num_vertices()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_area(polygon: *const DatapodPolygon) -> f64 {
    clear_last_error();
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return 0.0;
    }
    // SAFETY: validated non-null.
    unsafe { &*polygon }.inner.area()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_perimeter(polygon: *const DatapodPolygon) -> f64 {
    clear_last_error();
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return 0.0;
    }
    // SAFETY: validated non-null.
    unsafe { &*polygon }.inner.perimeter()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_contains(
    polygon: *const DatapodPolygon,
    point: DatapodPoint,
) -> bool {
    clear_last_error();
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return false;
    }
    // SAFETY: validated non-null.
    unsafe { &*polygon }.inner.contains(point.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_vertex(
    polygon: *const DatapodPolygon,
    index: usize,
    out: *mut DatapodPoint,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "output point").is_err() {
        return false;
    }
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return false;
    }
    // SAFETY: validated non-null.
    let polygon = unsafe { &*polygon };
    let Some(point) = polygon.inner.vertices.get(index).copied() else {
        set_last_error("polygon vertex index out of range");
        return false;
    };
    // SAFETY: validated non-null.
    unsafe {
        *out = point.into();
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_vertices(polygon: *const DatapodPolygon) -> DatapodBytes {
    clear_last_error();
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return DatapodBytes::empty();
    }
    // SAFETY: validated non-null.
    let polygon = unsafe { &*polygon };
    let Ok(byte_len) = ffi_byte_len_for_count(
        polygon.inner.vertices.len(),
        std::mem::size_of::<Point>(),
        "polygon vertices",
    ) else {
        return DatapodBytes::empty();
    };
    DatapodBytes {
        ptr: polygon.inner.vertices.as_ptr().cast(),
        len: byte_len,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_type_hash() -> u64 {
    ffi_type_hash::<Polygon>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_header_size() -> usize {
    ffi_header_size::<Polygon>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_to_header_bytes(
    polygon: *const DatapodPolygon,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return false;
    }
    write_datapod_header(&unsafe { &*polygon }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_payload(polygon: *const DatapodPolygon) -> DatapodBytes {
    clear_last_error();
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*polygon }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_to_wire(
    handle: *const DatapodPolygon,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null polygon handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_from_wire(ptr: *const u8, len: usize) -> *mut DatapodPolygon {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Polygon>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodPolygon, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodPolygonView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "polygon view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<Polygon>(
        crate::bind::emitted_type_hash::<Polygon>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodPolygonView {
            vertex_count: payload.len() / point_size,
            point_size,
            vertices: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodPolygonView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "polygon view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Polygon>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodPolygonView {
            vertex_count: payload.len() / point_size,
            point_size,
            vertices: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_new(ptr: *const u8, len: usize) -> *mut DatapodBytesValue {
    clear_last_error();
    let Some(data) = clone_bytes(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodBytesValue,
        Bytes { data }
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_new(ptr: *const u8, len: usize) -> *mut DatapodDpStr {
    clear_last_error();
    let Some(data) = clone_bytes(ptr, len) else {
        return ptr::null_mut();
    };
    let text = match std::str::from_utf8(&data) {
        Ok(text) => text,
        Err(error) => {
            set_last_error(error.to_string());
            return ptr::null_mut();
        }
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodDpStr,
        DpStr::from_str(text)
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_new(ptr: *const u8, len: usize) -> *mut DatapodDpString {
    clear_last_error();
    let Some(bytes) = clone_bytes(ptr, len) else {
        return ptr::null_mut();
    };
    let inner = DpString::new(bytes);
    if let Err(error) = DpString::validate_wire_parts(&inner.header(), inner.payload_bytes()) {
        set_last_error(error.to_string());
        return ptr::null_mut();
    }
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodDpString, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_new(
    points: *const DatapodPoint,
    len: usize,
) -> *mut DatapodLinestring {
    clear_last_error();
    let points = match unsafe { points_in(points, len) } {
        Ok(points) => match collect_ffi_values(points, "linestring points") {
            Ok(points) => points,
            Err(()) => return ptr::null_mut(),
        },
        Err(()) => return ptr::null_mut(),
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodLinestring,
        Linestring::new(points)
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_new(
    points: *const DatapodPoint,
    len: usize,
) -> *mut DatapodMultiPoint {
    clear_last_error();
    let points = match unsafe { points_in(points, len) } {
        Ok(points) => match collect_ffi_values(points, "multi_point points") {
            Ok(points) => points,
            Err(()) => return ptr::null_mut(),
        },
        Err(()) => return ptr::null_mut(),
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodMultiPoint,
        MultiPoint::new(points)
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_new(points: *const DatapodPoint, len: usize) -> *mut DatapodRing {
    clear_last_error();
    let points = match unsafe { points_in(points, len) } {
        Ok(points) => match collect_ffi_values(points, "ring points") {
            Ok(points) => points,
            Err(()) => return ptr::null_mut(),
        },
        Err(()) => return ptr::null_mut(),
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodRing,
        Ring::new(points)
    )))
}

unsafe fn poses_in<'a>(ptr: *const DatapodPose, len: usize) -> Result<&'a [DatapodPose], ()> {
    unsafe { typed_slice_in(ptr, len, "pose") }
}

unsafe fn states_in<'a>(ptr: *const DatapodState, len: usize) -> Result<&'a [DatapodState], ()> {
    unsafe { typed_slice_in(ptr, len, "state") }
}

fn collect_ffi_values<T, U>(items: &[T], label: &str) -> Result<Vec<U>, ()>
where
    T: Copy,
    U: From<T>,
{
    let mut values = Vec::new();
    values.try_reserve_exact(items.len()).map_err(|err| {
        set_last_error(format!(
            "failed to reserve {} {label} values: {err}",
            items.len()
        ));
    })?;
    values.extend(items.iter().copied().map(U::from));
    Ok(values)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_new(waypoints: *const DatapodPose, len: usize) -> *mut DatapodPath {
    clear_last_error();
    let waypoints = match unsafe { poses_in(waypoints, len) } {
        Ok(poses) => match collect_ffi_values(poses, "path waypoints") {
            Ok(poses) => poses,
            Err(()) => return ptr::null_mut(),
        },
        Err(()) => return ptr::null_mut(),
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodPath,
        Path::new(waypoints)
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_new(
    states: *const DatapodState,
    len: usize,
) -> *mut DatapodTrajectory {
    clear_last_error();
    let states = match unsafe { states_in(states, len) } {
        Ok(states) => match collect_ffi_values(states, "trajectory states") {
            Ok(states) => states,
            Err(()) => return ptr::null_mut(),
        },
        Err(()) => return ptr::null_mut(),
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodTrajectory,
        Trajectory::new(states)
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_new(
    rows: u32,
    cols: u32,
    encoding: u32,
    centered: bool,
    resolution: f64,
    pose: DatapodPose,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodGrid {
    clear_last_error();
    let Some(encoding) = encoding_from_u32(encoding) else {
        set_last_error("unknown grid encoding");
        return ptr::null_mut();
    };
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let inner = Grid::new(
        rows,
        cols,
        encoding,
        resolution,
        centered,
        pose.into(),
        data,
    );
    if let Err(error) = Grid::validate_wire_parts(&inner.header(), inner.payload_bytes()) {
        set_last_error(error.to_string());
        return ptr::null_mut();
    }
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodGrid, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_new(
    rows: u32,
    cols: u32,
    layers: u32,
    encoding: u32,
    centered: bool,
    resolution: f64,
    layer_height: f64,
    pose: DatapodPose,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodLayer {
    clear_last_error();
    let Some(encoding) = encoding_from_u32(encoding) else {
        set_last_error("unknown layer encoding");
        return ptr::null_mut();
    };
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let inner = Layer {
        rows,
        cols,
        layers,
        encoding,
        centered: if centered { 1 } else { 0 },
        _pad: 0,
        resolution,
        layer_height,
        pose: pose.into(),
        data,
    };
    if let Err(error) = Layer::validate_wire_parts(&inner.header(), inner.payload_bytes()) {
        set_last_error(error.to_string());
        return ptr::null_mut();
    }
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodLayer, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_new() -> *mut DatapodMap {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodMap, Map::new())))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_insert(
    map: *mut DatapodMap,
    key: *const u8,
    key_len: usize,
    value: *const u8,
    value_len: usize,
) -> bool {
    clear_last_error();
    if map.is_null() {
        set_last_error("null map handle");
        return false;
    }
    let key = match unsafe { bytes_in(key, key_len) } {
        Ok(key) => key,
        Err(()) => return false,
    };
    let value = match unsafe { bytes_in(value, value_len) } {
        Ok(value) => value,
        Err(()) => return false,
    };
    match unsafe { &mut *map }.inner.try_insert(key, value) {
        Ok(_) => true,
        Err(error) => {
            set_last_error(error.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_new() -> *mut DatapodSet {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodSet, Set::new())))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_insert(set: *mut DatapodSet, key: *const u8, key_len: usize) -> bool {
    clear_last_error();
    if set.is_null() {
        set_last_error("null set handle");
        return false;
    }
    let key = match unsafe { bytes_in(key, key_len) } {
        Ok(key) => key,
        Err(()) => return false,
    };
    match unsafe { &mut *set }.inner.try_insert(key) {
        Ok(inserted) => inserted,
        Err(error) => {
            set_last_error(error.to_string());
            false
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_from_bytes(
    element_size: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodVector {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Vector {
        element_size,
        _pad: 0,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodVector, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_from_bytes(
    rows: u32,
    cols: u32,
    element_size: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodMatrix {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Matrix {
        rows,
        cols,
        element_size,
        _pad: 0,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodMatrix, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_from_bytes(
    rows: u32,
    cols: u32,
    layers: u32,
    element_size: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodTensor {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Tensor {
        rows,
        cols,
        layers,
        element_size,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodTensor, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_from_bytes(
    bits: u64,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodBitVec {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(BitVec { bits, data }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodBitVec, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_from_bytes(
    element_size: u32,
    split_byte: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodDeque {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Deque {
        element_size,
        split_byte,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodDeque, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_from_bytes(
    element_size: u32,
    front: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodQueue {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Queue {
        element_size,
        front,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodQueue, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_from_bytes(
    element_size: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodStack {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Stack {
        element_size,
        _pad: 0,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodStack, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_empty(element_size: u32) -> *mut DatapodList {
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodList,
        List {
            head: crate::seq::LIST_NIL,
            tail: crate::seq::LIST_NIL,
            free_head: crate::seq::LIST_NIL,
            size_: 0,
            element_size,
            _pad: 0,
            data: Vec::new(),
        },
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_empty(element_size: u32) -> *mut DatapodForwardList {
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodForwardList,
        ForwardList {
            head: crate::seq::FORWARD_LIST_NIL,
            free_head: crate::seq::FORWARD_LIST_NIL,
            size_: 0,
            element_size,
            data: Vec::new(),
        },
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_from_bytes(
    element_size: u32,
    min_order: bool,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodHeap {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Heap {
        element_size,
        order: if min_order {
            crate::seq::HeapOrder::Min
        } else {
            crate::seq::HeapOrder::Max
        },
        _pad: [0; 3],
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodHeap, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_from_bytes(
    priority_size: u32,
    min_order: bool,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodIndexedHeap {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(IndexedHeap {
        priority_size,
        order: if min_order {
            crate::seq::HeapOrder::Min
        } else {
            crate::seq::HeapOrder::Max
        },
        _pad: [0; 3],
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodIndexedHeap,
        inner,
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_from_bytes(
    element_size: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodVecvec {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(Vecvec {
        element_size,
        _pad: 0,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodVecvec, inner,)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_from_bytes(
    element_size: u32,
    data: *const u8,
    data_len: usize,
) -> *mut DatapodPagedVecvec {
    clear_last_error();
    let Some(data) = clone_bytes(data, data_len) else {
        return ptr::null_mut();
    };
    let Some(inner) = validate_heap_inner(PagedVecvec {
        element_size,
        _pad: 0,
        data,
    }) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodPagedVecvec,
        inner,
    )))
}
#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_free(handle: *mut DatapodBytesValue) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_type_hash() -> u64 {
    ffi_type_hash::<Bytes>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_header_size() -> usize {
    ffi_header_size::<Bytes>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_to_header_bytes(
    handle: *const DatapodBytesValue,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null bytes_value handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_payload(handle: *const DatapodBytesValue) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null bytes_value handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_to_wire(
    handle: *const DatapodBytesValue,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null bytes_value handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_from_wire(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodBytesValue {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Bytes>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodBytesValue, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodBytesView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "bytes view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Bytes>(crate::bind::emitted_type_hash::<Bytes>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodBytesView {
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodBytesView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "bytes view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Bytes>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodBytesView {
            payload: DatapodBytes {
                ptr: view.as_slice().as_ptr(),
                len: view.as_slice().len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_free(handle: *mut DatapodDpStr) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_type_hash() -> u64 {
    ffi_type_hash::<DpStr>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_header_size() -> usize {
    ffi_header_size::<DpStr>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_to_header_bytes(
    handle: *const DatapodDpStr,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null dpstr handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_payload(handle: *const DatapodDpStr) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null dpstr handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_free(handle: *mut DatapodDpString) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_type_hash() -> u64 {
    ffi_type_hash::<DpString>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_header_size() -> usize {
    ffi_header_size::<DpString>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_to_header_bytes(
    handle: *const DatapodDpString,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null dpstring handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_payload(handle: *const DatapodDpString) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null dpstring handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_free(handle: *mut DatapodLinestring) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_type_hash() -> u64 {
    ffi_type_hash::<Linestring>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_header_size() -> usize {
    ffi_header_size::<Linestring>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_to_header_bytes(
    handle: *const DatapodLinestring,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null linestring handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_payload(handle: *const DatapodLinestring) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null linestring handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_free(handle: *mut DatapodMultiPoint) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_type_hash() -> u64 {
    ffi_type_hash::<MultiPoint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_header_size() -> usize {
    ffi_header_size::<MultiPoint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_to_header_bytes(
    handle: *const DatapodMultiPoint,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null multi_point handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_payload(handle: *const DatapodMultiPoint) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null multi_point handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_free(handle: *mut DatapodRing) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_type_hash() -> u64 {
    ffi_type_hash::<Ring>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_header_size() -> usize {
    ffi_header_size::<Ring>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_to_header_bytes(
    handle: *const DatapodRing,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null ring handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_payload(handle: *const DatapodRing) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null ring handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_free(handle: *mut DatapodPath) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_type_hash() -> u64 {
    ffi_type_hash::<Path>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_header_size() -> usize {
    ffi_header_size::<Path>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_to_header_bytes(
    handle: *const DatapodPath,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null path handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_payload(handle: *const DatapodPath) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null path handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_free(handle: *mut DatapodTrajectory) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_type_hash() -> u64 {
    ffi_type_hash::<Trajectory>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_header_size() -> usize {
    ffi_header_size::<Trajectory>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_to_header_bytes(
    handle: *const DatapodTrajectory,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null trajectory handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_payload(handle: *const DatapodTrajectory) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null trajectory handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_free(handle: *mut DatapodGrid) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_type_hash() -> u64 {
    ffi_type_hash::<Grid>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_header_size() -> usize {
    ffi_header_size::<Grid>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_to_header_bytes(
    handle: *const DatapodGrid,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null grid handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_payload(handle: *const DatapodGrid) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null grid handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_to_wire(
    handle: *const DatapodGrid,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null grid handle");
        return false;
    }
    let message = crate::to_wire_message(&unsafe { &*handle }.inner);
    unsafe {
        *out = owned_bytes(message.bytes);
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_from_wire(ptr: *const u8, len: usize) -> *mut DatapodGrid {
    clear_last_error();
    let Some(bytes) = clone_bytes(ptr, len) else {
        return ptr::null_mut();
    };
    let message = crate::WireMessage {
        type_hash: crate::bind::emitted_type_hash::<Grid>(),
        bytes,
    };
    let inner = match crate::from_wire_message::<Grid>(&message) {
        Ok(value) => value,
        Err(error) => {
            set_last_error(error.to_string());
            return ptr::null_mut();
        }
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodGrid, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodGridView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "grid view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Grid>(crate::bind::emitted_type_hash::<Grid>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodGridView {
            rows: view.header.rows,
            cols: view.header.cols,
            encoding: view.header.encoding.0,
            centered: view.header.centered != 0,
            resolution: view.header.resolution,
            pose: view.header.pose.into(),
            data: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodGridView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "grid view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Grid>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodGridView {
            rows: view.header.rows,
            cols: view.header.cols,
            encoding: view.header.encoding.0,
            centered: view.header.centered != 0,
            resolution: view.header.resolution,
            pose: view.header.pose.into(),
            data: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_free(handle: *mut DatapodLayer) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_type_hash() -> u64 {
    ffi_type_hash::<Layer>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_header_size() -> usize {
    ffi_header_size::<Layer>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_to_header_bytes(
    handle: *const DatapodLayer,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null layer handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_payload(handle: *const DatapodLayer) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null layer handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_free(handle: *mut DatapodMap) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_type_hash() -> u64 {
    ffi_type_hash::<Map>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_header_size() -> usize {
    ffi_header_size::<Map>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_to_header_bytes(
    handle: *const DatapodMap,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null map handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_payload(handle: *const DatapodMap) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null map handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_free(handle: *mut DatapodSet) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_type_hash() -> u64 {
    ffi_type_hash::<Set>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_header_size() -> usize {
    ffi_header_size::<Set>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_to_header_bytes(
    handle: *const DatapodSet,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null set handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_payload(handle: *const DatapodSet) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null set handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_free(handle: *mut DatapodVector) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_type_hash() -> u64 {
    ffi_type_hash::<Vector>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_header_size() -> usize {
    ffi_header_size::<Vector>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_to_header_bytes(
    handle: *const DatapodVector,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null vector handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_payload(handle: *const DatapodVector) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null vector handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_free(handle: *mut DatapodMatrix) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_type_hash() -> u64 {
    ffi_type_hash::<Matrix>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_header_size() -> usize {
    ffi_header_size::<Matrix>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_to_header_bytes(
    handle: *const DatapodMatrix,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null matrix handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_payload(handle: *const DatapodMatrix) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null matrix handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_to_wire(
    handle: *const DatapodMatrix,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null matrix handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_from_wire(ptr: *const u8, len: usize) -> *mut DatapodMatrix {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Matrix>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodMatrix, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodMatrixView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "matrix view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Matrix>(crate::bind::emitted_type_hash::<Matrix>(), bytes)
        {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodMatrixView {
            rows: view.header.rows,
            cols: view.header.cols,
            element_size: view.header.element_size,
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodMatrixView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "matrix view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Matrix>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodMatrixView {
            rows: view.header.rows,
            cols: view.header.cols,
            element_size: view.header.element_size,
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_archive(
    handle: *const DatapodMatrix,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null matrix handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodMatrixView,
) -> bool {
    datapod_matrix_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_matrix_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodMatrix {
    heap_handle_from_archive::<Matrix, DatapodMatrix>(frame, datapod_matrix_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_archive(
    handle: *const DatapodPolygon,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null polygon handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodPolygonView,
) -> bool {
    datapod_polygon_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodPolygon {
    heap_handle_from_archive::<Polygon, DatapodPolygon>(frame, datapod_polygon_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_archive(
    handle: *const DatapodBytesValue,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null bytes handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodBytesView,
) -> bool {
    datapod_bytes_value_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bytes_value_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodBytesValue {
    heap_handle_from_archive::<Bytes, DatapodBytesValue>(frame, datapod_bytes_value_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_archive(
    handle: *const DatapodGrid,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null grid handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodGridView,
) -> bool {
    datapod_grid_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_grid_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodGrid {
    heap_handle_from_archive::<Grid, DatapodGrid>(frame, datapod_grid_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_archive(
    handle: *const DatapodDpStr,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null dpstr handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodDpStrView,
) -> bool {
    datapod_dpstr_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodDpStr {
    heap_handle_from_archive::<DpStr, DatapodDpStr>(frame, datapod_dpstr_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_archive(
    handle: *const DatapodDpString,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null dpstring handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodDpStringView,
) -> bool {
    datapod_dpstring_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodDpString {
    heap_handle_from_archive::<DpString, DatapodDpString>(frame, datapod_dpstring_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_archive(
    handle: *const DatapodLinestring,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null linestring handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodLinestringView,
) -> bool {
    datapod_linestring_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodLinestring {
    heap_handle_from_archive::<Linestring, DatapodLinestring>(frame, datapod_linestring_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_archive(
    handle: *const DatapodMultiPoint,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null multi_point handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodMultiPointView,
) -> bool {
    datapod_multi_point_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodMultiPoint {
    heap_handle_from_archive::<MultiPoint, DatapodMultiPoint>(frame, datapod_multi_point_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_archive(
    handle: *const DatapodRing,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null ring handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodRingView,
) -> bool {
    datapod_ring_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodRing {
    heap_handle_from_archive::<Ring, DatapodRing>(frame, datapod_ring_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_archive(
    handle: *const DatapodPath,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null path handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodPathView,
) -> bool {
    datapod_path_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodPath {
    heap_handle_from_archive::<Path, DatapodPath>(frame, datapod_path_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_archive(
    handle: *const DatapodTrajectory,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null trajectory handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodTrajectoryView,
) -> bool {
    datapod_trajectory_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodTrajectory {
    heap_handle_from_archive::<Trajectory, DatapodTrajectory>(frame, datapod_trajectory_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_archive(
    handle: *const DatapodLayer,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null layer handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodLayerView,
) -> bool {
    datapod_layer_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodLayer {
    heap_handle_from_archive::<Layer, DatapodLayer>(frame, datapod_layer_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_archive(
    handle: *const DatapodMap,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null map handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodMapView,
) -> bool {
    datapod_map_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodMap {
    heap_handle_from_archive::<Map, DatapodMap>(frame, datapod_map_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_archive(
    handle: *const DatapodSet,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null set handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodSetView,
) -> bool {
    datapod_set_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodSet {
    heap_handle_from_archive::<Set, DatapodSet>(frame, datapod_set_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_archive(
    handle: *const DatapodVector,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null vector handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodVectorView,
) -> bool {
    datapod_vector_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodVector {
    heap_handle_from_archive::<Vector, DatapodVector>(frame, datapod_vector_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_archive(
    handle: *const DatapodTensor,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null tensor handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodTensorView,
) -> bool {
    datapod_tensor_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodTensor {
    heap_handle_from_archive::<Tensor, DatapodTensor>(frame, datapod_tensor_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_archive(
    handle: *const DatapodBitVec,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null bitvec handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodBitVecView,
) -> bool {
    datapod_bitvec_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodBitVec {
    heap_handle_from_archive::<BitVec, DatapodBitVec>(frame, datapod_bitvec_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_archive(
    handle: *const DatapodDeque,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null deque handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodDequeView,
) -> bool {
    datapod_deque_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodDeque {
    heap_handle_from_archive::<Deque, DatapodDeque>(frame, datapod_deque_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_archive(
    handle: *const DatapodQueue,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null queue handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodQueueView,
) -> bool {
    datapod_queue_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodQueue {
    heap_handle_from_archive::<Queue, DatapodQueue>(frame, datapod_queue_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_archive(
    handle: *const DatapodStack,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null stack handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodStackView,
) -> bool {
    datapod_stack_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodStack {
    heap_handle_from_archive::<Stack, DatapodStack>(frame, datapod_stack_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_archive(
    handle: *const DatapodList,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null list handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodListView,
) -> bool {
    datapod_list_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodList {
    heap_handle_from_archive::<List, DatapodList>(frame, datapod_list_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_archive(
    handle: *const DatapodForwardList,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null forward_list handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodForwardListView,
) -> bool {
    datapod_forward_list_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodForwardList {
    heap_handle_from_archive::<ForwardList, DatapodForwardList>(
        frame,
        datapod_forward_list_from_wire,
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_archive(
    handle: *const DatapodHeap,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null heap handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodHeapView,
) -> bool {
    datapod_heap_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodHeap {
    heap_handle_from_archive::<Heap, DatapodHeap>(frame, datapod_heap_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_archive(
    handle: *const DatapodIndexedHeap,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null indexed_heap handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodIndexedHeapView,
) -> bool {
    datapod_indexed_heap_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodIndexedHeap {
    heap_handle_from_archive::<IndexedHeap, DatapodIndexedHeap>(
        frame,
        datapod_indexed_heap_from_wire,
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_archive(
    handle: *const DatapodVecvec,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null vecvec handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodVecvecView,
) -> bool {
    datapod_vecvec_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_from_archive(frame: DatapodArchiveFrame) -> *mut DatapodVecvec {
    heap_handle_from_archive::<Vecvec, DatapodVecvec>(frame, datapod_vecvec_from_wire)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_archive(
    handle: *const DatapodPagedVecvec,
    out: *mut DatapodArchiveFrame,
) -> bool {
    clear_last_error();
    if prepare_wire_frame_output(out, "archive-frame").is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null paged_vecvec handle");
        return false;
    }
    let handle = unsafe { &*handle };
    heap_archive(&handle.inner, &handle.archive_header, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_view_from_archive(
    frame: DatapodArchiveFrame,
    out: *mut DatapodPagedVecvecView,
) -> bool {
    datapod_paged_vecvec_view_from_frame(frame, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_from_archive(
    frame: DatapodArchiveFrame,
) -> *mut DatapodPagedVecvec {
    heap_handle_from_archive::<PagedVecvec, DatapodPagedVecvec>(
        frame,
        datapod_paged_vecvec_from_wire,
    )
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_free(handle: *mut DatapodTensor) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_type_hash() -> u64 {
    ffi_type_hash::<Tensor>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_header_size() -> usize {
    ffi_header_size::<Tensor>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_to_header_bytes(
    handle: *const DatapodTensor,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null tensor handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_payload(handle: *const DatapodTensor) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null tensor handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_free(handle: *mut DatapodBitVec) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_type_hash() -> u64 {
    ffi_type_hash::<BitVec>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_header_size() -> usize {
    ffi_header_size::<BitVec>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_to_header_bytes(
    handle: *const DatapodBitVec,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null bitvec handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_payload(handle: *const DatapodBitVec) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null bitvec handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_free(handle: *mut DatapodDeque) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_type_hash() -> u64 {
    ffi_type_hash::<Deque>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_header_size() -> usize {
    ffi_header_size::<Deque>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_to_header_bytes(
    handle: *const DatapodDeque,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null deque handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_payload(handle: *const DatapodDeque) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null deque handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_free(handle: *mut DatapodQueue) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_type_hash() -> u64 {
    ffi_type_hash::<Queue>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_header_size() -> usize {
    ffi_header_size::<Queue>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_to_header_bytes(
    handle: *const DatapodQueue,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null queue handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_payload(handle: *const DatapodQueue) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null queue handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_free(handle: *mut DatapodStack) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_type_hash() -> u64 {
    ffi_type_hash::<Stack>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_header_size() -> usize {
    ffi_header_size::<Stack>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_to_header_bytes(
    handle: *const DatapodStack,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null stack handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_payload(handle: *const DatapodStack) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null stack handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_free(handle: *mut DatapodList) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_type_hash() -> u64 {
    ffi_type_hash::<List>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_header_size() -> usize {
    ffi_header_size::<List>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_to_header_bytes(
    handle: *const DatapodList,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null list handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_payload(handle: *const DatapodList) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null list handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_free(handle: *mut DatapodForwardList) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_type_hash() -> u64 {
    ffi_type_hash::<ForwardList>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_header_size() -> usize {
    ffi_header_size::<ForwardList>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_to_header_bytes(
    handle: *const DatapodForwardList,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null forward_list handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_payload(handle: *const DatapodForwardList) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null forward_list handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_free(handle: *mut DatapodHeap) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_type_hash() -> u64 {
    ffi_type_hash::<Heap>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_header_size() -> usize {
    ffi_header_size::<Heap>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_to_header_bytes(
    handle: *const DatapodHeap,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null heap handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_payload(handle: *const DatapodHeap) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null heap handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_free(handle: *mut DatapodIndexedHeap) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_type_hash() -> u64 {
    ffi_type_hash::<IndexedHeap>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_header_size() -> usize {
    ffi_header_size::<IndexedHeap>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_to_header_bytes(
    handle: *const DatapodIndexedHeap,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null indexed_heap handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_payload(handle: *const DatapodIndexedHeap) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null indexed_heap handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_free(handle: *mut DatapodVecvec) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_type_hash() -> u64 {
    ffi_type_hash::<Vecvec>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_header_size() -> usize {
    ffi_header_size::<Vecvec>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_to_header_bytes(
    handle: *const DatapodVecvec,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null vecvec handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_payload(handle: *const DatapodVecvec) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null vecvec handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_free(handle: *mut DatapodPagedVecvec) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_type_hash() -> u64 {
    ffi_type_hash::<PagedVecvec>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_header_size() -> usize {
    ffi_header_size::<PagedVecvec>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_to_header_bytes(
    handle: *const DatapodPagedVecvec,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null paged_vecvec handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_payload(handle: *const DatapodPagedVecvec) -> DatapodBytes {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null paged_vecvec handle");
        return DatapodBytes::empty();
    }
    datapod_payload(&unsafe { &*handle }.inner)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_to_wire(
    handle: *const DatapodDpStr,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null dpstr handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_from_wire(ptr: *const u8, len: usize) -> *mut DatapodDpStr {
    clear_last_error();
    let Some(inner) = heap_from_wire::<DpStr>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodDpStr, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodDpStrView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "dpstr view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<DpStr>(crate::bind::emitted_type_hash::<DpStr>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodDpStrView {
            utf8: DatapodBytes {
                ptr: view.as_bytes().as_ptr(),
                len: view.as_bytes().len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstr_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodDpStrView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "dpstr view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<DpStr>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodDpStrView {
            utf8: DatapodBytes {
                ptr: view.as_bytes().as_ptr(),
                len: view.as_bytes().len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_to_wire(
    handle: *const DatapodDpString,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null dpstring handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_from_wire(ptr: *const u8, len: usize) -> *mut DatapodDpString {
    clear_last_error();
    let Some(inner) = heap_from_wire::<DpString>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodDpString, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodDpStringView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "dpstring view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<DpString>(
        crate::bind::emitted_type_hash::<DpString>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    unsafe {
        *out = DatapodDpStringView {
            bytes: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_dpstring_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodDpStringView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "dpstring view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<DpString>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    unsafe {
        *out = DatapodDpStringView {
            bytes: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_to_wire(
    handle: *const DatapodLinestring,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null linestring handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_from_wire(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodLinestring {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Linestring>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodLinestring, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodLinestringView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "linestring view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<Linestring>(
        crate::bind::emitted_type_hash::<Linestring>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodLinestringView {
            point_count: payload.len() / point_size,
            point_size,
            points: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_linestring_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodLinestringView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "linestring view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Linestring>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodLinestringView {
            point_count: payload.len() / point_size,
            point_size,
            points: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_to_wire(
    handle: *const DatapodMultiPoint,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null multi_point handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_from_wire(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodMultiPoint {
    clear_last_error();
    let Some(inner) = heap_from_wire::<MultiPoint>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodMultiPoint, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodMultiPointView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "multi_point view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<MultiPoint>(
        crate::bind::emitted_type_hash::<MultiPoint>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodMultiPointView {
            point_count: payload.len() / point_size,
            point_size,
            points: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_multi_point_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodMultiPointView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "multi_point view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<MultiPoint>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodMultiPointView {
            point_count: payload.len() / point_size,
            point_size,
            points: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_to_wire(
    handle: *const DatapodRing,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null ring handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_from_wire(ptr: *const u8, len: usize) -> *mut DatapodRing {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Ring>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodRing, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodRingView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "ring view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Ring>(crate::bind::emitted_type_hash::<Ring>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodRingView {
            point_count: payload.len() / point_size,
            point_size,
            points: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_ring_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodRingView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "ring view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Ring>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let point_size = core::mem::size_of::<Point>();
    unsafe {
        *out = DatapodRingView {
            point_count: payload.len() / point_size,
            point_size,
            points: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_to_wire(
    handle: *const DatapodPath,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null path handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_from_wire(ptr: *const u8, len: usize) -> *mut DatapodPath {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Path>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodPath, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodPathView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "path view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Path>(crate::bind::emitted_type_hash::<Path>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    let payload = view.payload_bytes();
    let pose_size = core::mem::size_of::<Pose>();
    unsafe {
        *out = DatapodPathView {
            waypoint_count: payload.len() / pose_size,
            pose_size,
            waypoints: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_path_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodPathView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "path view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Path>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let pose_size = core::mem::size_of::<Pose>();
    unsafe {
        *out = DatapodPathView {
            waypoint_count: payload.len() / pose_size,
            pose_size,
            waypoints: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_to_wire(
    handle: *const DatapodTrajectory,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null trajectory handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_from_wire(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodTrajectory {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Trajectory>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodTrajectory, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodTrajectoryView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "trajectory view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<Trajectory>(
        crate::bind::emitted_type_hash::<Trajectory>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let state_size = core::mem::size_of::<State>();
    unsafe {
        *out = DatapodTrajectoryView {
            state_count: payload.len() / state_size,
            state_size,
            states: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_trajectory_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodTrajectoryView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "trajectory view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Trajectory>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let state_size = core::mem::size_of::<State>();
    unsafe {
        *out = DatapodTrajectoryView {
            state_count: payload.len() / state_size,
            state_size,
            states: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_to_wire(
    handle: *const DatapodLayer,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null layer handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_from_wire(ptr: *const u8, len: usize) -> *mut DatapodLayer {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Layer>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodLayer, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodLayerView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "layer view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Layer>(crate::bind::emitted_type_hash::<Layer>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodLayerView {
            rows: view.header.rows,
            cols: view.header.cols,
            layers: view.header.layers,
            encoding: view.header.encoding.0,
            centered: view.header.centered != 0,
            resolution: view.header.resolution,
            layer_height: view.header.layer_height,
            pose: view.header.pose.into(),
            data: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_layer_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodLayerView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "layer view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Layer>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodLayerView {
            rows: view.header.rows,
            cols: view.header.cols,
            layers: view.header.layers,
            encoding: view.header.encoding.0,
            centered: view.header.centered != 0,
            resolution: view.header.resolution,
            layer_height: view.header.layer_height,
            pose: view.header.pose.into(),
            data: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_to_wire(
    handle: *const DatapodMap,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null map handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_from_wire(ptr: *const u8, len: usize) -> *mut DatapodMap {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Map>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodMap, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodMapView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "map view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<Map>(crate::bind::emitted_type_hash::<Map>(), bytes)
    {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let count = view.size();
    let Ok((entries, blob)) =
        assoc_payload_ranges::<Map>(payload, count, core::mem::size_of::<MapEntry>(), "map")
    else {
        return false;
    };
    let Ok(count) = ffi_usize_to_u32::<Map>(count, "map count") else {
        return false;
    };
    unsafe {
        *out = DatapodMapView {
            count,
            entries: DatapodBytes {
                ptr: entries.as_ptr(),
                len: entries.len(),
            },
            blob: DatapodBytes {
                ptr: blob.as_ptr(),
                len: blob.len(),
            },
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_map_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodMapView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "map view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Map>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let count = view.size();
    let Ok((entries, blob)) =
        assoc_payload_ranges::<Map>(payload, count, core::mem::size_of::<MapEntry>(), "map")
    else {
        return false;
    };
    let Ok(count) = ffi_usize_to_u32::<Map>(count, "map count") else {
        return false;
    };
    unsafe {
        *out = DatapodMapView {
            count,
            entries: DatapodBytes {
                ptr: entries.as_ptr(),
                len: entries.len(),
            },
            blob: DatapodBytes {
                ptr: blob.as_ptr(),
                len: blob.len(),
            },
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_to_wire(
    handle: *const DatapodSet,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null set handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_from_wire(ptr: *const u8, len: usize) -> *mut DatapodSet {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Set>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodSet, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodSetView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "set view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<Set>(crate::bind::emitted_type_hash::<Set>(), bytes)
    {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let count = view.size();
    let Ok((entries, blob)) =
        assoc_payload_ranges::<Set>(payload, count, core::mem::size_of::<SetEntry>(), "set")
    else {
        return false;
    };
    let Ok(count) = ffi_usize_to_u32::<Set>(count, "set count") else {
        return false;
    };
    unsafe {
        *out = DatapodSetView {
            count,
            entries: DatapodBytes {
                ptr: entries.as_ptr(),
                len: entries.len(),
            },
            blob: DatapodBytes {
                ptr: blob.as_ptr(),
                len: blob.len(),
            },
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_set_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodSetView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "set view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Set>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload_bytes();
    let count = view.size();
    let Ok((entries, blob)) =
        assoc_payload_ranges::<Set>(payload, count, core::mem::size_of::<SetEntry>(), "set")
    else {
        return false;
    };
    let Ok(count) = ffi_usize_to_u32::<Set>(count, "set count") else {
        return false;
    };
    unsafe {
        *out = DatapodSetView {
            count,
            entries: DatapodBytes {
                ptr: entries.as_ptr(),
                len: entries.len(),
            },
            blob: DatapodBytes {
                ptr: blob.as_ptr(),
                len: blob.len(),
            },
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_to_wire(
    handle: *const DatapodVector,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null vector handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_from_wire(ptr: *const u8, len: usize) -> *mut DatapodVector {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Vector>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodVector, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodVectorView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "vector view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Vector>(crate::bind::emitted_type_hash::<Vector>(), bytes)
        {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodVectorView {
            element_size: view.header.element_size,
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vector_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodVectorView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "vector view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Vector>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodVectorView {
            element_size: view.header.element_size,
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_to_wire(
    handle: *const DatapodTensor,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null tensor handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_from_wire(ptr: *const u8, len: usize) -> *mut DatapodTensor {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Tensor>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodTensor, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodTensorView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "tensor view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Tensor>(crate::bind::emitted_type_hash::<Tensor>(), bytes)
        {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodTensorView {
            rows: view.header.rows,
            cols: view.header.cols,
            layers: view.header.layers,
            element_size: view.header.element_size,
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_tensor_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodTensorView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "tensor view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Tensor>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodTensorView {
            rows: view.header.rows,
            cols: view.header.cols,
            layers: view.header.layers,
            element_size: view.header.element_size,
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_to_wire(
    handle: *const DatapodBitVec,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null bitvec handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_from_wire(ptr: *const u8, len: usize) -> *mut DatapodBitVec {
    clear_last_error();
    let Some(inner) = heap_from_wire::<BitVec>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodBitVec, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodBitVecView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "bitvec view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<BitVec>(crate::bind::emitted_type_hash::<BitVec>(), bytes)
        {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodBitVecView {
            bits: view.header.bits,
            data: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_bitvec_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodBitVecView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "bitvec view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<BitVec>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodBitVecView {
            bits: view.header.bits,
            data: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_to_wire(
    handle: *const DatapodDeque,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null deque handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_from_wire(ptr: *const u8, len: usize) -> *mut DatapodDeque {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Deque>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodDeque, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodDequeView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "deque view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Deque>(crate::bind::emitted_type_hash::<Deque>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    let payload = view.payload;
    let Ok(split) = ffi_u32_to_usize::<Deque>(view.header.split_byte, "split_byte") else {
        return false;
    };
    let Ok(count) = ffi_element_count::<Deque>(payload, view.header.element_size, "deque") else {
        return false;
    };
    let Ok((front, back)) = deque_payload_ranges(payload, split) else {
        return false;
    };
    unsafe {
        *out = DatapodDequeView {
            element_size: view.header.element_size,
            split_byte: view.header.split_byte,
            element_count: count,
            front: DatapodBytes {
                ptr: front.as_ptr(),
                len: front.len(),
            },
            back: DatapodBytes {
                ptr: back.as_ptr(),
                len: back.len(),
            },
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_deque_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodDequeView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "deque view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Deque>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok(split) = ffi_u32_to_usize::<Deque>(view.header.split_byte, "split_byte") else {
        return false;
    };
    let Ok(count) = ffi_element_count::<Deque>(payload, view.header.element_size, "deque") else {
        return false;
    };
    let Ok((front, back)) = deque_payload_ranges(payload, split) else {
        return false;
    };
    unsafe {
        *out = DatapodDequeView {
            element_size: view.header.element_size,
            split_byte: view.header.split_byte,
            element_count: count,
            front: DatapodBytes {
                ptr: front.as_ptr(),
                len: front.len(),
            },
            back: DatapodBytes {
                ptr: back.as_ptr(),
                len: back.len(),
            },
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_to_wire(
    handle: *const DatapodQueue,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null queue handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_from_wire(ptr: *const u8, len: usize) -> *mut DatapodQueue {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Queue>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodQueue, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodQueueView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "queue view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Queue>(crate::bind::emitted_type_hash::<Queue>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    let payload = view.payload;
    let Ok(raw_count) = ffi_element_count::<Queue>(payload, view.header.element_size, "queue")
    else {
        return false;
    };
    let Ok(front) = ffi_u32_to_usize::<Queue>(view.header.front, "front") else {
        return false;
    };
    let Some(logical_count) = raw_count.checked_sub(front) else {
        set_last_error(
            crate::wire::invalid_header::<Queue>("queue front exceeds raw count").to_string(),
        );
        return false;
    };
    unsafe {
        *out = DatapodQueueView {
            element_size: view.header.element_size,
            front: view.header.front,
            raw_count,
            logical_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_queue_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodQueueView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "queue view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Queue>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok(raw_count) = ffi_element_count::<Queue>(payload, view.header.element_size, "queue")
    else {
        return false;
    };
    let Ok(front) = ffi_u32_to_usize::<Queue>(view.header.front, "front") else {
        return false;
    };
    let Some(logical_count) = raw_count.checked_sub(front) else {
        set_last_error(
            crate::wire::invalid_header::<Queue>("queue front exceeds raw count").to_string(),
        );
        return false;
    };
    unsafe {
        *out = DatapodQueueView {
            element_size: view.header.element_size,
            front: view.header.front,
            raw_count,
            logical_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_to_wire(
    handle: *const DatapodStack,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null stack handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_from_wire(ptr: *const u8, len: usize) -> *mut DatapodStack {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Stack>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodStack, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodStackView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "stack view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Stack>(crate::bind::emitted_type_hash::<Stack>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    let payload = view.payload;
    let Ok(count) = ffi_element_count::<Stack>(payload, view.header.element_size, "stack") else {
        return false;
    };
    unsafe {
        *out = DatapodStackView {
            element_size: view.header.element_size,
            element_count: count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_stack_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodStackView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "stack view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Stack>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok(count) = ffi_element_count::<Stack>(payload, view.header.element_size, "stack") else {
        return false;
    };
    unsafe {
        *out = DatapodStackView {
            element_size: view.header.element_size,
            element_count: count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_to_wire(
    handle: *const DatapodList,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null list handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_from_wire(ptr: *const u8, len: usize) -> *mut DatapodList {
    clear_last_error();
    let Some(inner) = heap_from_wire::<List>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodList, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodListView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "list view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<List>(crate::bind::emitted_type_hash::<List>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    let payload = view.payload;
    let Ok((node_size, slot_count)) =
        ffi_node_layout::<List>(payload, view.header.element_size, "list")
    else {
        return false;
    };
    unsafe {
        *out = DatapodListView {
            head: view.header.head,
            tail: view.header.tail,
            free_head: view.header.free_head,
            size: view.header.size_,
            element_size: view.header.element_size,
            node_size,
            slot_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_list_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodListView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "list view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<List>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok((node_size, slot_count)) =
        ffi_node_layout::<List>(payload, view.header.element_size, "list")
    else {
        return false;
    };
    unsafe {
        *out = DatapodListView {
            head: view.header.head,
            tail: view.header.tail,
            free_head: view.header.free_head,
            size: view.header.size_,
            element_size: view.header.element_size,
            node_size,
            slot_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_to_wire(
    handle: *const DatapodForwardList,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null forward_list handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_from_wire(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodForwardList {
    clear_last_error();
    let Some(inner) = heap_from_wire::<ForwardList>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodForwardList,
        inner
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodForwardListView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "forward_list view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<ForwardList>(
        crate::bind::type_hash::<ForwardList>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok((node_size, slot_count)) =
        ffi_node_layout::<ForwardList>(payload, view.header.element_size, "forward_list")
    else {
        return false;
    };
    unsafe {
        *out = DatapodForwardListView {
            head: view.header.head,
            free_head: view.header.free_head,
            size: view.header.size_,
            element_size: view.header.element_size,
            node_size,
            slot_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_forward_list_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodForwardListView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "forward_list view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<ForwardList>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok((node_size, slot_count)) =
        ffi_node_layout::<ForwardList>(payload, view.header.element_size, "forward_list")
    else {
        return false;
    };
    unsafe {
        *out = DatapodForwardListView {
            head: view.header.head,
            free_head: view.header.free_head,
            size: view.header.size_,
            element_size: view.header.element_size,
            node_size,
            slot_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_to_wire(
    handle: *const DatapodHeap,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null heap handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_from_wire(ptr: *const u8, len: usize) -> *mut DatapodHeap {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Heap>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodHeap, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodHeapView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "heap view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Heap>(crate::bind::emitted_type_hash::<Heap>(), bytes) {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    let payload = view.payload;
    let Ok(count) = ffi_element_count::<Heap>(payload, view.header.element_size, "heap") else {
        return false;
    };
    unsafe {
        *out = DatapodHeapView {
            element_size: view.header.element_size,
            order: view.header.order.0,
            element_count: count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_heap_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodHeapView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "heap view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Heap>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok(count) = ffi_element_count::<Heap>(payload, view.header.element_size, "heap") else {
        return false;
    };
    unsafe {
        *out = DatapodHeapView {
            element_size: view.header.element_size,
            order: view.header.order.0,
            element_count: count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_to_wire(
    handle: *const DatapodIndexedHeap,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null indexed_heap handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_from_wire(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodIndexedHeap {
    clear_last_error();
    let Some(inner) = heap_from_wire::<IndexedHeap>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodIndexedHeap,
        inner
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodIndexedHeapView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "indexed_heap view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<IndexedHeap>(
        crate::bind::type_hash::<IndexedHeap>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok((entry_size, entry_count)) = ffi_indexed_heap_layout(payload, view.header.priority_size)
    else {
        return false;
    };
    unsafe {
        *out = DatapodIndexedHeapView {
            priority_size: view.header.priority_size,
            order: view.header.order.0,
            entry_size,
            entry_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_indexed_heap_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodIndexedHeapView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "indexed_heap view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<IndexedHeap>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok((entry_size, entry_count)) = ffi_indexed_heap_layout(payload, view.header.priority_size)
    else {
        return false;
    };
    unsafe {
        *out = DatapodIndexedHeapView {
            priority_size: view.header.priority_size,
            order: view.header.order.0,
            entry_size,
            entry_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_to_wire(
    handle: *const DatapodVecvec,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null vecvec handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_from_wire(ptr: *const u8, len: usize) -> *mut DatapodVecvec {
    clear_last_error();
    let Some(inner) = heap_from_wire::<Vecvec>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(DatapodVecvec, inner)))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodVecvecView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "vecvec view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view =
        match crate::access_wire_bytes::<Vecvec>(crate::bind::emitted_type_hash::<Vecvec>(), bytes)
        {
            Ok(view) => view,
            Err(error) => {
                set_last_error(error.to_string());
                return false;
            }
        };
    unsafe {
        *out = DatapodVecvecView {
            element_size: view.header.element_size,
            bucket_count: view.bucket_count(),
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_vecvec_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodVecvecView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "vecvec view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<Vecvec>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    unsafe {
        *out = DatapodVecvecView {
            element_size: view.header.element_size,
            bucket_count: view.bucket_count(),
            payload: DatapodBytes {
                ptr: view.data.as_ptr(),
                len: view.data.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_to_wire(
    handle: *const DatapodPagedVecvec,
    out: *mut DatapodOwnedBytes,
) -> bool {
    clear_last_error();
    if prepare_owned_bytes_output(out).is_err() {
        return false;
    }
    if handle.is_null() {
        set_last_error("null paged_vecvec handle");
        return false;
    }
    heap_to_wire(&unsafe { &*handle }.inner, out)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_from_wire(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodPagedVecvec {
    clear_last_error();
    let Some(inner) = heap_from_wire::<PagedVecvec>(ptr, len) else {
        return ptr::null_mut();
    };
    std::boxed::Box::into_raw(std::boxed::Box::new(heap_handle!(
        DatapodPagedVecvec,
        inner
    )))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_view_from_wire(
    ptr: *const u8,
    len: usize,
    out: *mut DatapodPagedVecvecView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "paged_vecvec view").is_err() {
        return false;
    }
    let Ok(bytes) = (unsafe { bytes_in(ptr, len) }) else {
        return false;
    };
    let view = match crate::access_wire_bytes::<PagedVecvec>(
        crate::bind::type_hash::<PagedVecvec>(),
        bytes,
    ) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok(bucket_count) = read_u32_le_ffi(payload, 0, "paged_vecvec payload") else {
        return false;
    };
    unsafe {
        *out = DatapodPagedVecvecView {
            element_size: view.header.element_size,
            bucket_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_paged_vecvec_view_from_frame(
    frame: DatapodWireFrame,
    out: *mut DatapodPagedVecvecView,
) -> bool {
    clear_last_error();
    if prepare_default_output(out, "paged_vecvec view").is_err() {
        return false;
    }
    let Ok(frame) = wire_frame_in(frame) else {
        return false;
    };
    let view = match crate::access_wire_frame::<PagedVecvec>(frame) {
        Ok(view) => view,
        Err(error) => {
            set_last_error(error.to_string());
            return false;
        }
    };
    let payload = view.payload;
    let Ok(bucket_count) = read_u32_le_ffi(payload, 0, "paged_vecvec payload") else {
        return false;
    };
    unsafe {
        *out = DatapodPagedVecvecView {
            element_size: view.header.element_size,
            bucket_count,
            payload: DatapodBytes {
                ptr: payload.as_ptr(),
                len: payload.len(),
            },
        };
    }
    true
}

/// Opaque fixed-value handle for PointKey.
pub struct DatapodPointKeyHandle {
    inner: PointKey,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_key_new_default() -> *mut DatapodPointKeyHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodPointKeyHandle {
        inner: <PointKey>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_key_free(handle: *mut DatapodPointKeyHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_key_type_hash() -> u64 {
    ffi_type_hash::<PointKey>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_key_header_size() -> usize {
    ffi_header_size::<PointKey>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_key_to_header_bytes(
    handle: *const DatapodPointKeyHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null point_key handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_key_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodPointKeyHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<PointKey>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodPointKeyHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Size.
pub struct DatapodSizeHandle {
    inner: Size,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_new_default() -> *mut DatapodSizeHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSizeHandle {
        inner: <Size>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_free(handle: *mut DatapodSizeHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_type_hash() -> u64 {
    ffi_type_hash::<Size>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_header_size() -> usize {
    ffi_header_size::<Size>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_to_header_bytes(
    handle: *const DatapodSizeHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null size handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_size_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodSizeHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Size>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSizeHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Square.
pub struct DatapodSquareHandle {
    inner: Square,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_square_new_default() -> *mut DatapodSquareHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSquareHandle {
        inner: <Square>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_square_free(handle: *mut DatapodSquareHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_square_type_hash() -> u64 {
    ffi_type_hash::<Square>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_square_header_size() -> usize {
    ffi_header_size::<Square>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_square_to_header_bytes(
    handle: *const DatapodSquareHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null square handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_square_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodSquareHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Square>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSquareHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Obb.
pub struct DatapodObbHandle {
    inner: Obb,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_obb_new_default() -> *mut DatapodObbHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodObbHandle {
        inner: <Obb>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_obb_free(handle: *mut DatapodObbHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_obb_type_hash() -> u64 {
    ffi_type_hash::<Obb>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_obb_header_size() -> usize {
    ffi_header_size::<Obb>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_obb_to_header_bytes(
    handle: *const DatapodObbHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null obb handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_obb_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodObbHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Obb>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodObbHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for crate::Box.
pub struct DatapodBoxHandle {
    inner: crate::Box,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_new_default() -> *mut DatapodBoxHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodBoxHandle {
        inner: <crate::Box>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_free(handle: *mut DatapodBoxHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_type_hash() -> u64 {
    ffi_type_hash::<crate::Box>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_header_size() -> usize {
    ffi_header_size::<crate::Box>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_to_header_bytes(
    handle: *const DatapodBoxHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null box handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodBoxHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<crate::Box>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodBoxHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for GaussianPoint.
pub struct DatapodGaussianPointHandle {
    inner: GaussianPoint,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_point_new_default() -> *mut DatapodGaussianPointHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianPointHandle {
        inner: <GaussianPoint>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_point_free(handle: *mut DatapodGaussianPointHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_point_type_hash() -> u64 {
    ffi_type_hash::<GaussianPoint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_point_header_size() -> usize {
    ffi_header_size::<GaussianPoint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_point_to_header_bytes(
    handle: *const DatapodGaussianPointHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null gaussian_point handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_point_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodGaussianPointHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<GaussianPoint>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianPointHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for GaussianCircle.
pub struct DatapodGaussianCircleHandle {
    inner: GaussianCircle,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_circle_new_default() -> *mut DatapodGaussianCircleHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianCircleHandle {
        inner: <GaussianCircle>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_circle_free(handle: *mut DatapodGaussianCircleHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_circle_type_hash() -> u64 {
    ffi_type_hash::<GaussianCircle>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_circle_header_size() -> usize {
    ffi_header_size::<GaussianCircle>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_circle_to_header_bytes(
    handle: *const DatapodGaussianCircleHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null gaussian_circle handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_circle_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodGaussianCircleHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<GaussianCircle>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianCircleHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for GaussianRectangle.
pub struct DatapodGaussianRectangleHandle {
    inner: GaussianRectangle,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_rectangle_new_default() -> *mut DatapodGaussianRectangleHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianRectangleHandle {
        inner: <GaussianRectangle>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_rectangle_free(handle: *mut DatapodGaussianRectangleHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_rectangle_type_hash() -> u64 {
    ffi_type_hash::<GaussianRectangle>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_rectangle_header_size() -> usize {
    ffi_header_size::<GaussianRectangle>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_rectangle_to_header_bytes(
    handle: *const DatapodGaussianRectangleHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null gaussian_rectangle handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_rectangle_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodGaussianRectangleHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<GaussianRectangle>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianRectangleHandle {
                inner,
            }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for GaussianBox.
pub struct DatapodGaussianBoxHandle {
    inner: GaussianBox,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_box_new_default() -> *mut DatapodGaussianBoxHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianBoxHandle {
        inner: <GaussianBox>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_box_free(handle: *mut DatapodGaussianBoxHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_box_type_hash() -> u64 {
    ffi_type_hash::<GaussianBox>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_box_header_size() -> usize {
    ffi_header_size::<GaussianBox>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_box_to_header_bytes(
    handle: *const DatapodGaussianBoxHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null gaussian_box handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_gaussian_box_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodGaussianBoxHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<GaussianBox>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGaussianBoxHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Accel.
pub struct DatapodAccelHandle {
    inner: Accel,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_accel_new_default() -> *mut DatapodAccelHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodAccelHandle {
        inner: <Accel>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_accel_free(handle: *mut DatapodAccelHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_accel_type_hash() -> u64 {
    ffi_type_hash::<Accel>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_accel_header_size() -> usize {
    ffi_header_size::<Accel>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_accel_to_header_bytes(
    handle: *const DatapodAccelHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null accel handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_accel_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodAccelHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Accel>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodAccelHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for JointDynamics.
pub struct DatapodJointDynamicsHandle {
    inner: JointDynamics,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_dynamics_new_default() -> *mut DatapodJointDynamicsHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointDynamicsHandle {
        inner: <JointDynamics>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_dynamics_free(handle: *mut DatapodJointDynamicsHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_dynamics_type_hash() -> u64 {
    ffi_type_hash::<JointDynamics>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_dynamics_header_size() -> usize {
    ffi_header_size::<JointDynamics>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_dynamics_to_header_bytes(
    handle: *const DatapodJointDynamicsHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null joint_dynamics handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_dynamics_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodJointDynamicsHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<JointDynamics>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointDynamicsHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for JointMimic.
pub struct DatapodJointMimicHandle {
    inner: JointMimic,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_mimic_new_default() -> *mut DatapodJointMimicHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointMimicHandle {
        inner: <JointMimic>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_mimic_free(handle: *mut DatapodJointMimicHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_mimic_type_hash() -> u64 {
    ffi_type_hash::<JointMimic>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_mimic_header_size() -> usize {
    ffi_header_size::<JointMimic>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_mimic_to_header_bytes(
    handle: *const DatapodJointMimicHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null joint_mimic handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_mimic_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodJointMimicHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<JointMimic>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointMimicHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for JointSafetyController.
pub struct DatapodJointSafetyControllerHandle {
    inner: JointSafetyController,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_safety_controller_new_default()
-> *mut DatapodJointSafetyControllerHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointSafetyControllerHandle {
        inner: <JointSafetyController>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_safety_controller_free(
    handle: *mut DatapodJointSafetyControllerHandle,
) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_safety_controller_type_hash() -> u64 {
    ffi_type_hash::<JointSafetyController>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_safety_controller_header_size() -> usize {
    ffi_header_size::<JointSafetyController>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_safety_controller_to_header_bytes(
    handle: *const DatapodJointSafetyControllerHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null joint_safety_controller handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_safety_controller_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodJointSafetyControllerHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<JointSafetyController>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointSafetyControllerHandle {
                inner,
            }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for JointCalibration.
pub struct DatapodJointCalibrationHandle {
    inner: JointCalibration,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_calibration_new_default() -> *mut DatapodJointCalibrationHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointCalibrationHandle {
        inner: <JointCalibration>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_calibration_free(handle: *mut DatapodJointCalibrationHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_calibration_type_hash() -> u64 {
    ffi_type_hash::<JointCalibration>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_calibration_header_size() -> usize {
    ffi_header_size::<JointCalibration>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_calibration_to_header_bytes(
    handle: *const DatapodJointCalibrationHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null joint_calibration handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_calibration_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodJointCalibrationHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<JointCalibration>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointCalibrationHandle {
                inner,
            }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for KV.
pub struct DatapodKvHandle {
    inner: KV,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_kv_new_default() -> *mut DatapodKvHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodKvHandle {
        inner: <KV>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_kv_free(handle: *mut DatapodKvHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_kv_type_hash() -> u64 {
    ffi_type_hash::<KV>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_kv_header_size() -> usize {
    ffi_header_size::<KV>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_kv_to_header_bytes(
    handle: *const DatapodKvHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null kv handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_kv_from_header_bytes(ptr: *const u8, len: usize) -> *mut DatapodKvHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<KV>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodKvHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for BoxShape.
pub struct DatapodBoxShapeHandle {
    inner: BoxShape,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_shape_new_default() -> *mut DatapodBoxShapeHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodBoxShapeHandle {
        inner: <BoxShape>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_shape_free(handle: *mut DatapodBoxShapeHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_shape_type_hash() -> u64 {
    ffi_type_hash::<BoxShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_shape_header_size() -> usize {
    ffi_header_size::<BoxShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_shape_to_header_bytes(
    handle: *const DatapodBoxShapeHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null box_shape handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_box_shape_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodBoxShapeHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<BoxShape>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodBoxShapeHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for SphereShape.
pub struct DatapodSphereShapeHandle {
    inner: SphereShape,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sphere_shape_new_default() -> *mut DatapodSphereShapeHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSphereShapeHandle {
        inner: <SphereShape>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sphere_shape_free(handle: *mut DatapodSphereShapeHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sphere_shape_type_hash() -> u64 {
    ffi_type_hash::<SphereShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sphere_shape_header_size() -> usize {
    ffi_header_size::<SphereShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sphere_shape_to_header_bytes(
    handle: *const DatapodSphereShapeHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null sphere_shape handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sphere_shape_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodSphereShapeHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<SphereShape>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSphereShapeHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for CylinderShape.
pub struct DatapodCylinderShapeHandle {
    inner: CylinderShape,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_cylinder_shape_new_default() -> *mut DatapodCylinderShapeHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodCylinderShapeHandle {
        inner: <CylinderShape>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_cylinder_shape_free(handle: *mut DatapodCylinderShapeHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_cylinder_shape_type_hash() -> u64 {
    ffi_type_hash::<CylinderShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_cylinder_shape_header_size() -> usize {
    ffi_header_size::<CylinderShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_cylinder_shape_to_header_bytes(
    handle: *const DatapodCylinderShapeHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null cylinder_shape handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_cylinder_shape_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodCylinderShapeHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<CylinderShape>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodCylinderShapeHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for MeshShape.
pub struct DatapodMeshShapeHandle {
    inner: MeshShape,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mesh_shape_new_default() -> *mut DatapodMeshShapeHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodMeshShapeHandle {
        inner: <MeshShape>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mesh_shape_free(handle: *mut DatapodMeshShapeHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mesh_shape_type_hash() -> u64 {
    ffi_type_hash::<MeshShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mesh_shape_header_size() -> usize {
    ffi_header_size::<MeshShape>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mesh_shape_to_header_bytes(
    handle: *const DatapodMeshShapeHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null mesh_shape handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_mesh_shape_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodMeshShapeHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<MeshShape>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodMeshShapeHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for GeometryKind.
pub struct DatapodGeometryKindHandle {
    inner: GeometryKind,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_kind_new_default() -> *mut DatapodGeometryKindHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGeometryKindHandle {
        inner: <GeometryKind>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_kind_free(handle: *mut DatapodGeometryKindHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_kind_type_hash() -> u64 {
    ffi_type_hash::<GeometryKind>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_kind_header_size() -> usize {
    ffi_header_size::<GeometryKind>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_kind_to_header_bytes(
    handle: *const DatapodGeometryKindHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null geometry_kind handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_kind_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodGeometryKindHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<GeometryKind>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGeometryKindHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Geometry.
pub struct DatapodGeometryHandle {
    inner: Geometry,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_new_default() -> *mut DatapodGeometryHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGeometryHandle {
        inner: <Geometry>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_free(handle: *mut DatapodGeometryHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_type_hash() -> u64 {
    ffi_type_hash::<Geometry>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_header_size() -> usize {
    ffi_header_size::<Geometry>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_to_header_bytes(
    handle: *const DatapodGeometryHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null geometry handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geometry_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodGeometryHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Geometry>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodGeometryHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Identity.
pub struct DatapodIdentityHandle {
    inner: Identity,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_identity_new_default() -> *mut DatapodIdentityHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodIdentityHandle {
        inner: <Identity>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_identity_free(handle: *mut DatapodIdentityHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_identity_type_hash() -> u64 {
    ffi_type_hash::<Identity>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_identity_header_size() -> usize {
    ffi_header_size::<Identity>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_identity_to_header_bytes(
    handle: *const DatapodIdentityHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null identity handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_identity_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodIdentityHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Identity>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodIdentityHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Model.
pub struct DatapodModelHandle {
    inner: Model,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_model_new_default() -> *mut DatapodModelHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodModelHandle {
        inner: <Model>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_model_free(handle: *mut DatapodModelHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_model_type_hash() -> u64 {
    ffi_type_hash::<Model>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_model_header_size() -> usize {
    ffi_header_size::<Model>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_model_to_header_bytes(
    handle: *const DatapodModelHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null model handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_model_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodModelHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Model>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodModelHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Material.
pub struct DatapodMaterialHandle {
    inner: Material,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_material_new_default() -> *mut DatapodMaterialHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodMaterialHandle {
        inner: <Material>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_material_free(handle: *mut DatapodMaterialHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_material_type_hash() -> u64 {
    ffi_type_hash::<Material>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_material_header_size() -> usize {
    ffi_header_size::<Material>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_material_to_header_bytes(
    handle: *const DatapodMaterialHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null material handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_material_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodMaterialHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Material>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodMaterialHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Visual.
pub struct DatapodVisualHandle {
    inner: Visual,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_visual_new_default() -> *mut DatapodVisualHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodVisualHandle {
        inner: <Visual>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_visual_free(handle: *mut DatapodVisualHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_visual_type_hash() -> u64 {
    ffi_type_hash::<Visual>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_visual_header_size() -> usize {
    ffi_header_size::<Visual>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_visual_to_header_bytes(
    handle: *const DatapodVisualHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null visual handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_visual_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodVisualHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Visual>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodVisualHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Collision.
pub struct DatapodCollisionHandle {
    inner: Collision,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_collision_new_default() -> *mut DatapodCollisionHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodCollisionHandle {
        inner: <Collision>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_collision_free(handle: *mut DatapodCollisionHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_collision_type_hash() -> u64 {
    ffi_type_hash::<Collision>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_collision_header_size() -> usize {
    ffi_header_size::<Collision>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_collision_to_header_bytes(
    handle: *const DatapodCollisionHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null collision handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_collision_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodCollisionHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Collision>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodCollisionHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for JointType.
pub struct DatapodJointTypeHandle {
    inner: JointType,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_type_new_default() -> *mut DatapodJointTypeHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointTypeHandle {
        inner: <JointType>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_type_free(handle: *mut DatapodJointTypeHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_type_type_hash() -> u64 {
    ffi_type_hash::<JointType>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_type_header_size() -> usize {
    ffi_header_size::<JointType>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_type_to_header_bytes(
    handle: *const DatapodJointTypeHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null joint_type handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_type_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodJointTypeHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<JointType>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointTypeHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Joint.
pub struct DatapodJointHandle {
    inner: Joint,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_new_default() -> *mut DatapodJointHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointHandle {
        inner: <Joint>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_free(handle: *mut DatapodJointHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_type_hash() -> u64 {
    ffi_type_hash::<Joint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_header_size() -> usize {
    ffi_header_size::<Joint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_to_header_bytes(
    handle: *const DatapodJointHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null joint handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_joint_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodJointHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Joint>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodJointHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Link.
pub struct DatapodLinkHandle {
    inner: Link,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_link_new_default() -> *mut DatapodLinkHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodLinkHandle {
        inner: <Link>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_link_free(handle: *mut DatapodLinkHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_link_type_hash() -> u64 {
    ffi_type_hash::<Link>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_link_header_size() -> usize {
    ffi_header_size::<Link>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_link_to_header_bytes(
    handle: *const DatapodLinkHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null link handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_link_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodLinkHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Link>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodLinkHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Sensor.
pub struct DatapodSensorHandle {
    inner: Sensor,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sensor_new_default() -> *mut DatapodSensorHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSensorHandle {
        inner: <Sensor>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sensor_free(handle: *mut DatapodSensorHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sensor_type_hash() -> u64 {
    ffi_type_hash::<Sensor>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sensor_header_size() -> usize {
    ffi_header_size::<Sensor>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sensor_to_header_bytes(
    handle: *const DatapodSensorHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null sensor handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_sensor_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodSensorHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Sensor>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodSensorHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Robot.
pub struct DatapodRobotHandle {
    inner: Robot,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_robot_new_default() -> *mut DatapodRobotHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodRobotHandle {
        inner: <Robot>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_robot_free(handle: *mut DatapodRobotHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_robot_type_hash() -> u64 {
    ffi_type_hash::<Robot>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_robot_header_size() -> usize {
    ffi_header_size::<Robot>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_robot_to_header_bytes(
    handle: *const DatapodRobotHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null robot handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_robot_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodRobotHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Robot>(bytes) {
        Ok(inner) => std::boxed::Box::into_raw(std::boxed::Box::new(DatapodRobotHandle { inner })),
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Actuator.
pub struct DatapodActuatorHandle {
    inner: Actuator,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_actuator_new_default() -> *mut DatapodActuatorHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodActuatorHandle {
        inner: <Actuator>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_actuator_free(handle: *mut DatapodActuatorHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_actuator_type_hash() -> u64 {
    ffi_type_hash::<Actuator>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_actuator_header_size() -> usize {
    ffi_header_size::<Actuator>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_actuator_to_header_bytes(
    handle: *const DatapodActuatorHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null actuator handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_actuator_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodActuatorHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Actuator>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodActuatorHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for TransmissionJoint.
pub struct DatapodTransmissionJointHandle {
    inner: TransmissionJoint,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_joint_new_default() -> *mut DatapodTransmissionJointHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodTransmissionJointHandle {
        inner: <TransmissionJoint>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_joint_free(handle: *mut DatapodTransmissionJointHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_joint_type_hash() -> u64 {
    ffi_type_hash::<TransmissionJoint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_joint_header_size() -> usize {
    ffi_header_size::<TransmissionJoint>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_joint_to_header_bytes(
    handle: *const DatapodTransmissionJointHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null transmission_joint handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_joint_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodTransmissionJointHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<TransmissionJoint>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodTransmissionJointHandle {
                inner,
            }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Transmission.
pub struct DatapodTransmissionHandle {
    inner: Transmission,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_new_default() -> *mut DatapodTransmissionHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodTransmissionHandle {
        inner: <Transmission>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_free(handle: *mut DatapodTransmissionHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_type_hash() -> u64 {
    ffi_type_hash::<Transmission>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_header_size() -> usize {
    ffi_header_size::<Transmission>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_to_header_bytes(
    handle: *const DatapodTransmissionHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null transmission handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_transmission_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodTransmissionHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Transmission>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodTransmissionHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Encoding.
pub struct DatapodEncodingHandle {
    inner: Encoding,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_encoding_new_default() -> *mut DatapodEncodingHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodEncodingHandle {
        inner: <Encoding>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_encoding_free(handle: *mut DatapodEncodingHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_encoding_type_hash() -> u64 {
    ffi_type_hash::<Encoding>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_encoding_header_size() -> usize {
    ffi_header_size::<Encoding>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_encoding_to_header_bytes(
    handle: *const DatapodEncodingHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null encoding handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_encoding_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodEncodingHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Encoding>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodEncodingHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}

/// Opaque fixed-value handle for Envelope.
pub struct DatapodEnvelopeHandle {
    inner: Envelope,
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_envelope_new_default() -> *mut DatapodEnvelopeHandle {
    clear_last_error();
    std::boxed::Box::into_raw(std::boxed::Box::new(DatapodEnvelopeHandle {
        inner: <Envelope>::default(),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_envelope_free(handle: *mut DatapodEnvelopeHandle) {
    free_boxed_handle(handle);
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_envelope_type_hash() -> u64 {
    ffi_type_hash::<Envelope>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_envelope_header_size() -> usize {
    ffi_header_size::<Envelope>()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_envelope_to_header_bytes(
    handle: *const DatapodEnvelopeHandle,
    out: *mut u8,
    out_len: usize,
) -> bool {
    clear_last_error();
    if handle.is_null() {
        set_last_error("null envelope handle");
        return false;
    }
    write_datapod_header(&unsafe { &*handle }.inner, out, out_len)
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_envelope_from_header_bytes(
    ptr: *const u8,
    len: usize,
) -> *mut DatapodEnvelopeHandle {
    clear_last_error();
    let bytes = match unsafe { bytes_in(ptr, len) } {
        Ok(bytes) => bytes,
        Err(()) => return std::ptr::null_mut(),
    };
    match crate::bind::read_fixed_header::<Envelope>(bytes) {
        Ok(inner) => {
            std::boxed::Box::into_raw(std::boxed::Box::new(DatapodEnvelopeHandle { inner }))
        }
        Err(error) => {
            set_last_error(error);
            std::ptr::null_mut()
        }
    }
}
