use std::marker::PhantomData;
use std::rc::Rc;
use std::sync::Arc;

use super::Pair;

#[derive(Debug, Clone, Copy, Default)]
pub struct Conversions<T>(pub PhantomData<T>);

pub fn to_pair<A, B>(left: A, right: B) -> Pair<A, B> {
    (left, right)
}

pub fn into_rc<T>(value: T) -> Rc<T> {
    Rc::new(value)
}

pub fn into_arc<T>(value: T) -> Arc<T> {
    Arc::new(value)
}

pub fn into_box<T>(value: T) -> Box<T> {
    Box::new(value)
}

pub fn option_to_result<T, E>(opt: Option<T>, err: E) -> core::result::Result<T, E> {
    opt.ok_or(err)
}

pub fn option_to_result_with<T, E, F: FnOnce() -> E>(
    opt: Option<T>,
    f: F,
) -> core::result::Result<T, E> {
    opt.ok_or_else(f)
}

pub fn result_to_option<T, E>(res: core::result::Result<T, E>) -> Option<T> {
    res.ok()
}

pub fn result_err_to_option<T, E>(res: core::result::Result<T, E>) -> Option<E> {
    res.err()
}

pub fn transpose_option_result<T, E>(
    opt: Option<core::result::Result<T, E>>,
) -> core::result::Result<Option<T>, E> {
    match opt {
        None => Ok(None),
        Some(Ok(v)) => Ok(Some(v)),
        Some(Err(e)) => Err(e),
    }
}

pub fn transpose_result_option<T, E>(
    res: core::result::Result<Option<T>, E>,
) -> Option<core::result::Result<T, E>> {
    match res {
        Ok(Some(v)) => Some(Ok(v)),
        Ok(None) => None,
        Err(e) => Some(Err(e)),
    }
}

pub fn as_bytes<T: Sized>(value: &T) -> &[u8] {
    unsafe { core::slice::from_raw_parts(value as *const T as *const u8, core::mem::size_of::<T>()) }
}

pub fn swap_endian_u16(value: u16) -> u16 {
    value.swap_bytes()
}

pub fn swap_endian_u32(value: u32) -> u32 {
    value.swap_bytes()
}

pub fn swap_endian_u64(value: u64) -> u64 {
    value.swap_bytes()
}
