pub type SharedPtr<T> = std::sync::Arc<T>;
pub type WeakPtr<T> = std::sync::Weak<T>;

pub trait SharedPtrExt<T> {
    fn make(value: T) -> std::sync::Arc<T>;
    fn get_ext(&self) -> &T;
    fn use_count_ext(&self) -> usize;
    fn weak_count_ext(&self) -> usize;
    fn unique_ext(&self) -> bool;
    fn downgrade_ext(&self) -> std::sync::Weak<T>;
}

impl<T> SharedPtrExt<T> for std::sync::Arc<T> {
    fn make(value: T) -> std::sync::Arc<T> {
        std::sync::Arc::new(value)
    }

    fn get_ext(&self) -> &T {
        self.as_ref()
    }

    fn use_count_ext(&self) -> usize {
        std::sync::Arc::strong_count(self)
    }

    fn weak_count_ext(&self) -> usize {
        std::sync::Arc::weak_count(self)
    }

    fn unique_ext(&self) -> bool {
        std::sync::Arc::strong_count(self) == 1
    }

    fn downgrade_ext(&self) -> std::sync::Weak<T> {
        std::sync::Arc::downgrade(self)
    }
}

pub trait WeakPtrExt<T> {
    fn lock_ext(&self) -> Option<std::sync::Arc<T>>;
    fn use_count_ext(&self) -> usize;
    fn expired_ext(&self) -> bool;
}

impl<T> WeakPtrExt<T> for std::sync::Weak<T> {
    fn lock_ext(&self) -> Option<std::sync::Arc<T>> {
        self.upgrade()
    }

    fn use_count_ext(&self) -> usize {
        std::sync::Weak::strong_count(self)
    }

    fn expired_ext(&self) -> bool {
        self.upgrade().is_none()
    }
}

pub fn make_shared<T>(value: T) -> std::sync::Arc<T> {
    std::sync::Arc::new(value)
}
