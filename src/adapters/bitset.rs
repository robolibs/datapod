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
