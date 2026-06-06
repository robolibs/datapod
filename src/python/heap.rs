#![allow(deprecated, non_snake_case, clippy::wrong_self_convention)]

use std::ffi::c_char;

use pyo3::exceptions::{PyMemoryError, PyValueError};
use pyo3::ffi;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyModule};

use crate::wire::Encoding;
use crate::{
    BitVec, Bytes, DataPod, DataPodDecode, DataPodValidate, Deque, DpStr, DpString, ForwardList,
    Grid, GridHeader, Heap, IndexedHeap, Layer, Linestring, List, Map, Matrix, MatrixHeader,
    MultiPoint, PagedVecvec, Path, Point, Polygon, Pose, Queue, Ring, Set, Stack, State, Tensor,
    Trajectory, Vector, Vecvec,
};

fn header_bytes<T: DataPod>(value: &T) -> PyResult<Vec<u8>> {
    let mut out = zeroed_bytes_for_python("heap header", crate::bind::header_size::<T>())?;
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

fn wire_message_v1<T>(value: &T) -> PyResult<(u64, Vec<u8>)>
where
    T: DataPod + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    T::validate_wire_parts(&value.header(), value.payload_bytes())
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    let message = crate::to_wire_message_v1(value)
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    Ok((message.type_hash, message.bytes))
}

fn validate_owned<T>(inner: T) -> PyResult<T>
where
    T: DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    T::validate_wire_parts(&inner.header(), inner.payload_bytes())
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    Ok(inner)
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

fn wire_frame_object<T>(py: Python<'_>, owner: PyObject, value: &T) -> PyResult<PyObject>
where
    T: DataPod + DataPodValidate,
{
    T::validate_wire_parts(&value.header(), value.payload_bytes())
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    let module = PyModule::import(py, "datapod")?;
    let frame_cls = module.getattr("WireFrame")?;
    let header = header_bytes(value)?;
    let payload = payload_memoryview(py, value.payload_bytes())?;
    let frame = frame_cls.call1((crate::bind::type_hash::<T>(), header, payload, owner))?;
    Ok(frame.unbind())
}

#[pyclass(name = "PayloadView")]
pub struct PyPayloadView {
    #[pyo3(get)]
    header: PyObject,
    #[pyo3(get)]
    payload: PyObject,
    _owner: Option<PyObject>,
}

#[pymethods]
impl PyPayloadView {
    #[new]
    #[pyo3(signature = (header, payload, owner=None))]
    fn new(header: PyObject, payload: PyObject, owner: Option<PyObject>) -> Self {
        Self {
            header,
            payload,
            _owner: owner,
        }
    }

    fn payload_bytes(&self, py: Python<'_>) -> PyResult<Vec<u8>> {
        self.payload.bind(py).call_method0("tobytes")?.extract()
    }
}

fn frame_header_and_payload<'py>(
    frame: &Bound<'py, PyAny>,
    name: &str,
    expected_hash: u64,
) -> PyResult<(Vec<u8>, Bound<'py, PyAny>)> {
    let kind: u64 = frame
        .getattr("type_hash")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose type_hash")))?
        .extract()?;
    if kind != expected_hash {
        return Err(PyValueError::new_err(format!(
            "wrong {name} frame type hash: got {kind}, expected {expected_hash}"
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
    let payload = frame
        .getattr("payload")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose payload")))?;
    Ok((header, payload))
}

fn frame_payload_to_bytes(payload: &Bound<'_, PyAny>, name: &str) -> PyResult<Vec<u8>> {
    payload
        .call_method0("tobytes")
        .map_err(|_| PyValueError::new_err(format!("{name} frame payload must support tobytes()")))?
        .extract()
        .map_err(|_| {
            PyValueError::new_err(format!("{name} frame payload tobytes() must return bytes"))
        })
}

fn payload_view_from_frame(
    frame: &Bound<'_, PyAny>,
    name: &str,
    expected_hash: u64,
) -> PyResult<PyPayloadView> {
    let kind: u64 = frame
        .getattr("type_hash")
        .map_err(|_| PyValueError::new_err(format!("{name} frame must expose type_hash")))?
        .extract()?;
    if kind != expected_hash {
        return Err(PyValueError::new_err(format!(
            "wrong {name} frame type hash: got {kind}, expected {expected_hash}"
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
    Ok(PyPayloadView {
        header: header.unbind(),
        payload: payload.unbind(),
        _owner: Some(frame.clone().unbind()),
    })
}

macro_rules! impl_payload_view_only {
    ($py:ty, $rust:ty, $name:literal) => {
        #[pymethods]
        impl $py {
            #[staticmethod]
            fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                payload_view_from_frame(frame, $name, crate::bind::type_hash::<$rust>())
            }
        }
    };
}

macro_rules! impl_decode_and_view {
    ($py:ty, $rust:ty, $name:literal) => {
        #[pymethods]
        impl $py {
            #[staticmethod]
            fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                payload_view_from_frame(frame, $name, crate::bind::type_hash::<$rust>())
            }

            #[staticmethod]
            fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                let (header, payload) =
                    frame_header_and_payload(frame, $name, crate::bind::type_hash::<$rust>())?;
                let payload = frame_payload_to_bytes(&payload, $name)?;
                Self::from_wire(header, payload)
            }

            #[staticmethod]
            fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
        }
    };
}

macro_rules! impl_full_archive {
    ($py:ty, $rust:ty, $name:literal) => {
        #[pymethods]
        impl $py {
            fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }

            fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }

            fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }

            fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame_v1(slf, py)
            }

            #[staticmethod]
            fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                Self::view_from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
                payload_view_from_frame(frame, $name, crate::bind::type_hash::<$rust>())
            }

            #[staticmethod]
            fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }

            #[staticmethod]
            fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                let (header, payload) =
                    frame_header_and_payload(frame, $name, crate::bind::type_hash::<$rust>())?;
                let payload = frame_payload_to_bytes(&payload, $name)?;
                Self::from_wire(header, payload)
            }

            #[staticmethod]
            fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
        }
    };
}

