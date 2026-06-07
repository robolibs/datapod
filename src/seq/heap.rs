//! Binary heap Pod — holds any `T: bytemuck::Pod + PartialOrd`.
//!
//! Order tag (max / min) rides in the header. Closure-based comparators
//! aren't wire-shippable, so this is a structural simplification of the
//! old `Heap<T, Compare>`.
//!
//! All comparison-based methods (`push`, `pop`, `sift_*`) take `T` as a
//! type parameter. NaN comparisons collapse to `Equal` for stability.

use crate::{DataPodValidate, WireError};
use std::cmp::Ordering;

/// 0 = max-heap (largest on top), 1 = min-heap (smallest on top).
///
/// This is a transparent newtype rather than a Rust enum so untrusted wire
/// bytes can be copied into a header before validation without creating an
/// invalid enum discriminant.
#[repr(transparent)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct HeapOrder(pub u8);

#[allow(non_upper_case_globals)]
impl HeapOrder {
    pub const Max: Self = Self(0);
    pub const Min: Self = Self(1);

    pub fn is_valid(self) -> bool {
        self == Self::Max || self == Self::Min
    }
}

unsafe impl bytemuck::Zeroable for HeapOrder {}
unsafe impl bytemuck::Pod for HeapOrder {}

impl crate::LeWireHeader for HeapOrder {
    const LE_WIRE_SIZE: usize = <u8 as crate::LeWireHeader>::LE_WIRE_SIZE;

    fn write_le(&self, out: &mut Vec<u8>) {
        <u8 as crate::LeWireHeader>::write_le(&self.0, out);
    }

    fn read_le(bytes: &[u8]) -> Result<Self, crate::WireError> {
        Ok(Self(<u8 as crate::LeWireHeader>::read_le(bytes)?))
    }
}

impl crate::schema::SchemaFieldType for HeapOrder {
    fn field_type() -> crate::schema::FieldType {
        crate::schema::FieldType::Scalar(crate::schema::ScalarType::U8)
    }
}

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Heap {
    pub element_size: u32,
    pub order: HeapOrder,
    pub _pad: [u8; 3],
    #[dp(bytes)]
    pub data: Vec<u8>,
}

/// Max-heap (default). Largest on top.
pub type MaxHeap = Heap;
/// Priority queue alias.
pub type PriorityQueue = Heap;

