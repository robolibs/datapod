pub type Pin<P> = std::pin::Pin<P>;

pub fn pin_box<T>(value: T) -> std::pin::Pin<Box<T>> {
    Box::pin(value)
}

pub fn pin_ref<T>(value: &T) -> std::pin::Pin<&T>
where
    T: Unpin,
{
    std::pin::Pin::new(value)
}

pub fn pin_mut<T>(value: &mut T) -> std::pin::Pin<&mut T>
where
    T: Unpin,
{
    std::pin::Pin::new(value)
}

pub unsafe fn pin_unchecked<'a, T>(value: &'a T) -> std::pin::Pin<&'a T> {
    unsafe { std::pin::Pin::new_unchecked(value) }
}

pub unsafe fn pin_unchecked_mut<'a, T>(value: &'a mut T) -> std::pin::Pin<&'a mut T> {
    unsafe { std::pin::Pin::new_unchecked(value) }
}
