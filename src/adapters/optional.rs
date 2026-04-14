pub type Optional<T> = Option<T>;

pub trait OptionalExt<T> {
    fn has_value(&self) -> bool;
    fn value(&self) -> &T;
    fn value_mut(&mut self) -> &mut T;
    fn value_or(self, default_value: T) -> T;
    fn is_some_and_fn<F: FnOnce(&T) -> bool>(&self, predicate: F) -> bool;
    fn is_none_or_fn<F: FnOnce(&T) -> bool>(&self, predicate: F) -> bool;
    fn filter_fn<F: FnOnce(&T) -> bool>(self, predicate: F) -> Option<T>;
    fn inspect_fn<F: FnOnce(&T)>(self, f: F) -> Option<T>;
    fn expect_msg(self, msg: &str) -> T;
    fn take_ext(&mut self) -> Option<T>;
    fn take_if_fn<F: FnOnce(&T) -> bool>(&mut self, predicate: F) -> Option<T>;
    fn replace_ext(&mut self, value: T) -> Option<T>;
    fn zip_with<U, R, F: FnOnce(T, U) -> R>(self, other: Option<U>, f: F) -> Option<R>;
    fn unwrap_or_default_ext(self) -> T
    where
        T: Default;
    fn get_or_insert_ext(&mut self, value: T) -> &mut T;
    fn get_or_insert_with_fn<F: FnOnce() -> T>(&mut self, f: F) -> &mut T;
    fn reset_ext(&mut self);
    fn emplace_ext(&mut self, value: T) -> &mut T;
    fn ok_or_ext<E>(self, err: E) -> core::result::Result<T, E>;
    fn ok_or_else_ext<E, F: FnOnce() -> E>(self, f: F) -> core::result::Result<T, E>;
}

impl<T> OptionalExt<T> for Option<T> {
    fn has_value(&self) -> bool {
        self.is_some()
    }

    fn value(&self) -> &T {
        self.as_ref().expect("bad optional access")
    }

    fn value_mut(&mut self) -> &mut T {
        self.as_mut().expect("bad optional access")
    }

    fn value_or(self, default_value: T) -> T {
        self.unwrap_or(default_value)
    }

    fn is_some_and_fn<F: FnOnce(&T) -> bool>(&self, predicate: F) -> bool {
        matches!(self, Some(v) if predicate(v))
    }

    fn is_none_or_fn<F: FnOnce(&T) -> bool>(&self, predicate: F) -> bool {
        match self {
            Some(v) => predicate(v),
            None => true,
        }
    }

    fn filter_fn<F: FnOnce(&T) -> bool>(self, predicate: F) -> Option<T> {
        match self {
            Some(v) if predicate(&v) => Some(v),
            _ => None,
        }
    }

    fn inspect_fn<F: FnOnce(&T)>(self, f: F) -> Option<T> {
        if let Some(ref v) = self {
            f(v);
        }
        self
    }

    fn expect_msg(self, msg: &str) -> T {
        self.unwrap_or_else(|| panic!("{}", msg))
    }

    fn take_ext(&mut self) -> Option<T> {
        self.take()
    }

    fn take_if_fn<F: FnOnce(&T) -> bool>(&mut self, predicate: F) -> Option<T> {
        if matches!(self, Some(v) if predicate(v)) {
            self.take()
        } else {
            None
        }
    }

    fn replace_ext(&mut self, value: T) -> Option<T> {
        self.replace(value)
    }

    fn zip_with<U, R, F: FnOnce(T, U) -> R>(self, other: Option<U>, f: F) -> Option<R> {
        match (self, other) {
            (Some(a), Some(b)) => Some(f(a, b)),
            _ => None,
        }
    }

    fn unwrap_or_default_ext(self) -> T
    where
        T: Default,
    {
        self.unwrap_or_default()
    }

    fn get_or_insert_ext(&mut self, value: T) -> &mut T {
        self.get_or_insert(value)
    }

    fn get_or_insert_with_fn<F: FnOnce() -> T>(&mut self, f: F) -> &mut T {
        self.get_or_insert_with(f)
    }

    fn reset_ext(&mut self) {
        *self = None;
    }

    fn emplace_ext(&mut self, value: T) -> &mut T {
        *self = Some(value);
        self.as_mut().unwrap()
    }

    fn ok_or_ext<E>(self, err: E) -> core::result::Result<T, E> {
        self.ok_or(err)
    }

    fn ok_or_else_ext<E, F: FnOnce() -> E>(self, f: F) -> core::result::Result<T, E> {
        self.ok_or_else(f)
    }
}

pub fn make_optional<T>(value: T) -> Option<T> {
    Some(value)
}
