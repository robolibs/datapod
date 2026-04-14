use crate::Vector;

pub const MIN_PAGE_SIZE: usize = 4;
pub const MAX_PAGE_SIZE: usize = 1 << 15;

fn next_power_of_two(n: usize) -> usize {
    if n <= 1 {
        1
    } else {
        let mut p = 1usize;
        while p < n {
            p <<= 1;
        }
        p
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Page {
    pub size: usize,
    pub capacity: usize,
    pub start: usize,
}

impl Page {
    pub fn valid(&self) -> bool {
        self.capacity != 0
    }

    pub fn size(&self) -> usize {
        self.size
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Paged<T: Default + Clone> {
    data: Vector<T>,
    free_heads: Vec<Option<usize>>,
    links: Vec<Option<usize>>,
    min_page_size: usize,
    max_page_size: usize,
}

impl<T: Default + Clone> Default for Paged<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: Default + Clone> Paged<T> {
    pub fn new() -> Self {
        Self::with_limits(MIN_PAGE_SIZE, MAX_PAGE_SIZE)
    }

    pub fn with_limits(min_page_size: usize, max_page_size: usize) -> Self {
        let min = next_power_of_two(min_page_size.max(1));
        let max = next_power_of_two(max_page_size.max(min));
        let list_len = Self::free_list_index_for(max, min) + 1;
        Self {
            data: Vector::new(),
            free_heads: vec![None; list_len],
            links: Vec::new(),
            min_page_size: min,
            max_page_size: max,
        }
    }

    fn free_list_index_for(capacity: usize, min_page_size: usize) -> usize {
        let cap_zeros = capacity.trailing_zeros() as usize;
        let min_zeros = min_page_size.trailing_zeros() as usize;
        cap_zeros - min_zeros
    }

    fn free_list_index(&self, capacity: usize) -> usize {
        Self::free_list_index_for(capacity, self.min_page_size)
    }

    fn ensure_links(&mut self) {
        if self.links.len() < self.data.len() {
            self.links.resize(self.data.len(), None);
        }
    }

    pub fn create_page(&mut self, size: usize) -> Page {
        let needed = if size > self.min_page_size {
            size
        } else {
            self.min_page_size
        };
        let capacity = next_power_of_two(needed);
        assert!(
            capacity <= self.max_page_size,
            "paged::create_page: size > max capacity"
        );
        let slot = self.free_list_index(capacity);

        if let Some(start) = self.pop_free(slot) {
            Page {
                size,
                capacity,
                start,
            }
        } else {
            let start = self.data.len();
            self.data.resize(self.data.len() + capacity, T::default());
            self.ensure_links();
            Page {
                size,
                capacity,
                start,
            }
        }
    }

    pub fn free_page(&mut self, p: Page) {
        if !p.valid() {
            return;
        }
        let slot = self.free_list_index(p.capacity);
        self.push_free(slot, p.start);
    }

    pub fn resize_page(&mut self, p: Page, new_size: usize) -> Page {
        if new_size <= p.capacity {
            Page {
                size: new_size,
                capacity: p.capacity,
                start: p.start,
            }
        } else {
            let new_page = self.create_page(new_size);
            let copy_len = p.size;
            for k in 0..copy_len {
                let v = self.data[p.start + k].clone();
                self.data[new_page.start + k] = v;
            }
            self.free_page(p);
            new_page
        }
    }

    fn push_free(&mut self, slot: usize, start: usize) {
        self.ensure_links();
        let prev = self.free_heads[slot];
        self.links[start] = prev;
        self.free_heads[slot] = Some(start);
    }

    fn pop_free(&mut self, slot: usize) -> Option<usize> {
        let head = self.free_heads[slot]?;
        self.ensure_links();
        let next = self.links[head];
        self.free_heads[slot] = next;
        Some(head)
    }

    pub fn data(&self, p: Page) -> &[T] {
        if self.data.is_empty() || !p.valid() {
            &[]
        } else {
            let slice: &[T] = &self.data;
            &slice[p.start..p.start + p.size]
        }
    }

    pub fn data_mut(&mut self, p: Page) -> &mut [T] {
        if self.data.is_empty() || !p.valid() {
            &mut []
        } else {
            let slice: &mut [T] = &mut self.data;
            &mut slice[p.start..p.start + p.size]
        }
    }

    pub fn copy_from(&mut self, to: Page, from: Page) {
        let n = from.size;
        for k in 0..n {
            let v = self.data[from.start + k].clone();
            self.data[to.start + k] = v;
        }
    }

    pub fn clear(&mut self) {
        self.data.clear();
        self.links.clear();
        for head in &mut self.free_heads {
            *head = None;
        }
    }

    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn min_page_size(&self) -> usize {
        self.min_page_size
    }

    pub fn max_page_size(&self) -> usize {
        self.max_page_size
    }
}
