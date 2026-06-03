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
        Self {
            uri_id: STRING_NONE,
            _pad: 0,
            scale: [1.0, 1.0, 1.0],
        }
    }
}

/// Tag for the active variant of a [`Geometry`] record.
///
/// This is a transparent newtype instead of a Rust enum so untrusted wire
/// bytes can be copied into a header and then semantically validated.
#[repr(transparent)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct GeometryKind(pub u32);

#[allow(non_upper_case_globals)]
impl GeometryKind {
    pub const Box: Self = Self(0);
    pub const Sphere: Self = Self(1);
    pub const Cylinder: Self = Self(2);
    pub const Mesh: Self = Self(3);

    pub fn is_valid(self) -> bool {
        self.0 <= Self::Mesh.0
    }
}

unsafe impl bytemuck::Zeroable for GeometryKind {}
unsafe impl bytemuck::Pod for GeometryKind {}
unsafe impl crate::ZeroCopySend for GeometryKind {}
impl crate::LeWireHeader for GeometryKind {
    const LE_WIRE_SIZE: usize = <u32 as crate::LeWireHeader>::LE_WIRE_SIZE;

    fn write_le(&self, out: &mut Vec<u8>) {
        <u32 as crate::LeWireHeader>::write_le(&self.0, out);
    }

    fn read_le(bytes: &[u8]) -> Result<Self, crate::WireError> {
        Ok(Self(<u32 as crate::LeWireHeader>::read_le(bytes)?))
    }
}

impl crate::DataPod for GeometryKind {
    type Header = GeometryKind;
    type Payload = ();
    fn header(&self) -> GeometryKind {
        *self
    }
    fn payload_bytes(&self) -> &[u8] {
        &[]
    }
}

impl crate::DataPodDecode for GeometryKind {
    fn from_wire_parts(header: Self::Header, payload: Vec<u8>) -> Result<Self, crate::WireError> {
        <Self as crate::DataPodValidate>::validate_wire_parts(&header, &payload)?;
        Ok(header)
    }
}

impl crate::DataPodValidate for GeometryKind {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), crate::WireError> {
        if !payload.is_empty() {
            return Err(crate::WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            });
        }
        if !header.is_valid() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "unknown geometry kind tag {}",
                header.0
            )));
        }
        Ok(())
    }
}

impl crate::DataPodAccess for GeometryKind {
    type View<'a> = crate::FixedView<Self>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, crate::WireError> {
        <Self as crate::DataPodValidate>::validate_wire_parts(&header, payload)?;
        Ok(crate::FixedView { value: header })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        _payload: &'a [u8],
    ) -> Self::View<'a> {
        crate::FixedView { value: header }
    }
}

/// Geometry record — a tag plus all four variant payloads inline.
/// Only the variant indicated by `kind` is meaningful; the others are
/// zeroed. This is the Pod equivalent of the C++/Rust `enum` variants.
#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Geometry {
    pub kind: GeometryKind,
    pub _pad: u32,
    pub box_shape: BoxShape,
    pub sphere: SphereShape,
    pub cylinder: CylinderShape,
    pub mesh: MeshShape,
}

impl crate::DataPodValidate for Geometry {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), crate::WireError> {
        if !payload.is_empty() {
            return Err(crate::WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            });
        }
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        if !header.kind.is_valid() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "unknown geometry kind tag {}",
                header.kind.0
            )));
        }
        if header.sphere.radius < 0.0 || !header.sphere.radius.is_finite() {
            return Err(crate::wire::invalid_header::<Self>(
                "sphere radius must be finite and non-negative",
            ));
        }
        if header.cylinder.radius < 0.0
            || !header.cylinder.radius.is_finite()
            || header.cylinder.length < 0.0
            || !header.cylinder.length.is_finite()
        {
            return Err(crate::wire::invalid_header::<Self>(
                "cylinder radius/length must be finite and non-negative",
            ));
        }
        if header.mesh._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "mesh reserved _pad field must be zero",
            ));
        }
        if header.mesh.scale.iter().any(|value| !value.is_finite()) {
            return Err(crate::wire::invalid_header::<Self>(
                "mesh scale must contain finite values",
            ));
        }
        Ok(())
    }
}

impl crate::DataPodAccess for Geometry {
    type View<'a> = crate::FixedView<Self>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, crate::WireError> {
        <Self as crate::DataPodValidate>::validate_wire_parts(&header, payload)?;
        Ok(crate::FixedView { value: header })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        _payload: &'a [u8],
    ) -> Self::View<'a> {
        crate::FixedView { value: header }
    }
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
            mesh: MeshShape {
                uri_id,
                _pad: 0,
                scale,
            },
            ..Self::default()
        }
    }

    pub fn is_box(&self) -> bool {
        self.kind == GeometryKind::Box
    }
    pub fn is_sphere(&self) -> bool {
        self.kind == GeometryKind::Sphere
    }
    pub fn is_cylinder(&self) -> bool {
        self.kind == GeometryKind::Cylinder
    }
    pub fn is_mesh(&self) -> bool {
        self.kind == GeometryKind::Mesh
    }

    pub fn as_box(&self) -> Option<&BoxShape> {
        self.is_box().then_some(&self.box_shape)
    }
    pub fn as_sphere(&self) -> Option<&SphereShape> {
        self.is_sphere().then_some(&self.sphere)
    }
    pub fn as_cylinder(&self) -> Option<&CylinderShape> {
        self.is_cylinder().then_some(&self.cylinder)
    }
    pub fn as_mesh(&self) -> Option<&MeshShape> {
        self.is_mesh().then_some(&self.mesh)
    }
}
