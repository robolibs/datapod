#![allow(deprecated, non_snake_case, clippy::wrong_self_convention)]

use std::ffi::c_char;

use pyo3::exceptions::{PyIndexError, PyMemoryError, PyValueError};
use pyo3::ffi;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyModule};

use crate::{DataPod, DataPodValidate, Geo, Point, Polygon, Segment};

fn wire_message_v1<T>(value: &T) -> PyResult<(u64, Vec<u8>)>
where
    T: DataPod + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    let message = crate::to_wire_message_v1(value)
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    Ok((message.type_hash, message.bytes))
}

fn decode_parts<T>(header: Vec<u8>, payload: Vec<u8>) -> PyResult<T>
where
    T: DataPod + crate::DataPodDecode + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    let mut bytes = header;
    bytes.try_reserve_exact(payload.len()).map_err(|error| {
        PyMemoryError::new_err(format!(
            "failed to reserve {} decoded payload bytes: {error}",
            payload.len()
        ))
    })?;
    bytes.extend_from_slice(&payload);
    decode_message::<T>(crate::bind::emitted_type_hash::<T>(), bytes)
}

fn decode_message<T>(kind: u64, data: Vec<u8>) -> PyResult<T>
where
    T: DataPod + crate::DataPodDecode + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    let message = crate::WireMessage {
        type_hash: kind,
        bytes: data,
    };
    crate::from_wire_message::<T>(&message)
        .map_err(|error| PyValueError::new_err(error.to_string()))
}

fn header_bytes<T: DataPod>(value: &T) -> PyResult<Vec<u8>> {
    let mut out = zeroed_bytes_for_python("geometry header", crate::bind::header_size::<T>())?;
    crate::bind::write_header(value, &mut out).map_err(PyValueError::new_err)?;
    Ok(out)
}

fn zeroed_bytes_for_python(label: &str, len: usize) -> PyResult<Vec<u8>> {
    let mut out = Vec::new();
    out.try_reserve_exact(len).map_err(|error| {
        PyMemoryError::new_err(format!("failed to reserve {len} {label} bytes: {error}"))
    })?;
    out.resize(len, 0);
    Ok(out)
}

fn copy_bytes_for_python(label: &str, bytes: &[u8]) -> PyResult<Vec<u8>> {
    let mut out = Vec::new();
    out.try_reserve_exact(bytes.len()).map_err(|error| {
        PyMemoryError::new_err(format!(
            "failed to reserve {} {label} bytes: {error}",
            bytes.len()
        ))
    })?;
    out.extend_from_slice(bytes);
    Ok(out)
}

fn payload_memoryview(py: Python<'_>, payload: &[u8]) -> PyResult<PyObject> {
    unsafe {
        let ptr = payload.as_ptr() as *mut c_char;
        const PYBUF_READ: std::os::raw::c_int = 0x100;
        let view = ffi::PyMemoryView_FromMemory(ptr, payload.len() as isize, PYBUF_READ);
        if view.is_null() {
            Err(PyErr::fetch(py))
        } else {
            Ok(Py::<PyAny>::from_owned_ptr(py, view))
        }
    }
}

fn frame_object<T>(py: Python<'_>, owner: PyObject, value: &T) -> PyResult<PyObject>
where
    T: DataPod + DataPodValidate,
{
    T::validate_wire_parts(&value.header(), value.payload_bytes())
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    let module = PyModule::import(py, "datapod")?;
    let frame_cls = module.getattr("ArchiveFrame")?;
    let payload = payload_memoryview(py, value.payload_bytes())?;
    let frame = frame_cls.call1((
        crate::bind::type_hash::<T>(),
        header_bytes(value)?,
        payload,
        owner,
    ))?;
    Ok(frame.unbind())
}

fn frame_header_payload<'py>(
    frame: &Bound<'py, PyAny>,
    name: &str,
    expected_hash: u64,
) -> PyResult<(Vec<u8>, Vec<u8>)> {
    let kind: u64 = frame
        .getattr("type_hash")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose type_hash")))?
        .extract()?;
    if kind != expected_hash {
        return Err(PyValueError::new_err(format!(
            "wrong {name} archive type hash: got {kind}, expected {expected_hash}"
        )));
    }
    let header: Vec<u8> = frame
        .getattr("header")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose header")))?
        .call_method0("tobytes")
        .map_err(|_| PyValueError::new_err(format!("{name} frame header must support tobytes()")))?
        .extract()
        .map_err(|_| {
            PyValueError::new_err(format!("{name} frame header tobytes() must return bytes"))
        })?;
    let payload: Vec<u8> = frame
        .getattr("payload")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose payload")))?
        .call_method0("tobytes")
        .map_err(|_| PyValueError::new_err(format!("{name} frame payload must support tobytes()")))?
        .extract()
        .map_err(|_| {
            PyValueError::new_err(format!("{name} frame payload tobytes() must return bytes"))
        })?;
    Ok((header, payload))
}