fn split_wire<T: DataPod>(data: Vec<u8>) -> PyResult<(Vec<u8>, Vec<u8>)> {
    let header_size = crate::bind::header_size::<T>();
    if data.len() < header_size {
        return Err(PyValueError::new_err(format!(
            "wire message too short: got {}, need at least {header_size}",
            data.len()
        )));
    }
    let header = data
        .get(..header_size)
        .ok_or_else(|| PyValueError::new_err("wire header range is out of bounds"))?;
    let payload = data
        .get(header_size..)
        .ok_or_else(|| PyValueError::new_err("wire payload range is out of bounds"))?;
    Ok((
        copy_bytes_for_python("wire header", header)?,
        copy_bytes_for_python("wire payload", payload)?,
    ))
}

fn read_header<H: bytemuck::Pod>(name: &str, header: &[u8]) -> PyResult<H> {
    bytemuck::try_pod_read_unaligned(header)
        .map_err(|_| PyValueError::new_err(format!("invalid {name} header bytes")))
}

fn decode_parts<T>(header: Vec<u8>, payload: Vec<u8>) -> PyResult<T>
where
    T: DataPod + DataPodDecode + DataPodValidate,
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
    let message = crate::WireMessage {
        type_hash: crate::bind::emitted_type_hash::<T>(),
        bytes,
    };
    crate::from_wire_message::<T>(&message)
        .map_err(|error| PyValueError::new_err(error.to_string()))
}

fn decode_message<T>(kind: u64, data: Vec<u8>) -> PyResult<T>
where
    T: DataPod + DataPodDecode + DataPodValidate,
    T::Header: crate::LeWireHeader,
{
    let message = crate::WireMessage {
        type_hash: kind,
        bytes: data,
    };
    crate::from_wire_message::<T>(&message)
        .map_err(|error| PyValueError::new_err(error.to_string()))
}

