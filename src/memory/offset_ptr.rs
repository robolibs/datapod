use std::marker::PhantomData;

pub const NULL_OFFSET: isize = isize::MIN;

#[derive(Debug)]
pub struct OffsetPtr<T> {
    offset: isize,
    marker: PhantomData<*const T>,
}

impl<T> OffsetPtr<T> {
    pub const fn new(offset: usize) -> Self {
        Self {
            offset: offset as isize,
            marker: PhantomData,
        }
    }

    pub const fn null() -> Self {
        Self {
            offset: NULL_OFFSET,
            marker: PhantomData,
        }
    }

    pub const fn from_offset(offset: isize) -> Self {
        Self {
            offset,
            marker: PhantomData,
        }
    }

    pub const fn from_index(index: usize) -> Self {
        Self {
            offset: index as isize,
            marker: PhantomData,
        }
    }

    pub fn offset(&self) -> isize {
        self.offset
    }

    pub fn set_offset(&mut self, offset: isize) {
        self.offset = offset;
    }

    pub fn is_null(&self) -> bool {
        self.offset == NULL_OFFSET
    }

    pub fn index(&self) -> Option<usize> {
        if self.is_null() || self.offset < 0 {
            None
        } else {
            Some(self.offset as usize)
        }
    }

    pub fn get<'a>(&self, base: &'a [T]) -> Option<&'a T> {
        base.get(self.index()?)
    }

    pub fn get_mut<'a>(&self, base: &'a mut [T]) -> Option<&'a mut T> {
        let idx = self.index()?;
        base.get_mut(idx)
    }

    pub fn increment(&mut self) {
        if !self.is_null() {
            self.offset += 1;
        }
    }

    pub fn decrement(&mut self) {
        if !self.is_null() {
            self.offset -= 1;
        }
    }

    pub fn add(&self, n: isize) -> Self {
        if self.is_null() {
            Self::null()
        } else {
            Self::from_offset(self.offset + n)
        }
    }

    pub fn sub(&self, n: isize) -> Self {
        if self.is_null() {
            Self::null()
        } else {
            Self::from_offset(self.offset - n)
        }
    }

    pub fn difference(&self, other: &Self) -> Option<isize> {
        if self.is_null() || other.is_null() {
            None
        } else {
            Some(self.offset - other.offset)
        }
    }
}

impl<T> Default for OffsetPtr<T> {
    fn default() -> Self {
        Self::null()
    }
}

impl<T> Clone for OffsetPtr<T> {
    fn clone(&self) -> Self {
        *self
    }
}

impl<T> Copy for OffsetPtr<T> {}

impl<T> PartialEq for OffsetPtr<T> {
    fn eq(&self, other: &Self) -> bool {
        self.offset == other.offset
    }
}

impl<T> Eq for OffsetPtr<T> {}

impl<T> PartialOrd for OffsetPtr<T> {
    fn partial_cmp(&self, other: &Self) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl<T> Ord for OffsetPtr<T> {
    fn cmp(&self, other: &Self) -> core::cmp::Ordering {
        self.offset.cmp(&other.offset)
    }
}

impl<T> core::hash::Hash for OffsetPtr<T> {
    fn hash<H: core::hash::Hasher>(&self, state: &mut H) {
        self.offset.hash(state);
    }
}
