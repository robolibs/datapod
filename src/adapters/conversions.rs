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
