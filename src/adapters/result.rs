pub type Result<T, E> = core::result::Result<T, E>;

pub trait ResultExt<T, E> {
    fn is_ok_and_fn<F: FnOnce(&T) -> bool>(&self, predicate: F) -> bool;
    fn is_err_and_fn<F: FnOnce(&E) -> bool>(&self, predicate: F) -> bool;
    fn inspect_fn<F: FnOnce(&T)>(self, f: F) -> core::result::Result<T, E>;
    fn inspect_err_fn<F: FnOnce(&E)>(self, f: F) -> core::result::Result<T, E>;
    fn if_ok<F: FnOnce(&T)>(self, f: F) -> core::result::Result<T, E>;
    fn if_err<F: FnOnce(&E)>(self, f: F) -> core::result::Result<T, E>;
    fn match_fn<R, OkF: FnOnce(T) -> R, ErrF: FnOnce(E) -> R>(self, ok_fn: OkF, err_fn: ErrF) -> R;
    fn value_or_ext(self, default_value: T) -> T;
    fn ref_value(&self) -> &T;
    fn ref_value_mut(&mut self) -> &mut T;
    fn ref_error(&self) -> &E;
    fn ref_error_mut(&mut self) -> &mut E;
}

impl<T, E> ResultExt<T, E> for core::result::Result<T, E> {
    fn is_ok_and_fn<F: FnOnce(&T) -> bool>(&self, predicate: F) -> bool {
        matches!(self, Ok(v) if predicate(v))
    }

    fn is_err_and_fn<F: FnOnce(&E) -> bool>(&self, predicate: F) -> bool {
        matches!(self, Err(e) if predicate(e))
    }

    fn inspect_fn<F: FnOnce(&T)>(self, f: F) -> core::result::Result<T, E> {
        if let Ok(ref v) = self {
            f(v);
        }
        self
    }

    fn inspect_err_fn<F: FnOnce(&E)>(self, f: F) -> core::result::Result<T, E> {
        if let Err(ref e) = self {
            f(e);
        }
        self
    }

    fn if_ok<F: FnOnce(&T)>(self, f: F) -> core::result::Result<T, E> {
        self.inspect_fn(f)
    }

    fn if_err<F: FnOnce(&E)>(self, f: F) -> core::result::Result<T, E> {
        self.inspect_err_fn(f)
    }

    fn match_fn<R, OkF: FnOnce(T) -> R, ErrF: FnOnce(E) -> R>(self, ok_fn: OkF, err_fn: ErrF) -> R {
        match self {
            Ok(v) => ok_fn(v),
            Err(e) => err_fn(e),
        }
    }

    fn value_or_ext(self, default_value: T) -> T {
        self.unwrap_or(default_value)
    }

    fn ref_value(&self) -> &T {
        match self {
            Ok(v) => v,
            Err(_) => panic!("Result::value called on Err"),
        }
    }

    fn ref_value_mut(&mut self) -> &mut T {
        match self {
            Ok(v) => v,
            Err(_) => panic!("Result::value called on Err"),
        }
    }

    fn ref_error(&self) -> &E {
        match self {
            Err(e) => e,
            Ok(_) => panic!("Result::error called on Ok"),
        }
    }

    fn ref_error_mut(&mut self) -> &mut E {
        match self {
            Err(e) => e,
            Ok(_) => panic!("Result::error called on Ok"),
        }
    }
}

pub fn make_ok<T, E>(value: T) -> core::result::Result<T, E> {
    Ok(value)
}

pub fn make_err<T, E>(error: E) -> core::result::Result<T, E> {
    Err(error)
}
