mod ip;
mod mac_addr;
mod uuid;

pub use ip::Ip;
pub use mac_addr::MacAddr;
pub use uuid::Uuid;

pub type IP = Ip;
pub type UUID = Uuid;
