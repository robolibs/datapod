use super::Stamp;
use super::stamp::now_nanos;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Event<T> {
    pub stamp: Stamp,
    pub value: T,
}

impl<T> Event<T> {
    pub const fn new(stamp: Stamp, value: T) -> Self {
        Self { stamp, value }
    }
}

#[derive(Debug, Clone, Copy, Default)]
pub struct TypedEvent<T> {
    pub timestamp: i64,
    pub event_type: u32,
    pub payload: T,
}

impl<T> TypedEvent<T> {
    pub const fn new(timestamp: i64, event_type: u32, payload: T) -> Self {
        Self {
            timestamp,
            event_type,
            payload,
        }
    }

    pub fn with_now(event_type: u32, payload: T) -> Self {
        Self {
            timestamp: now_nanos(),
            event_type,
            payload,
        }
    }

    pub fn now() -> i64 {
        now_nanos()
    }

    pub fn age(&self) -> i64 {
        now_nanos() - self.timestamp
    }

    pub fn seconds(&self) -> f64 {
        self.timestamp as f64 / 1e9
    }
}

impl<T: PartialEq> PartialEq for TypedEvent<T> {
    fn eq(&self, other: &Self) -> bool {
        self.timestamp == other.timestamp
            && self.event_type == other.event_type
            && self.payload == other.payload
    }
}

impl<T: Eq> Eq for TypedEvent<T> {}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct LogEvent {
    pub message: String,
    pub level: u8,
}

impl LogEvent {
    pub const DEBUG: u8 = 0;
    pub const INFO: u8 = 1;
    pub const WARN: u8 = 2;
    pub const ERROR: u8 = 3;

    pub fn new(message: impl Into<String>, level: u8) -> Self {
        Self {
            message: message.into(),
            level,
        }
    }

    pub const fn is_debug(&self) -> bool {
        self.level == Self::DEBUG
    }
    pub const fn is_info(&self) -> bool {
        self.level == Self::INFO
    }
    pub const fn is_warn(&self) -> bool {
        self.level == Self::WARN
    }
    pub const fn is_error(&self) -> bool {
        self.level == Self::ERROR
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct SystemEvent {
    pub component: String,
    pub action: String,
}

impl SystemEvent {
    pub fn new(component: impl Into<String>, action: impl Into<String>) -> Self {
        Self {
            component: component.into(),
            action: action.into(),
        }
    }
}

pub type LogEventStamped = TypedEvent<LogEvent>;
pub type SystemEventStamped = TypedEvent<SystemEvent>;
