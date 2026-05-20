use crate::spatial::Size;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct BoxShape {
    pub size: Size,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct SphereShape {
    pub radius: f64,
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct CylinderShape {
    pub radius: f64,
    pub length: f64,
}

#[derive(Debug, Clone, PartialEq)]
pub struct MeshShape {
    pub uri: String,
    pub scale: [f64; 3],
}

impl Default for MeshShape {
    fn default() -> Self {
        Self {
            uri: String::new(),
            scale: [1.0, 1.0, 1.0],
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum Geometry {
    Box(BoxShape),
    Sphere(SphereShape),
    Cylinder(CylinderShape),
    Mesh(MeshShape),
}

impl Default for Geometry {
    fn default() -> Self {
        Self::Box(BoxShape::default())
    }
}

impl Geometry {
    pub fn box_shape(size: Size) -> Self {
        Self::Box(BoxShape { size })
    }

    pub fn sphere(radius: f64) -> Self {
        Self::Sphere(SphereShape { radius })
    }

    pub fn cylinder(radius: f64, length: f64) -> Self {
        Self::Cylinder(CylinderShape { radius, length })
    }

    pub fn mesh(uri: impl Into<String>, scale: [f64; 3]) -> Self {
        Self::Mesh(MeshShape {
            uri: uri.into(),
            scale,
        })
    }

    pub fn is_box(&self) -> bool {
        matches!(self, Self::Box(_))
    }
    pub fn is_sphere(&self) -> bool {
        matches!(self, Self::Sphere(_))
    }
    pub fn is_cylinder(&self) -> bool {
        matches!(self, Self::Cylinder(_))
    }
    pub fn is_mesh(&self) -> bool {
        matches!(self, Self::Mesh(_))
    }

    pub fn as_box(&self) -> Option<&BoxShape> {
        if let Self::Box(s) = self {
            Some(s)
        } else {
            None
        }
    }
    pub fn as_sphere(&self) -> Option<&SphereShape> {
        if let Self::Sphere(s) = self {
            Some(s)
        } else {
            None
        }
    }
    pub fn as_cylinder(&self) -> Option<&CylinderShape> {
        if let Self::Cylinder(s) = self {
            Some(s)
        } else {
            None
        }
    }
    pub fn as_mesh(&self) -> Option<&MeshShape> {
        if let Self::Mesh(s) = self {
            Some(s)
        } else {
            None
        }
    }

    pub fn as_box_mut(&mut self) -> Option<&mut BoxShape> {
        if let Self::Box(s) = self {
            Some(s)
        } else {
            None
        }
    }
    pub fn as_sphere_mut(&mut self) -> Option<&mut SphereShape> {
        if let Self::Sphere(s) = self {
            Some(s)
        } else {
            None
        }
    }
    pub fn as_cylinder_mut(&mut self) -> Option<&mut CylinderShape> {
        if let Self::Cylinder(s) = self {
            Some(s)
        } else {
            None
        }
    }
    pub fn as_mesh_mut(&mut self) -> Option<&mut MeshShape> {
        if let Self::Mesh(s) = self {
            Some(s)
        } else {
            None
        }
    }
}
