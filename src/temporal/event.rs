use super::Stamp;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Event<T> {
    pub stamp: Stamp,
    pub value: T,
}
