#![allow(non_snake_case, clippy::wrong_self_convention)]

use pyo3::exceptions::PyValueError;
use pyo3::prelude::*;
use pyo3::types::PyModule;

use crate::wire::{Encoding, Envelope};
use crate::{
    Aabb, Accel, Acceleration, Actuator, BoundingSphere, BoxShape, Circle, Collision,
    CylinderShape, Euler, GaussianBox, GaussianCircle, GaussianPoint, GaussianRectangle, Geo,
    Geometry, GeometryKind, Identity, Inertial, Ip, Joint, JointCalibration, JointDynamics,
    JointLimits, JointMimic, JointSafetyController, JointType, KV, Line, Link, Loc, MacAddr,
    MapEntry, Material, MeshShape, Model, Obb, Odom, Point, PointKey, Pose, Quaternion, Rectangle,
    Robot, Segment, Sensor, SetEntry, Size, SphereShape, Square, State, Transform, Transmission,
    TransmissionJoint, Triangle, Twist, Utm, Uuid, Velocity, Visual, Wrench,
};

fn fixed_header_bytes<T: crate::DataPod>(value: &T) -> PyResult<Vec<u8>> {
    let mut out = vec![0_u8; crate::bind::header_size::<T>()];
    crate::bind::write_header(value, &mut out).map_err(PyValueError::new_err)?;
    Ok(out)
}

fn reject_payload(type_name: &str, payload: &[u8]) -> PyResult<()> {
    if payload.is_empty() {
        Ok(())
    } else {
        Err(PyValueError::new_err(format!(
            "{type_name} payload must be empty"
        )))
    }
}

macro_rules! py_fixed {
    (
        $py:ident, $name:literal, $rust:ty,
        fields { $($field:ident : $ty:ty),+ $(,)? },
        to_rust |$self_name:ident| $to_rust:expr,
        from_rust |$rust_name:ident| $from_rust:expr
    ) => {
        #[pyclass(name = $name)]
        #[derive(Clone, Copy)]
        pub struct $py {
            $(
                #[pyo3(get, set)]
                pub $field: $ty,
            )+
        }

        impl From<$rust> for $py {
            fn from($rust_name: $rust) -> Self {
                $from_rust
            }
        }

        impl From<$py> for $rust {
            fn from($self_name: $py) -> Self {
                $to_rust
            }
        }

        #[pymethods]
        impl $py {
            #[new]
            fn new($($field: $ty),+) -> Self {
                Self { $($field),+ }
            }

            #[staticmethod]
            fn default() -> Self {
                <$rust>::default().into()
            }

            #[classattr]
            fn TYPE_HASH() -> u64 {
                crate::bind::type_hash::<$rust>()
            }

            fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
                fixed_header_bytes(&<$rust>::from(*self))
            }

            fn payload_bytes(&self) -> Vec<u8> {
                Vec::new()
            }

            #[staticmethod]
            fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
                reject_payload($name, &payload)?;
                crate::bind::read_fixed_header::<$rust>(&header)
                    .map(Self::from)
                    .map_err(PyValueError::new_err)
            }

            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                Ok((crate::bind::type_hash::<$rust>(), self.to_header_bytes()?))
            }

            #[staticmethod]
            fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
                let expected = crate::bind::type_hash::<$rust>();
                if kind != expected {
                    return Err(PyValueError::new_err(format!(
                        "wrong type hash: got {kind}, expected {expected}"
                    )));
                }
                Self::from_wire(data, Vec::new())
            }

            fn __repr__(&self) -> String {
                let fields = vec![$(format!("{}={:?}", stringify!($field), self.$field)),+];
                format!("{}({})", $name, fields.join(", "))
            }
        }
    };
}