fn encoding(value: u32) -> PyResult<Encoding> {
    match value {
        0 => Ok(Encoding::U8),
        1 => Ok(Encoding::U16),
        2 => Ok(Encoding::U32),
        3 => Ok(Encoding::U64),
        4 => Ok(Encoding::I8),
        5 => Ok(Encoding::I16),
        6 => Ok(Encoding::I32),
        7 => Ok(Encoding::I64),
        8 => Ok(Encoding::F32),
        9 => Ok(Encoding::F64),
        10 => Ok(Encoding::Rgb8),
        11 => Ok(Encoding::Rgba8),
        12 => Ok(Encoding::Mono16),
        13 => Ok(Encoding::Mono8),
        _ => Err(PyValueError::new_err("unknown datapod encoding")),
    }
}

fn point_list(vertices: Vec<(f64, f64, f64)>) -> PyResult<Vec<Point>> {
    let mut points = Vec::new();
    points.try_reserve_exact(vertices.len()).map_err(|err| {
        PyMemoryError::new_err(format!(
            "failed to reserve {} point payload vertices: {err}",
            vertices.len()
        ))
    })?;
    points.extend(vertices.into_iter().map(|(x, y, z)| Point::new(x, y, z)));
    Ok(points)
}

fn pose_from_vec(v: &[f64]) -> PyResult<Pose> {
    if v.len() != 7 {
        return Err(PyValueError::new_err("pose rows must contain 7 floats"));
    }
    Ok(Pose::from_mat([v[0], v[1], v[2], v[3], v[4], v[5], v[6]]))
}

fn state_from_vec(v: &[f64]) -> PyResult<State> {
    if v.len() != 13 {
        return Err(PyValueError::new_err("state rows must contain 13 floats"));
    }
    Ok(State::from_mat([
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12],
    ]))
}

fn pose_list(rows: &[Vec<f64>]) -> PyResult<Vec<Pose>> {
    let mut poses = Vec::new();
    poses.try_reserve_exact(rows.len()).map_err(|err| {
        PyMemoryError::new_err(format!(
            "failed to reserve {} path poses: {err}",
            rows.len()
        ))
    })?;
    for row in rows {
        poses.push(pose_from_vec(row)?);
    }
    Ok(poses)
}

fn state_list(rows: &[Vec<f64>]) -> PyResult<Vec<State>> {
    let mut states = Vec::new();
    states.try_reserve_exact(rows.len()).map_err(|err| {
        PyMemoryError::new_err(format!(
            "failed to reserve {} trajectory states: {err}",
            rows.len()
        ))
    })?;
    for row in rows {
        states.push(state_from_vec(row)?);
    }
    Ok(states)
}

macro_rules! byte_container {
    ($py:ident, $name:literal, $rust:ty, |$data:ident| $inner:expr) => {
        #[pyclass(name = $name)]
        pub struct $py {
            inner: $rust,
        }
        #[pymethods]
        impl $py {
            #[new]
            fn new($data: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: validate_owned::<$rust>($inner)?,
                })
            }
            #[classattr]
            fn TYPE_HASH() -> u64 {
                crate::bind::type_hash::<$rust>()
            }
            fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
                header_bytes(&self.inner)
            }
            fn payload_bytes(&self) -> PyResult<Vec<u8>> {
                copy_bytes_for_python($name, self.inner.payload_bytes())
            }
            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                wire_message_v1(&self.inner)
            }
            fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
                wire_message_v1(&self.inner)
            }
            fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }
            fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }
            fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }
            fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame_v1(slf, py)
            }
            fn payload_len(&self) -> usize {
                self.inner.payload_bytes().len()
            }
            #[staticmethod]
            fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: decode_parts::<$rust>(header, payload)?,
                })
            }
            #[staticmethod]
            fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: decode_message::<$rust>(kind, data)?,
                })
            }
            #[staticmethod]
            fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
            #[staticmethod]
            fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
            #[staticmethod]
            fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                let (header, payload) =
                    frame_header_and_payload(frame, $name, crate::bind::type_hash::<$rust>())?;
                let payload = frame_payload_to_bytes(&payload, $name)?;
                Self::from_wire(header, payload)
            }
            #[staticmethod]
            fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
        }
    };
}

