use std::cell::UnsafeCell;
use std::sync::Mutex;

pub struct Lazy<T, F = fn() -> T> {
    value: UnsafeCell<Option<T>>,
    initializer: Mutex<Option<F>>,
}

unsafe impl<T: Send + Sync, F: Send> Sync for Lazy<T, F> {}
unsafe impl<T: Send, F: Send> Send for Lazy<T, F> {}

impl<T, F: FnOnce() -> T> Lazy<T, F> {
    pub fn new(init: F) -> Self {
        Self {
            value: UnsafeCell::new(None),
            initializer: Mutex::new(Some(init)),
        }
    }

    pub fn is_initialized(&self) -> bool {
        unsafe { (*self.value.get()).is_some() }
    }

    pub fn get(&self) -> &T {
        self.force();
        unsafe { (*self.value.get()).as_ref().unwrap() }
    }

    pub fn get_mut(&mut self) -> &mut T {
        self.force();
        unsafe { (*self.value.get()).as_mut().unwrap() }
    }

    pub fn force(&self) {
        let mut init_guard = self.initializer.lock().unwrap();
        if unsafe { (*self.value.get()).is_none() } {
            if let Some(init) = init_guard.take() {
                unsafe {
                    *self.value.get() = Some(init());
                }
            }
        }
    }

    pub fn take(&mut self) -> Option<T> {
        unsafe { (*self.value.get()).take() }
    }

    pub fn peek(&self) -> Option<&T> {
        unsafe { (*self.value.get()).as_ref() }
    }

    pub fn reset(&self)
    where
        F: Clone,
    {
        let mut init_guard = self.initializer.lock().unwrap();
        unsafe {
            *self.value.get() = None;
        }
        // Note: without a reinit, reset just clears the value.
        // Caller should use LazyWithReset if they need re-initialization.
        drop(init_guard);
    }
}

impl<T: std::fmt::Debug, F> std::fmt::Debug for Lazy<T, F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        unsafe {
            match &*self.value.get() {
                Some(v) => f.debug_struct("Lazy").field("value", v).finish(),
                None => f.debug_struct("Lazy").field("value", &"<uninit>").finish(),
            }
        }
    }
}

pub fn make_lazy<T, F: FnOnce() -> T>(f: F) -> Lazy<T, F> {
    Lazy::new(f)
}