macro_rules! py_default_fixed {
    ($py:ident, $name:literal, $rust:ty) => {
        #[pyclass(name = $name)]
        #[derive(Clone, Copy)]
        pub struct $py {
            inner: $rust,
        }

        #[pymethods]
        impl $py {
            #[new]
            fn new() -> Self {
                Self {
                    inner: <$rust>::default(),
                }
            }

            #[staticmethod]
            fn default() -> Self {
                Self::new()
            }

            #[classattr]
            fn TYPE_HASH() -> u64 {
                crate::bind::type_hash::<$rust>()
            }

            fn to_header_bytes(&self) -> PyResult<Vec<u8>> {
                fixed_header_bytes(&self.inner)
            }

            fn payload_bytes(&self) -> Vec<u8> {
                Vec::new()
            }

            #[staticmethod]
            fn from_wire(header: Vec<u8>, payload: Vec<u8>) -> PyResult<Self> {
                reject_payload($name, &payload)?;
                crate::bind::read_fixed_header::<$rust>(&header)
                    .map(|inner| Self { inner })
                    .map_err(PyValueError::new_err)
            }

            fn to_wire_message(&self) -> PyResult<(u64, Vec<u8>)> {
                Ok((crate::bind::type_hash::<$rust>(), self.to_header_bytes()?))
            }

            #[staticmethod]
            fn from_wire_message(kind: u64, data: Vec<u8>) -> PyResult<Self> {
                let expected = crate::bind::type_hash::<$rust>();
                if kind != expected {
                    return Err(PyValueError::new_err(format!(
                        "wrong type hash: got {kind}, expected {expected}"
                    )));
                }
                Self::from_wire(data, Vec::new())
            }

            fn __repr__(&self) -> String {
                format!("{}()", $name)
            }
        }
    };
}

#[pyclass(name = "MapEntry")]
#[derive(Clone, Copy)]
pub struct PyMapEntry {
    #[pyo3(get, set)]
    pub key_off: u32,
    #[pyo3(get, set)]
    pub key_len: u32,
    #[pyo3(get, set)]
    pub value_off: u32,
    #[pyo3(get, set)]
    pub value_len: u32,
}

impl From<MapEntry> for PyMapEntry {
    fn from(value: MapEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
            value_off: value.value_off,
            value_len: value.value_len,
        }
    }
}

impl From<PyMapEntry> for MapEntry {
    fn from(value: PyMapEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
            value_off: value.value_off,
            value_len: value.value_len,
        }
    }
}

#[pymethods]
impl PyMapEntry {
    #[new]
    #[pyo3(signature = (key_off=0, key_len=0, value_off=0, value_len=0))]
    fn new(key_off: u32, key_len: u32, value_off: u32, value_len: u32) -> Self {
        Self {
            key_off,
            key_len,
            value_off,
            value_len,
        }
    }

    #[staticmethod]
    fn default() -> Self {
        MapEntry::default().into()
    }

    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<MapEntry>()
    }

    #[classattr]
    fn BYTE_SIZE() -> usize {
        std::mem::size_of::<MapEntry>()
    }

    fn to_bytes(&self) -> Vec<u8> {
        let value = MapEntry::from(*self);
        bytemuck::bytes_of(&value).to_vec()
    }

    #[staticmethod]
    fn from_bytes(data: Vec<u8>) -> PyResult<Self> {
        let size = std::mem::size_of::<MapEntry>();
        if data.len() < size {
            return Err(PyValueError::new_err(format!(
                "MapEntry byte buffer too small: need {size}, got {}",
                data.len()
            )));
        }
        Ok(bytemuck::pod_read_unaligned::<MapEntry>(&data[..size]).into())
    }

    fn __repr__(&self) -> String {
        format!(
            "MapEntry(key_off={}, key_len={}, value_off={}, value_len={})",
            self.key_off, self.key_len, self.value_off, self.value_len
        )
    }
}

#[pyclass(name = "SetEntry")]
#[derive(Clone, Copy)]
pub struct PySetEntry {
    #[pyo3(get, set)]
    pub key_off: u32,
    #[pyo3(get, set)]
    pub key_len: u32,
}

impl From<SetEntry> for PySetEntry {
    fn from(value: SetEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
        }
    }
}

impl From<PySetEntry> for SetEntry {
    fn from(value: PySetEntry) -> Self {
        Self {
            key_off: value.key_off,
            key_len: value.key_len,
        }
    }
}

#[pymethods]
impl PySetEntry {
    #[new]
    #[pyo3(signature = (key_off=0, key_len=0))]
    fn new(key_off: u32, key_len: u32) -> Self {
        Self { key_off, key_len }
    }