macro_rules! raw_element_container {
    ($py:ident, $name:literal, $rust:ty, |$element_size:ident, $data:ident| $inner:expr) => {
        #[pyclass(name = $name)]
        pub struct $py {
            inner: $rust,
        }
        #[pymethods]
        impl $py {
            #[new]
            fn new($element_size: u32, $data: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: validate_owned::<$rust>($inner)?,
                })
            }
            #[classattr]
            fn TYPE_HASH() -> u64 {
                crate::bind::type_hash::<$rust>()
            }
            fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
                header_bytes(&self.inner)
            }
            fn payload_bytes(&self) -> PyResult<Vec<u8>> {
                copy_bytes_for_python($name, self.inner.payload_bytes())
            }
            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                wire_message_v1(&self.inner)
            }
            fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
                wire_message_v1(&self.inner)
            }
            fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }
            fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }
            fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }
            fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame_v1(slf, py)
            }
            fn payload_len(&self) -> usize {
                self.inner.payload_bytes().len()
            }
            #[staticmethod]
            fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: decode_parts::<$rust>(header, payload)?,
                })
            }
            #[staticmethod]
            fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: decode_message::<$rust>(kind, data)?,
                })
            }
            #[staticmethod]
            fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
            #[staticmethod]
            fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
            #[staticmethod]
            fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                let (header, payload) =
                    frame_header_and_payload(frame, $name, crate::bind::type_hash::<$rust>())?;
                let payload = frame_payload_to_bytes(&payload, $name)?;
                Self::from_wire(header, payload)
            }
            #[staticmethod]
            fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
                Self::from_wire_frame_v1(frame)
            }
        }
    };
}

byte_container!(PyBytesPod, "Bytes", Bytes, |data| Bytes { data });
byte_container!(PyDpString, "DpString", DpString, |data| DpString::new(data));

#[pyclass(name = "DpStr")]
pub struct PyDpStr {
    inner: DpStr,
}
#[pymethods]
impl PyDpStr {
    #[new]
    fn new(text: &str) -> Self {
        Self {
            inner: DpStr::from_str(text),
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<DpStr>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<DpStr>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<DpStr>(kind, data)?,
        })
    }
    #[staticmethod]
    fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyPayloadView> {
        payload_view_from_frame(frame, "DpStr", crate::bind::type_hash::<DpStr>())
    }
    #[staticmethod]
    fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        let (header, payload) =
            frame_header_and_payload(frame, "DpStr", crate::bind::type_hash::<DpStr>())?;
        let payload = frame_payload_to_bytes(&payload, "DpStr")?;
        Self::from_wire(header, payload)
    }
    #[staticmethod]
    fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
}

macro_rules! point_payload {
    ($py:ident, $name:literal, $rust:ty, $ctor:path, $len_method:ident) => {
        #[pyclass(name = $name)]
        pub struct $py {
            inner: $rust,
        }
        #[pymethods]
        impl $py {
            #[new]
            fn new(vertices: Vec<(f64, f64, f64)>) -> PyResult<Self> {
                Ok(Self {
                    inner: $ctor(point_list(vertices)?),
                })
            }
            fn len(&self) -> usize {
                self.inner.$len_method()
            }
            #[classattr]
            fn TYPE_HASH() -> u64 {
                crate::bind::type_hash::<$rust>()
            }
            fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
                header_bytes(&self.inner)
            }
            fn payload_bytes(&self) -> PyResult<Vec<u8>> {
                copy_bytes_for_python($name, self.inner.payload_bytes())
            }
            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                wire_message_v1(&self.inner)
            }
            fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
                wire_message_v1(&self.inner)
            }
            fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }
            fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame(slf, py)
            }
            fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                let owner = (&slf).into_py(py);
                wire_frame_object(py, owner, &slf.inner)
            }
            fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
                Self::to_wire_frame_v1(slf, py)
            }
            fn payload_len(&self) -> usize {
                self.inner.payload_bytes().len()
            }
            #[staticmethod]
            fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: decode_parts::<$rust>(header, payload)?,
                })
            }
            #[staticmethod]
            fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
                Ok(Self {
                    inner: decode_message::<$rust>(kind, data)?,
                })
            }
        }
    };
}

