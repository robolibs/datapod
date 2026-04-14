//! FIFO queue matching `datapod::Queue<T>` from the C++ library.
//!
//! C++ uses a two-vector amortised design; we alias to `VecDeque<T>` because
//! every existing call site already treats this as a standard FIFO, and expose
//! the throwing-style C++ API via an extension trait.

use std::collections::VecDeque;

pub type Queue<T> = VecDeque<T>;

/// Alias matching the `Fifo<T>` typedef exported by the C++ header.
pub type Fifo<T> = Queue<T>;

/// C++ style API for a FIFO queue backed by `VecDeque<T>`.
pub trait QueueExt<T> {
    fn push_value(&mut self, value: T);
    fn pop_value(&mut self) -> T;
    fn try_pop(&mut self) -> Option<T>;
    fn front(&self) -> &T;
    fn front_mut(&mut self) -> &mut T;
    fn back(&self) -> &T;
    fn back_mut(&mut self) -> &mut T;
    fn try_front(&self) -> Option<&T>;
    fn try_back(&self) -> Option<&T>;
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn clear_queue(&mut self);
}

impl<T> QueueExt<T> for VecDeque<T> {
    #[inline]
    fn push_value(&mut self, value: T) {
        self.push_back(value);
    }

    #[inline]
    fn pop_value(&mut self) -> T {
        self.pop_front().expect("Queue::pop: empty")
    }

    #[inline]
    fn try_pop(&mut self) -> Option<T> {
        self.pop_front()
    }

    #[inline]
    fn front(&self) -> &T {
        self.front_opt().expect("Queue::front: empty")
    }

    #[inline]
    fn front_mut(&mut self) -> &mut T {
        VecDeque::front_mut(self).expect("Queue::front: empty")
    }

    #[inline]
    fn back(&self) -> &T {
        self.back_opt().expect("Queue::back: empty")
    }

    #[inline]
    fn back_mut(&mut self) -> &mut T {
        VecDeque::back_mut(self).expect("Queue::back: empty")
    }

    #[inline]
    fn try_front(&self) -> Option<&T> {
        self.front_opt()
    }

    #[inline]
    fn try_back(&self) -> Option<&T> {
        self.back_opt()
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
    fn clear_queue(&mut self) {
        self.clear();
    }
}

/// Private helpers to disambiguate the inherent `front`/`back` methods on
/// `VecDeque` from our trait methods without using fully-qualified syntax
/// across the extension trait implementations.
trait VecDequeEndsRef<T> {
    fn front_opt(&self) -> Option<&T>;
    fn back_opt(&self) -> Option<&T>;
}

impl<T> VecDequeEndsRef<T> for VecDeque<T> {
    #[inline]
    fn front_opt(&self) -> Option<&T> {
        VecDeque::front(self)
    }

    #[inline]
    fn back_opt(&self) -> Option<&T> {
        VecDeque::back(self)
    }
}
