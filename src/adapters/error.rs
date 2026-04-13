#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error<E = String> {
    pub message: E,
}

impl<E> Error<E> {
    pub fn new(message: E) -> Self {
        Self { message }
    }
}