point_payload!(
    PyLinestring,
    "Linestring",
    Linestring,
    Linestring::new,
    num_points
);
point_payload!(
    PyMultiPoint,
    "MultiPoint",
    MultiPoint,
    MultiPoint::new,
    num_points
);
point_payload!(PyRing, "Ring", Ring, Ring::new, num_points);
point_payload!(
    PyPolygonWire,
    "PolygonWire",
    Polygon,
    Polygon::new,
    num_vertices
);

#[pyclass(name = "Path")]
pub struct PyPath {
    inner: Path,
}
#[pymethods]
impl PyPath {
    #[new]
    fn new(waypoints: Vec<Vec<f64>>) -> PyResult<Self> {
        Ok(Self {
            inner: Path::new(pose_list(&waypoints)?),
        })
    }
    fn len(&self) -> usize {
        self.inner.size()
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Path>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Path>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Path>(kind, data)?,
        })
    }
}

#[pyclass(name = "Trajectory")]
pub struct PyTrajectory {
    inner: Trajectory,
}
#[pymethods]
impl PyTrajectory {
    #[new]
    fn new(states: Vec<Vec<f64>>) -> PyResult<Self> {
        Ok(Self {
            inner: Trajectory::new(state_list(&states)?),
        })
    }
    fn len(&self) -> usize {
        self.inner.size()
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Trajectory>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Trajectory>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Trajectory>(kind, data)?,
        })
    }
}

#[pyclass(name = "GridView")]
pub struct PyGridView {
    rows: u32,
    cols: u32,
    encoding_id: u32,
    centered: bool,
    resolution: f64,
    payload: PyObject,
    _owner: Option<PyObject>,
}

#[pymethods]
impl PyGridView {
    #[getter]
    fn rows(&self) -> u32 {
        self.rows
    }
    #[getter]
    fn cols(&self) -> u32 {
        self.cols
    }
    #[getter]
    fn encoding_id(&self) -> u32 {
        self.encoding_id
    }
    #[getter]
    fn centered(&self) -> bool {
        self.centered
    }
    #[getter]
    fn resolution(&self) -> f64 {
        self.resolution
    }
    #[getter]
    fn payload(&self, py: Python<'_>) -> PyObject {
        self.payload.clone_ref(py)
    }
}

#[pyclass(name = "Grid")]
pub struct PyGrid {
    inner: Grid,
}
#[pymethods]
impl PyGrid {
    #[new]
    fn new(
        rows: u32,
        cols: u32,
        encoding_id: u32,
        centered: bool,
        resolution: f64,
        pose: Vec<f64>,
        data: Vec<u8>,
    ) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(Grid::new(
                rows,
                cols,
                encoding(encoding_id)?,
                resolution,
                centered,
                pose_from_vec(&pose)?,
                data,
            ))?,
        })
    }
    fn len(&self) -> usize {
        self.inner.size()
    }
    #[getter]
    fn rows(&self) -> u32 {
        self.inner.rows
    }
    #[getter]
    fn cols(&self) -> u32 {
        self.inner.cols
    }
    #[getter]
    fn encoding_id(&self) -> u32 {
        self.inner.encoding.0
    }
    #[getter]
    fn centered(&self) -> bool {
        self.inner.centered != 0
    }
    #[getter]
    fn resolution(&self) -> f64 {
        self.inner.resolution
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Grid>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Grid>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        let expected = crate::bind::type_hash::<Grid>();
        if !crate::bind::is_type_hash_for::<Grid>(kind) {
            return Err(PyValueError::new_err(format!(
                "wrong type hash: got {kind}, expected {expected}"
            )));
        }
        let (header, payload) = split_wire::<Grid>(data)?;
        Self::from_wire(header, payload)
    }
    #[staticmethod]
    fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        let (header, payload) =
            frame_header_and_payload(frame, "Grid", crate::bind::type_hash::<Grid>())?;
        let payload = frame_payload_to_bytes(&payload, "Grid")?;
        Self::from_wire(header, payload)
    }
    #[staticmethod]
    fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<PyGridView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<PyGridView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyGridView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyGridView> {
        let (header, payload) =
            frame_header_and_payload(frame, "Grid", crate::bind::type_hash::<Grid>())?;
        let h: GridHeader = read_header("Grid", &header)?;
        let payload_len = payload.len()?;
        Grid::validate_wire_len(&h, payload_len)
            .map_err(|error| PyValueError::new_err(error.to_string()))?;
        Ok(PyGridView {
            rows: h.rows,
            cols: h.cols,
            encoding_id: h.encoding.0,
            centered: h.centered != 0,
            resolution: h.resolution,
            payload: payload.unbind(),
            _owner: Some(frame.clone().unbind()),
        })
    }
}

