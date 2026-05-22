//! Associative container Pods — wire-shippable maps and sets that can hold
//! arbitrary key and value types (strings, structs, scalars, anything).
//!
//! All entries live in a single `Vec<u8>` payload using a sorted-array
//! layout with offsets into a byte-blob region. This means lookups are
//! O(log n) binary search comparing the actual key bytes — no in-memory
//! hash table needed (and none could ride the wire anyway).
//!
//! Because the wire format is sorted by key bytes, `OMap`/`OSet`
//! ("ordered" variants in the old port) are simply aliases for
//! `Map`/`Set` — the ordering IS the wire format.

mod map;
mod set;

pub use map::{Map, MapEntry, OMap};
pub use set::{OSet, Set, SetEntry};