    #[staticmethod]
    fn default() -> Self {
        SetEntry::default().into()
    }

    #[classattr]
    fn TYPE_HASH() -> u64 {
        crate::bind::type_hash::<SetEntry>()
    }

    #[classattr]
    fn BYTE_SIZE() -> usize {
        std::mem::size_of::<SetEntry>()
    }

    fn to_bytes(&self) -> Vec<u8> {
        let value = SetEntry::from(*self);
        bytemuck::bytes_of(&value).to_vec()
    }

    #[staticmethod]
    fn from_bytes(data: Vec<u8>) -> PyResult<Self> {
        let size = std::mem::size_of::<SetEntry>();
        if data.len() < size {
            return Err(PyValueError::new_err(format!(
                "SetEntry byte buffer too small: need {size}, got {}",
                data.len()
            )));
        }
        Ok(bytemuck::pod_read_unaligned::<SetEntry>(&data[..size]).into())
    }

    fn __repr__(&self) -> String {
        format!(
            "SetEntry(key_off={}, key_len={})",
            self.key_off, self.key_len
        )
    }
}

py_fixed!(
    PyEuler,
    "Euler",
    Euler,
    fields {
        roll: f64,
        pitch: f64,
        yaw: f64
    },
    to_rust | value | Euler::new(value.roll, value.pitch, value.yaw),
    from_rust
        | value
        | Self {
            roll: value.roll,
            pitch: value.pitch,
            yaw: value.yaw
        }
);

py_fixed!(
    PyQuaternion,
    "Quaternion",
    Quaternion,
    fields {
        w: f64,
        x: f64,
        y: f64,
        z: f64
    },
    to_rust | value | Quaternion::new(value.w, value.x, value.y, value.z),
    from_rust
        | value
        | Self {
            w: value.w,
            x: value.x,
            y: value.y,
            z: value.z
        }
);

py_fixed!(
    PyVelocity,
    "Velocity",
    Velocity,
    fields {
        vx: f64,
        vy: f64,
        vz: f64
    },
    to_rust
        | value
        | Velocity {
            vx: value.vx,
            vy: value.vy,
            vz: value.vz
        },
    from_rust
        | value
        | Self {
            vx: value.vx,
            vy: value.vy,
            vz: value.vz
        }
);

py_fixed!(
    PyAcceleration,
    "Acceleration",
    Acceleration,
    fields {
        ax: f64,
        ay: f64,
        az: f64
    },
    to_rust
        | value
        | Acceleration {
            ax: value.ax,
            ay: value.ay,
            az: value.az
        },
    from_rust
        | value
        | Self {
            ax: value.ax,
            ay: value.ay,
            az: value.az
        }
);

py_fixed!(
    PyPose,
    "Pose",
    Pose,
    fields {
        x: f64,
        y: f64,
        z: f64,
        qw: f64,
        qx: f64,
        qy: f64,
        qz: f64
    },
    to_rust
        | value
        | Pose {
            point: Point::new(value.x, value.y, value.z),
            rotation: Quaternion::new(value.qw, value.qx, value.qy, value.qz)
        },
    from_rust
        | value
        | Self {
            x: value.point.x,
            y: value.point.y,
            z: value.point.z,
            qw: value.rotation.w,
            qx: value.rotation.x,
            qy: value.rotation.y,
            qz: value.rotation.z
        }
);

py_fixed!(
    PyTransform,
    "Transform",
    Transform,
    fields {
        rw: f64,
        rx: f64,
        ry: f64,
        rz: f64,
        dw: f64,
        dx: f64,
        dy: f64,
        dz: f64
    },
    to_rust
        | value
        | Transform {
            rw: value.rw,
            rx: value.rx,
            ry: value.ry,
            rz: value.rz,
            dw: value.dw,
            dx: value.dx,
            dy: value.dy,
            dz: value.dz
        },
    from_rust
        | value
        | Self {
            rw: value.rw,
            rx: value.rx,
            ry: value.ry,
            rz: value.rz,
            dw: value.dw,
            dx: value.dx,
            dy: value.dy,
            dz: value.dz
        }
);

