use crate::Vector;
use crate::associative::Map;

#[derive(Debug, Clone, PartialEq)]
pub struct MultiSeries<T> {
    pub timestamps: Vector<i64>,
    pub series: Map<String, Vector<T>>,
}

impl<T> Default for MultiSeries<T> {
    fn default() -> Self {
        Self {
            timestamps: Vector::new(),
            series: Map::new(),
        }
    }
}

impl<T: Clone + Default> MultiSeries<T> {
    pub fn new() -> Self {
        Self {
            timestamps: Vector::new(),
            series: Map::new(),
        }
    }

    pub fn add_series(&mut self, name: impl Into<String>) {
        let name = name.into();
        if !self.series.contains_key(&name) {
            let mut v = Vector::<T>::new();
            if !self.timestamps.is_empty() {
                v.resize(self.timestamps.len(), T::default());
            }
            self.series.insert(name, v);
        }
    }

    pub fn remove_series(&mut self, name: &str) {
        self.series.remove(name);
    }

    pub fn has_series(&self, name: &str) -> bool {
        self.series.contains_key(name)
    }

    pub fn num_series(&self) -> usize {
        self.series.len()
    }

    pub fn append(&mut self, ts: i64, values: &Map<String, T>) {
        self.timestamps.push(ts);
        for (name, vec) in self.series.iter_mut() {
            match values.get(name) {
                Some(v) => vec.push(v.clone()),
                None => vec.push(T::default()),
            }
        }
    }

    pub fn reserve(&mut self, n: usize) {
        self.timestamps.reserve(n);
        for vec in self.series.values_mut() {
            vec.reserve(n);
        }
    }

    pub fn clear(&mut self) {
        self.timestamps.clear();
        for vec in self.series.values_mut() {
            vec.clear();
        }
    }

    pub fn size(&self) -> usize {
        self.timestamps.len()
    }

    pub fn len(&self) -> usize {
        self.timestamps.len()
    }

    pub fn is_empty(&self) -> bool {
        self.timestamps.is_empty()
    }

    pub fn get(&self, name: &str) -> Option<&Vector<T>> {
        self.series.get(name)
    }

    pub fn get_mut(&mut self, name: &str) -> Option<&mut Vector<T>> {
        self.series.get_mut(name)
    }

    pub fn query(&self, start: i64, end: i64) -> (usize, usize) {
        let ts = self.timestamps.as_slice();
        let start_idx = ts.partition_point(|&t| t < start);
        let end_idx = ts[start_idx..].partition_point(|&t| t < end) + start_idx;
        (start_idx, end_idx)
    }

    pub fn is_sorted(&self) -> bool {
        let ts = self.timestamps.as_slice();
        ts.windows(2).all(|w| w[0] <= w[1])
    }
}

impl MultiSeries<f64> {
    pub fn mean(&self, name: &str) -> f64 {
        match self.series.get(name) {
            Some(v) if !v.is_empty() => {
                let sum: f64 = v.iter().copied().sum();
                sum / v.len() as f64
            }
            _ => 0.0,
        }
    }

    pub fn min(&self, name: &str) -> f64 {
        match self.series.get(name) {
            Some(v) if !v.is_empty() => v.iter().copied().fold(f64::INFINITY, f64::min),
            _ => 0.0,
        }
    }

    pub fn max(&self, name: &str) -> f64 {
        match self.series.get(name) {
            Some(v) if !v.is_empty() => v.iter().copied().fold(f64::NEG_INFINITY, f64::max),
            _ => 0.0,
        }
    }
}
