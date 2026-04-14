pub type MaybeUninit<T> = std::mem::MaybeUninit<T>;

pub trait MaybeUninitExt<T> {
    fn uninit() -> Self;
    fn init(value: T) -> Self;
    fn write_value(&mut self, value: T) -> &mut T;
    fn as_ptr_ext(&self) -> *const T;
    fn as_mut_ptr_ext(&mut self) -> *mut T;
    unsafe fn assume_init_ref_ext(&self) -> &T;
    unsafe fn assume_init_mut_ext(&mut self) -> &mut T;
    unsafe fn assume_init_ext(self) -> T;
    unsafe fn drop_ext(&mut self);
    fn zeroed_ext() -> Self;
}

impl<T> MaybeUninitExt<T> for std::mem::MaybeUninit<T> {
    fn uninit() -> Self {
        std::mem::MaybeUninit::uninit()
    }

    fn init(value: T) -> Self {
        std::mem::MaybeUninit::new(value)
    }

    fn write_value(&mut self, value: T) -> &mut T {
        self.write(value)
    }

    fn as_ptr_ext(&self) -> *const T {
        self.as_ptr()
    }

    fn as_mut_ptr_ext(&mut self) -> *mut T {
        self.as_mut_ptr()
    }

    unsafe fn assume_init_ref_ext(&self) -> &T {
        unsafe { self.assume_init_ref() }
    }

    unsafe fn assume_init_mut_ext(&mut self) -> &mut T {
        unsafe { self.assume_init_mut() }
    }

    unsafe fn assume_init_ext(self) -> T {
        unsafe { self.assume_init() }
    }

    unsafe fn drop_ext(&mut self) {
        unsafe {
            core::ptr::drop_in_place(self.as_mut_ptr());
        }
    }

    fn zeroed_ext() -> Self {
        std::mem::MaybeUninit::zeroed()
    }
}

pub fn uninit<T>() -> std::mem::MaybeUninit<T> {
    std::mem::MaybeUninit::uninit()
}

pub fn init<T>(value: T) -> std::mem::MaybeUninit<T> {
    std::mem::MaybeUninit::new(value)
}
