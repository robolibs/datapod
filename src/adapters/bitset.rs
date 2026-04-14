const BITS_PER_BLOCK: usize = 64;

const fn num_blocks(size: usize) -> usize {
    if size == 0 {
        1
    } else {
        size / BITS_PER_BLOCK + if size % BITS_PER_BLOCK == 0 { 0 } else { 1 }
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Bitset<const N: usize> {
    blocks: Vec<u64>,
}

impl<const N: usize> Default for Bitset<N> {
    fn default() -> Self {
        Self {
            blocks: vec![0u64; num_blocks(N)],
        }
    }
}

impl<const N: usize> Bitset<N> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn from_str(s: &str) -> Self {
        let mut b = Self::default();
        b.set_from_str(s);
        b
    }

    pub fn max_value() -> Self {
        let mut b = Self::default();
        for block in b.blocks.iter_mut() {
            *block = u64::MAX;
        }
        b
    }

    pub fn set_from_str(&mut self, s: &str) -> &mut Self {
        let bytes = s.as_bytes();
        let max_size = core::cmp::min(N, bytes.len());
        for i in 0..max_size {
            let ch = bytes[bytes.len() - i - 1];
            self.set(i, ch != b'0');
        }
        self
    }

    pub fn set(&mut self, index: usize, value: bool) -> &mut Self {
        debug_assert!(index < N, "Bitset::set index out of bounds");
        let block_idx = index / BITS_PER_BLOCK;
        let bit = index % BITS_PER_BLOCK;
        let mask = 1u64 << bit;
        if value {
            self.blocks[block_idx] |= mask;
        } else {
            self.blocks[block_idx] &= !mask;
        }
        self
    }

    pub fn set_all(&mut self) -> &mut Self {
        for block in self.blocks.iter_mut() {
            *block = u64::MAX;
        }
        self
    }

    pub fn reset(&mut self, index: usize) -> &mut Self {
        self.set(index, false)
    }

    pub fn reset_all(&mut self) {
        for block in self.blocks.iter_mut() {
            *block = 0;
        }
    }

    pub fn flip(&mut self, index: usize) -> &mut Self {
        debug_assert!(index < N, "Bitset::flip index out of bounds");
        let block_idx = index / BITS_PER_BLOCK;
        let bit = index % BITS_PER_BLOCK;
        self.blocks[block_idx] ^= 1u64 << bit;
        self
    }

    pub fn flip_all(&mut self) -> &mut Self {
        for block in self.blocks.iter_mut() {
            *block = !*block;
        }
        self
    }

    pub fn test(&self, index: usize) -> bool {
        if index >= N {
            return false;
        }
        let block = self.blocks[index / BITS_PER_BLOCK];
        let bit = index % BITS_PER_BLOCK;
        (block & (1u64 << bit)) != 0
    }

    pub fn get(&self, index: usize) -> bool {
        self.test(index)
    }

    pub fn size(&self) -> usize {
        N
    }

    fn sanitized_last_block(&self) -> u64 {
        let last = *self.blocks.last().unwrap_or(&0);
        if N % BITS_PER_BLOCK != 0 {
            last & !(u64::MAX << (N % BITS_PER_BLOCK))
        } else {
            last
        }
    }

    pub fn count(&self) -> usize {
        let n = self.blocks.len();
        if n == 0 {
            return 0;
        }
        let mut sum = 0usize;
        for i in 0..n - 1 {
            sum += self.blocks[i].count_ones() as usize;
        }
        sum + self.sanitized_last_block().count_ones() as usize
    }

    pub fn count_ones(&self) -> usize {
        self.count()
    }

    pub fn count_zeros(&self) -> usize {
        N - self.count()
    }

    pub fn any(&self) -> bool {
        let n = self.blocks.len();
        if n == 0 {
            return false;
        }
        for i in 0..n - 1 {
            if self.blocks[i] != 0 {
                return true;
            }
        }
        self.sanitized_last_block() != 0
    }

    pub fn none(&self) -> bool {
        !self.any()
    }

    pub fn all(&self) -> bool {
        let n = self.blocks.len();
        if n == 0 {
            return true;
        }
        for i in 0..n - 1 {
            if self.blocks[i] != u64::MAX {
                return false;
            }
        }
        if N % BITS_PER_BLOCK != 0 {
            let expected_mask = !(u64::MAX << (N % BITS_PER_BLOCK));
            (self.blocks[n - 1] & expected_mask) == expected_mask
        } else {
            self.blocks[n - 1] == u64::MAX
        }
    }

    pub fn for_each_set_bit<F: FnMut(usize)>(&self, mut f: F) {
        for i in 0..N {
            if self.test(i) {
                f(i);
            }
        }
    }

    pub fn to_string(&self) -> String {
        let mut s = String::with_capacity(N);
        for i in 0..N {
            s.push(if self.test(N - i - 1) { '1' } else { '0' });
        }
        s
    }

    pub fn to_u64(&self) -> u64 {
        if N == 0 {
            return 0;
        }
        if N < 64 {
            self.blocks[0] & ((1u64 << N) - 1)
        } else {
            self.blocks[0]
        }
    }

    pub fn leading_zeros(&self) -> usize {
        if N == 0 {
            return 0;
        }
        let blocks = self.blocks.len();
        let mut total_lz = 0usize;
        for i in (0..blocks).rev() {
            let block = if i == blocks - 1 {
                self.sanitized_last_block()
            } else {
                self.blocks[i]
            };
            if block != 0 {
                let lz = block.leading_zeros() as usize;
                if i == blocks - 1 && N % BITS_PER_BLOCK != 0 {
                    let unused_bits = BITS_PER_BLOCK - (N % BITS_PER_BLOCK);
                    return total_lz + (lz - unused_bits);
                }
                return total_lz + lz;
            }
            if i == blocks - 1 && N % BITS_PER_BLOCK != 0 {
                total_lz += N % BITS_PER_BLOCK;
            } else {
                total_lz += BITS_PER_BLOCK;
            }
        }
        N
    }

    pub fn trailing_zeros(&self) -> usize {
        if N == 0 {
            return 0;
        }
        for (i, block) in self.blocks.iter().enumerate() {
            if *block != 0 {
                return i * BITS_PER_BLOCK + block.trailing_zeros() as usize;
            }
        }
        N
    }

    pub fn and_with(&mut self, other: &Self) -> &mut Self {
        for (a, b) in self.blocks.iter_mut().zip(other.blocks.iter()) {
            *a &= *b;
        }
        self
    }

    pub fn or_with(&mut self, other: &Self) -> &mut Self {
        for (a, b) in self.blocks.iter_mut().zip(other.blocks.iter()) {
            *a |= *b;
        }
        self
    }

    pub fn xor_with(&mut self, other: &Self) -> &mut Self {
        for (a, b) in self.blocks.iter_mut().zip(other.blocks.iter()) {
            *a ^= *b;
        }
        self
    }

    pub fn not_inplace(&mut self) -> &mut Self {
        for block in self.blocks.iter_mut() {
            *block = !*block;
        }
        self
    }
}
