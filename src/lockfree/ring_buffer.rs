use std::cell::UnsafeCell;
use std::mem::MaybeUninit;
use std::sync::atomic::{AtomicU64, Ordering};

const MAGIC: u32 = 0x53505343;
const VERSION: u32 = 1;

pub struct RingBuffer<T> {
    write_pos: AtomicU64,
    read_pos: AtomicU64,
    capacity: u64,
    magic: u32,
    version: u32,
    buffer: Box<[UnsafeCell<MaybeUninit<T>>]>,
}

unsafe impl<T: Send> Send for RingBuffer<T> {}
unsafe impl<T: Send> Sync for RingBuffer<T> {}

impl<T> std::fmt::Debug for RingBuffer<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RingBuffer")
            .field("write_pos", &self.write_pos.load(Ordering::Relaxed))
            .field("read_pos", &self.read_pos.load(Ordering::Relaxed))
            .field("capacity", &self.capacity)
            .finish()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Snapshot {
    pub write_pos: u64,
    pub read_pos: u64,
    pub capacity: u64,
    pub magic: u32,
    pub version: u32,
}

impl<T> RingBuffer<T> {
    pub fn new(capacity: usize) -> Self {
        let capacity = if capacity == 0 { 1 } else { capacity };
        let mut buf: Vec<UnsafeCell<MaybeUninit<T>>> = Vec::with_capacity(capacity);
        for _ in 0..capacity {
            buf.push(UnsafeCell::new(MaybeUninit::uninit()));
        }
        Self {
            write_pos: AtomicU64::new(0),
            read_pos: AtomicU64::new(0),
            capacity: capacity as u64,
            magic: MAGIC,
            version: VERSION,
            buffer: buf.into_boxed_slice(),
        }
    }

    pub fn capacity(&self) -> usize {
        self.capacity as usize
    }

    pub fn len(&self) -> usize {
        let w = self.write_pos.load(Ordering::Acquire);
        let r = self.read_pos.load(Ordering::Acquire);
        (w - r) as usize
    }

    pub fn size(&self) -> usize {
        self.len()
    }

    pub fn is_empty(&self) -> bool {
        let r = self.read_pos.load(Ordering::Relaxed);
        let w = self.write_pos.load(Ordering::Acquire);
        w == r
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn is_full(&self) -> bool {
        let w = self.write_pos.load(Ordering::Relaxed);
        let r = self.read_pos.load(Ordering::Acquire);
        (w - r) >= self.capacity
    }

    pub fn full(&self) -> bool {
        self.is_full()
    }

    pub fn push(&self, item: T) -> Result<(), T> {
        let w = self.write_pos.load(Ordering::Relaxed);
        let r = self.read_pos.load(Ordering::Acquire);
        if w - r >= self.capacity {
            return Err(item);
        }
        let idx = (w % self.capacity) as usize;
        unsafe {
            let slot = &mut *self.buffer[idx].get();
            *slot = MaybeUninit::new(item);
        }
        std::sync::atomic::fence(Ordering::Release);
        self.write_pos.store(w + 1, Ordering::Release);
        Ok(())
    }

    pub fn pop(&self) -> Option<T> {
        let r = self.read_pos.load(Ordering::Relaxed);
        let w = self.write_pos.load(Ordering::Acquire);
        if w == r {
            return None;
        }
        let idx = (r % self.capacity) as usize;
        let item = unsafe {
            let slot = &mut *self.buffer[idx].get();
            std::mem::replace(slot, MaybeUninit::uninit()).assume_init()
        };
        self.read_pos.store(r + 1, Ordering::Release);
        Some(item)
    }

    pub fn drain(&self) -> Vec<T> {
        let mut out = Vec::new();
        while let Some(v) = self.pop() {
            out.push(v);
        }
        out
    }

    pub fn snapshot(&self) -> Snapshot {
        Snapshot {
            write_pos: self.write_pos.load(Ordering::Acquire),
            read_pos: self.read_pos.load(Ordering::Acquire),
            capacity: self.capacity,
            magic: self.magic,
            version: self.version,
        }
    }
}

impl<T> Drop for RingBuffer<T> {
    fn drop(&mut self) {
        while self.pop().is_some() {}
    }
}

impl<T> Default for RingBuffer<T> {
    fn default() -> Self {
        Self::new(1)
    }
}

impl<T: PartialEq> PartialEq for RingBuffer<T> {
    fn eq(&self, other: &Self) -> bool {
        self.capacity == other.capacity
            && self.write_pos.load(Ordering::Acquire) == other.write_pos.load(Ordering::Acquire)
            && self.read_pos.load(Ordering::Acquire) == other.read_pos.load(Ordering::Acquire)
    }
}
