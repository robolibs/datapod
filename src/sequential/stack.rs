//! LIFO stack matching `datapod::Stack<T>` from the C++ library.
//!
//! The C++ type is a thin adapter over `Vector<T>` with a throwing
//! `top()` / `pop()` pair. We keep the public alias `Stack<T> = Vec<T>` so
//! existing call sites that initialise the stack with `vec![..]` still
//! compile, and provide the throwing-style methods via an extension trait.

pub type Stack<T> = Vec<T>;

/// Alias matching the `Lifo<T>` typedef exported by the C++ header.
pub type Lifo<T> = Stack<T>;

/// C++-style API for a LIFO stack backed by `Vec<T>`.
pub trait StackExt<T> {
    fn push_value(&mut self, value: T);
    fn pop_value(&mut self) -> T;
    fn try_pop(&mut self) -> Option<T>;
    fn top(&self) -> &T;
    fn top_mut(&mut self) -> &mut T;
    fn try_top(&self) -> Option<&T>;
    fn try_top_mut(&mut self) -> Option<&mut T>;
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn clear_stack(&mut self);
}

impl<T> StackExt<T> for Vec<T> {
    #[inline]
    fn push_value(&mut self, value: T) {
        self.push(value);
    }

    #[inline]
    fn pop_value(&mut self) -> T {
        self.pop().expect("Stack::pop: empty")
    }

    #[inline]
    fn try_pop(&mut self) -> Option<T> {
        self.pop()
    }

    #[inline]
    fn top(&self) -> &T {
        self.last().expect("Stack::top: empty")
    }

    #[inline]
    fn top_mut(&mut self) -> &mut T {
        self.last_mut().expect("Stack::top: empty")
    }

    #[inline]
    fn try_top(&self) -> Option<&T> {
        self.last()
    }

    #[inline]
    fn try_top_mut(&mut self) -> Option<&mut T> {
        self.last_mut()
    }

    #[inline]
    fn size(&self) -> usize {
        self.len()
    }

    #[inline]
    fn empty(&self) -> bool {
        self.is_empty()
    }

    #[inline]
    fn clear_stack(&mut self) {
        self.clear();
    }
}
