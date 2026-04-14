//! String container matching `datapod::String` (template `BasicString`).
//!
//! The C++ type is a small-string-optimised UTF-8 string exposing the usual
//! suspects from `std::string`: find/rfind variants, starts/ends_with,
//! substr, replace, append-by-value, `operator+` concatenation and so on.
//!
//! Downstream Rust modules consume `datapod::sequential::String` as a plain
//! synonym for `std::string::String`, so we keep that alias intact and
//! surface the C++ API through the `StringExt` extension trait. That way
//! existing code continues to compile while tests can still rely on the
//! C++-flavoured method names.

pub type String = std::string::String;

pub const STRING_NPOS: usize = usize::MAX;

/// C++-style helpers for `std::string::String`.
pub trait StringExt {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn capacity_bytes(&self) -> usize;
    fn c_str_bytes(&self) -> *const u8;
    fn data_bytes(&self) -> *const u8;
    fn front_char(&self) -> char;
    fn back_char(&self) -> char;
    fn at_byte(&self, i: usize) -> u8;
    fn push_byte(&mut self, b: u8);
    fn pop_char(&mut self);
    fn append_str<S: AsRef<str>>(&mut self, other: S);
    fn insert_str_at(&mut self, pos: usize, text: &str);
    fn erase(&mut self, pos: usize, count: usize);
    fn resize_fill(&mut self, new_len: usize, fill: char);
    fn find_byte(&self, needle: u8, pos: usize) -> usize;
    fn find_substr(&self, needle: &str, pos: usize) -> usize;
    fn rfind_byte(&self, needle: u8, pos: usize) -> usize;
    fn rfind_substr(&self, needle: &str, pos: usize) -> usize;
    fn find_first_of(&self, chars: &str, pos: usize) -> usize;
    fn find_last_of(&self, chars: &str, pos: usize) -> usize;
    fn find_first_not_of(&self, chars: &str, pos: usize) -> usize;
    fn find_last_not_of(&self, chars: &str, pos: usize) -> usize;
    fn starts_with_str(&self, prefix: &str) -> bool;
    fn ends_with_str(&self, suffix: &str) -> bool;
    fn contains_str(&self, needle: &str) -> bool;
    fn substr(&self, pos: usize, count: usize) -> String;
    fn replace_range_str(&mut self, pos: usize, count: usize, replacement: &str);
    fn compare_str(&self, other: &str) -> std::cmp::Ordering;
    fn to_upper(&self) -> String;
    fn to_lower(&self) -> String;
    fn trim_copy(&self) -> String;
    fn trim_start_copy(&self) -> String;
    fn trim_end_copy(&self) -> String;
    fn split_on(&self, delim: char) -> Vec<String>;
}

impl StringExt for String {
    #[inline]
    fn size(&self) -> usize {
        self.len()
    }

    #[inline]
    fn empty(&self) -> bool {
        self.is_empty()
    }

    #[inline]
    fn capacity_bytes(&self) -> usize {
        self.capacity()
    }

    #[inline]
    fn c_str_bytes(&self) -> *const u8 {
        self.as_bytes().as_ptr()
    }

    #[inline]
    fn data_bytes(&self) -> *const u8 {
        self.as_bytes().as_ptr()
    }

    fn front_char(&self) -> char {
        self.chars().next().expect("String::front_char: empty")
    }

    fn back_char(&self) -> char {
        self.chars().next_back().expect("String::back_char: empty")
    }

    fn at_byte(&self, i: usize) -> u8 {
        self.as_bytes()[i]
    }

    fn push_byte(&mut self, b: u8) {
        // Appends a raw byte; only safe when `b` is ASCII. We enforce that
        // at runtime for safety in mixed ASCII/UTF-8 contexts.
        if b.is_ascii() {
            self.push(b as char);
        } else {
            // Non-ASCII: append via UTF-8 replacement char to avoid corrupting
            // the string.
            self.push(char::REPLACEMENT_CHARACTER);
        }
    }

    fn pop_char(&mut self) {
        self.pop();
    }

    fn append_str<S: AsRef<str>>(&mut self, other: S) {
        self.push_str(other.as_ref());
    }

    fn insert_str_at(&mut self, pos: usize, text: &str) {
        self.insert_str(pos, text);
    }

    fn erase(&mut self, pos: usize, count: usize) {
        if pos >= self.len() {
            return;
        }
        let end = pos.saturating_add(count).min(self.len());
        self.replace_range(pos..end, "");
    }

    fn resize_fill(&mut self, new_len: usize, fill: char) {
        if new_len <= self.len() {
            self.truncate(new_len);
        } else {
            let delta = new_len - self.len();
            for _ in 0..delta {
                self.push(fill);
            }
        }
    }

