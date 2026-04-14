use crate::Vector;

pub const DEFAULT_ARENA_BLOCK_SIZE: usize = 65536;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Arena<T> {
    values: Vector<T>,
    block_size: usize,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct ArenaHandle {
    pub index: usize,
}

impl<T> Default for Arena<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T> Arena<T> {
    pub fn new() -> Self {
        Self {
            values: Vector::new(),
            block_size: DEFAULT_ARENA_BLOCK_SIZE,
        }
    }

    pub fn with_block_size(block_size: usize) -> Self {
        Self {
            values: Vector::new(),
            block_size,
        }
    }

    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            values: Vector::with_capacity(capacity),
            block_size: DEFAULT_ARENA_BLOCK_SIZE,
        }
    }

    pub fn allocate(&mut self, value: T) -> ArenaHandle {
        let index = self.values.len();
        self.values.push_back(value);
        ArenaHandle { index }
    }

    pub fn get(&self, handle: ArenaHandle) -> Option<&T> {
        self.values.get(handle.index)
    }

    pub fn get_mut(&mut self, handle: ArenaHandle) -> Option<&mut T> {
        self.values.get_mut(handle.index)
    }

    pub fn len(&self) -> usize {
        self.values.len()
    }

    pub fn is_empty(&self) -> bool {
        self.values.is_empty()
    }

    pub fn capacity(&self) -> usize {
        self.values.capacity()
    }

    pub fn block_size(&self) -> usize {
        self.block_size
    }

    pub fn bytes_used(&self) -> usize {
        self.values.len() * core::mem::size_of::<T>()
    }

    pub fn bytes_capacity(&self) -> usize {
        self.values.capacity() * core::mem::size_of::<T>()
    }

    pub fn reset(&mut self) {
        self.values.clear();
    }

    pub fn clear(&mut self) {
        self.values.clear();
        self.values.shrink_to_fit();
    }

    pub fn reserve(&mut self, additional: usize) {
        self.values.reserve(additional);
    }

    pub fn max_size(&self) -> usize {
        usize::MAX / core::mem::size_of::<T>().max(1)
    }

    pub fn iter(&self) -> impl Iterator<Item = &T> {
        self.values.iter()
    }

    pub fn iter_mut(&mut self) -> impl Iterator<Item = &mut T> {
        self.values.iter_mut()
    }
}

impl<T> core::ops::Index<ArenaHandle> for Arena<T> {
    type Output = T;
    fn index(&self, handle: ArenaHandle) -> &T {
        &self.values[handle.index]
    }
}

impl<T> core::ops::IndexMut<ArenaHandle> for Arena<T> {
    fn index_mut(&mut self, handle: ArenaHandle) -> &mut T {
        &mut self.values[handle.index]
    }
}
