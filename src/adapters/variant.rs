#[derive(Debug, Clone, PartialEq, Eq, Default, Hash)]
pub struct Variant<T>(pub T);

impl<T> Variant<T> {
    pub fn new(value: T) -> Self {
        Self(value)
    }

    pub fn valid(&self) -> bool {
        true
    }

    pub fn index(&self) -> usize {
        0
    }

    pub fn get(&self) -> &T {
        &self.0
    }

    pub fn get_mut(&mut self) -> &mut T {
        &mut self.0
    }

    pub fn into_inner(self) -> T {
        self.0
    }

    pub fn emplace(&mut self, value: T) -> &mut T {
        self.0 = value;
        &mut self.0
    }

    pub fn apply<R, F: FnOnce(&T) -> R>(&self, f: F) -> R {
        f(&self.0)
    }

    pub fn apply_mut<R, F: FnOnce(&mut T) -> R>(&mut self, f: F) -> R {
        f(&mut self.0)
    }

    pub fn swap(&mut self, other: &mut Self) {
        core::mem::swap(&mut self.0, &mut other.0);
    }
}

impl<T> From<T> for Variant<T> {
    fn from(value: T) -> Self {
        Self(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Variant2<A, B> {
    V0(A),
    V1(B),
}

impl<A, B> Variant2<A, B> {
    pub fn index(&self) -> usize {
        match self {
            Self::V0(_) => 0,
            Self::V1(_) => 1,
        }
    }

    pub fn valid(&self) -> bool {
        true
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Variant3<A, B, C> {
    V0(A),
    V1(B),
    V2(C),
}

impl<A, B, C> Variant3<A, B, C> {
    pub fn index(&self) -> usize {
        match self {
            Self::V0(_) => 0,
            Self::V1(_) => 1,
            Self::V2(_) => 2,
        }
    }

    pub fn valid(&self) -> bool {
        true
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Variant4<A, B, C, D> {
    V0(A),
    V1(B),
    V2(C),
    V3(D),
}

impl<A, B, C, D> Variant4<A, B, C, D> {
    pub fn index(&self) -> usize {
        match self {
            Self::V0(_) => 0,
            Self::V1(_) => 1,
            Self::V2(_) => 2,
            Self::V3(_) => 3,
        }
    }

    pub fn valid(&self) -> bool {
        true
    }
}