fn payload_view_object(
    frame: &Bound<'_, PyAny>,
    name: &str,
    expected_hash: u64,
) -> PyResult<PyObject> {
    let kind: u64 = frame
        .getattr("type_hash")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose type_hash")))?
        .extract()?;
    if kind != expected_hash {
        return Err(PyValueError::new_err(format!(
            "wrong {name} archive type hash: got {kind}, expected {expected_hash}"
        )));
    }
    let header = frame
        .getattr("header")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose header")))?;
    let payload = frame
        .getattr("payload")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose payload")))?;
    let module = PyModule::import(frame.py(), "datapod")?;
    module
        .getattr("validate_wire_frame_v1")?
        .call1((expected_hash, &header, &payload))
        .map_err(|error| {
            if error.is_instance_of::<PyValueError>(frame.py()) {
                error
            } else {
                PyValueError::new_err(format!("{name} frame validation failed: {error}"))
            }
        })?;
    let view_cls = module.getattr("PayloadView")?;
    Ok(view_cls.call1((header, payload, frame))?.unbind())
}

macro_rules! impl_fixed_archive {
    ($py:ty, $rust:ty, $name:literal, $to_rust:expr) => {
        #[pymethods]
        impl $py {
            fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                let rust_value: $rust = $to_rust(&slf);
                frame_object(py, owner, &rust_value)
            }

            fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }

            fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }

            fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame_v1(slf, py)
            }

            #[staticmethod]
            fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                let (header, payload) =
                    frame_header_payload(frame, $name, crate::bind::type_hash::<$rust>())?;
                Self::from_wire(header, payload)
            }

            #[staticmethod]
            fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
        }
    };
}

macro_rules! impl_heap_archive {
    ($py:ty, $rust:ty, $name:literal) => {
        #[pymethods]
        impl $py {
            fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                frame_object(py, owner, &slf.inner)
            }

            fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }

            fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }

            fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame_v1(slf, py)
            }

            #[staticmethod]
            fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                let (header, payload) =
                    frame_header_payload(frame, $name, crate::bind::type_hash::<$rust>())?;
                Self::from_wire(header, payload)
            }

            #[staticmethod]
            fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<PyObject> {
                payload_view_object(frame, $name, crate::bind::type_hash::<$rust>())
            }

            #[staticmethod]
            fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyObject> {
                Self::view_from_wire_frame(frame)
            }

            #[staticmethod]
            fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<PyObject> {
                Self::view_from_wire_frame(frame)
            }

            #[staticmethod]
            fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyObject> {
                Self::view_from_wire_frame(frame)
            }
        }
    };
}

#[pyclass(name = "Point")]
#[derive(Clone, Copy)]
pub struct PyPoint {
    #[pyo3(get, set)]
    pub x: f64,
    #[pyo3(get, set)]
    pub y: f64,
    #[pyo3(get, set)]
    pub z: f64,
}

impl From<Point> for PyPoint {
    fn from(value: Point) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
        }
    }
}

impl From<PyPoint> for Point {
    fn from(value: PyPoint) -> Self {
        Self::new(value.x, value.y, value.z)
    }
}

#[pymethods]
impl PyPoint {
    #[new]
    #[pyo3(signature = (x=0.0, y=0.0, z=0.0))]
    fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    fn magnitude(&self) -> f64 {
        Point::from(*self).magnitude()
    }

    fn distance_to(&self, other: PyRef<'_, PyPoint>) -> f64 {
        Point::from(*self).distance_to(Point::from(*other))
    }

    fn distance_to_2d(&self, other: PyRef<'_, PyPoint>) -> f64 {
        Point::from(*self).distance_to_2d(Point::from(*other))
    }

