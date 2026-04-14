use std::alloc::{self, Layout};
use std::marker::PhantomData;
use std::ptr::NonNull;

pub trait Allocate<T> {
    fn allocate(&mut self, n: usize) -> Option<NonNull<T>>;
    fn deallocate(&mut self, ptr: NonNull<T>, n: usize);
    fn max_size(&self) -> usize {
        usize::MAX / core::mem::size_of::<T>().max(1)
    }
}

#[derive(Debug, Default)]
pub struct Allocator<T> {
    _marker: PhantomData<T>,
}

impl<T> Allocator<T> {
    pub const fn new() -> Self {
        Self {
            _marker: PhantomData,
        }
    }
}

impl<T> Clone for Allocator<T> {
    fn clone(&self) -> Self {
        Self::new()
    }
}

impl<T> Copy for Allocator<T> {}

impl<T> PartialEq for Allocator<T> {
    fn eq(&self, _other: &Self) -> bool {
        true
    }
}

impl<T> Eq for Allocator<T> {}

impl<T> Allocate<T> for Allocator<T> {
    fn allocate(&mut self, n: usize) -> Option<NonNull<T>> {
        if n == 0 || n > self.max_size() {
            return None;
        }
        let layout = Layout::array::<T>(n).ok()?;
        let ptr = unsafe { alloc::alloc(layout) } as *mut T;
        NonNull::new(ptr)
    }

    fn deallocate(&mut self, ptr: NonNull<T>, n: usize) {
        if n == 0 {
            return;
        }
        if let Ok(layout) = Layout::array::<T>(n) {
            unsafe {
                alloc::dealloc(ptr.as_ptr() as *mut u8, layout);
            }
        }
    }
}

impl<T> Allocator<T> {
    pub fn construct<U>(&self, ptr: *mut U, value: U) {
        unsafe {
            core::ptr::write(ptr, value);
        }
    }

    pub fn destroy<U>(&self, ptr: *mut U) {
        unsafe {
            core::ptr::drop_in_place(ptr);
        }
    }
}
