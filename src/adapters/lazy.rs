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
