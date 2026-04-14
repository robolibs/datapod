use super::offset_ptr::OffsetPtr;

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct RawMode;

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct OffsetMode;

pub trait PtrMode {}
impl PtrMode for RawMode {}
impl PtrMode for OffsetMode {}

pub type Ptr<T> = Box<T>;

pub trait IsPtrType {}
impl<T> IsPtrType for Box<T> {}
impl<T> IsPtrType for OffsetPtr<T> {}

pub mod raw {
    pub type Ptr<T> = Box<T>;
}

pub mod offset {
    pub type Ptr<T> = super::OffsetPtr<T>;
}