py_fixed!(
    PyState,
    "State",
    State,
    fields {
        x: f64,
        y: f64,
        z: f64,
        qw: f64,
        qx: f64,
        qy: f64,
        qz: f64,
        vx: f64,
        vy: f64,
        vz: f64,
        wx: f64,
        wy: f64,
        wz: f64
    },
    to_rust
        | value
        | State::from_mat([
            value.x, value.y, value.z, value.qw, value.qx, value.qy, value.qz, value.vx, value.vy,
            value.vz, value.wx, value.wy, value.wz
        ]),
    from_rust | value | {
        let m = value.to_mat();
        Self {
            x: m[0],
            y: m[1],
            z: m[2],
            qw: m[3],
            qx: m[4],
            qy: m[5],
            qz: m[6],
            vx: m[7],
            vy: m[8],
            vz: m[9],
            wx: m[10],
            wy: m[11],
            wz: m[12],
        }
    }
);

py_fixed!(
    PyLoc,
    "Loc",
    Loc,
    fields {
        x: f64,
        y: f64,
        z: f64,
        latitude: f64,
        longitude: f64,
        altitude: f64
    },
    to_rust
        | value
        | Loc {
            local: Point::new(value.x, value.y, value.z),
            origin: Geo::new(value.latitude, value.longitude, value.altitude)
        },
    from_rust
        | value
        | Self {
            x: value.local.x,
            y: value.local.y,
            z: value.local.z,
            latitude: value.origin.latitude,
            longitude: value.origin.longitude,
            altitude: value.origin.altitude
        }
);

py_fixed!(
    PyUtm,
    "Utm",
    Utm,
    fields {
        zone: i32,
        band: u32,
        easting: f64,
        northing: f64,
        altitude: f64
    },
    to_rust
        | value
        | Utm {
            zone: value.zone,
            band: value.band,
            easting: value.easting,
            northing: value.northing,
            altitude: value.altitude
        },
    from_rust
        | value
        | Self {
            zone: value.zone,
            band: value.band,
            easting: value.easting,
            northing: value.northing,
            altitude: value.altitude
        }
);

py_fixed!(
    PyLine,
    "Line",
    Line,
    fields {
        ox: f64,
        oy: f64,
        oz: f64,
        dx: f64,
        dy: f64,
        dz: f64
    },
    to_rust
        | value
        | Line::new(
            Point::new(value.ox, value.oy, value.oz),
            Point::new(value.dx, value.dy, value.dz)
        ),
    from_rust
        | value
        | Self {
            ox: value.origin.x,
            oy: value.origin.y,
            oz: value.origin.z,
            dx: value.direction.x,
            dy: value.direction.y,
            dz: value.direction.z
        }
);

py_fixed!(
    PyRectangle,
    "Rectangle",
    Rectangle,
    fields {
        tlx: f64,
        tly: f64,
        tlz: f64,
        trx: f64,
        try_: f64,
        trz: f64,
        blx: f64,
        bly: f64,
        blz: f64,
        brx: f64,
        bry: f64,
        brz: f64
    },
    to_rust
        | value
        | Rectangle::from_mat([
            value.tlx, value.tly, value.tlz, value.trx, value.try_, value.trz, value.blx,
            value.bly, value.blz, value.brx, value.bry, value.brz
        ]),
    from_rust | value | {
        let m = value.to_mat();
        Self {
            tlx: m[0],
            tly: m[1],
            tlz: m[2],
            trx: m[3],
            try_: m[4],
            trz: m[5],
            blx: m[6],
            bly: m[7],
            blz: m[8],
            brx: m[9],
            bry: m[10],
            brz: m[11],
        }
    }
);

py_fixed!(
    PyAabb,
    "Aabb",
    Aabb,
    fields {
        min_x: f64,
        min_y: f64,
        min_z: f64,
        max_x: f64,
        max_y: f64,
        max_z: f64
    },
    to_rust
        | value
        | Aabb::new(
            Point::new(value.min_x, value.min_y, value.min_z),
            Point::new(value.max_x, value.max_y, value.max_z)
        ),
    from_rust
        | value
        | Self {
            min_x: value.min_point.x,
            min_y: value.min_point.y,
            min_z: value.min_point.z,
            max_x: value.max_point.x,
            max_y: value.max_point.y,
            max_z: value.max_point.z
        }
);

