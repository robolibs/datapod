pub type RefCell<T> = std::cell::RefCell<T>;
pub type Ref<'a, T> = std::cell::Ref<'a, T>;
pub type RefMut<'a, T> = std::cell::RefMut<'a, T>;

pub trait RefCellExt<T> {
    fn borrow_ext(&self) -> std::cell::Ref<'_, T>;
    fn borrow_mut_ext(&self) -> std::cell::RefMut<'_, T>;
    fn try_borrow_ext(&self) -> core::result::Result<std::cell::Ref<'_, T>, std::cell::BorrowError>;
    fn try_borrow_mut_ext(
        &self,
    ) -> core::result::Result<std::cell::RefMut<'_, T>, std::cell::BorrowMutError>;
    fn replace_ext(&self, value: T) -> T;
    fn swap_ext(&self, other: &std::cell::RefCell<T>);
    fn take_ext(&self) -> T
    where
        T: Default;
    fn set_ext(&self, value: T);
    fn get_copy(&self) -> T
    where
        T: Clone;
    fn is_borrowed_mut(&self) -> bool;
}

impl<T> RefCellExt<T> for std::cell::RefCell<T> {
    fn borrow_ext(&self) -> std::cell::Ref<'_, T> {
        self.borrow()
    }

    fn borrow_mut_ext(&self) -> std::cell::RefMut<'_, T> {
        self.borrow_mut()
    }

    fn try_borrow_ext(&self) -> core::result::Result<std::cell::Ref<'_, T>, std::cell::BorrowError> {
        self.try_borrow()
    }

    fn try_borrow_mut_ext(
        &self,
    ) -> core::result::Result<std::cell::RefMut<'_, T>, std::cell::BorrowMutError> {
        self.try_borrow_mut()
    }

    fn replace_ext(&self, value: T) -> T {
        self.replace(value)
    }

    fn swap_ext(&self, other: &std::cell::RefCell<T>) {
        self.swap(other)
    }

    fn take_ext(&self) -> T
    where
        T: Default,
    {
        self.take()
    }

    fn set_ext(&self, value: T) {
        *self.borrow_mut() = value;
    }

    fn get_copy(&self) -> T
    where
        T: Clone,
    {
        self.borrow().clone()
    }

    fn is_borrowed_mut(&self) -> bool {
        self.try_borrow().is_err()
    }
}
