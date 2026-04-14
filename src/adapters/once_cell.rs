pub type OnceCell<T> = std::cell::OnceCell<T>;
pub type SyncOnceCell<T> = std::sync::OnceLock<T>;

pub trait OnceCellExt<T> {
    fn get_ext(&self) -> Option<&T>;
    fn set_ext(&self, value: T) -> core::result::Result<(), T>;
    fn get_or_init_ext<F: FnOnce() -> T>(&self, f: F) -> &T;
    fn is_initialized(&self) -> bool;
    fn into_inner_ext(self) -> Option<T>;
}

impl<T> OnceCellExt<T> for std::cell::OnceCell<T> {
    fn get_ext(&self) -> Option<&T> {
        self.get()
    }

    fn set_ext(&self, value: T) -> core::result::Result<(), T> {
        self.set(value)
    }

    fn get_or_init_ext<F: FnOnce() -> T>(&self, f: F) -> &T {
        self.get_or_init(f)
    }

    fn is_initialized(&self) -> bool {
        self.get().is_some()
    }

    fn into_inner_ext(self) -> Option<T> {
        self.into_inner()
    }
}

pub trait SyncOnceCellExt<T> {
    fn get_ext(&self) -> Option<&T>;
    fn set_ext(&self, value: T) -> core::result::Result<(), T>;
    fn get_or_init_ext<F: FnOnce() -> T>(&self, f: F) -> &T;
    fn is_initialized(&self) -> bool;
    fn into_inner_ext(self) -> Option<T>;
}

impl<T> SyncOnceCellExt<T> for std::sync::OnceLock<T> {
    fn get_ext(&self) -> Option<&T> {
        self.get()
    }

    fn set_ext(&self, value: T) -> core::result::Result<(), T> {
        self.set(value)
    }

    fn get_or_init_ext<F: FnOnce() -> T>(&self, f: F) -> &T {
        self.get_or_init(f)
    }

    fn is_initialized(&self) -> bool {
        self.get().is_some()
    }

    fn into_inner_ext(self) -> Option<T> {
        self.into_inner()
    }
}
