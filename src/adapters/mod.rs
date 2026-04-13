use std::borrow::Cow as StdCow;
use std::cell::{OnceCell as CellOnceCell, RefCell as StdRefCell};
use std::marker::PhantomData;
use std::mem::MaybeUninit as StdMaybeUninit;
use std::pin::Pin as StdPin;
use std::ptr::NonNull as StdNonNull;
use std::rc::Rc;
use std::sync::{Arc, OnceLock};

pub type Optional<T> = Option<T>;
pub type Result<T, E> = std::result::Result<T, E>;
pub type Pair<A, B> = (A, B);
pub type Tuple<T> = T;
pub type SharedPtr<T> = Arc<T>;
pub type UniquePtr<T> = Box<T>;
pub type RefCell<T> = StdRefCell<T>;
pub type NonNull<T> = StdNonNull<T>;
pub type MaybeUninit<T> = StdMaybeUninit<T>;
pub type Pin<T> = StdPin<T>;
pub type Cow<'a, T> = StdCow<'a, T>;
pub type OnceCell<T> = CellOnceCell<T>;
pub type SyncOnceCell<T> = OnceLock<T>;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Either<L, R> {
    Left(L),
    Right(R),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error<E = String> {
    pub message: E,
}

impl<E> Error<E> {
    pub fn new(message: E) -> Self {
        Self { message }
    }
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct Lazy<T, F = fn() -> T> {
    init: F,
    value: Option<T>,
}

impl<T, F> Lazy<T, F>
where
    F: FnOnce() -> T + Clone,
{
    pub fn new(init: F) -> Self {
        Self { init, value: None }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Bitset<const N: usize> {
    bits: [bool; N],
}

impl<const N: usize> Default for Bitset<N> {
    fn default() -> Self {
        Self { bits: [false; N] }
    }
}

impl<const N: usize> Bitset<N> {
    pub fn set(&mut self, index: usize, value: bool) {
        self.bits[index] = value;
    }

    pub fn get(&self, index: usize) -> bool {
        self.bits[index]
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Variant<T>(pub T);

pub fn to_pair<A, B>(left: A, right: B) -> Pair<A, B> {
    (left, right)
}

pub fn into_rc<T>(value: T) -> Rc<T> {
    Rc::new(value)
}

pub fn into_arc<T>(value: T) -> Arc<T> {
    Arc::new(value)
}

#[derive(Debug, Clone, Copy, Default)]
pub struct Conversions<T>(PhantomData<T>);
