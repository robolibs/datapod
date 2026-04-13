#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
pub struct Stamp {
    pub nanos: i64,
}

impl Stamp {
    pub fn new(nanos: i64) -> Self {
        Self { nanos }
    }
}
