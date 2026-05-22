//! Identifier / network primitive Pods: `Uuid`, `Ip`, `MacAddr`, `DpString`.
//! These aren't spatial — they live here on their own.

mod ip;
mod mac_addr;
mod string;
mod uuid;

pub use ip::Ip;
pub use mac_addr::MacAddr;
pub use string::{DpString, STRING_NONE};
pub use uuid::Uuid;

/// Legacy uppercase aliases — some consumers imported these names.
pub type IP = Ip;
pub type UUID = Uuid;
