pub type UniquePtr<T> = Box<T>;

pub trait UniquePtrExt<T> {
    fn get_ext(&self) -> &T;
    fn get_mut_ext(&mut self) -> &mut T;
    fn release_ext(self) -> *mut T;
    fn into_raw_ext(self) -> *mut T;
    unsafe fn from_raw_ext(ptr: *mut T) -> Box<T>;
}

impl<T> UniquePtrExt<T> for Box<T> {
    fn get_ext(&self) -> &T {
        self.as_ref()
    }

    fn get_mut_ext(&mut self) -> &mut T {
        self.as_mut()
    }

    fn release_ext(self) -> *mut T {
        Box::into_raw(self)
    }

    fn into_raw_ext(self) -> *mut T {
        Box::into_raw(self)
    }

    unsafe fn from_raw_ext(ptr: *mut T) -> Box<T> {
        unsafe { Box::from_raw(ptr) }
    }
}

pub fn make_unique<T>(value: T) -> Box<T> {
    Box::new(value)
}