#[pyclass(name = "Layer")]
pub struct PyLayer {
    inner: Layer,
}
#[pymethods]
impl PyLayer {
    #[new]
    fn new(
        rows: u32,
        cols: u32,
        layers: u32,
        encoding_id: u32,
        centered: bool,
        resolution: f64,
        layer_height: f64,
        pose: Vec<f64>,
        data: Vec<u8>,
    ) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(Layer {
                rows,
                cols,
                layers,
                encoding: encoding(encoding_id)?,
                centered: if centered { 1 } else { 0 },
                _pad: 0,
                resolution,
                layer_height,
                pose: pose_from_vec(&pose)?,
                data,
            })?,
        })
    }
    fn len(&self) -> usize {
        self.inner.size()
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Layer>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Layer>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Layer>(kind, data)?,
        })
    }
}

#[pyclass(name = "Map")]
pub struct PyMap {
    inner: Map,
}
#[pymethods]
impl PyMap {
    #[new]
    fn new() -> Self {
        Self { inner: Map::new() }
    }
    fn insert(&mut self, key: Vec<u8>, value: Vec<u8>) -> PyResult<Option<Vec<u8>>> {
        self.inner
            .try_insert(&key, &value)
            .map_err(|error| PyValueError::new_err(error.to_string()))
    }
    fn len(&self) -> usize {
        self.inner.size()
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Map>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Map>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Map>(kind, data)?,
        })
    }
}

#[pyclass(name = "Set")]
pub struct PySet {
    inner: Set,
}
#[pymethods]
impl PySet {
    #[new]
    fn new() -> Self {
        Self { inner: Set::new() }
    }
    fn insert(&mut self, key: Vec<u8>) -> PyResult<bool> {
        self.inner
            .try_insert(&key)
            .map_err(|error| PyValueError::new_err(error.to_string()))
    }
    fn len(&self) -> usize {
        self.inner.size()
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Set>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Set>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Set>(kind, data)?,
        })
    }
}

raw_element_container!(PyVector, "Vector", Vector, |element_size, data| Vector {
    element_size,
    _pad: 0,
    data
});
raw_element_container!(PyStack, "Stack", Stack, |element_size, data| Stack {
    element_size,
    _pad: 0,
    data
});
raw_element_container!(PyVecvec, "Vecvec", Vecvec, |element_size, data| Vecvec {
    element_size,
    _pad: 0,
    data
});
raw_element_container!(
    PyPagedVecvec,
    "PagedVecvec",
    PagedVecvec,
    |element_size, data| PagedVecvec {
        element_size,
        _pad: 0,
        data
    }
);

#[pyclass(name = "MatrixView")]
pub struct PyMatrixView {
    rows: u32,
    cols: u32,
    element_size: u32,
    payload: PyObject,
    _owner: Option<PyObject>,
}

#[pymethods]
impl PyMatrixView {
    #[getter]
    fn rows(&self) -> u32 {
        self.rows
    }
    #[getter]
    fn cols(&self) -> u32 {
        self.cols
    }
    #[getter]
    fn element_size(&self) -> u32 {
        self.element_size
    }
    #[getter]
    fn payload(&self, py: Python<'_>) -> PyObject {
        self.payload.clone_ref(py)
    }
}

