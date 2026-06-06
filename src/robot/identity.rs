use crate::id::{Ip, STRING_NONE, Uuid};

/// Per-device identity record. The name is shipped separately as a
/// [`DpString`](crate::DpString); `name_id` references it
/// via an application-side table. `STRING_NONE` marks "no name".
#[datapod::datapod]
#[derive(Eq, Default)]
pub struct Identity {
    pub name_id: u32,
    /// Robot control interface tag. Was `u8` in the C++ port; widened to
    /// `u32` to keep the struct padding-free.
    pub rci: u32,
    pub uuid: Uuid,
    pub ip: Ip,
}

impl Identity {
    pub fn new(name_id: u32, uuid: Uuid) -> Self {
        Self {
            name_id,
            rci: 0,
            uuid,
            ip: Ip::default(),
        }
    }

    pub fn with_ip(name_id: u32, uuid: Uuid, ip: Ip) -> Self {
        Self {
            name_id,
            rci: 0,
            uuid,
            ip,
        }
    }

    pub fn has_name(&self) -> bool {
        self.name_id != STRING_NONE
    }
}