    fn is_set(&self) -> bool {
        Point::from(*self).is_set()
    }

    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Point>()
    }

    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        let mut out = zeroed_bytes_for_python("Point header", crate::bind::header_size::<Point>())?;
        crate::bind::write_header(&Point::from(*self), &mut out).map_err(PyValueError::new_err)?;
        Ok(out)
    }

    fn payload_bytes(&self) -> Vec<u8> {
        Vec::new()
    }

    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        decode_parts::<Point>(header, payload).map(PyPoint::from)
    }

    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&Point::from(*self))
    }

    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&Point::from(*self))
    }

    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        frame_object(py, owner, &Point::from(*slf))
    }

    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }

    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        decode_message::<Point>(kind, data).map(PyPoint::from)
    }

    #[staticmethod]
    fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        let (header, payload) =
            frame_header_payload(frame, "Point", crate::bind::type_hash::<Point>())?;
        Self::from_wire(header, payload)
    }

    #[staticmethod]
    fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    fn __repr__(&self) -> String {
        format!("Point(x={}, y={}, z={})", self.x, self.y, self.z)
    }
}

#[pyclass(name = "Geo")]
#[derive(Clone, Copy)]
pub struct PyGeo {
    #[pyo3(get, set)]
    pub latitude: f64,
    #[pyo3(get, set)]
    pub longitude: f64,
    #[pyo3(get, set)]
    pub altitude: f64,
}

impl From<Geo> for PyGeo {
    fn from(value: Geo) -> Self {
        Self {
            latitude: value.latitude,
            longitude: value.longitude,
            altitude: value.altitude,
        }
    }
}

impl From<PyGeo> for Geo {
    fn from(value: PyGeo) -> Self {
        Self::new(value.latitude, value.longitude, value.altitude)
    }
}

#[pymethods]
impl PyGeo {
    #[new]
    #[pyo3(signature = (latitude=0.0, longitude=0.0, altitude=0.0))]
    fn new(latitude: f64, longitude: f64, altitude: f64) -> Self {
        Self {
            latitude,
            longitude,
            altitude,
        }
    }

    fn is_valid(&self) -> bool {
        Geo::from(*self).is_valid()
    }

    fn is_set(&self) -> bool {
        Geo::from(*self).is_set()
    }

    fn has_altitude(&self) -> bool {
        Geo::from(*self).has_altitude()
    }

    fn distance_to(&self, other: PyRef<'_, PyGeo>) -> f64 {
        Geo::from(*self).distance_to(Geo::from(*other))
    }

    fn bearing_to(&self, other: PyRef<'_, PyGeo>) -> f64 {
        Geo::from(*self).bearing_to(Geo::from(*other))
    }

    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Geo>()
    }

    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        let mut out = zeroed_bytes_for_python("Geo header", crate::bind::header_size::<Geo>())?;
        crate::bind::write_header(&Geo::from(*self), &mut out).map_err(PyValueError::new_err)?;
        Ok(out)
    }

    fn payload_bytes(&self) -> Vec<u8> {
        Vec::new()
    }

    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        decode_parts::<Geo>(header, payload).map(PyGeo::from)
    }

    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&Geo::from(*self))
    }

    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&Geo::from(*self))
    }

    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        decode_message::<Geo>(kind, data).map(PyGeo::from)
    }

    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        frame_object(py, owner, &Geo::from(*slf))
    }

    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }

    #[staticmethod]
    fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        let (header, payload) =
            frame_header_payload(frame, "Geo", crate::bind::type_hash::<Geo>())?;
        Self::from_wire(header, payload)
    }

    #[staticmethod]
    fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    fn __repr__(&self) -> String {
        format!(
            "Geo(latitude={}, longitude={}, altitude={})",
            self.latitude, self.longitude, self.altitude
        )
    }
}

#[pyclass(name = "Segment")]
#[derive(Clone, Copy)]
pub struct PySegment {
    inner: Segment,
}

#[pymethods]
impl PySegment {
    #[new]
    fn new(start: PyRef<'_, PyPoint>, end: PyRef<'_, PyPoint>) -> Self {
        Self {
            inner: Segment::new(Point::from(*start), Point::from(*end)),
        }
    }

    #[getter]
    fn start(&self) -> PyPoint {
        self.inner.start.into()
    }

    #[getter]
    fn end(&self) -> PyPoint {
        self.inner.end.into()
    }

    fn length(&self) -> f64 {
        self.inner.length()
    }

    fn midpoint(&self) -> PyPoint {
        self.inner.midpoint().into()
    }

    fn closest_point(&self, point: PyRef<'_, PyPoint>) -> PyPoint {
        self.inner.closest_point(Point::from(*point)).into()
    }