#[pyclass(name = "Matrix")]
pub struct PyMatrix {
    inner: Matrix,
}
#[pymethods]
impl PyMatrix {
    #[new]
    fn new(rows: u32, cols: u32, element_size: u32, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(Matrix {
                rows,
                cols,
                element_size,
                _pad: 0,
                data,
            })?,
        })
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Matrix>()
    }
    #[getter]
    fn rows(&self) -> u32 {
        self.inner.rows
    }
    #[getter]
    fn cols(&self) -> u32 {
        self.inner.cols
    }
    #[getter]
    fn element_size(&self) -> u32 {
        self.inner.element_size
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Matrix>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        let expected = crate::bind::type_hash::<Matrix>();
        if !crate::bind::is_type_hash_for::<Matrix>(kind) {
            return Err(PyValueError::new_err(format!(
                "wrong type hash: got {kind}, expected {expected}"
            )));
        }
        let (header, payload) = split_wire::<Matrix>(data)?;
        Self::from_wire(header, payload)
    }
    #[staticmethod]
    fn from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn from_archive(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        let (header, payload) =
            frame_header_and_payload(frame, "Matrix", crate::bind::type_hash::<Matrix>())?;
        let payload = frame_payload_to_bytes(&payload, "Matrix")?;
        Self::from_wire(header, payload)
    }
    #[staticmethod]
    fn from_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<Self> {
        Self::from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_from_wire_frame(frame: &Bound<'_, PyAny>) -> PyResult<PyMatrixView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_archive(frame: &Bound<'_, PyAny>) -> PyResult<PyMatrixView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_archive_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyMatrixView> {
        Self::view_from_wire_frame_v1(frame)
    }
    #[staticmethod]
    fn view_from_wire_frame_v1(frame: &Bound<'_, PyAny>) -> PyResult<PyMatrixView> {
        let (header, payload) =
            frame_header_and_payload(frame, "Matrix", crate::bind::type_hash::<Matrix>())?;
        let h: MatrixHeader = read_header("Matrix", &header)?;
        let payload_len = payload.len()?;
        Matrix::validate_wire_len(&h, payload_len)
            .map_err(|error| PyValueError::new_err(error.to_string()))?;
        Ok(PyMatrixView {
            rows: h.rows,
            cols: h.cols,
            element_size: h.element_size,
            payload: payload.unbind(),
            _owner: Some(frame.clone().unbind()),
        })
    }
}

#[pyclass(name = "Tensor")]
pub struct PyTensor {
    inner: Tensor,
}
#[pymethods]
impl PyTensor {
    #[new]
    fn new(rows: u32, cols: u32, layers: u32, element_size: u32, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(Tensor {
                rows,
                cols,
                layers,
                element_size,
                data,
            })?,
        })
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Tensor>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Tensor>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Tensor>(kind, data)?,
        })
    }
}

#[pyclass(name = "BitVec")]
pub struct PyBitVec {
    inner: BitVec,
}
#[pymethods]
impl PyBitVec {
    #[new]
    fn new(bits: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(BitVec { bits, data })?,
        })
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<BitVec>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<BitVec>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<BitVec>(kind, data)?,
        })
    }
}

#[pyclass(name = "Deque")]
pub struct PyDeque {
    inner: Deque,
}
#[pymethods]
impl PyDeque {
    #[new]
    fn new(element_size: u32, split_byte: u32, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(Deque {
                element_size,
                split_byte,
                data,
            })?,
        })
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Deque>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Deque>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Deque>(kind, data)?,
        })
    }
}

#[pyclass(name = "Queue")]
pub struct PyQueue {
    inner: Queue,
}
#[pymethods]
impl PyQueue {
    #[new]
    fn new(element_size: u32, front: u32, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(Queue {
                element_size,
                front,
                data,
            })?,
        })
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Queue>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Queue>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Queue>(kind, data)?,
        })
    }
}

#[pyclass(name = "List")]
pub struct PyList {
    inner: List,
}
#[pymethods]
impl PyList {
    #[new]
    fn new(element_size: u32) -> Self {
        Self {
            inner: List {
                head: crate::seq::LIST_NIL,
                tail: crate::seq::LIST_NIL,
                free_head: crate::seq::LIST_NIL,
                size_: 0,
                element_size,
                _pad: 0,
                data: Vec::new(),
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<List>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<List>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<List>(kind, data)?,
        })
    }
}

