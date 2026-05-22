use crate::geom::shapes::Size;
use crate::id::STRING_NONE;

#[datapod::datapod]
#[derive(Default)]
pub struct BoxShape {
    pub size: Size,
}

#[datapod::datapod]
#[derive(Default)]
pub struct SphereShape {
    pub radius: f64,
}

#[datapod::datapod]
#[derive(Default)]
pub struct CylinderShape {
    pub radius: f64,
    pub length: f64,
}

/// Mesh shape. The URI is shipped separately as a
/// [`DpString`](crate::spatial::sugar::DpString); `uri_id` references it.
#[datapod::datapod]
pub struct MeshShape {
    pub uri_id: u32,
    pub _pad: u32,
    pub scale: [f64; 3],
}

impl Default for MeshShape {
    fn default() -> Self {
        Self { uri_id: STRING_NONE, _pad: 0, scale: [1.0, 1.0, 1.0] }
    }
}

/// Tag for the active variant of a [`Geometry`] record.
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GeometryKind {
    Box = 0,
    Sphere = 1,
    Cylinder = 2,
    Mesh = 3,
}

impl Default for GeometryKind {
    fn default() -> Self {
        Self::Box
    }
}

unsafe impl bytemuck::Zeroable for GeometryKind {}
unsafe impl bytemuck::Pod for GeometryKind {}
unsafe impl crate::ZeroCopySend for GeometryKind {}
impl crate::DataPod for GeometryKind {
    type Header = GeometryKind;
    type Payload = ();
    fn header(&self) -> GeometryKind { *self }
    fn payload_bytes(&self) -> &[u8] { &[] }
}

/// Geometry record — a tag plus all four variant payloads inline.
/// Only the variant indicated by `kind` is meaningful; the others are
/// zeroed. This is the Pod equivalent of the C++/Rust `enum` variants.
#[datapod::datapod]
#[derive(Default)]
pub struct Geometry {
    pub kind: GeometryKind,
    pub _pad: u32,
    pub box_shape: BoxShape,
    pub sphere: SphereShape,
    pub cylinder: CylinderShape,
    pub mesh: MeshShape,
}

impl Geometry {
    pub fn box_shape(size: Size) -> Self {
        Self {
            kind: GeometryKind::Box,
            box_shape: BoxShape { size },
            ..Self::default()
        }
    }

    pub fn sphere(radius: f64) -> Self {
        Self {
            kind: GeometryKind::Sphere,
            sphere: SphereShape { radius },
            ..Self::default()
        }
    }

    pub fn cylinder(radius: f64, length: f64) -> Self {
        Self {
            kind: GeometryKind::Cylinder,
            cylinder: CylinderShape { radius, length },
            ..Self::default()
        }
    }

    pub fn mesh(uri_id: u32, scale: [f64; 3]) -> Self {
        Self {
            kind: GeometryKind::Mesh,
            mesh: MeshShape { uri_id, _pad: 0, scale },
            ..Self::default()
        }
    }

    pub fn is_box(&self) -> bool { self.kind == GeometryKind::Box }
    pub fn is_sphere(&self) -> bool { self.kind == GeometryKind::Sphere }
    pub fn is_cylinder(&self) -> bool { self.kind == GeometryKind::Cylinder }
    pub fn is_mesh(&self) -> bool { self.kind == GeometryKind::Mesh }

    pub fn as_box(&self) -> Option<&BoxShape> { self.is_box().then_some(&self.box_shape) }
    pub fn as_sphere(&self) -> Option<&SphereShape> { self.is_sphere().then_some(&self.sphere) }
    pub fn as_cylinder(&self) -> Option<&CylinderShape> { self.is_cylinder().then_some(&self.cylinder) }
    pub fn as_mesh(&self) -> Option<&MeshShape> { self.is_mesh().then_some(&self.mesh) }
}
