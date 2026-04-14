#![allow(non_camel_case_types)]

pub type i8 = std::primitive::i8;
pub type i16 = std::primitive::i16;
pub type i32 = std::primitive::i32;
pub type i64 = std::primitive::i64;

pub type u8 = std::primitive::u8;
pub type u16 = std::primitive::u16;
pub type u32 = std::primitive::u32;
pub type u64 = std::primitive::u64;

pub type f32 = std::primitive::f32;
pub type f64 = std::primitive::f64;

pub type usize = std::primitive::usize;
pub type isize = std::primitive::isize;

pub type boolean = bool;
pub type byte = u8;

pub type char8 = u8;
pub type char16 = u16;
pub type char32 = std::primitive::char;

const _: () = {
    assert!(std::mem::size_of::<i8>() == 1);
    assert!(std::mem::size_of::<i16>() == 2);
    assert!(std::mem::size_of::<i32>() == 4);
    assert!(std::mem::size_of::<i64>() == 8);
    assert!(std::mem::size_of::<u8>() == 1);
    assert!(std::mem::size_of::<u16>() == 2);
    assert!(std::mem::size_of::<u32>() == 4);
    assert!(std::mem::size_of::<u64>() == 8);
    assert!(std::mem::size_of::<f32>() == 4);
    assert!(std::mem::size_of::<f64>() == 8);
    assert!(std::mem::size_of::<byte>() == 1);
};