    fn find_byte(&self, needle: u8, pos: usize) -> usize {
        if pos >= self.len() {
            return STRING_NPOS;
        }
        for (i, b) in self.as_bytes()[pos..].iter().enumerate() {
            if *b == needle {
                return pos + i;
            }
        }
        STRING_NPOS
    }

    fn find_substr(&self, needle: &str, pos: usize) -> usize {
        if pos > self.len() {
            return STRING_NPOS;
        }
        match self[pos..].find(needle) {
            Some(idx) => pos + idx,
            None => STRING_NPOS,
        }
    }

    fn rfind_byte(&self, needle: u8, pos: usize) -> usize {
        if self.is_empty() {
            return STRING_NPOS;
        }
        let bytes = self.as_bytes();
        let start = pos.min(bytes.len() - 1);
        let mut i = start + 1;
        while i > 0 {
            i -= 1;
            if bytes[i] == needle {
                return i;
            }
        }
        STRING_NPOS
    }

    fn rfind_substr(&self, needle: &str, pos: usize) -> usize {
        let search_end = if pos == STRING_NPOS {
            self.len()
        } else {
            (pos + needle.len()).min(self.len())
        };
        match self[..search_end].rfind(needle) {
            Some(idx) => idx,
            None => STRING_NPOS,
        }
    }

    fn find_first_of(&self, chars: &str, pos: usize) -> usize {
        if pos >= self.len() {
            return STRING_NPOS;
        }
        for (offset, byte) in self.as_bytes()[pos..].iter().enumerate() {
            if chars.as_bytes().contains(byte) {
                return pos + offset;
            }
        }
        STRING_NPOS
    }

    fn find_last_of(&self, chars: &str, pos: usize) -> usize {
        if self.is_empty() {
            return STRING_NPOS;
        }
        let bytes = self.as_bytes();
        let start = pos.min(bytes.len() - 1);
        let mut i = start + 1;
        while i > 0 {
            i -= 1;
            if chars.as_bytes().contains(&bytes[i]) {
                return i;
            }
        }
        STRING_NPOS
    }

    fn find_first_not_of(&self, chars: &str, pos: usize) -> usize {
        if pos >= self.len() {
            return STRING_NPOS;
        }
        for (offset, byte) in self.as_bytes()[pos..].iter().enumerate() {
            if !chars.as_bytes().contains(byte) {
                return pos + offset;
            }
        }
        STRING_NPOS
    }

    fn find_last_not_of(&self, chars: &str, pos: usize) -> usize {
        if self.is_empty() {
            return STRING_NPOS;
        }
        let bytes = self.as_bytes();
        let start = pos.min(bytes.len() - 1);
        let mut i = start + 1;
        while i > 0 {
            i -= 1;
            if !chars.as_bytes().contains(&bytes[i]) {
                return i;
            }
        }
        STRING_NPOS
    }

    fn starts_with_str(&self, prefix: &str) -> bool {
        self.starts_with(prefix)
    }

    fn ends_with_str(&self, suffix: &str) -> bool {
        self.ends_with(suffix)
    }

    fn contains_str(&self, needle: &str) -> bool {
        self.contains(needle)
    }

    fn substr(&self, pos: usize, count: usize) -> String {
        if pos >= self.len() {
            return String::new();
        }
        let end = pos.saturating_add(count).min(self.len());
        // Clamp to char boundaries to keep the string UTF-8-safe.
        let pos = find_char_boundary(self, pos);
        let end = find_char_boundary(self, end);
        self[pos..end].to_owned()
    }

    fn replace_range_str(&mut self, pos: usize, count: usize, replacement: &str) {
        let end = pos.saturating_add(count).min(self.len());
        let pos = find_char_boundary(self, pos);
        let end = find_char_boundary(self, end);
        self.replace_range(pos..end, replacement);
    }

    fn compare_str(&self, other: &str) -> std::cmp::Ordering {
        self.as_str().cmp(other)
    }

    fn to_upper(&self) -> String {
        self.to_uppercase()
    }

    fn to_lower(&self) -> String {
        self.to_lowercase()
    }

    fn trim_copy(&self) -> String {
        self.trim().to_owned()
    }

    fn trim_start_copy(&self) -> String {
        self.trim_start().to_owned()
    }

    fn trim_end_copy(&self) -> String {
        self.trim_end().to_owned()
    }

    fn split_on(&self, delim: char) -> Vec<String> {
        self.split(delim).map(|s| s.to_owned()).collect()
    }
}

fn find_char_boundary(s: &str, mut idx: usize) -> usize {
    if idx >= s.len() {
        return s.len();
    }
    while !s.is_char_boundary(idx) {
        idx += 1;
    }
    idx
}