impl Heap {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let heap = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            order: HeapOrder::Max,
            _pad: [0; 3],
            data: Vec::new(),
        };
        heap.validate_owned()?;
        Ok(heap)
    }

    pub fn new_min<T: bytemuck::Pod>() -> Self {
        Self::try_new_min::<T>().unwrap_or_default()
    }

    pub fn try_new_min<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let heap = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            order: HeapOrder::Min,
            _pad: [0; 3],
            data: Vec::new(),
        };
        heap.validate_owned()?;
        Ok(heap)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical element count for callers handling potentially
    /// malformed owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let element_size = self.try_element_size()?;
        if element_size == 0 {
            Ok(0)
        } else {
            Ok(self.data.len() / element_size)
        }
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    /// Fallible emptiness check for callers handling potentially malformed
    /// owned buffers.
    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    fn value_at<T: bytemuck::Pod>(&self, index: usize) -> T {
        let Ok(es) = self.try_element_size() else {
            return bytemuck::Zeroable::zeroed();
        };
        let Some(start) = index.checked_mul(es) else {
            return bytemuck::Zeroable::zeroed();
        };
        let Some(bytes) = start
            .checked_add(es)
            .and_then(|end| self.data.get(start..end))
        else {
            return bytemuck::Zeroable::zeroed();
        };
        bytemuck::pod_read_unaligned(bytes)
    }

    fn try_element_size(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.element_size, "element_size")
    }

    fn cmp<T: bytemuck::Pod + PartialOrd>(&self, a: usize, b: usize) -> Ordering {
        let lhs = self.value_at::<T>(a);
        let rhs = self.value_at::<T>(b);
        let raw = lhs.partial_cmp(&rhs).unwrap_or(Ordering::Equal);
        if self.order == HeapOrder::Min {
            raw.reverse()
        } else {
            raw
        }
    }

    pub fn push<T: bytemuck::Pod + PartialOrd>(&mut self, value: T) {
        let _ = self.try_push(value);
    }

    pub fn try_push<T: bytemuck::Pod + PartialOrd>(&mut self, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        self.validate_typed_heap_order::<T>()?;
        let bytes = bytemuck::bytes_of(&value);
        self.data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("heap payload allocation failed: {err}"))
        })?;
        self.data.extend_from_slice(bytes);
        let last = self.try_size()?.checked_sub(1).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("heap length underflowed after push")
        })?;
        self.sift_up::<T>(last);
        Ok(())
    }

    pub fn pop<T: bytemuck::Pod + PartialOrd>(&mut self) -> Option<T> {
        self.try_pop().unwrap_or(None)
    }

    pub fn try_pop<T: bytemuck::Pod + PartialOrd>(&mut self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        self.validate_typed_heap_order::<T>()?;
        let n = self.try_size()?;
        if n == 0 {
            return Ok(None);
        }
        let top = self.value_at::<T>(0);
        if n == 1 {
            self.data.clear();
            return Ok(Some(top));
        }
        let es = std::mem::size_of::<T>();
        let last_start = (n - 1).checked_mul(es).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("last element offset overflowed")
        })?;
        let last_end = last_start
            .checked_add(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("last element end overflowed"))?;
        if self.data.get(last_start..last_end).is_none() {
            return Err(crate::wire::invalid_payload::<Self>(
                "last element range is out of bounds",
            ));
        }
        self.data.copy_within(last_start..last_end, 0);
        let truncate_len = self
            .data
            .len()
            .checked_sub(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("heap truncate underflowed"))?;
        self.data.truncate(truncate_len);
        self.try_sift_down::<T>(0)?;
        Ok(Some(top))
    }

    pub fn top<T: bytemuck::Pod>(&self) -> Option<T> {
        self.try_top().unwrap_or(None)
    }

    pub fn try_top<T: bytemuck::Pod>(&self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        if self.try_empty()? {
            Ok(None)
        } else {
            Ok(Some(self.value_at::<T>(0)))
        }
    }

    fn sift_up<T: bytemuck::Pod + PartialOrd>(&mut self, mut idx: usize) {
        while idx > 0 {
            let parent = (idx - 1) / 2;
            if self.cmp::<T>(idx, parent) == Ordering::Greater {
                self.swap_values::<T>(idx, parent);
                idx = parent;
            } else {
                break;
            }
        }
    }

    fn try_sift_down<T: bytemuck::Pod + PartialOrd>(
        &mut self,
        mut idx: usize,
    ) -> Result<(), WireError> {
        let n = self.try_size()?;
        loop {
            let Some(l) = idx.checked_mul(2).and_then(|base| base.checked_add(1)) else {
                break;
            };
            let Some(r) = l.checked_add(1) else {
                break;
            };
            let mut best = idx;
            if l < n && self.cmp::<T>(l, best) == Ordering::Greater {
                best = l;
            }
            if r < n && self.cmp::<T>(r, best) == Ordering::Greater {
                best = r;
            }
            if best == idx {
                break;
            }
            self.swap_values::<T>(idx, best);
            idx = best;
        }
        Ok(())
    }

    fn swap_values<T: bytemuck::Pod>(&mut self, a: usize, b: usize) {
        let es = core::mem::size_of::<T>();
        let Some(oa) = a.checked_mul(es) else {
            return;
        };
        let Some(ob) = b.checked_mul(es) else {
            return;
        };
        for offset in 0..es {
            let Some(ia) = oa.checked_add(offset) else {
                return;
            };
            let Some(ib) = ob.checked_add(offset) else {
                return;
            };
            if ia >= self.data.len() || ib >= self.data.len() {
                return;
            }
            self.data.swap(ia, ib);
        }
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &HeapHeader {
                element_size: self.element_size,
                order: self.order,
                _pad: self._pad,
            },
            &self.data,
        )
    }

    fn validate_typed_heap_order<T: bytemuck::Pod + PartialOrd>(&self) -> Result<(), WireError> {
        let n = self.try_size()?;
        for child in 1..n {
            let parent = (child - 1) / 2;
            if self.cmp::<T>(child, parent) == Ordering::Greater {
                return Err(crate::wire::invalid_payload::<Self>(format!(
                    "heap invariant violated at child {child} parent {parent}"
                )));
            }
        }
        Ok(())
    }
}

/// Min-heap constructor — wire shape identical to [`Heap`] with `order = Min`.
pub struct MinHeap;

impl MinHeap {
    pub fn new<T: bytemuck::Pod>() -> Heap {
        Heap::new_min::<T>()
    }
}

fn check_element_size<P: 'static, T>(stored: u32) -> Result<(), WireError> {
    let actual = std::mem::size_of::<T>();
    if actual == 0 {
        return Err(crate::wire::invalid_header::<P>(
            "zero-sized Pod elements cannot be represented in byte-counted datapod containers",
        ));
    }
    let stored = super::checked_u32_to_usize::<P>(stored, "element_size")?;
    if actual != stored {
        return Err(crate::wire::invalid_header::<P>(format!(
            "element size mismatch: T is {actual}, container expects {stored}"
        )));
    }
    Ok(())
}
