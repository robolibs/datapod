use crate::Vector;

pub const DEFAULT_POOL_CHUNK_SIZE: usize = 64;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct PoolHandle {
    pub index: usize,
}

#[derive(Debug, Clone)]
enum Slot<T> {
    Occupied(T),
    Free(Option<usize>),
}

#[derive(Debug, Clone)]
pub struct Pool<T> {
    slots: Vector<Slot<T>>,
    free_head: Option<usize>,
    allocated_count: usize,
    chunk_size: usize,
}

impl<T> Default for Pool<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T> Pool<T> {
    pub fn new() -> Self {
        Self {
            slots: Vector::new(),
            free_head: None,
            allocated_count: 0,
            chunk_size: DEFAULT_POOL_CHUNK_SIZE,
        }
    }

    pub fn with_chunk_size(chunk_size: usize) -> Self {
        Self {
            slots: Vector::new(),
            free_head: None,
            allocated_count: 0,
            chunk_size,
        }
    }

    pub fn allocate(&mut self, value: T) -> PoolHandle {
        self.allocated_count += 1;
        if let Some(idx) = self.free_head {
            let next = match &self.slots[idx] {
                Slot::Free(n) => *n,
                Slot::Occupied(_) => unreachable!(),
            };
            self.free_head = next;
            self.slots[idx] = Slot::Occupied(value);
            PoolHandle { index: idx }
        } else {
            let idx = self.slots.len();
            self.slots.push_back(Slot::Occupied(value));
            PoolHandle { index: idx }
        }
    }

    pub fn deallocate(&mut self, handle: PoolHandle) -> Option<T> {
        let idx = handle.index;
        if idx >= self.slots.len() {
            return None;
        }
        let old = core::mem::replace(&mut self.slots[idx], Slot::Free(self.free_head));
        match old {
            Slot::Occupied(v) => {
                self.free_head = Some(idx);
                if self.allocated_count > 0 {
                    self.allocated_count -= 1;
                }
                Some(v)
            }
            Slot::Free(prev) => {
                self.slots[idx] = Slot::Free(prev);
                None
            }
        }
    }

    pub fn get(&self, handle: PoolHandle) -> Option<&T> {
        match self.slots.get(handle.index)? {
            Slot::Occupied(v) => Some(v),
            Slot::Free(_) => None,
        }
    }

    pub fn get_mut(&mut self, handle: PoolHandle) -> Option<&mut T> {
        match self.slots.get_mut(handle.index)? {
            Slot::Occupied(v) => Some(v),
            Slot::Free(_) => None,
        }
    }

    pub fn allocated_count(&self) -> usize {
        self.allocated_count
    }

    pub fn free_count(&self) -> usize {
        let mut count = 0usize;
        let mut head = self.free_head;
        while let Some(idx) = head {
            count += 1;
            match &self.slots[idx] {
                Slot::Free(next) => head = *next,
                Slot::Occupied(_) => break,
            }
        }
        count
    }

    pub fn capacity(&self) -> usize {
        self.slots.len()
    }

    pub fn chunk_size(&self) -> usize {
        self.chunk_size
    }

    pub fn chunk_count(&self) -> usize {
        if self.chunk_size == 0 {
            return 0;
        }
        self.slots.len().div_ceil(self.chunk_size)
    }

    pub fn clear(&mut self) {
        self.slots.clear();
        self.free_head = None;
        self.allocated_count = 0;
    }

    pub fn max_size(&self) -> usize {
        usize::MAX / core::mem::size_of::<T>().max(1)
    }
}

impl<T: PartialEq> PartialEq for Pool<T> {
    fn eq(&self, _other: &Self) -> bool {
        false
    }
}