py_fixed!(
    PyBoundingSphere,
    "BoundingSphere",
    BoundingSphere,
    fields {
        x: f64,
        y: f64,
        z: f64,
        radius: f64
    },
    to_rust
        | value
        | BoundingSphere {
            center: Point::new(value.x, value.y, value.z),
            radius: value.radius
        },
    from_rust
        | value
        | Self {
            x: value.center.x,
            y: value.center.y,
            z: value.center.z,
            radius: value.radius
        }
);

py_fixed!(
    PyCircle,
    "Circle",
    Circle,
    fields {
        x: f64,
        y: f64,
        z: f64,
        radius: f64
    },
    to_rust
        | value
        | Circle {
            center: Point::new(value.x, value.y, value.z),
            radius: value.radius
        },
    from_rust
        | value
        | Self {
            x: value.center.x,
            y: value.center.y,
            z: value.center.z,
            radius: value.radius
        }
);

py_fixed!(
    PyTriangle,
    "Triangle",
    Triangle,
    fields {
        ax: f64,
        ay: f64,
        az: f64,
        bx: f64,
        by: f64,
        bz: f64,
        cx: f64,
        cy: f64,
        cz: f64
    },
    to_rust
        | value
        | Triangle {
            a: Point::new(value.ax, value.ay, value.az),
            b: Point::new(value.bx, value.by, value.bz),
            c: Point::new(value.cx, value.cy, value.cz)
        },
    from_rust
        | value
        | Self {
            ax: value.a.x,
            ay: value.a.y,
            az: value.a.z,
            bx: value.b.x,
            by: value.b.y,
            bz: value.b.z,
            cx: value.c.x,
            cy: value.c.y,
            cz: value.c.z
        }
);

py_fixed!(
    PyTwist,
    "Twist",
    Twist,
    fields {
        vx: f64,
        vy: f64,
        vz: f64,
        wx: f64,
        wy: f64,
        wz: f64
    },
    to_rust
        | value
        | Twist::from_components(value.vx, value.vy, value.vz, value.wx, value.wy, value.wz),
    from_rust | value | {
        let m = value.to_mat();
        Self {
            vx: m[0],
            vy: m[1],
            vz: m[2],
            wx: m[3],
            wy: m[4],
            wz: m[5],
        }
    }
);

py_fixed!(
    PyWrench,
    "Wrench",
    Wrench,
    fields {
        fx: f64,
        fy: f64,
        fz: f64,
        tx: f64,
        ty: f64,
        tz: f64
    },
    to_rust
        | value
        | Wrench::from_components(value.fx, value.fy, value.fz, value.tx, value.ty, value.tz),
    from_rust | value | {
        let m = value.to_mat();
        Self {
            fx: m[0],
            fy: m[1],
            fz: m[2],
            tx: m[3],
            ty: m[4],
            tz: m[5],
        }
    }
);

py_fixed!(
    PyOdom,
    "Odom",
    Odom,
    fields {
        x: f64,
        y: f64,
        z: f64,
        qw: f64,
        qx: f64,
        qy: f64,
        qz: f64,
        vx: f64,
        vy: f64,
        vz: f64,
        wx: f64,
        wy: f64,
        wz: f64
    },
    to_rust
        | value
        | Odom::from_mat([
            value.x, value.y, value.z, value.qw, value.qx, value.qy, value.qz, value.vx, value.vy,
            value.vz, value.wx, value.wy, value.wz
        ]),
    from_rust | value | {
        let m = value.to_mat();
        Self {
            x: m[0],
            y: m[1],
            z: m[2],
            qw: m[3],
            qx: m[4],
            qy: m[5],
            qz: m[6],
            vx: m[7],
            vy: m[8],
            vz: m[9],
            wx: m[10],
            wy: m[11],
            wz: m[12],
        }
    }
);

