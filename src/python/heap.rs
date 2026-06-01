#![allow(non_snake_case, clippy::wrong_self_convention)]

use pyo3::exceptions::PyValueError;
use pyo3::prelude::*;
use pyo3::types::PyModule;

use crate::wire::Encoding;
use crate::{
    BitVec, Bytes, DataPod, Deque, DpStr, DpString, ForwardList, Grid, Heap, IndexedHeap, Layer,
    Linestring, List, Map, Matrix, MultiPoint, PagedVecvec, Path, Point, Polygon, Pose, Queue,
    Ring, Set, Stack, State, Tensor, Trajectory, Vector, Vecvec,
};

fn header_bytes<T: DataPod>(value: &T) -> PyResult<Vec<u8>> {
    let mut out = vec![0_u8; crate::bind::header_size::<T>()];
    crate::bind::write_header(value, &mut out).map_err(PyValueError::new_err)?;
    Ok(out)
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

fn point_list(vertices: Vec<(f64, f64, f64)>) -> Vec<Point> {
    vertices
        .into_iter()
        .map(|(x, y, z)| Point::new(x, y, z))
        .collect()
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

macro_rules! byte_container {
    ($py:ident, $name:literal, $rust:ty, |$data:ident| $inner:expr) => {
        #[pyclass(name = $name)]
        pub struct $py {
            inner: $rust,
        }
        #[pymethods]
        impl $py {
            #[new]
            fn new($data: Vec<u8>) -> Self {
                Self { inner: $inner }
            }
            #[classattr]
            fn TYPE_HASH() -> u64 {
                crate::bind::type_hash::<$rust>()
            }
            fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
                header_bytes(&self.inner)
            }
            fn payload_bytes(&self) -> Vec<u8> {
                self.inner.payload_bytes().to_vec()
            }
            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                let mut data = header_bytes(&self.inner)?;
                data.extend_from_slice(self.inner.payload_bytes());
                Ok((crate::bind::type_hash::<$rust>(), data))
            }
            fn payload_len(&self) -> usize {
                self.inner.payload_bytes().len()
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
            fn new($element_size: u32, $data: Vec<u8>) -> Self {
                Self { inner: $inner }
            }
            #[classattr]
            fn TYPE_HASH() -> u64 {
                crate::bind::type_hash::<$rust>()
            }
            fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
                header_bytes(&self.inner)
            }
            fn payload_bytes(&self) -> Vec<u8> {
                self.inner.payload_bytes().to_vec()
            }
            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                let mut data = header_bytes(&self.inner)?;
                data.extend_from_slice(self.inner.payload_bytes());
                Ok((crate::bind::type_hash::<$rust>(), data))
            }
            fn payload_len(&self) -> usize {
                self.inner.payload_bytes().len()
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<DpStr>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
            fn new(vertices: Vec<(f64, f64, f64)>) -> Self {
                Self {
                    inner: $ctor(point_list(vertices)),
                }
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
            fn payload_bytes(&self) -> Vec<u8> {
                self.inner.payload_bytes().to_vec()
            }
            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                let mut data = header_bytes(&self.inner)?;
                data.extend_from_slice(self.inner.payload_bytes());
                Ok((crate::bind::type_hash::<$rust>(), data))
            }
            fn payload_len(&self) -> usize {
                self.inner.payload_bytes().len()
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
            inner: Path::new(
                waypoints
                    .iter()
                    .map(|v| pose_from_vec(v))
                    .collect::<PyResult<Vec<_>>>()?,
            ),
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Path>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
            inner: Trajectory::new(
                states
                    .iter()
                    .map(|v| state_from_vec(v))
                    .collect::<PyResult<Vec<_>>>()?,
            ),
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Trajectory>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
            inner: Grid::new(
                rows,
                cols,
                encoding(encoding_id)?,
                resolution,
                centered,
                pose_from_vec(&pose)?,
                data,
            ),
        })
    }
    fn len(&self) -> usize {
        self.inner.size()
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Grid>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Grid>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
            inner: Layer {
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
            },
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Layer>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
    fn insert(&mut self, key: Vec<u8>, value: Vec<u8>) {
        self.inner.insert(&key, &value);
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Map>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
    fn insert(&mut self, key: Vec<u8>) -> bool {
        self.inner.insert(&key)
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Set>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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

#[pyclass(name = "Matrix")]
pub struct PyMatrix {
    inner: Matrix,
}
#[pymethods]
impl PyMatrix {
    #[new]
    fn new(rows: u32, cols: u32, element_size: u32, data: Vec<u8>) -> Self {
        Self {
            inner: Matrix {
                rows,
                cols,
                element_size,
                _pad: 0,
                data,
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Matrix>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Matrix>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
}

#[pyclass(name = "Tensor")]
pub struct PyTensor {
    inner: Tensor,
}
#[pymethods]
impl PyTensor {
    #[new]
    fn new(rows: u32, cols: u32, layers: u32, element_size: u32, data: Vec<u8>) -> Self {
        Self {
            inner: Tensor {
                rows,
                cols,
                layers,
                element_size,
                data,
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Tensor>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Tensor>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
}

#[pyclass(name = "BitVec")]
pub struct PyBitVec {
    inner: BitVec,
}
#[pymethods]
impl PyBitVec {
    #[new]
    fn new(bits: u64, data: Vec<u8>) -> Self {
        Self {
            inner: BitVec { bits, data },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<BitVec>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<BitVec>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
}

#[pyclass(name = "Deque")]
pub struct PyDeque {
    inner: Deque,
}
#[pymethods]
impl PyDeque {
    #[new]
    fn new(element_size: u32, split_byte: u32, data: Vec<u8>) -> Self {
        Self {
            inner: Deque {
                element_size,
                split_byte,
                data,
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Deque>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Deque>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
}

#[pyclass(name = "Queue")]
pub struct PyQueue {
    inner: Queue,
}
#[pymethods]
impl PyQueue {
    #[new]
    fn new(element_size: u32, front: u32, data: Vec<u8>) -> Self {
        Self {
            inner: Queue {
                element_size,
                front,
                data,
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Queue>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Queue>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<List>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
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
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<ForwardList>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
}

#[pyclass(name = "Heap")]
pub struct PyHeap {
    inner: Heap,
}
#[pymethods]
impl PyHeap {
    #[new]
    fn new(element_size: u32, min_order: bool, data: Vec<u8>) -> Self {
        Self {
            inner: Heap {
                element_size,
                order: if min_order {
                    crate::seq::HeapOrder::Min
                } else {
                    crate::seq::HeapOrder::Max
                },
                _pad: [0; 3],
                data,
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<Heap>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<Heap>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
}

#[pyclass(name = "IndexedHeap")]
pub struct PyIndexedHeap {
    inner: IndexedHeap,
}
#[pymethods]
impl PyIndexedHeap {
    #[new]
    fn new(priority_size: u32, min_order: bool, data: Vec<u8>) -> Self {
        Self {
            inner: IndexedHeap {
                priority_size,
                order: if min_order {
                    crate::seq::HeapOrder::Min
                } else {
                    crate::seq::HeapOrder::Max
                },
                _pad: [0; 3],
                data,
            },
        }
    }
    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<IndexedHeap>()
    }
    fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
        header_bytes(&self.inner)
    }
    fn payload_bytes(&self) -> Vec<u8> {
        self.inner.payload_bytes().to_vec()
    }
    fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
        let mut data = header_bytes(&self.inner)?;
        data.extend_from_slice(self.inner.payload_bytes());
        Ok((crate::bind::type_hash::<IndexedHeap>(), data))
    }
    fn payload_len(&self) -> usize {
        self.inner.payload_bytes().len()
    }
}

pub fn register(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_class::<PyBytesPod>()?;
    m.add_class::<PyDpStr>()?;
    m.add_class::<PyDpString>()?;
    m.add_class::<PyLinestring>()?;
    m.add_class::<PyMultiPoint>()?;
    m.add_class::<PyRing>()?;
    m.add_class::<PyPath>()?;
    m.add_class::<PyTrajectory>()?;
    m.add_class::<PyGrid>()?;
    m.add_class::<PyLayer>()?;
    m.add_class::<PyMap>()?;
    m.add_class::<PySet>()?;
    m.add_class::<PyVector>()?;
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