#[pyclass(name = "ForwardList")]
pub struct PyForwardList {
    inner: ForwardList,
}
#[pymethods]
impl PyForwardList {
    #[new]
    fn new(element_size: u32) -> Self {
        Self {
            inner: ForwardList {
                head: crate::seq::FORWARD_LIST_NIL,
                free_head: crate::seq::FORWARD_LIST_NIL,
                size_: 0,
                element_size,
                data: Vec::new(),
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<ForwardList>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<ForwardList>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<ForwardList>(kind, data)?,
        })
    }
}

#[pyclass(name = "Heap")]
pub struct PyHeap {
    inner: Heap,
}
#[pymethods]
impl PyHeap {
    #[new]
    fn new(element_size: u32, min_order: bool, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(Heap {
                element_size,
                order: if min_order {
                    crate::seq::HeapOrder::Min
                } else {
                    crate::seq::HeapOrder::Max
                },
                _pad: [0; 3],
                data,
            })?,
        })
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Heap>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<Heap>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<Heap>(kind, data)?,
        })
    }
}

#[pyclass(name = "IndexedHeap")]
pub struct PyIndexedHeap {
    inner: IndexedHeap,
}
#[pymethods]
impl PyIndexedHeap {
    #[new]
    fn new(priority_size: u32, min_order: bool, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: validate_owned(IndexedHeap {
                priority_size,
                order: if min_order {
                    crate::seq::HeapOrder::Min
                } else {
                    crate::seq::HeapOrder::Max
                },
                _pad: [0; 3],
                data,
            })?,
        })
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<IndexedHeap>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> PyResult<Vec<u8>> {
        copy_bytes_for_python("payload", self.inner.payload_bytes())
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_message_v1(&self) -> PyResult<(u64, Vec<u8>)> {
        wire_message_v1(&self.inner)
    }
    fn to_wire_frame(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame(slf, py)
    }
    fn to_wire_frame_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        let owner = (&slf).into_py(py);
        wire_frame_object(py, owner, &slf.inner)
    }
    fn archive_v1(slf: PyRef<'_, Self>, py: Python<'_>) -> PyResult<PyObject> {
        Self::to_wire_frame_v1(slf, py)
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
    #[staticmethod]
    fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_parts::<IndexedHeap>(header, payload)?,
        })
    }
    #[staticmethod]
    fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
        Ok(Self {
            inner: decode_message::<IndexedHeap>(kind, data)?,
        })
    }
}

pub fn register(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_class::<PyPayloadView>()?;
    m.add_class::<PyBytesPod>()?;
    m.add_class::<PyDpStr>()?;
    m.add_class::<PyDpString>()?;
    m.add_class::<PyLinestring>()?;
    m.add_class::<PyMultiPoint>()?;
    m.add_class::<PyRing>()?;
    m.add_class::<PyPath>()?;
    m.add_class::<PyTrajectory>()?;
    m.add_class::<PyGridView>()?;
    m.add_class::<PyGrid>()?;
    m.add_class::<PyLayer>()?;
    m.add_class::<PyMap>()?;
    m.add_class::<PySet>()?;
    m.add_class::<PyVector>()?;
    m.add_class::<PyMatrixView>()?;
    m.add_class::<PyMatrix>()?;
    m.add_class::<PyTensor>()?;
    m.add_class::<PyBitVec>()?;
    m.add_class::<PyDeque>()?;
    m.add_class::<PyQueue>()?;
    m.add_class::<PyStack>()?;
    m.add_class::<PyList>()?;
    m.add_class::<PyForwardList>()?;
    m.add_class::<PyHeap>()?;
    m.add_class::<PyIndexedHeap>()?;
    m.add_class::<PyVecvec>()?;
    m.add_class::<PyPagedVecvec>()?;
    m.add_class::<PyPolygonWire>()?;
    Ok(())
}