py_fixed!(
    PyJointLimits,
    "JointLimits",
    JointLimits,
    fields {
        lower: f64,
        upper: f64,
        effort: f64,
        velocity: f64
    },
    to_rust
        | value
        | JointLimits {
            lower: value.lower,
            upper: value.upper,
            effort: value.effort,
            velocity: value.velocity
        },
    from_rust
        | value
        | Self {
            lower: value.lower,
            upper: value.upper,
            effort: value.effort,
            velocity: value.velocity
        }
);

py_fixed!(
    PyInertial,
    "Inertial",
    Inertial,
    fields {
        x: f64,
        y: f64,
        z: f64,
        qw: f64,
        qx: f64,
        qy: f64,
        qz: f64,
        mass: f64,
        ixx: f64,
        ixy: f64,
        ixz: f64,
        iyy: f64,
        iyz: f64,
        izz: f64
    },
    to_rust
        | value
        | Inertial {
            origin: Pose {
                point: Point::new(value.x, value.y, value.z),
                rotation: Quaternion::new(value.qw, value.qx, value.qy, value.qz)
            },
            mass: value.mass,
            ixx: value.ixx,
            ixy: value.ixy,
            ixz: value.ixz,
            iyy: value.iyy,
            iyz: value.iyz,
            izz: value.izz
        },
    from_rust
        | value
        | Self {
            x: value.origin.point.x,
            y: value.origin.point.y,
            z: value.origin.point.z,
            qw: value.origin.rotation.w,
            qx: value.origin.rotation.x,
            qy: value.origin.rotation.y,
            qz: value.origin.rotation.z,
            mass: value.mass,
            ixx: value.ixx,
            ixy: value.ixy,
            ixz: value.ixz,
            iyy: value.iyy,
            iyz: value.iyz,
            izz: value.izz
        }
);

py_fixed!(
    PyUuid,
    "Uuid",
    Uuid,
    fields { bytes: [u8; 16] },
    to_rust | value | Uuid { bytes: value.bytes },
    from_rust | value | Self { bytes: value.bytes }
);

py_fixed!(
    PyIp,
    "Ip",
    Ip,
    fields {
        family: u32,
        bytes: [u8; 16]
    },
    to_rust
        | value
        | Ip {
            family: value.family,
            _pad: 0,
            bytes: value.bytes
        },
    from_rust
        | value
        | Self {
            family: value.family,
            bytes: value.bytes
        }
);

py_fixed!(
    PyMacAddr,
    "MacAddr",
    MacAddr,
    fields { bytes: [u8; 6] },
    to_rust
        | value
        | MacAddr {
            bytes: value.bytes,
            _pad: [0; 2]
        },
    from_rust | value | Self { bytes: value.bytes }
);

py_default_fixed!(PyEnvelope, "Envelope", Envelope);
py_default_fixed!(PyEncoding, "Encoding", Encoding);
py_default_fixed!(PyPointKey, "PointKey", PointKey);
py_default_fixed!(PySize, "Size", Size);
py_default_fixed!(PySquare, "Square", Square);
py_default_fixed!(PyObb, "Obb", Obb);
py_default_fixed!(PyBoxPod, "Box", crate::Box);
py_default_fixed!(PyGaussianPoint, "GaussianPoint", GaussianPoint);
py_default_fixed!(PyGaussianCircle, "GaussianCircle", GaussianCircle);
py_default_fixed!(PyGaussianRectangle, "GaussianRectangle", GaussianRectangle);
py_default_fixed!(PyGaussianBox, "GaussianBox", GaussianBox);
py_default_fixed!(PyAccel, "Accel", Accel);
py_default_fixed!(PyJointDynamics, "JointDynamics", JointDynamics);
py_default_fixed!(PyJointMimic, "JointMimic", JointMimic);
py_default_fixed!(
    PyJointSafetyController,
    "JointSafetyController",
    JointSafetyController
);
py_default_fixed!(PyJointCalibration, "JointCalibration", JointCalibration);
py_default_fixed!(PyKV, "KV", KV);
py_default_fixed!(PyBoxShape, "BoxShape", BoxShape);
py_default_fixed!(PySphereShape, "SphereShape", SphereShape);
py_default_fixed!(PyCylinderShape, "CylinderShape", CylinderShape);
py_default_fixed!(PyMeshShape, "MeshShape", MeshShape);
py_default_fixed!(PyGeometryKind, "GeometryKind", GeometryKind);
py_default_fixed!(PyGeometry, "Geometry", Geometry);
py_default_fixed!(PyIdentity, "Identity", Identity);
py_default_fixed!(PyModel, "Model", Model);
py_default_fixed!(PyMaterial, "Material", Material);
py_default_fixed!(PyVisual, "Visual", Visual);
py_default_fixed!(PyCollision, "Collision", Collision);
py_default_fixed!(PyJointType, "JointType", JointType);
py_default_fixed!(PyJoint, "Joint", Joint);
py_default_fixed!(PyLink, "Link", Link);
py_default_fixed!(PySensor, "Sensor", Sensor);
py_default_fixed!(PyRobot, "Robot", Robot);
py_default_fixed!(PyActuator, "Actuator", Actuator);
py_default_fixed!(PyTransmissionJoint, "TransmissionJoint", TransmissionJoint);
py_default_fixed!(PyTransmission, "Transmission", Transmission);