    fn distance_to(&self, point: PyRef<'_, PyPoint>) -> f64 {
        self.inner.distance_to(Point::from(*point))
    }

    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Segment>()
    }

    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        let mut out =
            zeroed_bytes_for_python("Segment header", crate::bind::header_size::<Segment>())?;
        crate::bind::write_header(&self.inner, &mut out).map_err(PyValueError::new_err)?;
        Ok(out)
    }

    fn payload_bytes(&self) -> Vec<u8> {
        Vec::new()
    }

    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        decode_parts::<Segment>(header, payload).map(|inner| Self { inner })
    }

    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }

    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }

    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        decode_message::<Segment>(kind, data).map(|inner| Self { inner })
    }

    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        frame_object(py, owner, &slf.inner)
    }

    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }

    #[staticmethod]
    fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        let (header, payload) =
            frame_header_payload(frame, "Segment", crate::bind::type_hash::<Segment>())?;
        Self::from_wire(header, payload)
    }

    #[staticmethod]
    fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    #[staticmethod]
    fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }

    fn __repr__(&self) -> String {
        format!(
            "Segment(start=Point(x={}, y={}, z={}), end=Point(x={}, y={}, z={}))",
            self.inner.start.x,
            self.inner.start.y,
            self.inner.start.z,
            self.inner.end.x,
            self.inner.end.y,
            self.inner.end.z
        )
    }
}

#[pyclass(name = "Polygon")]
pub struct PyPolygon {
    inner: Polygon,
}

fn point_list(vertices: Vec<(f64, f64, f64)>) -> PyResult<Vec<Point>> {
    let mut points = Vec::new();
    points.try_reserve_exact(vertices.len()).map_err(|err| {
        PyMemoryError::new_err(format!(
            "failed to reserve {} polygon vertices: {err}",
            vertices.len()
        ))
    })?;
    points.extend(vertices.into_iter().map(|(x, y, z)| Point::new(x, y, z)));
    Ok(points)
}

#[pymethods]
impl PyPolygon {
    #[new]
    fn new(vertices: Vec<(f64, f64, f64)>) -> PyResult<Self> {
        Ok(Self {
            inner: Polygon::new(point_list(vertices)?),
        })
    }

    #[staticmethod]
    fn from_xy(vertices: Vec<(f64, f64)>) -> PyResult<Self> {
        let mut points = Vec::new();
        points.try_reserve_exact(vertices.len()).map_err(|err| {
            PyMemoryError::new_err(format!(
                "failed to reserve {} polygon vertices: {err}",
                vertices.len()
            ))
        })?;
        points.extend(vertices.into_iter().map(|(x, y)| Point::new(x, y, 0.0)));
        Ok(Self {
            inner: Polygon::new(points),
        })
    }

    fn len(&self) -> usize {
        self.inner.num_vertices()
    }

    fn is_empty(&self) -> bool {
        self.inner.empty()
    }

    fn is_valid(&self) -> bool {
        self.inner.is_valid()
    }

    fn area(&self) -> f64 {
        self.inner.area()
    }

    fn perimeter(&self) -> f64 {
        self.inner.perimeter()
    }

    fn contains(&self, point: PyRef<'_, PyPoint>) -> bool {
        self.inner.contains(Point::from(*point))
    }

    fn vertex(&self, index: usize) -> PyResult<PyPoint> {
        self.inner
            .vertices
            .get(index)
            .copied()
            .map(PyPoint::from)
            .ok_or_else(|| PyIndexError::new_err("polygon vertex index out of range"))
    }

    fn vertices(&self) -> PyResult<Vec<PyPoint>> {
        let mut points = Vec::new();
        points
            .try_reserve_exact(self.inner.vertices.len())
            .map_err(|err| {
                PyMemoryError::new_err(format!(
                    "failed to reserve {} polygon vertex objects: {err}",
                    self.inner.vertices.len()
                ))
            })?;
        points.extend(self.inner.vertices.iter().copied().map(PyPoint::from));
        Ok(points)
    }

    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Polygon>()
    }

    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        let mut out =
            zeroed_bytes_for_python("Polygon header", crate::bind::header_size::<Polygon>())?;
        crate::bind::write_header(&self.inner, &mut out).map_err(PyValueError::new_err)?;
        Ok(out)
    }

    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("Polygon payload", self.inner.payload_bytes())
    }

    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        decode_parts::<Polygon>(header, payload).map(|inner| Self { inner })
    }

    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }

    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }

    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        frame_object(py, owner, &slf.inner)
    }

    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }

    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }

    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        decode_message::<Polygon>(kind, data).map(|inner| Self { inner })
    }

    fn __len__(&self) -> usize {
        self.inner.num_vertices()
    }
}

pub(super) fn register(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_class::<PyPoint>()?;
    m.add_class::<PyGeo>()?;
    m.add_class::<PySegment>()?;
    m.add_class::<PyPolygon>()?;
    Ok(())
}
