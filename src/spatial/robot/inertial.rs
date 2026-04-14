use crate::spatial::{Point, Pose};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Inertial {
    pub origin: Pose,
    pub mass: f64,
    pub ixx: f64,
    pub ixy: f64,
    pub ixz: f64,
    pub iyy: f64,
    pub iyz: f64,
    pub izz: f64,
}

impl Inertial {
    pub fn new(
        origin: Pose,
        mass: f64,
        ixx: f64,
        ixy: f64,
        ixz: f64,
        iyy: f64,
        iyz: f64,
        izz: f64,
    ) -> Self {
        Self { origin, mass, ixx, ixy, ixz, iyy, iyz, izz }
    }

    /// Create inertial with only position (identity rotation)
    pub fn at_point(
        com: Point,
        mass: f64,
        ixx: f64,
        ixy: f64,
        ixz: f64,
        iyy: f64,
        iyz: f64,
        izz: f64,
    ) -> Self {
        Self {
            origin: Pose { point: com, rotation: Default::default() },
            mass,
            ixx,
            ixy,
            ixz,
            iyy,
            iyz,
            izz,
        }
    }

    /// Create inertial with diagonal tensor (no products of inertia)
    pub fn diagonal(origin: Pose, mass: f64, ixx: f64, iyy: f64, izz: f64) -> Self {
        Self { origin, mass, ixx, ixy: 0.0, ixz: 0.0, iyy, iyz: 0.0, izz }
    }

    /// Create inertial for a point mass
    pub fn point_mass(mass: f64, com: Point) -> Self {
        Self {
            origin: Pose { point: com, rotation: Default::default() },
            mass,
            ixx: 0.0,
            ixy: 0.0,
            ixz: 0.0,
            iyy: 0.0,
            iyz: 0.0,
            izz: 0.0,
        }
    }

    /// Create inertial for a uniform sphere at origin
    pub fn sphere(mass: f64, radius: f64) -> Self {
        let i = 0.4 * mass * radius * radius;
        Self {
            origin: Pose::default(),
            mass,
            ixx: i,
            ixy: 0.0,
            ixz: 0.0,
            iyy: i,
            iyz: 0.0,
            izz: i,
        }
    }

    /// Create inertial for a uniform box at origin
    pub fn box_inertial(mass: f64, width: f64, height: f64, depth: f64) -> Self {
        let ixx = (mass / 12.0) * (height * height + depth * depth);
        let iyy = (mass / 12.0) * (width * width + depth * depth);
        let izz = (mass / 12.0) * (width * width + height * height);
        Self { origin: Pose::default(), mass, ixx, ixy: 0.0, ixz: 0.0, iyy, iyz: 0.0, izz }
    }

    /// Create inertial for a uniform cylinder at origin (axis along z)
    pub fn cylinder(mass: f64, radius: f64, height: f64) -> Self {
        let ixx = (mass / 12.0) * (3.0 * radius * radius + height * height);
        let iyy = ixx;
        let izz = 0.5 * mass * radius * radius;
        Self { origin: Pose::default(), mass, ixx, ixy: 0.0, ixz: 0.0, iyy, iyz: 0.0, izz }
    }

    pub fn is_set(&self) -> bool {
        self.mass != 0.0
            || self.origin.is_set()
            || self.ixx != 0.0
            || self.iyy != 0.0
            || self.izz != 0.0
    }

    pub fn trace(&self) -> f64 {
        self.ixx + self.iyy + self.izz
    }

    pub fn is_diagonal(&self) -> bool {
        self.ixy == 0.0 && self.ixz == 0.0 && self.iyz == 0.0
    }
}
