pub type NonNull<T> = std::ptr::NonNull<T>;

pub trait NonNullExt<T> {
    fn from_ref_ext(reference: &T) -> std::ptr::NonNull<T>;
    fn get_ext(&self) -> *mut T;
    unsafe fn deref_ext(&self) -> &T;
    unsafe fn deref_mut_ext(&mut self) -> &mut T;
    fn cast_to<U>(self) -> std::ptr::NonNull<U>;
}

impl<T> NonNullExt<T> for std::ptr::NonNull<T> {
    fn from_ref_ext(reference: &T) -> std::ptr::NonNull<T> {
        std::ptr::NonNull::from(reference)
    }

    fn get_ext(&self) -> *mut T {
        self.as_ptr()
    }

    unsafe fn deref_ext(&self) -> &T {
        unsafe { self.as_ref() }
    }

    unsafe fn deref_mut_ext(&mut self) -> &mut T {
        unsafe { self.as_mut() }
    }

    fn cast_to<U>(self) -> std::ptr::NonNull<U> {
        self.cast::<U>()
    }
}

pub fn make_non_null<T>(ptr: *mut T) -> std::ptr::NonNull<T> {
    std::ptr::NonNull::new(ptr).expect("NonNull constructed with nullptr")
}