pub fn register(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_class::<PyMapEntry>()?;
    m.add_class::<PySetEntry>()?;
    m.add_class::<PyEuler>()?;
    m.add_class::<PyQuaternion>()?;
    m.add_class::<PyVelocity>()?;
    m.add_class::<PyAcceleration>()?;
    m.add_class::<PyPose>()?;
    m.add_class::<PyTransform>()?;
    m.add_class::<PyState>()?;
    m.add_class::<PyLoc>()?;
    m.add_class::<PyUtm>()?;
    m.add_class::<PyLine>()?;
    m.add_class::<PyRectangle>()?;
    m.add_class::<PyAabb>()?;
    m.add_class::<PyBoundingSphere>()?;
    m.add_class::<PyCircle>()?;
    m.add_class::<PyTriangle>()?;
    m.add_class::<PyTwist>()?;
    m.add_class::<PyWrench>()?;
    m.add_class::<PyOdom>()?;
    m.add_class::<PyJointLimits>()?;
    m.add_class::<PyInertial>()?;
    m.add_class::<PyUuid>()?;
    m.add_class::<PyIp>()?;
    m.add_class::<PyMacAddr>()?;
    m.add_class::<PyEnvelope>()?;
    m.add_class::<PyEncoding>()?;
    m.add_class::<PyPointKey>()?;
    m.add_class::<PySize>()?;
    m.add_class::<PySquare>()?;
    m.add_class::<PyObb>()?;
    m.add_class::<PyBoxPod>()?;
    m.add_class::<PyGaussianPoint>()?;
    m.add_class::<PyGaussianCircle>()?;
    m.add_class::<PyGaussianRectangle>()?;
    m.add_class::<PyGaussianBox>()?;
    m.add_class::<PyAccel>()?;
    m.add_class::<PyJointDynamics>()?;
    m.add_class::<PyJointMimic>()?;
    m.add_class::<PyJointSafetyController>()?;
    m.add_class::<PyJointCalibration>()?;
    m.add_class::<PyKV>()?;
    m.add_class::<PyBoxShape>()?;
    m.add_class::<PySphereShape>()?;
    m.add_class::<PyCylinderShape>()?;
    m.add_class::<PyMeshShape>()?;
    m.add_class::<PyGeometryKind>()?;
    m.add_class::<PyGeometry>()?;
    m.add_class::<PyIdentity>()?;
    m.add_class::<PyModel>()?;
    m.add_class::<PyMaterial>()?;
    m.add_class::<PyVisual>()?;
    m.add_class::<PyCollision>()?;
    m.add_class::<PyJointType>()?;
    m.add_class::<PyJoint>()?;
    m.add_class::<PyLink>()?;
    m.add_class::<PySensor>()?;
    m.add_class::<PyRobot>()?;
    m.add_class::<PyActuator>()?;
    m.add_class::<PyTransmissionJoint>()?;
    m.add_class::<PyTransmission>()?;
    Ok(())
}
