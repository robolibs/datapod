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
use std::ffi::{CString, c_char};
use std::ptr;

use crate::{Geo, Point, Polygon, Segment};

thread_local! {
    static LAST_ERROR: RefCell<Option<CString>> = const { RefCell::new(None) };
}

fn clear_last_error() {
    LAST_ERROR.with(|slot| *slot.borrow_mut() = None);
}

fn set_last_error(message: impl Into<String>) {
    let message = message.into().replace('\0', " ");
    LAST_ERROR.with(|slot| {
        *slot.borrow_mut() = Some(
            CString::new(message).unwrap_or_else(|_| CString::new("datapod ffi error").unwrap()),
        );
    });
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
#[derive(Debug, Clone, Copy)]
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

/// Opaque polygon handle.
pub struct DatapodPolygon {
    inner: Polygon,
}

unsafe fn points_in<'a>(ptr: *const DatapodPoint, len: usize) -> Result<&'a [DatapodPoint], ()> {
    if ptr.is_null() {
        if len == 0 {
            Ok(&[])
        } else {
            set_last_error("null point array with non-zero length");
            Err(())
        }
    } else {
        // SAFETY: caller promises `len` valid DatapodPoint values at `ptr`.
        Ok(unsafe { std::slice::from_raw_parts(ptr, len) })
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_new(x: f64, y: f64, z: f64) -> DatapodPoint {
    DatapodPoint { x, y, z }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_magnitude(point: DatapodPoint) -> f64 {
    Point::from(point).magnitude()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_distance_to(a: DatapodPoint, b: DatapodPoint) -> f64 {
    Point::from(a).distance_to(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_point_distance_to_2d(a: DatapodPoint, b: DatapodPoint) -> f64 {
    Point::from(a).distance_to_2d(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_new(latitude: f64, longitude: f64, altitude: f64) -> DatapodGeo {
    DatapodGeo {
        latitude,
        longitude,
        altitude,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_is_valid(geo: DatapodGeo) -> bool {
    Geo::from(geo).is_valid()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_distance_to(a: DatapodGeo, b: DatapodGeo) -> f64 {
    Geo::from(a).distance_to(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_geo_bearing_to(a: DatapodGeo, b: DatapodGeo) -> f64 {
    Geo::from(a).bearing_to(b.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_new(start: DatapodPoint, end: DatapodPoint) -> DatapodSegment {
    DatapodSegment { start, end }
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_length(segment: DatapodSegment) -> f64 {
    Segment::from(segment).length()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_midpoint(segment: DatapodSegment) -> DatapodPoint {
    Segment::from(segment).midpoint().into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_closest_point(
    segment: DatapodSegment,
    point: DatapodPoint,
) -> DatapodPoint {
    Segment::from(segment).closest_point(point.into()).into()
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_segment_distance_to(segment: DatapodSegment, point: DatapodPoint) -> f64 {
    Segment::from(segment).distance_to(point.into())
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_new(
    vertices: *const DatapodPoint,
    len: usize,
) -> *mut DatapodPolygon {
    clear_last_error();
    // SAFETY: pointer/length are validated before conversion.
    let vertices = match unsafe { points_in(vertices, len) } {
        Ok(vertices) => vertices.iter().copied().map(Point::from).collect(),
        Err(()) => return ptr::null_mut(),
    };
    Box::into_raw(Box::new(DatapodPolygon {
        inner: Polygon::new(vertices),
    }))
}

#[unsafe(no_mangle)]
pub extern "C" fn datapod_polygon_free(polygon: *mut DatapodPolygon) {
    if polygon.is_null() {
        return;
    }
    // SAFETY: originated from Box::into_raw in datapod_polygon_new.
    unsafe { drop(Box::from_raw(polygon)) };
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
    if polygon.is_null() {
        set_last_error("null polygon handle");
        return false;
    }
    if out.is_null() {
        set_last_error("null output point");
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
    DatapodBytes {
        ptr: polygon.inner.vertices.as_ptr().cast(),
        len: polygon.inner.vertices.len() * std::mem::size_of::<Point>(),
    }
}
